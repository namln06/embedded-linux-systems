#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>

#define SAMPLE_COUNT 1000
#define PERIOD_US    1000

static uint64_t jitter_ns[SAMPLE_COUNT];

static int compare_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;

    return (x > y) - (x < y);
}

static void add_ns(struct timespec *t, long ns)
{
    t->tv_nsec += ns;

    while (t->tv_nsec >= 1000000000L) {
        t->tv_nsec -= 1000000000L;
        t->tv_sec++;
    }
}

static uint64_t diff_ns(struct timespec end, struct timespec start)
{
    int64_t diff = (int64_t)(end.tv_sec - start.tv_sec) * 1000000000LL
                 + end.tv_nsec - start.tv_nsec;

    return diff > 0 ? (uint64_t)diff : 0;
}

int main(int argc, char *argv[])
{
    int policy = SCHED_OTHER;
    const char *policy_name = "SCHED_OTHER";
    int priority = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "fifo") == 0) {
            policy = SCHED_FIFO;
            policy_name = "SCHED_FIFO";
            priority = 50;
        } else if (strcmp(argv[1], "other") != 0) {
            fprintf(stderr, "Usage: %s [other|fifo]\n", argv[0]);
            return 1;
        }
    }

    struct sched_param param = {
        .sched_priority = priority
    };

    if (sched_setscheduler(0, policy, &param) == -1) {
        perror("sched_setscheduler");
        return 1;
    }

    printf("Scheduling policy: %s\n", policy_name);
    printf("Priority: %d\n", priority);

    if (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) {
        printf("Memory locking: SUCCESS\n");
    } else {
        printf("Memory locking: FAILED (%s)\n", strerror(errno));
    }

    struct timespec deadline;
    struct timespec actual;

    if (clock_gettime(CLOCK_MONOTONIC, &deadline) == -1) {
        perror("clock_gettime");
        return 1;
    }

    long period_ns = PERIOD_US * 1000L;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        add_ns(&deadline, period_ns);

        int ret;
        do {
            ret = clock_nanosleep(
                CLOCK_MONOTONIC,
                TIMER_ABSTIME,
                &deadline,
                NULL
            );
        } while (ret == EINTR);

        if (ret != 0) {
            fprintf(stderr, "clock_nanosleep: %s\n", strerror(ret));
            return 1;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &actual) == -1) {
            perror("clock_gettime");
            return 1;
        }

        jitter_ns[i] = diff_ns(actual, deadline);
    }

    qsort(jitter_ns, SAMPLE_COUNT, sizeof(uint64_t), compare_u64);

    uint64_t min = jitter_ns[0];
    uint64_t max = jitter_ns[SAMPLE_COUNT - 1];
    uint64_t p99 = jitter_ns[989];

    long double sum = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        sum += jitter_ns[i];
    }

    printf("\n===== JITTER REPORT =====\n");
    printf("Samples : %d\n", SAMPLE_COUNT);
    printf("Period  : %d us\n", PERIOD_US);
    printf("Min     : %.3f us\n", min / 1000.0);
    printf("Average : %.3Lf us\n", sum / SAMPLE_COUNT / 1000.0);
    printf("Max     : %.3f us\n", max / 1000.0);
    printf("P99     : %.3f us\n", p99 / 1000.0);

    int histogram[5] = {0};

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        double us = jitter_ns[i] / 1000.0;

        if (us < 50)
            histogram[0]++;
        else if (us < 100)
            histogram[1]++;
        else if (us < 200)
            histogram[2]++;
        else if (us < 500)
            histogram[3]++;
        else
            histogram[4]++;
    }

    printf("\n===== JITTER HISTOGRAM =====\n");
    printf("  0 -  50 us : %d\n", histogram[0]);
    printf(" 50 - 100 us : %d\n", histogram[1]);
    printf("100 - 200 us : %d\n", histogram[2]);
    printf("200 - 500 us : %d\n", histogram[3]);
    printf("     >=500 us: %d\n", histogram[4]);

    munlockall();
    return 0;
}
