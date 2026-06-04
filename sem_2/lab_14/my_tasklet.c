#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/string.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Popov");

#define IRQ_NUM 1
#define PROC_NAME "irq_tasklet"

static char *ascii[84] = {
	" ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "Backspace",
	"Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Ctrl",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "\"", "'", "Shift (left)", "|",
	"Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Shift (right)",
	"*", "Alt", "Space", "CapsLock",
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
	"NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
	" ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"
};

static struct tasklet_struct *tasklet = NULL;
static ktime_t irq_time;
static char key_name[20];
static int code = -1;
static s64 exec_ns;

static irqreturn_t irq_handler(int irq, void *dev_id)
{
	int code;
	int is_release;
	int press_code;

	if (irq == IRQ_NUM) {
		code = inb(0x60);
		is_release = (code & 0x80) ? 1 : 0;
		press_code = code & 0x7F;

		if (is_release && press_code < 84 && press_code != 28) {
			irq_time = ktime_get();
			tasklet->data = (unsigned long)code;
			tasklet_schedule(tasklet);
			return IRQ_HANDLED;
		}
	}
	return IRQ_NONE;
}

static void tasklet_fn(unsigned long data)
{
	code = (int)data;
	int press_code = code & 0x7F;
	strncpy(key_name, ascii[press_code], sizeof(key_name) - 1);
	key_name[sizeof(key_name) - 1] = '\0';
	exec_ns = ktime_to_ns(ktime_sub(ktime_get(), irq_time));
	printk(KERN_INFO "irq_tasklet: key=%s code=%#x exec_ns=%lld\n", key_name, code, exec_ns);
}

static int proc_show(struct seq_file *m, void *v)
{
	if (code != -1)
		seq_printf(m, "key=%s code=%#x\n", key_name, code);
	seq_printf(m, "exec_ns=%lld\n", exec_ns);
	return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_fops = {
	.proc_open = proc_open,
	.proc_read = seq_read,
	.proc_release = single_release,
};

static int __init my_init(void)
{
	int ret;

	tasklet = kmalloc(sizeof(*tasklet), GFP_KERNEL);
	if (!tasklet)
		return -ENOMEM;

	tasklet_init(tasklet, tasklet_fn, 0);

	ret = request_irq(IRQ_NUM, irq_handler, IRQF_SHARED, "irq_handler_task", irq_handler);
	if (ret) {
		kfree(tasklet);
		return ret;
	}

	if (!proc_create(PROC_NAME, 0444, NULL, &proc_fops)) {
		free_irq(IRQ_NUM, irq_handler);
		kfree(tasklet);
		return -ENOMEM;
	}

	printk(KERN_INFO "irq_tasklet: loaded\n");
	return 0;
}

static void __exit my_exit(void)
{
	remove_proc_entry(PROC_NAME, NULL);
	synchronize_irq(IRQ_NUM);
	tasklet_kill(tasklet);
	free_irq(IRQ_NUM, irq_handler);
	kfree(tasklet);
	printk(KERN_INFO "irq_tasklet: unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
