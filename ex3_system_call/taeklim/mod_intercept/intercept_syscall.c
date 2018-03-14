#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <asm/pgtable.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/semaphore.h>
#include <asm/cacheflush.h>
#include <asm/set_memory.h> // declared set_memory_rw & set_memory_ro
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tlimkim");

void ** sys_call_table;

pid_t pid_mod;
int address_mod;

asmlinkage int (*original_call) (pid_t pid, int address);

asmlinkage int our_call (pid_t pid, int address)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct vm_area_struct *mmap;

    printk("Intercepted my_syscall \n");
    printk("pid: %d address: %x \n", pid, address);
    
    pid = 1;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    
    mm = task->mm;
    mmap = mm->mmap;

    do {

	printk("vm address range: %lx - %lx \n", mmap->vm_start, mmap->vm_end);
    } while((mmap = mmap->vm_next));

    return 0;
}

void set_addr_rw(unsigned long addr)
{
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);

    if (pte->pte & ~_PAGE_RW)
	pte->pte |= _PAGE_RW;
}

static int __init intercept_entry (void)
{
    printk(KERN_ALERT "Module Intercept Inserted \n");

    sys_call_table = (void*)kallsyms_lookup_name("sys_call_table");
    
    original_call = sys_call_table[333];

    //set_memory_rw((long unsigned int)sys_call_table, 1);
    set_addr_rw(sys_call_table);

    sys_call_table[333] = our_call;

    printk("pid: %d \n", pid_mod);

    return 0;
}   

static void __exit intercept_exit (void)
{
    sys_call_table[333] = original_call;

    printk(KERN_ALERT "Module Intercept Removed \n");
}

module_init(intercept_entry);
module_exit(intercept_exit);

