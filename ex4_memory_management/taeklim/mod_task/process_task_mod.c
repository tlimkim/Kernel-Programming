#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tlimkim");

//struct task_struct *proc = current;

static int pid = 1; //

module_param (pid, int, 0);

static int __init proc_init (void)
{
    printk(KERN_ALERT "Module Started \n");
    printk("PID: %d \n", pid);
    
    struct task_struct *task;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);

    printk("Process name: %s \n", task->comm);

    return 0;
}

static void __exit proc_exit (void)
{

    printk(KERN_ALERT "Module Removed \n");
}

module_init(proc_init);
module_exit(proc_exit);
