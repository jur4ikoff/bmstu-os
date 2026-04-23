#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/bitops.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yurii Popov");

static char *format_binary(unsigned int flags)
{
    int first_one_idx = 0;
    static char buf[33] = {0};
    for (int i = 0; i < 32; ++i) {
        buf[i] = (flags & (1UL << (31 - i))) ? '1' : '0';
        first_one_idx = first_one_idx ? first_one_idx : ((flags & (1UL << (31 - i))) ? i : first_one_idx);
    }
    return buf + first_one_idx;
}

static void print_task(struct task_struct *task)
{
    printk(
        KERN_INFO "comm=%s, pid=%d, parent_comm=%s, ppid=%d, tgid=%d, state=0x%x, flags=0x%x, on_cpu=%d, prio=%d, policy=%u, exit_state=%d, utime=%llu, stime=%llu, last_switch_count=%lu, last_switch_time=%lu, sessionid=%u\n",
        task->comm,
        task->pid,
        task->parent->comm,
        task->parent->pid,
        task->tgid,
        task->__state,
        task->flags,
        task->on_cpu,
        task->prio,
        task->policy,
        task->exit_state,
        (unsigned long long)task->utime,
        (unsigned long long)task->stime,
        task->last_switch_count,
        task->last_switch_time,
        task->sessionid
    );
}

void md1_ls_proc(void)
{
    struct task_struct *task = &init_task;

    printk(KERN_INFO "+ md1_ls_proc: start listing processes\n");
    do {
        print_task(task);
    } while ((task = next_task(task)) != &init_task);
    
    print_task(current);

    printk(KERN_INFO "+ md1_ls_proc: finish listing processes\n");
}

static int __init md_init(void)
{
    printk(KERN_INFO "+ module md1 start\n");
    md1_ls_proc();
    return 0;
}

static void __exit md_exit(void)
{
    printk(KERN_INFO "+ module md1 unloaded\n");
}

module_init(md_init);
module_exit(md_exit);