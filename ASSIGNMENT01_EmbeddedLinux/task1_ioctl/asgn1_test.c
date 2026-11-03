#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "asgn1_ioctl.h"

#define DEVICE_PATH "/dev/asgn1"

static int failures;

static void check(int condition, const char *description)
{
    if (condition) {
        printf("[PASS] %s\n", description);
    } else {
        printf("[FAIL] %s\n", description);
        failures++;
    }
}

static int set_mode(int fd, int mode)
{
    return ioctl(fd, ASGN1_SET_MODE, &mode);
}

static int read_buffer(int fd, char *buf, size_t size)
{
    if (lseek(fd, 0, SEEK_SET) < 0)
        return -1;

    ssize_t n = read(fd, buf, size - 1);
    if (n < 0)
        return -1;

    buf[n] = '\0';
    return (int)n;
}

int main(void)
{
    int fd;
    int mode;
    char buffer[128];
    char version[64] = {0};
    struct asgn1_stats stats;
    const char *echo_text = "hello-echo";
    const char *kernel_text = "hello-kernel";

    printf("===== Assignment 01 ioctl test =====\n");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open " DEVICE_PATH);
        return 2;
    }

    /* Test 1: reset buffer and counters. */
    check(ioctl(fd, ASGN1_RESET_BUFFER) == 0,
          "ASGN1_RESET_BUFFER succeeds");

    memset(&stats, 0, sizeof(stats));
    check(ioctl(fd, ASGN1_GET_STATS, &stats) == 0,
          "ASGN1_GET_STATS succeeds");

    check(stats.buffer_len == 0 &&
          stats.write_count == 0 &&
          stats.read_count == 0,
          "Reset clears buffer and read/write counters");

    /* Test 2: version string. */
    check(ioctl(fd, ASGN1_GET_VERSION, version) == 0,
          "ASGN1_GET_VERSION succeeds");

    printf("Driver version: %s\n", version);
    check(version[0] != '\0',
          "Version string is not empty");

    /* Test 3: Echo mode stores raw data. */
    mode = ASGN1_MODE_ECHO;
    check(set_mode(fd, mode) == 0,
          "ASGN1_SET_MODE selects Echo mode");

    check(write(fd, echo_text, strlen(echo_text)) ==
          (ssize_t)strlen(echo_text),
          "Write succeeds in Echo mode");

    memset(buffer, 0, sizeof(buffer));
    int n = read_buffer(fd, buffer, sizeof(buffer));

    check(n == (int)strlen(echo_text) &&
          strcmp(buffer, echo_text) == 0,
          "Echo mode returns unmodified data");

    /* Test 4: Kernel mode prepends its required prefix. */
    mode = ASGN1_MODE_KERNEL;
    check(set_mode(fd, mode) == 0,
          "ASGN1_SET_MODE selects Kernel mode");

    check(write(fd, kernel_text, strlen(kernel_text)) ==
          (ssize_t)strlen(kernel_text),
          "Write succeeds in Kernel mode");

    memset(buffer, 0, sizeof(buffer));
    n = read_buffer(fd, buffer, sizeof(buffer));

    check(n >= 0 &&
          strcmp(buffer, "[KERNEL]hello-kernel") == 0,
          "Kernel mode prepends [KERNEL]");

    /* Test 5: statistics. */
    memset(&stats, 0, sizeof(stats));
    check(ioctl(fd, ASGN1_GET_STATS, &stats) == 0,
          "GET_STATS works after read/write operations");

    printf("Statistics: open=%d write=%d read=%d "
           "buffer_len=%d last_write_size=%d\n",
           stats.open_count, stats.write_count, stats.read_count,
           stats.buffer_len, stats.last_write_size);

    check(stats.open_count >= 1,
          "Open count records device opens");

    check(stats.write_count == 2,
          "Write count records both writes");

    check(stats.read_count == 2,
          "Read count records both successful reads");

    check(stats.buffer_len == (int)strlen("[KERNEL]hello-kernel"),
          "Buffer length includes the Kernel mode prefix");

    check(stats.last_write_size == (int)strlen(kernel_text),
          "Last write size records payload bytes");

    /* Test 6: invalid mode should be rejected. */
    mode = 99;
    errno = 0;
    check(set_mode(fd, mode) == -1 && errno == EINVAL,
          "Invalid mode is rejected with EINVAL");

    /* Test 7: unknown ioctl should return ENOTTY. */
    errno = 0;
    check(ioctl(fd, _IO(ASGN1_IOC_MAGIC, 99)) == -1 &&
          errno == ENOTTY,
          "Unknown ioctl returns ENOTTY");

    /* Test 8: reset again and verify state. */
    check(ioctl(fd, ASGN1_RESET_BUFFER) == 0,
          "Final reset succeeds");

    memset(&stats, 0, sizeof(stats));
    check(ioctl(fd, ASGN1_GET_STATS, &stats) == 0 &&
          stats.buffer_len == 0 &&
          stats.write_count == 0 &&
          stats.read_count == 0,
          "Final reset leaves an empty buffer and zero read/write counts");

    close(fd);

    printf("===== Result: %s (%d failure(s)) =====\n",
           failures == 0 ? "PASS" : "FAIL", failures);

    return failures == 0 ? 0 : 1;
}
