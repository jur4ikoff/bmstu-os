#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/input.h>

#include "key.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Popov");

static char last_key_name[32];
static int last_key_code = -1;
static ktime_t irq_start_time;

static void my_tasklet_fun(struct tasklet_struct *t) 
{
    ktime_t end_time = ktime_get();
    s64 nsecs = ktime_to_ns(ktime_sub(end_time, irq_start_time));
    
    printk(KERN_INFO "+ [TASKLET] Key: %s (Code: %d) Time: %lld ns\n", 
           last_key_name, last_key_code, nsecs);
}

DECLARE_TASKLET(my_tasklet, my_tasklet_fun);

static bool input_handler(struct input_handle *handle, unsigned int type, unsigned int code, int value)
{
    if (type == EV_KEY && value == 0) {
        irq_start_time = ktime_get();
        last_key_code = code;
        
        strncpy(last_key_name, get_key_name(code), sizeof(last_key_name) - 1);
        last_key_name[sizeof(last_key_name) - 1] = '\0';

        tasklet_schedule(&my_tasklet);
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
    handle->name = "my_tasklet";

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
    .name = "my_key_logger",
    .id_table = my_ids,
};

static int __init my_init(void) 
{
    int error;
    
    printk(KERN_INFO "+ Module tasklet loading\n");
    
    error = input_register_handler(&my_handler);
    if (error) {
        printk(KERN_ERR "Failed to register input handler: %d\n", error);
        return error;
    }

    printk(KERN_INFO "+ Module tasklet loaded\n");
    return 0;
}

static void __exit my_exit(void) 
{
    tasklet_kill(&my_tasklet);
    input_unregister_handler(&my_handler);
    
    printk(KERN_INFO "+ Module unloaded.\n");
}

module_init(my_init);
module_exit(my_exit);
