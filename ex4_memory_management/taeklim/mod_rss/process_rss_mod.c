#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/string.h>

#include <linux/hugetlb.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tlimkim");

static int pid = 1;

module_param (pid, int, 0);

static int __init proc_rss_entry(void)
{
        struct task_struct *task;
	struct mm_struct *mm;
    	struct vm_area_struct *mmap;
	struct file *file;
	    
	char perm[16];

        task = pid_task(find_vpid(pid), PIDTYPE_PID);

	mm = task->mm;
	mmap = mm->mmap;
	printk(KERN_ALERT "Module Started \n");

    	do {
	    printk("Size: %8lu kB", (mmap->vm_end - mmap->vm_start) >> 10);

       	} while((mmap = mmap->vm_next));
}

static void __exit proc_rss_exit(void)
{

}

module_init(proc_rss_entry);
module_exit(proc_rss_exit);
