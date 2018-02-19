#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/mm.h>

#include <linux/seq_file.h>
#include <linux/hugetlb.h> // for 'vma_kernel_pagesize'

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
    unsigned long Shmem_thp;
    unsigned long swap;
    unsigned long shared_hugetlb;
    unsigned long private_hugetlb;
    unsigned long first_vma_start;
    u64 pss;
    u64 pss_locked;
    u64 swap_pss;
    bool check_shmem_swap;
};

struct proc_maps_private {
    struct inode *inode;
    struct task_struct *task;
    struct mm_struct *mm;
    struct mem_size_stats *rollup;

    struct vm_area_struct *tail_vma;
};

static int __init proc_rss_entry(void)
{
        struct task_struct *task;
	struct mm_struct *mm;
    	struct vm_area_struct *mmap;
	struct file *file;
	struct seq_file *m;
	
	//struct proc_maps_private *priv = m->private;
	struct mem_size_stats *mss;
/*
       	struct mm_walk smaps_walk = {
//	    .pmd_entry = smaps_pte_range,
//	    .hugetlb_entry = smaps_hugetlb_range,
	    .mm = mmap->vm_mm,
	    .private = &mss,
	};

	memset(&mss, 0, sizeof(mss));	
	
	walk_page_range(mmap->vm_start, mmap->vm_end, &smaps_walk);
*/
	char perm[16];

        task = pid_task(find_vpid(pid), PIDTYPE_PID);

	mm = task->mm;
	mmap = mm->mmap;
	printk(KERN_ALERT "Module Started \n");
	
	struct mm_walk smaps_walk = {
//	    .pmd_entry = smaps_pte_range,
//	    .hugetlb_entry = smaps_hugetlb_range,
	    .mm = mmap->vm_mm,
	    .private = &mss,
	};

//	memset(&mss, 0, sizeof(mss));	
	
//	walk_page_range(mmap->vm_start, mmap->vm_end, &smaps_walk);

    	do {
	    printk("Size:	    %8lu kB \n"
		   "KernelPageSize: %8lu kB \n"
		   "MMUPageSize:    %8lu kB \n",
		   ((mmap->vm_end - mmap->vm_start) >> 10),
		   vma_kernel_pagesize(mmap) >> 10,
		   vma_kernel_pagesize(mmap) >> 10); // MMU page size seems to be same as kernel page

	    walk_page_range(mmap->vm_start, mmap->vm_end, &smaps_walk);
	    printk("done walking \n");
	    printk("Rss:	    %8lu kB \n", mss->resident >> 10);

       	} while((mmap = mmap->vm_next));

	return 0;
}

static void __exit proc_rss_exit(void)
{

}

module_init(proc_rss_entry);
module_exit(proc_rss_exit);
