#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tlimkim");

static int __init hello_module(void)
{
    printk(KERN_ALERT "Hello Module! \n");
    return 0;
}

static void __exit bye_module(void)
{
    printk(KERN_ALERT "Bye Module! \n");
}

module_init(hello_module);
module_exit(bye_module);

