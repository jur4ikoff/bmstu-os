#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/errno.h>

#define PROC_FILE_NAME "fortune"
#define PROC_DIR_NAME  "fortune_dir"
#define PROC_SYM_NAME  "fortune_symlink"

#define COOKIE_BUF_SIZE PAGE_SIZE

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhizhin Nikita");

static char *cookie_buf;

static unsigned int write_index;
static unsigned int read_index;

static struct proc_dir_entry *proc_file;
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_symlink_entry;

static ssize_t fortune_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    size_t len;

    if (count == 0)
        return 0;

    if (count > COOKIE_BUF_SIZE - 1)
        return -ENOSPC;

    if (write_index + count + 1 > COOKIE_BUF_SIZE) {
        return -ENOSPC;
    }

    len = count;

    if (copy_from_user(cookie_buf + write_index, user_buf, len)) {
        return -EFAULT;
    }

    cookie_buf[write_index + len] = '\0';
    printk(KERN_INFO "fortune: write %s", cookie_buf + write_index);
    write_index += len + 1;

    return len;
}

static ssize_t fortune_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    size_t len;
    size_t available;

    if (*ppos > 0)
        return 0;

    if (count == 0)
        return 0;

    if (write_index == 0)
        return 0;

    if (read_index >= write_index)
        read_index = 0;

    available = write_index - read_index;
    len = strnlen(cookie_buf + read_index, available);

    if (len == 0) {
        read_index = 0;
        len = strnlen(cookie_buf, write_index);

        if (len == 0)
            return 0;
    }

    if (len > count)
        len = count;

    if (copy_to_user(user_buf, cookie_buf + read_index, len)) {
        return -EFAULT;
    }

    printk(KERN_INFO "fortune: read %s", cookie_buf + read_index);
    read_index += len + 1;

    if (read_index >= write_index)
        read_index = 0;

    *ppos += len;
    return len;
}

static int fortune_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "fortune: open");
    return 0;
}

static int fortune_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "fortune: release");
    return 0;
}

static const struct proc_ops fortune_proc_ops = {
    .proc_open = fortune_open,
    .proc_read = fortune_read,
    .proc_write = fortune_write,
    .proc_release = fortune_release,
};

static int __init fortune_init(void)
{
    cookie_buf = vmalloc(COOKIE_BUF_SIZE);
    if (!cookie_buf) {
        printk(KERN_ERR "fortune: cannot allocate cookie buffer");
        return -ENOMEM;
    }

    memset(cookie_buf, 0, COOKIE_BUF_SIZE);
    write_index = 0;
    read_index = 0;

    proc_dir = proc_mkdir(PROC_DIR_NAME, NULL);
    if (!proc_dir) {
        printk(KERN_ERR "fortune: cannot create /proc/%s", PROC_DIR_NAME);
        vfree(cookie_buf);
        return -ENOMEM;
    }

    proc_file = proc_create(PROC_FILE_NAME, 0666, proc_dir, &fortune_proc_ops);
    if (!proc_file) {
        printk(KERN_ERR "fortune: cannot create /proc/%s", PROC_FILE_NAME);
        proc_remove(proc_dir);
        vfree(cookie_buf);
        return -ENOMEM;
    }

    proc_symlink_entry = proc_symlink(PROC_SYM_NAME, NULL, "/proc/" PROC_DIR_NAME "/" PROC_FILE_NAME);
    if (!proc_symlink_entry) {
        printk(KERN_ERR "fortune: cannot create /proc/%s", PROC_SYM_NAME);
        proc_remove(proc_dir);
        proc_remove(proc_file);
        vfree(cookie_buf);
        return -ENOMEM;
    }

    printk(KERN_INFO "fortune: module loaded");
    return 0;
}

static void __exit fortune_exit(void)
{
    if (proc_symlink_entry)
        proc_remove(proc_symlink_entry);

    if (proc_file)
        proc_remove(proc_file);

    if (proc_dir)
        proc_remove(proc_dir);

    if (cookie_buf)
        vfree(cookie_buf);

    printk(KERN_INFO "fortune: module unloaded");
}

module_init(fortune_init);
module_exit(fortune_exit);
