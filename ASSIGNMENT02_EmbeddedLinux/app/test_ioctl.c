#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

int main(void)
{
    int fd = open("/dev/sms_sensor", O_RDONLY);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    errno = 0;

    int ret = ioctl(fd, 0x12345678, 0);

    printf("ioctl return: %d\n", ret);
    printf("errno: %d (%s)\n", errno, strerror(errno));

    close(fd);
    return 0;
}
