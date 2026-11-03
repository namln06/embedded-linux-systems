#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "../driver/sms_sensor.h"

int main(void)
{
    int fd;
    unsigned int interval;

    fd = open("/dev/sms_sensor", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    interval = 0;
    errno = 0;
    printf("Interval 0: ioctl=%d, errno=%d (%s)\n",
           ioctl(fd, SMS_SET_INTERVAL, &interval),
           errno, strerror(errno));

    interval = 60001;
    errno = 0;
    printf("Interval 60001: ioctl=%d, errno=%d (%s)\n",
           ioctl(fd, SMS_SET_INTERVAL, &interval),
           errno, strerror(errno));

    close(fd);
    return 0;
}
