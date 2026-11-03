#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DRIVER_NAME "counter"
#define PROC_NAME   "counter_info"
#define INPUT_SIZE  64

static atomic_t counter_value = ATOMIC_INIT(0);
static atomic_t total_reads = ATOMIC_INIT(0);
static atomic_t total_increments = ATOMIC_INIT(0);
static atomic_t total_writes = ATOMIC_INIT(0);
static atomic_t total_resets = ATOMIC_INIT(0);

/*
 * Serialize read/write transactions so a read returns a consistent
 * counter value and increments it only after successfully copying data.
 */
static DEFINE_MUTEX(counter_mutex);

static ssize_t counter_read(struct file *file,
                            char __user *user_buf,
                            size_t count,
                            loff_t *ppos)
{
    char output[32];
    int current_value;
    int len;
    size_t bytes;

    if (*ppos > 0)
        return 0;

    if (count == 0)
        return 0;

    mutex_lock(&counter_mutex);

    current_value = atomic_read(&counter_value);
    len = scnprintf(output, sizeof(output), "%d\n", current_value);
    bytes = min_t(size_t, count, len);

    if (copy_to_user(user_buf, output, bytes)) {
        mutex_unlock(&counter_mutex);
        return -EFAULT;
    }

    *ppos += bytes;
    atomic_inc(&total_reads);
    atomic_inc(&counter_value);
    atomic_inc(&total_increments);

    mutex_unlock(&counter_mutex);

    pr_info("counter: read value=%d, next=%d\n",
            current_value, atomic_read(&counter_value));

    return bytes;
}

static ssize_t counter_write(struct file *file,
                             const char __user *user_buf,
                             size_t count,
                             loff_t *ppos)
{
    char input[INPUT_SIZE];
    int value;
    int ret;

    if (count == 0)
        return 0;

    /*
     * Reserve one byte for the terminating NUL.
     * Oversized input is rejected instead of silently truncated.
     */
    if (count >= sizeof(input))
        return -EINVAL;

    if (copy_from_user(input, user_buf, count))
        return -EFAULT;

    input[count] = '\0';

    /* Permit a trailing newline from echo or a terminal. */
    if (count > 0 && input[count - 1] == '\n')
        input[count - 1] = '\0';

    ret = kstrtoint(input, 10, &value);
    if (ret || value < 0)
        return -EINVAL;

    mutex_lock(&counter_mutex);

    atomic_set(&counter_value, value);
    atomic_inc(&total_writes);

    if (value == 0)
        atomic_inc(&total_resets);

    mutex_unlock(&counter_mutex);

    pr_info("counter: write set value=%d\n", value);

    return count;
}

static const struct file_operations counter_fops = {
    .owner  = THIS_MODULE,
    .read   = counter_read,
    .write  = counter_write,
    .llseek = no_llseek,
};

static struct miscdevice counter_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DRIVER_NAME,
    .fops  = &counter_fops,
    .mode  = 0666,
};

static int counter_info_show(struct seq_file *m, void *v)
{
    seq_printf(m, "current_counter: %d\n",
               atomic_read(&counter_value));
    seq_printf(m, "total_reads: %d\n",
               atomic_read(&total_reads));
    seq_printf(m, "total_increments: %d\n",
               atomic_read(&total_increments));
    seq_printf(m, "total_writes: %d\n",
               atomic_read(&total_writes));
    seq_printf(m, "total_resets: %d\n",
               atomic_read(&total_resets));

    return 0;
}

static int counter_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_info_show, NULL);
}

static const struct proc_ops counter_info_fops = {
    .proc_open    = counter_info_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init counter_init(void)
{
    int ret;

    ret = misc_register(&counter_misc_device);
    if (ret) {
        pr_err("counter: misc_register failed: %d\n", ret);
        return ret;
    }

    if (!proc_create(PROC_NAME, 0444, NULL, &counter_info_fops)) {
        misc_deregister(&counter_misc_device);
        pr_err("counter: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    pr_info("counter: loaded, device=/dev/%s, proc=/proc/%s\n",
            DRIVER_NAME, PROC_NAME);

    return 0;
}

static void __exit counter_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    misc_deregister(&counter_misc_device);

    pr_info("counter: unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Le Nhat Nam - SE205267");
MODULE_DESCRIPTION("Assignment 01: misc counter device with atomic statistics");
MODULE_VERSION("1.0");
