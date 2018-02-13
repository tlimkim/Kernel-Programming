#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>

asmlinkage long my_syscall(void)
{
    struct task_struct *process;

    process = current;
    
    printk(KERN_EMERG "Hello World! \n");
    printk("** PID_Number: %d ** \n", process->pid);
    return 0;
}
