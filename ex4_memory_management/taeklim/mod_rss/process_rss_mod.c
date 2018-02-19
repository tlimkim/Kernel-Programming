#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/mm.h>

#include <linux/seq_file.h>
#include <linux/hugetlb.h> // for 'vma_kernel_pagesize'
#include <linux/huge_mm.h>
#include <asm/pgtable.h>
#include <linux/spinlock.h>
#include <asm-generic/pgtable.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/pagemap.h>
#include <linux/mm_types.h>
#include <linux/page_idle.h>

#define PSS_SHIFT 12

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tlimkim");

static int pid = 1;

module_param (pid, int, 0);

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
	bool compound, bool young, bool dirty);

static void smaps_pte_entry(pte_t *pte, unsigned long addr,
	struct mm_walk *walk);

static void smaps_pmd_entry(pmd_t *pmd, unsigned long addr,
	struct mm_walk *walk);


static int smaps_pte_range(pmd_t *pmd, unsigned long addr, 
	unsigned long end, struct mm_walk *walk) // source from /proc/task_mmu.c
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

    pte = pte_offset_map_lock(vma->vm_mm, pmd, addr, &ptl);
    for (; addr != end; pte++, addr += PAGE_SIZE)
	smaps_pte_entry(pte, addr, walk);
    pte_unmap_unlock(pte - 1, ptl);

out:
    cond_resched();
    return 0;
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
    struct mem_size_stats *mss = walk->private;
    struct vm_area_struct *vma = walk->vma;
    struct page *page;

    /* FOLL_DUMP will return -EFAULT on huge zero page */
    page = follow_trans_huge_pmd(vma, addr, pmd, FOLL_DUMP);
    if (IS_ERR_OR_NULL(page))
	return;
    if (PageAnon(page))
	mss->anonymous_thp += HPAGE_PMD_SIZE;
    else if (PageSwapBacked(page))
	mss->shmem_thp += HPAGE_PMD_SIZE;
    else if (is_zone_device_page(page))
	/* pass */;
    else
	VM_BUG_ON_PAGE(1, page);
    smaps_account(mss, page, true, pmd_young(*pmd), pmd_dirty(*pmd));
}

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
     * page_count(page) == 1 guarantees the page is mapped exactly once.
     * If any subpage of the compound page mapped with PTE it would elevate
     * page_count().
     */
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

static int __init proc_rss_entry(void)
{
        struct task_struct *task;
	struct mm_struct *mm;
    	struct vm_area_struct *mmap;
	
	struct mem_size_stats mss;
//	char perm[16];

        task = pid_task(find_vpid(pid), PIDTYPE_PID);

	mm = task->mm;
	mmap = mm->mmap;
	printk(KERN_ALERT "Module Started \n");
	
	struct mm_walk smaps_walk = {
	    .pmd_entry = smaps_pte_range,
//	    .hugetlb_entry = smaps_hugetlb_range,
	    .mm = mmap->vm_mm,
	    .private = &mss,
	};

    	do {
	    memset(&mss, 0, sizeof(mss));
	    walk_page_vma(mmap, &smaps_walk);
	    
	    printk("Size:	    %11lu kB \n"
		   "KernelPageSize: %8lu kB \n"
		   "MMUPageSize:    %8lu kB \n"
		   "Rss:	    %11lu kB \n"
		   "Pss:	    %11lu kB \n"
		   "Shared_Clean:   %8lu kB \n"
		   "Shared_Dirty:   %8lu kB \n"
		   "Private_Clean:  %8lu kB \n"
		   "Private_Dirty:  %8lu kB \n"
		   "Referenced:	    %3lu kB \n"
		   "Anonymous:	    %3lu kB \n"
		   "LazyFree:	    %3lu kB \n"
		   "AnonHugePages:  %8lu kB \n"
		   "ShmemPmdMapped: %8lu kB \n"
		   "Shared_Hugetlb: %8lu kB \n"
		   "Private_Hugetlb:%8lu kB \n"
		   "Swap:	    %11lu kB \n"
		   "SwapPss:	    %11lu kB \n"
		   "Locked:	    %11lu kB \n",
		   ((mmap->vm_end - mmap->vm_start) >> 10),
		   vma_kernel_pagesize(mmap) >> 10,
		   vma_kernel_pagesize(mmap) >> 10, // MMU page size seems to be same as kernel page
		   mss.resident >> 10,
		   (unsigned long)(mss.pss >> (10 + PSS_SHIFT)),
		   mss.shared_clean >> 10,
		   mss.shared_dirty >> 10,
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

       	} while((mmap = mmap->vm_next));

	return 0;
}

static void __exit proc_rss_exit(void)
{

}

module_init(proc_rss_entry);
module_exit(proc_rss_exit);
