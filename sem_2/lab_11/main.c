#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Popov");
MODULE_DESCRIPTION("md_proc3");

static struct proc_dir_entry *proc_file;
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_link;

#define POT_SIZE 4096
static char *cookie_pot;
static unsigned int write_index = 0;
static unsigned int read_index = 0;

static int fortune_open(struct inode *inode, struct file *file) {
  printk(KERN_INFO "fortune: open");
  return 0;
}

static int fortune_release(struct inode *inode, struct file *file) {
  printk(KERN_INFO "fortune: release");
  return 0;
}

static ssize_t fortune_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
  if (len + 1 > POT_SIZE - write_index)
    return -ENOSPC;
  if (copy_from_user(&cookie_pot[write_index], buffer, len)) {
    return -EFAULT;
  }
  write_index += len + 1;
  cookie_pot[write_index - 1] = '\0';
  *offset += len;
  printk(KERN_INFO "fortune: write: %s", &cookie_pot[write_index - len - 1]);
  return len;
}

static ssize_t fortune_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
  int read_len;

  if (*offset > 0 || write_index == 0)
    return 0;
  if (read_index >= write_index)
    read_index = 0;
  read_len = strlen(&cookie_pot[read_index]);
  if (read_len > len)
    read_len = len;
  if (copy_to_user(buffer, &cookie_pot[read_index], read_len) > 0)
    return -EFAULT;
  read_index += read_len + 1;
  *offset += read_len;
  printk(KERN_INFO "fortune: read: %s", &cookie_pot[read_index - read_len - 1]);
  return read_len;
}

static const struct proc_ops fortune_ops = {
    .proc_open = fortune_open,
    .proc_release = fortune_release,
    .proc_read = fortune_read,
    .proc_write = fortune_write,
};

static int __init fortune_init(void) {
  cookie_pot = vmalloc(POT_SIZE);
  if (cookie_pot == NULL)
    return -ENOMEM;
  memset(cookie_pot, 0, POT_SIZE);
  proc_file = proc_create("fortune", 0666, NULL, &fortune_ops);
  if (proc_file == NULL) {
    vfree(cookie_pot);
    return -ENOMEM;
  }
  proc_dir = proc_mkdir("fortune_dir", NULL);
  if (proc_dir == NULL) {
    proc_remove(proc_file);
    vfree(cookie_pot);
    return -ENOMEM;
  }
  proc_link = proc_symlink("fortune_link", NULL, "/proc/fortune");
  if (proc_link == NULL) {
    proc_remove(proc_dir);
    proc_remove(proc_file);
    vfree(cookie_pot);
    return -ENOMEM;
  }
  printk(KERN_INFO "fortune: module loaded");
  return 0;
}

static void __exit fortune_exit(void) {
  proc_remove(proc_link);
  proc_remove(proc_dir);
  proc_remove(proc_file);
  vfree(cookie_pot);
  printk(KERN_INFO "fortune: module unloaded");
}

module_init(fortune_init);
module_exit(fortune_exit);