#ifndef ASGN1_IOCTL_H
#define ASGN1_IOCTL_H

#include <linux/ioctl.h>

/* Unique ioctl magic number for Assignment 01 */
#define ASGN1_IOC_MAGIC 'A'

/* Driver statistics returned to userspace */
struct asgn1_stats {
    int open_count;
    int write_count;
    int read_count;
    int buffer_len;
    int last_write_size;
};

/* Supported driver modes */
#define ASGN1_MODE_KERNEL 0
#define ASGN1_MODE_ECHO   1

/* Clear buffer and reset read/write counters */
#define ASGN1_RESET_BUFFER \
    _IO(ASGN1_IOC_MAGIC, 0)

/* Return driver statistics to userspace */
#define ASGN1_GET_STATS \
    _IOR(ASGN1_IOC_MAGIC, 1, struct asgn1_stats)

/* Set driver operating mode */
#define ASGN1_SET_MODE \
    _IOW(ASGN1_IOC_MAGIC, 2, int)

/* Return driver version string to userspace */
#define ASGN1_GET_VERSION \
    _IOR(ASGN1_IOC_MAGIC, 3, char[64])

#endif /* ASGN1_IOCTL_H */
