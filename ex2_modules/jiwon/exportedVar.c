#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
int init_module(void) 
{
		printk(KERN_INFO "var module init\n");
		printk(KERN_INFO "Current jiffies: %lu.\n", jiffies);
		return 0;
}
void cleanup_module(void)
{
		printk(KERN_INFO "var module exit\n");

}

