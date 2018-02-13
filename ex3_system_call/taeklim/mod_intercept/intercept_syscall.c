#include <linux/mm.h>
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

void ** sys_call_table;

asmlinkage int (*original_call) (const char*, int, int);

asmlinkage int our_call (const char* file, int flags, int mode)
{
    printk("Intercepted my_syscall \n");
    return 0;
    //return original_call(file, flags, mode);
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
    return 0;
}   

static void __exit intercept_exit (void)
{
    sys_call_table[333] = original_call;

    printk(KERN_ALERT "Module Intercept Removed \n");
}

module_init(intercept_entry);
module_exit(intercept_exit);

