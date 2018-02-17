#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/mm.h> // for VM_READ ..
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tlimkim");

static int pid = 1; //

module_param (pid, int, 0);

static int __init proc_mem_entry(void)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct vm_area_struct *mmap;
    struct file *file;

    char perm[16];
    
    //unsigned long long pgoff = 0;
    //unsigned long ino = 0;
    //dev_t dev = 0;    

    task = pid_task(find_vpid(pid), PIDTYPE_PID);

    mm = task->mm;
    mmap = mm->mmap;
    
    printk(KERN_ALERT "Module Started \n");
     
    do {
	char path[64] = " ";
	unsigned long long pgoff = 0;
	unsigned long ino = 0;
	dev_t dev = 0;   

	if ((mmap->vm_flags & VM_READ) == VM_READ)
	    perm[0] = 'r';
	else
	    perm[0] = '-';
    
	if ((mmap->vm_flags & VM_WRITE) == VM_WRITE)
	    perm[1] = 'w';
	else
	    perm[1] = '-';

	if ((mmap->vm_flags & VM_EXEC) == VM_EXEC)
	    perm[2] = 'x';
	else
	    perm[2] = '-';

	if ((mmap->vm_flags & VM_SHARED) == VM_SHARED)
	    perm[3] = '-';
	else
	    perm[3] = 'p';

	if (mmap->vm_file) {
	    struct inode *inode = file_inode(mmap->vm_file);
	    dev = inode->i_sb->s_dev;
	    ino = inode->i_ino;
	    pgoff = (loff_t)mmap->vm_pgoff << PAGE_SHIFT;

	    strcpy(path, mmap->vm_file->f_path.dentry->d_iname);
	}

	if (mmap->vm_file) {
	    strcpy(path, mmap->vm_file->f_path.dentry->d_iname);
	} else if (!(mmap->vm_mm)) {
	    strcpy(path, "[vdso]");
	} else if (mmap->vm_start <= mmap->vm_mm->brk && mmap->vm_end >= mmap->vm_mm->start_brk) {
	    strcpy(path, "[heap]");
	} else if (mmap->vm_start <= mmap->vm_mm->start_stack && mmap->vm_end >= mmap->vm_mm->start_stack) {
	    strcpy(path, "[stack]");
	}

	printk("%lx - %lx %c%c%c%c %08llx %02x:%02x %lu \t %s",		
		mmap->vm_start, mmap->vm_end,
	       	perm[0], perm[1], perm[2], perm[3], 
		pgoff, 
		MAJOR(dev), MINOR(dev), ino,
		path);
	
    } while((mmap = mmap->vm_next));

    return 0;
}

static void __exit proc_mem_exit(void)
{

}

module_init(proc_mem_entry);
module_exit(proc_mem_exit);
