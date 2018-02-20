#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tlimkim");

static int cond = 1;

module_param(cond, int, 0);

static int __init mod_entry(void)
{   
    printk(KERN_ALERT "Module Started \n");

    // Condition
    if (cond == 1) {
	printk("parameter: %d \n", cond);
    } else
	printk("wrong param \n");

    return 0;
}

static void __exit mod_exit(void)
{
    printk(KERN_ALERT "Module Exited");
}

module_init(mod_entry);
module_exit(mod_exit);
