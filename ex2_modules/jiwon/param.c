#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");

static int myint = 420;
static long int myLong = 9999;
static char *mystring = "blah";
static int myintArray[2]={-1,-1};
static int arr_argc = 0;
module_param(myint, int, 0000);
module_param(myLong, long, 0000);
module_param(mystring, charp, 0000);
module_param_array(myintArray, int, &arr_argc, 0000);

static int __init param_init(void)
{
		int i;
		printk(KERN_INFO "param module init\n");
		printk(KERN_INFO "myint is an integer: %d\n", myint);
		printk(KERN_INFO "myLong is an long integer: %ld\n", myLong);
		printk(KERN_INFO "mystring is a string: %s\n", mystring);

		for (i = 0; i < (sizeof myintArray / sizeof (int)); i++)
				printk(KERN_INFO "myintArray[%d] = %d\n", i, myintArray[i]);

		printk(KERN_INFO "got %d arguments for myintArray.\n", arr_argc);
		return 0;
}

static void __exit param_exit(void)
{
		printk(KERN_INFO "param module exit\n");
}

module_init(param_init);
module_exit(param_exit);

