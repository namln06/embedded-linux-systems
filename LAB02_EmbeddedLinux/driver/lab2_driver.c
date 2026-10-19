#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "lab2"
#define CLASS_NAME  "lab2_class"
#define LAB2_MAJOR  240
#define BUFFER_SIZE 1024

static dev_t lab2_devno;
static struct cdev lab2_cdev;
static struct class *lab2_class;
static struct device *lab2_device;

static char device_buffer[BUFFER_SIZE];
static size_t data_len;
static int open_count;
static DEFINE_MUTEX(buffer_mutex);

static struct proc_dir_entry *proc_entry;

/* Character device operations */
static int lab2_open(struct inode *inode, struct file *file)
{
    mutex_lock(&buffer_mutex);
    open_count++;
    mutex_unlock(&buffer_mutex);

    pr_info("lab2: device opened\n");
    return 0;
}

static int lab2_release(struct inode *inode, struct file *file)
{
    mutex_lock(&buffer_mutex);
    if (open_count > 0)
        open_count--;
    mutex_unlock(&buffer_mutex);

    pr_info("lab2: device closed\n");
    return 0;
}

static ssize_t lab2_read(struct file *file, char __user *user_buffer,
                         size_t count, loff_t *position)
{
    size_t available;
    size_t bytes_to_read;

    if (mutex_lock_interruptible(&buffer_mutex))
        return -ERESTARTSYS;

    if (*position >= data_len) {
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
    mutex_unlock(&buffer_mutex);

    pr_info("lab2: read %zu bytes\n", bytes_to_read);
    return bytes_to_read;
}

static ssize_t lab2_write(struct file *file, const char __user *user_buffer,
                          size_t count, loff_t *position)
{
    size_t bytes_to_write;

    if (mutex_lock_interruptible(&buffer_mutex))
        return -ERESTARTSYS;

    if (*position >= BUFFER_SIZE - 1) {
        mutex_unlock(&buffer_mutex);
        return -ENOSPC;
    }

    bytes_to_write = min(count, (size_t)(BUFFER_SIZE - 1 - *position));

    if (copy_from_user(device_buffer + *position, user_buffer,
                       bytes_to_write)) {
        mutex_unlock(&buffer_mutex);
        return -EFAULT;
    }

    *position += bytes_to_write;
    device_buffer[*position] = '\0';
    data_len = *position;

    mutex_unlock(&buffer_mutex);

    pr_info("lab2: wrote %zu bytes\n", bytes_to_write);
    return bytes_to_write;
}

static const struct file_operations lab2_fops = {
    .owner   = THIS_MODULE,
    .open    = lab2_open,
    .release = lab2_release,
    .read    = lab2_read,
    .write   = lab2_write,
    .llseek  = default_llseek,
};

/* procfs: /proc/lab2_info */
static int lab2_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&buffer_mutex);

    seq_printf(m, "LAB-02 Driver Statistics\n");
    seq_printf(m, "=========================\n");
    seq_printf(m, "Driver name  : %s\n", DEVICE_NAME);
    seq_printf(m, "Major number : %d\n", LAB2_MAJOR);
    seq_printf(m, "Buffer size  : %d bytes\n", BUFFER_SIZE);
    seq_printf(m, "Data length  : %zu bytes\n", data_len);
    seq_printf(m, "Open count   : %d\n", open_count);

    if (data_len > 0)
        seq_printf(m, "Last data    : [%s]\n", device_buffer);

    mutex_unlock(&buffer_mutex);
    return 0;
}

static int lab2_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, lab2_proc_show, NULL);
}

static const struct proc_ops lab2_proc_fops = {
    .proc_open    = lab2_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* sysfs attributes: read-only */
static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    size_t len;

    mutex_lock(&buffer_mutex);
    len = data_len;
    mutex_unlock(&buffer_mutex);

    return sysfs_emit(buf, "%zu\n", len);
}

static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    int count;

    mutex_lock(&buffer_mutex);
    count = open_count;
    mutex_unlock(&buffer_mutex);

    return sysfs_emit(buf, "%d\n", count);
}

static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    ssize_t ret;

    mutex_lock(&buffer_mutex);
    if (data_len > 0)
        ret = sysfs_emit(buf, "%s\n", device_buffer);
    else
        ret = sysfs_emit(buf, "(empty)\n");
    mutex_unlock(&buffer_mutex);

    return ret;
}

static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);

static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    NULL,
};

static const struct attribute_group lab2_attr_group = {
    .attrs = lab2_attrs,
};

/* Module initialization */
static int __init lab2_init(void)
{
    int ret;

    lab2_devno = MKDEV(LAB2_MAJOR, 0);

    ret = register_chrdev_region(lab2_devno, 1, DEVICE_NAME);
    if (ret) {
        pr_err("lab2: register_chrdev_region failed: %d\n", ret);
        return ret;
    }

    cdev_init(&lab2_cdev, &lab2_fops);
    lab2_cdev.owner = THIS_MODULE;

    ret = cdev_add(&lab2_cdev, lab2_devno, 1);
    if (ret) {
        pr_err("lab2: cdev_add failed: %d\n", ret);
        goto err_unregister;
    }

    lab2_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(lab2_class)) {
        ret = PTR_ERR(lab2_class);
        pr_err("lab2: class_create failed: %d\n", ret);
        goto err_cdev;
    }

    lab2_device = device_create(lab2_class, NULL, lab2_devno,
                                NULL, DEVICE_NAME);
    if (IS_ERR(lab2_device)) {
        ret = PTR_ERR(lab2_device);
        pr_err("lab2: device_create failed: %d\n", ret);
        goto err_class;
    }

    ret = sysfs_create_group(&lab2_device->kobj, &lab2_attr_group);
    if (ret) {
        pr_err("lab2: sysfs_create_group failed: %d\n", ret);
        goto err_device;
    }

    proc_entry = proc_create("lab2_info", 0444, NULL, &lab2_proc_fops);
    if (!proc_entry)
        pr_warn("lab2: failed to create /proc/lab2_info\n");
    else
        pr_info("lab2: /proc/lab2_info created\n");

    pr_info("lab2: loaded, major=%d minor=0\n", LAB2_MAJOR);
    return 0;

err_device:
    device_destroy(lab2_class, lab2_devno);
err_class:
    class_destroy(lab2_class);
err_cdev:
    cdev_del(&lab2_cdev);
err_unregister:
    unregister_chrdev_region(lab2_devno, 1);
    return ret;
}

/* Module cleanup */
static void __exit lab2_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);

    sysfs_remove_group(&lab2_device->kobj, &lab2_attr_group);
    device_destroy(lab2_class, lab2_devno);
    class_destroy(lab2_class);
    cdev_del(&lab2_cdev);
    unregister_chrdev_region(lab2_devno, 1);

    pr_info("lab2: unloaded\n");
}

module_init(lab2_init);
module_exit(lab2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("LAB-02 character driver with procfs and sysfs");
MODULE_VERSION("2.0");
