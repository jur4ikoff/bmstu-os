#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/input.h>

#include "key.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Popov");

static struct workqueue_struct *my_wq;

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
    const char *kname;

    printk(KERN_INFO "+ [WQ1] Begin processing\n");

    kname = get_key_name(mw->code);

    end_time = ktime_get();
    diff_ns = ktime_to_ns(ktime_sub(end_time, mw->start_time));

    spin_lock(&data_lock);
    last_key_code = mw->code;
    strncpy(last_key_name, kname, sizeof(last_key_name) - 1);
    last_key_name[sizeof(last_key_name) - 1] = '\0';
    spin_unlock(&data_lock);

    printk(KERN_INFO "+ [WQ1] Key: %s Code: %d time: %lld ns\n",
           last_key_name, last_key_code, diff_ns);
    printk(KERN_INFO "+ [WQ1] End\n");
}

void work2_func(struct work_struct *work)
{
    struct my_work_struct *mw = container_of(work, struct my_work_struct, work);
    ktime_t end_time;
    s64 diff_ns;
    const char *kname;

    printk(KERN_INFO "+ [WQ2] Begin processing\n");

    msleep(10);

    kname = get_key_name(mw->code);

    end_time = ktime_get();
    diff_ns = ktime_to_ns(ktime_sub(end_time, mw->start_time));

    spin_lock(&data_lock);
    last_key_code = mw->code;
    strncpy(last_key_name, kname, sizeof(last_key_name) - 1);
    last_key_name[sizeof(last_key_name) - 1] = '\0';
    spin_unlock(&data_lock);

    printk(KERN_INFO "+ [WQ2] Key: %s Code: %d time: %lld ns\n",
           last_key_name, last_key_code, diff_ns);
    printk(KERN_INFO "+ [WQ2] End\n");
}

static bool input_handler(struct input_handle *handle,
                          unsigned int type,
                          unsigned int code,
                          int value)
{
    if (type == EV_KEY && value == 0) {
        work_item1.start_time = ktime_get();
        work_item1.code = code;

        work_item2.start_time = ktime_get();
        work_item2.code = code;

        queue_work(my_wq, &work_item1.work);
        queue_work(my_wq, &work_item2.work);
    }

    return false;
}

static int my_input_connect(struct input_handler *handler,
                            struct input_dev *dev,
                            const struct input_device_id *id)
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
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
        .evbit = { BIT_MASK(EV_KEY) }
    },
    { }
};

static struct input_handler my_handler = {
    .filter = input_handler,
    .connect = my_input_connect,
    .disconnect = my_input_disconnect,
    .name = "my_wq_logger",
    .id_table = my_ids,
};


static int __init my_workqueue_init(void)
{
    int ret;

    printk(KERN_INFO "+ load my_workqueue\n");

    my_wq = alloc_workqueue("my_wq", WQ_MEM_RECLAIM, 1);
    if (!my_wq) {
        printk(KERN_ERR "+ Create queue error\n");
        return -ENOMEM;
    }

    INIT_WORK(&work_item1.work, work1_func);
    INIT_WORK(&work_item2.work, work2_func);

    ret = input_register_handler(&my_handler);
    if (ret) {
        printk(KERN_ERR "Failed to register input handler: %d\n", ret);
        destroy_workqueue(my_wq);
        return ret;
    }

    printk(KERN_INFO "+ Module workqueue loaded.\n");
    return 0;
}

static void __exit my_workqueue_exit(void)
{

    input_unregister_handler(&my_handler);

    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);

    printk(KERN_INFO "+ Module workqueue unloaded.\n");
}

module_init(my_workqueue_init);
module_exit(my_workqueue_exit);
