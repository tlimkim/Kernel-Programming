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

#define FILENAME "ex4"

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

int proc_show(struct seq_file *m, void *v) {
		const char *name = NULL;
		mm = task->mm;
		vma = mm->mmap;
		file = vma->vm_file;

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
