#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#include <linux/ioctl.h>

#define SMS_IOCTL_MAGIC 'S'
#define SMS_SET_INTERVAL _IOW(SMS_IOCTL_MAGIC, 1, unsigned int)

#endif
