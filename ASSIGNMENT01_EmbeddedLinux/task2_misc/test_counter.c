#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define DEVICE "/dev/counter"
#define PROC_INFO "/proc/counter_info"

static int failures = 0;

static void check(int condition, const char *message)
{
    if (condition) {
        printf("[PASS] %s\n", message);
    } else {
        printf("[FAIL] %s\n", message);
        failures++;
    }
}

static int read_counter(int *value)
{
    char buffer[32];
    int fd = open(DEVICE, O_RDONLY);
    if (fd < 0)
        return -1;

    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (n <= 0)
        return -1;

    buffer[n] = '\0';
    return (sscanf(buffer, "%d", value) == 1) ? 0 : -1;
}

static int write_counter(const char *value)
{
    int fd = open(DEVICE, O_WRONLY);
    if (fd < 0)
        return -1;

    ssize_t n = write(fd, value, strlen(value));
    int saved_errno = errno;
    close(fd);

    if (n < 0) {
        errno = saved_errno;
        return -1;
    }

    return (int)n;
}

static void test_eof(void)
{
    char buffer[32];
    int fd = open(DEVICE, O_RDONLY);
    if (fd < 0) {
        check(0, "Open device for EOF test");
        return;
    }

    ssize_t first = read(fd, buffer, sizeof(buffer));
    ssize_t second = read(fd, buffer, sizeof(buffer));

    close(fd);

    check(first > 0, "First read returns data");
    check(second == 0, "Second read returns EOF");
}

static void test_proc(void)
{
    char buffer[512];
    int fd = open(PROC_INFO, O_RDONLY);

    if (fd < 0) {
        check(0, "Open /proc/counter_info");
        return;
    }

    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (n < 0) {
        check(0, "Read /proc/counter_info");
        return;
    }

    buffer[n] = '\0';

    printf("\n----- /proc/counter_info -----\n%s", buffer);
    printf("------------------------------\n");

    check(strstr(buffer, "current_counter:") != NULL,
          "Proc reports current counter");
    check(strstr(buffer, "total_reads:") != NULL,
          "Proc reports total reads");
    check(strstr(buffer, "total_increments:") != NULL,
          "Proc reports total increments");
    check(strstr(buffer, "total_writes:") != NULL,
          "Proc reports total writes");
    check(strstr(buffer, "total_resets:") != NULL,
          "Proc reports total resets");
}

int main(void)
{
    int value = -1;

    printf("====================================\n");
    printf(" Assignment 01 - Task 2 counter test\n");
    printf("====================================\n");

    check(read_counter(&value) == 0 && value == 0,
          "Initial counter value is zero");

    check(read_counter(&value) == 0 && value == 1,
          "Second read returns one");

    check(write_counter("10\n") == 3,
          "Write decimal value 10");

    check(read_counter(&value) == 0 && value == 10,
          "Read returns written value 10");

    check(write_counter("-5\n") < 0 && errno == EINVAL,
          "Negative value is rejected with EINVAL");

    check(write_counter("abc\n") < 0 && errno == EINVAL,
          "Non-numeric value is rejected with EINVAL");

    check(write_counter("0\n") == 2,
          "Write zero to reset counter");

    check(read_counter(&value) == 0 && value == 0,
          "Counter reads zero after reset");

    test_eof();
    test_proc();

    printf("\n====================================\n");
    if (failures == 0)
        printf("Result: PASS (0 failures)\n");
    else
        printf("Result: FAIL (%d failures)\n", failures);
    printf("====================================\n");

    return failures ? 1 : 0;
}
