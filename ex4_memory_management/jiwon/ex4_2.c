#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/seq_file.h>    // for seq_printf
#include <linux/mm.h>			// for flags
#include <linux/fs.h>		// file_inode
#include <linux/types.h>	// loff_t, dev_t
#include <linux/kdev_t.h>
#include <linux/proc_fs.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/pagemap.h>
#include <linux/page_idle.h>
#include <linux/hugetlb.h>
#include <linux/mm.h>
#include <linux/swap.h>
#define PSS_SHIFT 12
#define FILENAME "rss"

MODULE_LICENSE("GPL");

static int npid=1;
module_param(npid,int,0);    

struct task_struct *task;
struct mm_struct *mm;
struct pid* pid;
struct vm_area_struct *vma;
struct file *file;
int i=0, error=0;
unsigned long start, end;

struct mem_size_stats {
    bool first;
    unsigned long resident;
    unsigned long shared_clean;
    unsigned long shared_dirty;
    unsigned long private_clean;
    unsigned long private_dirty;
    unsigned long referenced;
    unsigned long anonymous;
    unsigned long lazyfree;
    unsigned long anonymous_thp;
    unsigned long shmem_thp;
    unsigned long swap;
    unsigned long shared_hugetlb;
    unsigned long private_hugetlb;
    unsigned long first_vma_start;
    u64 pss;
    u64 pss_locked;
    u64 swap_pss;
    bool check_shmem_swap;
};

static void smaps_account(struct mem_size_stats *mss, struct page *page,
        bool compound, bool young, bool dirty)
{
    int i, nr = compound ? 1 << compound_order(page) : 1;
    unsigned long size = nr * PAGE_SIZE;

    if (PageAnon(page)) {
        mss->anonymous += size;
        if (!PageSwapBacked(page) && !dirty && !PageDirty(page))
            mss->lazyfree += size;
    }

    mss->resident += size;
    /* Accumulate the size in pages that have been accessed. */
    if (young || page_is_young(page) || PageReferenced(page))
        mss->referenced += size;

    /*
     *   * page_count(page) == 1 guarantees the page is mapped exactly once.
     *       * If any subpage of the compound page mapped with PTE it would elevate
     *           * page_count().
     *               */
    if (page_count(page) == 1) {
        if (dirty || PageDirty(page))
            mss->private_dirty += size;
        else
            mss->private_clean += size;
        mss->pss += (u64)size << PSS_SHIFT;
        return;
    }

    for (i = 0; i < nr; i++, page++) {
        int mapcount = page_mapcount(page);

        if (mapcount >= 2) {
            if (dirty || PageDirty(page))
                mss->shared_dirty += PAGE_SIZE;
            else
                mss->shared_clean += PAGE_SIZE;
            mss->pss += (PAGE_SIZE << PSS_SHIFT) / mapcount;
        } else {
            if (dirty || PageDirty(page))
                mss->private_dirty += PAGE_SIZE;
            else
                mss->private_clean += PAGE_SIZE;
            mss->pss += PAGE_SIZE << PSS_SHIFT;
        }
    }
}

static void smaps_pte_entry(pte_t *pte, unsigned long addr,
        struct mm_walk *walk)
{
    struct mem_size_stats *mss = walk->private;
    struct vm_area_struct *vma = walk->vma;
    struct page *page = NULL;

    if (pte_present(*pte)) {
        page = vm_normal_page(vma, addr, *pte);
    } else if (is_swap_pte(*pte)) {
        swp_entry_t swpent = pte_to_swp_entry(*pte);

        if (!non_swap_entry(swpent)) {
            int mapcount;

            mss->swap += PAGE_SIZE;
            mapcount = swp_swapcount(swpent);
            if (mapcount >= 2) {
                u64 pss_delta = (u64)PAGE_SIZE << PSS_SHIFT;

                do_div(pss_delta, mapcount);
                mss->swap_pss += pss_delta;
            } else {
                mss->swap_pss += (u64)PAGE_SIZE << PSS_SHIFT;
            }
        } else if (is_migration_entry(swpent))
            page = migration_entry_to_page(swpent);
        else if (is_device_private_entry(swpent))
            page = device_private_entry_to_page(swpent);
    } else if (unlikely(IS_ENABLED(CONFIG_SHMEM) && mss->check_shmem_swap
                && pte_none(*pte))) {
        page = find_get_entry(vma->vm_file->f_mapping,
                linear_page_index(vma, addr));
        if (!page)
            return;

        if (radix_tree_exceptional_entry(page))
            mss->swap += PAGE_SIZE;
        else
            put_page(page);

        return;
    }

    if (!page)
        return;

    smaps_account(mss, page, false, pte_young(*pte), pte_dirty(*pte));
}
static void smaps_pmd_entry(pmd_t *pmd, unsigned long addr,
        struct mm_walk *walk)
{
}

static int smaps_pte_range(pmd_t *pmd, unsigned long addr, unsigned long end,
        struct mm_walk *walk)
{
    struct vm_area_struct *vma = walk->vma;
    pte_t *pte;
    spinlock_t *ptl;

    ptl = pmd_trans_huge_lock(pmd, vma);
    if (ptl) {
        if (pmd_present(*pmd))
            smaps_pmd_entry(pmd, addr, walk);
        spin_unlock(ptl);
        goto out;
    }

    if (pmd_trans_unstable(pmd))
        goto out;
    /*
     *   * The mmap_sem held all the way back in m_start() is what
     *       * keeps khugepaged out of here and from collapsing things
     *           * in here.
     *               */
    pte = pte_offset_map_lock(vma->vm_mm, pmd, addr, &ptl);
    for (; addr != end; pte++, addr += PAGE_SIZE)
        smaps_pte_entry(pte, addr, walk);
    pte_unmap_unlock(pte - 1, ptl);
out:
    cond_resched();
    return 0;
}
static void show_smap_vma_flags(struct seq_file *m, struct vm_area_struct *vma)
{
    /*
     *   * Don't forget to update Documentation/ on changes.
     *       */
    static const char mnemonics[BITS_PER_LONG][2] = {
        /*
         *       * In case if we meet a flag we don't know about.
         *               */
        [0 ... (BITS_PER_LONG-1)] = "??",

        [ilog2(VM_READ)]    = "rd",
        [ilog2(VM_WRITE)]   = "wr",
        [ilog2(VM_EXEC)]    = "ex",
        [ilog2(VM_SHARED)]  = "sh",
        [ilog2(VM_MAYREAD)] = "mr",
        [ilog2(VM_MAYWRITE)]    = "mw",
        [ilog2(VM_MAYEXEC)] = "me",
        [ilog2(VM_MAYSHARE)]    = "ms",
        [ilog2(VM_GROWSDOWN)]   = "gd",
        [ilog2(VM_PFNMAP)]  = "pf",
        [ilog2(VM_DENYWRITE)]   = "dw",
#ifdef CONFIG_X86_INTEL_MPX
        [ilog2(VM_MPX)]     = "mp",
#endif
        [ilog2(VM_LOCKED)]  = "lo",
        [ilog2(VM_IO)]      = "io",
        [ilog2(VM_SEQ_READ)]    = "sr",
        [ilog2(VM_RAND_READ)]   = "rr",
        [ilog2(VM_DONTCOPY)]    = "dc",
        [ilog2(VM_DONTEXPAND)]  = "de",
        [ilog2(VM_ACCOUNT)] = "ac",
        [ilog2(VM_NORESERVE)]   = "nr",
        [ilog2(VM_HUGETLB)] = "ht",
        [ilog2(VM_ARCH_1)]  = "ar",
        [ilog2(VM_WIPEONFORK)]  = "wf",
        [ilog2(VM_DONTDUMP)]    = "dd",
#ifdef CONFIG_MEM_SOFT_DIRTY
        [ilog2(VM_SOFTDIRTY)]   = "sd",
#endif
        [ilog2(VM_MIXEDMAP)]    = "mm",
        [ilog2(VM_HUGEPAGE)]    = "hg",
        [ilog2(VM_NOHUGEPAGE)]  = "nh",
        [ilog2(VM_MERGEABLE)]   = "mg",
        [ilog2(VM_UFFD_MISSING)]= "um",
        [ilog2(VM_UFFD_WP)] = "uw",
#ifdef CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS
        /* These come out via ProtectionKey: */
        [ilog2(VM_PKEY_BIT0)]   = "",
        [ilog2(VM_PKEY_BIT1)]   = "",
        [ilog2(VM_PKEY_BIT2)]   = "",
        [ilog2(VM_PKEY_BIT3)]   = "",
#endif
    };
    size_t i;

    seq_puts(m, "VmFlags: ");
    for (i = 0; i < BITS_PER_LONG; i++) {
        if (!mnemonics[i][0])
            continue;
        if (vma->vm_flags & (1UL << i)) {
            seq_printf(m, "%c%c ",
                    mnemonics[i][0], mnemonics[i][1]);
        }
    }
    seq_putc(m, '\n');
}

int proc_show(struct seq_file *m, void *v) {
    const char *name = NULL;
    struct mem_size_stats mss;
    mm = task->mm;
    vma = mm->mmap;
    file = vma->vm_file;
    struct mm_walk smaps_walk = {
        .pmd_entry = smaps_pte_range,
        .mm = vma->vm_mm,
        .private = &mss,
    };

    seq_setwidth(m, 25 + sizeof(void *) * 6 - 1);

    do {
        unsigned long ino = 0;
        unsigned long long pgoff = 0;
        dev_t dev = 0;
        start = vma->vm_start;
        end = vma->vm_end;
        file = vma->vm_file;
        if (file) {
            struct inode *inode = file->f_inode;
            ino = inode->i_ino;
            dev = inode->i_sb->s_dev;
            pgoff = ((loff_t)vma->vm_pgoff) << PAGE_SHIFT;
        }

        seq_setwidth(m, 25 + sizeof(void *) * 6 - 1);
        seq_printf(m,"%08lx-%08lx %c%c%c%c %08llx %02x:%02x %lu ",
                start, end, vma->vm_flags & VM_READ ? 'r': '-', vma->vm_flags & VM_WRITE ? 'w': '-', vma->vm_flags & VM_EXEC ? 'x': '-', vma->vm_flags & VM_MAYSHARE ? 's': 'p', pgoff,  MAJOR(dev), MINOR(dev), ino);

        if (file) {
            seq_pad(m, ' ');
            seq_file_path(m, file, "\n");
            goto done;
        }

        if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso) {
            name = "[vdso]";
        } else {
            name = NULL;
        }

        if (!name){
            if (!mm) {
                name = "[vdso]";
                goto done;
            }
            if (vma->vm_start <= mm->brk &&
                    vma->vm_end >= mm->start_brk) {
                name = "[heap]";
                goto done;
            }

            if (vma->vm_start <= vma->vm_mm->start_stack &&
                    vma->vm_end >= vma->vm_mm->start_stack) {
                name = "[stack]";
            }
        }
done:
        if (name) {
            seq_pad(m,' ');
            seq_puts(m, name);
        }

        seq_putc(m, '\n');
        memset(&mss, 0, sizeof(mss));
        walk_page_vma(vma, &smaps_walk);
        seq_printf(m,
                "Size:           %8lu kB\n"
                "KernelPageSize: %8lu kB\n"
                "MMUPageSize:    %8lu kB\n",
                (vma->vm_end - vma->vm_start) >> 10,
                vma_kernel_pagesize(vma) >> 10,
                vma_kernel_pagesize(vma) >> 10);
        seq_printf(m,
                "Rss:            %8lu kB\n"
                "Pss:            %8lu kB\n"
                "Shared_Clean:   %8lu kB\n"
                "Shared_Dirty:   %8lu kB\n"
                "Private_Clean:  %8lu kB\n"
                "Private_Dirty:  %8lu kB\n"
                "Referenced:     %8lu kB\n"
                "Anonymous:      %8lu kB\n"
                "LazyFree:       %8lu kB\n"
                "AnonHugePages:  %8lu kB\n"
                "ShmemPmdMapped: %8lu kB\n"
                "Shared_Hugetlb: %8lu kB\n"
                "Private_Hugetlb: %7lu kB\n"
                "Swap:           %8lu kB\n"
                "SwapPss:        %8lu kB\n"
                "Locked:         %8lu kB\n",
                mss.resident >> 10,
                (unsigned long)(mss.pss >> (10 + PSS_SHIFT)),
                mss.shared_clean  >> 10,
                mss.shared_dirty  >> 10,
                mss.private_clean >> 10,
                mss.private_dirty >> 10,
                mss.referenced >> 10,
                mss.anonymous >> 10,
                mss.lazyfree >> 10,
                mss.anonymous_thp >> 10,
                mss.shmem_thp >> 10,
                mss.shared_hugetlb >> 10,
                mss.private_hugetlb >> 10,
                mss.swap >> 10,
                (unsigned long)(mss.swap_pss >> (10 + PSS_SHIFT)),
                (unsigned long)(mss.pss >> (10 + PSS_SHIFT)));
        show_smap_vma_flags(m, vma);

    } while (vma = vma->vm_next);

    return 0;
}


int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);
}

const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};


static int __init mod_start(void) {
    rcu_read_lock();
    pid=find_get_pid(npid);
    task=pid_task(pid,PIDTYPE_PID);

    if(!task) {
        error=-EINVAL;
        goto out;
    }

    proc_create(FILENAME, 0, NULL, &fops);
out:
    put_pid(pid);
    rcu_read_unlock();
    return error;

}
static void __exit mod_end(void) {
    remove_proc_entry(FILENAME,NULL);

}

module_init(mod_start);
module_exit(mod_end);
