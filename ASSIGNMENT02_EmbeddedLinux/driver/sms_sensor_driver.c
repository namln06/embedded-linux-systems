/*
 * sms_sensor_driver.c
 * Sensor Monitoring System - Kernel Sensor Driver
 * Author: Le Nhat Nam
 * Date: 2026-11-03
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/timekeeping.h>

#include "sms_sensor.h"

#define DEVICE_NAME "sms_sensor"
#define PROC_NAME   "sms_stats"
#define DEFAULT_INTERVAL_MS 100

static int major_number;
static struct proc_dir_entry *proc_entry;

static DEFINE_MUTEX(sensor_mutex);

static unsigned int interval_ms = DEFAULT_INTERVAL_MS;
static unsigned long read_count;
static unsigned long long start_time_ms;

static int last_temperature;
static int last_humidity;
static unsigned long long last_read_ms;

static int sensor_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int sensor_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t sensor_read(struct file *file,
                           char __user *buffer,
                           size_t count,
                           loff_t *offset)
{
    char data[128];
    int len;
    unsigned int random_temp;
    unsigned int random_humidity;
    unsigned long long timestamp;
    unsigned int delay_ms;


    mutex_lock(&sensor_mutex);
    delay_ms = interval_ms;
    mutex_unlock(&sensor_mutex);

    if (last_read_ms != 0) {
        unsigned long long now = ktime_to_ms(ktime_get());

        if (now - last_read_ms < delay_ms)
            msleep(delay_ms - (unsigned int)(now - last_read_ms));
    }

    random_temp = get_random_u32() % 3001;
    random_humidity = get_random_u32() % 6001;

    timestamp = ktime_to_ms(ktime_get());

    mutex_lock(&sensor_mutex);

    last_temperature = 1500 + random_temp;
    last_humidity = 3000 + random_humidity;
    last_read_ms = timestamp;
    read_count++;

    len = scnprintf(data, sizeof(data), "%llu,%d.%02d,%d.%02d\n",
                    timestamp, last_temperature / 100, last_temperature % 100,
                    last_humidity / 100, last_humidity % 100);

    mutex_unlock(&sensor_mutex);

    if (count < len)
        return -EINVAL;

    if (copy_to_user(buffer, data, len))
        return -EFAULT;


    return len;
}

static long sensor_ioctl(struct file *file,
                         unsigned int cmd,
                         unsigned long arg)
{
    unsigned int new_interval;

    if (cmd != SMS_SET_INTERVAL)
        return -ENOTTY;

    if (copy_from_user(&new_interval,
                       (unsigned int __user *)arg,
                       sizeof(new_interval)))
        return -EFAULT;

    if (new_interval < 1 || new_interval > 60000)
        return -EINVAL;

    mutex_lock(&sensor_mutex);
    interval_ms = new_interval;
    mutex_unlock(&sensor_mutex);

    return 0;
}

static const struct file_operations sensor_fops = {
    .open = sensor_open,
    .read = sensor_read,
    .release = sensor_release,
    .unlocked_ioctl = sensor_ioctl,
};

static int sms_stats_show(struct seq_file *m, void *v)
{
    unsigned long count;
    unsigned int interval;
    int temperature;
    int humidity;
    unsigned long long uptime;

    mutex_lock(&sensor_mutex);

    count = read_count;
    interval = interval_ms;
    temperature = last_temperature;
    humidity = last_humidity;
    uptime = (unsigned int)(ktime_to_ms(ktime_get()) - start_time_ms) / 1000U;

    mutex_unlock(&sensor_mutex);

    seq_printf(m, "read_count: %lu\n", count);
    seq_printf(m, "last_temperature: %d.%02d\n", temperature / 100, temperature % 100);
    seq_printf(m, "last_humidity: %d.%02d\n", humidity / 100, humidity % 100);
    seq_printf(m, "uptime: %llu seconds\n", uptime);
    seq_printf(m, "interval_ms: %u\n", interval);

    return 0;
}

static int sms_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, sms_stats_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open = sms_stats_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init sms_sensor_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &sensor_fops);

    if (major_number < 0)
        return major_number;

    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_fops);

    if (!proc_entry) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return -ENOMEM;
    }

    start_time_ms = ktime_to_ms(ktime_get());

    pr_info("sms_sensor: registered major=%d\n", major_number);

    return 0;
}

static void __exit sms_sensor_exit(void)
{
    proc_remove(proc_entry);
    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("sms_sensor: unloaded\n");
}

module_init(sms_sensor_init);
module_exit(sms_sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Le Nhat Nam");
MODULE_DESCRIPTION("SMS temperature and humidity sensor driver");
MODULE_VERSION("1.0");
