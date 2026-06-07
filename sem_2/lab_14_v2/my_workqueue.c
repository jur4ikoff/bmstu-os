#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/input.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yaroslavd");

#define DIR_NAME "key_buf_wq"

static const char *ascii[] = {
    "Reserved", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace",
    "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Left Ctrl",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", "Left Shift", "\\",
    "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/", "Right Shift", "*",
    "Left Alt", "Space", "CapsLock", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "NumLock", "ScrollLock", "Keypad 7", "Up", "Keypad 9", "-", "Left", "Keypad 5", "Right", "+",
    "End", "Down", "Page Down", "Insert", "Delete"
};

static struct workqueue_struct *my_wq1;
static struct workqueue_struct *my_wq2;

struct my_work_struct {
    struct work_struct work;
    ktime_t start_time;
    int code;
};

static struct my_work_struct work_item1;
static struct my_work_struct work_item2;

static char last_key_name[32];
static int last_key_code = -1;
static DEFINE_SPINLOCK(data_lock);

static struct proc_dir_entry *proc_entry = NULL;

void work1_func(struct work_struct *work)
{
    struct my_work_struct *mw = container_of(work, struct my_work_struct, work);
    ktime_t end_time;
    s64 diff_ns;
    const char *kname = "Unknown";

    printk(KERN_INFO "+ [WQ1] Begin processing\n");

    if (mw->code < ARRAY_SIZE(ascii)) {
        kname = ascii[mw->code];
    }
    
    end_time = ktime_get();
    diff_ns = ktime_to_ns(ktime_sub(end_time, mw->start_time));

    spin_lock(&data_lock);
    last_key_code = mw->code;
    strncpy(last_key_name, kname, sizeof(last_key_name) - 1);
    last_key_name[sizeof(last_key_name) - 1] = '\0';
    spin_unlock(&data_lock);

    printk(KERN_INFO "+ [WQ1] Key: %s Code: %d Execution time: %lld ns\n", 
           last_key_name, last_key_code, diff_ns);
    printk(KERN_INFO "+ [WQ1] End\n");
}

void work2_func(struct work_struct *work)
{
    struct my_work_struct *mw = container_of(work, struct my_work_struct, work);
    const char *kname = "Unknown";

    printk(KERN_INFO "+ [WQ2] Begin processing\n");
    
    msleep(10); 

    if (mw->code < ARRAY_SIZE(ascii)) {
        kname = ascii[mw->code];
    }

    spin_lock(&data_lock);
    last_key_code = mw->code;
    strncpy(last_key_name, kname, sizeof(last_key_name) - 1);
    last_key_name[sizeof(last_key_name) - 1] = '\0';
    spin_unlock(&data_lock);

    printk(KERN_INFO "+ [WQ2] Key: %s Code: %d\n", last_key_name, last_key_code);
    printk(KERN_INFO "+ [WQ2] End\n");
}

static bool input_handler(struct input_handle *handle, unsigned int type, unsigned int code, int value)
{
    if (type == EV_KEY && value == 1) {
        work_item1.start_time = ktime_get();
        work_item1.code = code;
        
        work_item2.start_time = ktime_get();
        work_item2.code = code;

        queue_work(my_wq1, &work_item1.work);
        queue_work(my_wq2, &work_item2.work);
    }
    return false;
}

static int my_input_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id)
{
    struct input_handle *handle;
    int error;

    handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;

    handle->dev = dev;
    handle->handler = handler;
    handle->name = "my_workqueue";

    error = input_register_handle(handle);
    if (error)
        goto err_free;

    error = input_open_device(handle);
    if (error)
        goto err_unregister;

    printk(KERN_INFO "+ Connected to input device: %s\n", dev->name);
    return 0;

err_unregister:
    input_unregister_handle(handle);
err_free:
    kfree(handle);
    return error;
}

static void my_input_disconnect(struct input_handle *handle)
{
    printk(KERN_INFO "+ Disconnected from input device: %s\n", handle->dev->name);
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
}

static const struct input_device_id my_ids[] = {
    { .flags = INPUT_DEVICE_ID_MATCH_EVBIT, .evbit = { BIT_MASK(EV_KEY) } },
    { }
};

static struct input_handler my_handler = {
    .filter = input_handler,
    .connect = my_input_connect,
    .disconnect = my_input_disconnect,
    .name = "my_wq_logger",
    .id_table = my_ids,
};

static int key_buf_show(struct seq_file *m, void *v)
{
    spin_lock(&data_lock);
    if (last_key_code != -1)
        seq_printf(m, "Key: %s Code: %d\n", last_key_name, last_key_code);
    else
        seq_printf(m, "No keys pressed yet\n");
    spin_unlock(&data_lock);
    return 0;
}

static int key_buf_open(struct inode *inode, struct file *file)
{
    return single_open(file, key_buf_show, NULL);
}

static const struct proc_ops key_buf_fops = {
    .proc_open = key_buf_open,
    .proc_read = seq_read,
    .proc_release = single_release,
};

static int __init my_workqueue_init(void)
{
    int ret;

    printk(KERN_INFO "+ Module loading...\n");

    my_wq1 = alloc_workqueue("my_wq1", WQ_MEM_RECLAIM, 1);
    my_wq2 = alloc_workqueue("my_wq2", WQ_MEM_RECLAIM, 1);

    if (!my_wq1 || !my_wq2) {
        printk(KERN_ERR "+ Create queue error\n");
        if (my_wq1) destroy_workqueue(my_wq1);
        if (my_wq2) destroy_workqueue(my_wq2);
        return -ENOMEM;
    }

    INIT_WORK(&work_item1.work, work1_func);
    INIT_WORK(&work_item2.work, work2_func);

    ret = input_register_handler(&my_handler);
    if (ret) {
        printk(KERN_ERR "Failed to register input handler: %d\n", ret);
        destroy_workqueue(my_wq1);
        destroy_workqueue(my_wq2);
        return ret;
    }

    proc_entry = proc_create(DIR_NAME, 0444, NULL, &key_buf_fops);
    if (!proc_entry) {
        printk(KERN_ERR "+ Failed to create proc entry\n");
        input_unregister_handler(&my_handler);
        destroy_workqueue(my_wq1);
        destroy_workqueue(my_wq2);
        return -ENOMEM;
    }

    printk(KERN_INFO "+ Module loaded successfully.\n");
    return 0;
}

static void __exit my_workqueue_exit(void)
{
    printk(KERN_INFO "+ Module exiting...\n");

    if (proc_entry)
        proc_remove(proc_entry);

    input_unregister_handler(&my_handler);

    flush_workqueue(my_wq1);
    flush_workqueue(my_wq2);

    destroy_workqueue(my_wq1);
    destroy_workqueue(my_wq2);

    printk(KERN_INFO "+ Module unloaded.\n");
}

module_init(my_workqueue_init);
module_exit(my_workqueue_exit);
