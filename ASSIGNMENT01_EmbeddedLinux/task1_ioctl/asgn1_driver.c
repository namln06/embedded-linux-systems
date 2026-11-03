#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "asgn1_ioctl.h"

#define DEVICE_NAME "asgn1"
#define CLASS_NAME  "asgn1_class"
#define ASGN1_MAJOR 241
#define ASGN1_MINOR 0
#define BUFFER_SIZE 1024
#define ASGN1_VERSION "ASGN1 Driver v1.0"

static dev_t asgn1_devno;
static struct cdev asgn1_cdev;
static struct class *asgn1_class;
static struct device *asgn1_device;

static char device_buffer[BUFFER_SIZE];
static size_t data_len;
static int open_count;
static int write_count;
static int read_count;
static int last_write_size;
static int driver_mode = ASGN1_MODE_ECHO;

static DEFINE_MUTEX(buffer_mutex);

static int asgn1_open(struct inode *inode, struct file *file)
{
    if (mutex_lock_interruptible(&buffer_mutex))
        return -ERESTARTSYS;

    open_count++;
    mutex_unlock(&buffer_mutex);

    pr_info("asgn1: device opened\n");
    return 0;
}

static int asgn1_release(struct inode *inode, struct file *file)
{
    pr_info("asgn1: device closed\n");
    return 0;
}

static ssize_t asgn1_read(struct file *file, char __user *user_buffer,
                          size_t count, loff_t *position)
{
    size_t available;
    size_t bytes_to_read;

    if (mutex_lock_interruptible(&buffer_mutex))
        return -ERESTARTSYS;

    if (*position >= data_len || count == 0) {
        mutex_unlock(&buffer_mutex);
        return 0;
    }

    available = data_len - *position;
    bytes_to_read = min(count, available);

    if (copy_to_user(user_buffer, device_buffer + *position,
                     bytes_to_read)) {
        mutex_unlock(&buffer_mutex);
        return -EFAULT;
    }

    *position += bytes_to_read;
    read_count++;

    mutex_unlock(&buffer_mutex);

    pr_info("asgn1: read %zu bytes\n", bytes_to_read);
    return bytes_to_read;
}

static ssize_t asgn1_write(struct file *file, const char __user *user_buffer,
                           size_t count, loff_t *position)
{
    char input[BUFFER_SIZE];
    size_t input_len;
    size_t prefix_len = 0;
    size_t bytes_to_store;
    const char prefix[] = "[KERNEL]";

    if (count == 0)
        return 0;

    if (mutex_lock_interruptible(&buffer_mutex))
        return -ERESTARTSYS;

    if (driver_mode != ASGN1_MODE_ECHO &&
        driver_mode != ASGN1_MODE_KERNEL) {
        mutex_unlock(&buffer_mutex);
        return -EINVAL;
    }

    input_len = min(count, (size_t)(BUFFER_SIZE - 1));

    if (copy_from_user(input, user_buffer, input_len)) {
        mutex_unlock(&buffer_mutex);
        return -EFAULT;
    }

    if (driver_mode == ASGN1_MODE_KERNEL)
        prefix_len = sizeof(prefix) - 1;

    bytes_to_store = min(input_len, (size_t)(BUFFER_SIZE - 1 - prefix_len));

    memcpy(device_buffer, prefix, prefix_len);
    memcpy(device_buffer + prefix_len, input, bytes_to_store);

    data_len = prefix_len + bytes_to_store;
    device_buffer[data_len] = '\0';

    write_count++;
    last_write_size = bytes_to_store;

    /* Start reading from the beginning after each write. */
    *position = 0;

    mutex_unlock(&buffer_mutex);

    pr_info("asgn1: write accepted=%zu stored=%zu mode=%d\n",
            count, data_len, driver_mode);

    return bytes_to_store;
}

static long asgn1_ioctl(struct file *file, unsigned int cmd,
                        unsigned long arg)
{
    int mode;
    struct asgn1_stats stats;
    char version[64] = ASGN1_VERSION;

    switch (cmd) {
    case ASGN1_RESET_BUFFER:
        if (mutex_lock_interruptible(&buffer_mutex))
            return -ERESTARTSYS;

        memset(device_buffer, 0, sizeof(device_buffer));
        data_len = 0;
        write_count = 0;
        read_count = 0;
        last_write_size = 0;

        mutex_unlock(&buffer_mutex);
        pr_info("asgn1: ioctl RESET_BUFFER\n");
        return 0;

    case ASGN1_GET_STATS:
        if (mutex_lock_interruptible(&buffer_mutex))
            return -ERESTARTSYS;

        stats.open_count = open_count;
        stats.write_count = write_count;
        stats.read_count = read_count;
        stats.buffer_len = data_len;
        stats.last_write_size = last_write_size;

        mutex_unlock(&buffer_mutex);

        if (copy_to_user((void __user *)arg, &stats, sizeof(stats)))
            return -EFAULT;

        pr_info("asgn1: ioctl GET_STATS\n");
        return 0;

    case ASGN1_SET_MODE:
        if (copy_from_user(&mode, (int __user *)arg, sizeof(mode)))
            return -EFAULT;

        if (mode != ASGN1_MODE_KERNEL && mode != ASGN1_MODE_ECHO)
            return -EINVAL;

        if (mutex_lock_interruptible(&buffer_mutex))
            return -ERESTARTSYS;

        driver_mode = mode;

        mutex_unlock(&buffer_mutex);
        pr_info("asgn1: ioctl SET_MODE mode=%d\n", mode);
        return 0;

    case ASGN1_GET_VERSION:
        if (copy_to_user((void __user *)arg, version, sizeof(version)))
            return -EFAULT;

        pr_info("asgn1: ioctl GET_VERSION\n");
        return 0;

    default:
        pr_warn("asgn1: unsupported ioctl command=0x%x\n", cmd);
        return -ENOTTY;
    }
}

static const struct file_operations asgn1_fops = {
    .owner          = THIS_MODULE,
    .open           = asgn1_open,
    .release        = asgn1_release,
    .read            = asgn1_read,
    .write           = asgn1_write,
    .unlocked_ioctl  = asgn1_ioctl,
    .llseek          = default_llseek,
};

static int __init asgn1_init(void)
{
    int ret;

    asgn1_devno = MKDEV(ASGN1_MAJOR, ASGN1_MINOR);

    ret = register_chrdev_region(asgn1_devno, 1, DEVICE_NAME);
    if (ret) {
        pr_err("asgn1: register_chrdev_region failed: %d\n", ret);
        return ret;
    }

    cdev_init(&asgn1_cdev, &asgn1_fops);
    asgn1_cdev.owner = THIS_MODULE;

    ret = cdev_add(&asgn1_cdev, asgn1_devno, 1);
    if (ret)
        goto err_unregister;

    asgn1_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(asgn1_class)) {
        ret = PTR_ERR(asgn1_class);
        goto err_cdev;
    }

    asgn1_device = device_create(asgn1_class, NULL, asgn1_devno,
                                 NULL, DEVICE_NAME);
    if (IS_ERR(asgn1_device)) {
        ret = PTR_ERR(asgn1_device);
        goto err_class;
    }

    memset(device_buffer, 0, sizeof(device_buffer));
    data_len = 0;

    pr_info("asgn1: loaded version=%s major=%d minor=%d\n",
            ASGN1_VERSION, ASGN1_MAJOR, ASGN1_MINOR);
    return 0;

err_class:
    class_destroy(asgn1_class);
err_cdev:
    cdev_del(&asgn1_cdev);
err_unregister:
    unregister_chrdev_region(asgn1_devno, 1);
    pr_err("asgn1: initialization failed: %d\n", ret);
    return ret;
}

static void __exit asgn1_exit(void)
{
    device_destroy(asgn1_class, asgn1_devno);
    class_destroy(asgn1_class);
    cdev_del(&asgn1_cdev);
    unregister_chrdev_region(asgn1_devno, 1);

    pr_info("asgn1: unloaded and resources released\n");
}

module_init(asgn1_init);
module_exit(asgn1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("Assignment 01 character device driver with ioctl");
MODULE_VERSION("1.0");
