#include "sensor_thread.h"
#include "../driver/sms_sensor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>

void *sensor_thread(void *arg)
{
    sensor_thread_args_t *args = arg;

    struct sched_param sched = {
        .sched_priority = 50
    };

    int sched_ret = pthread_setschedparam(
        pthread_self(), SCHED_FIFO, &sched);

    if (sched_ret != 0) {
        fprintf(stderr, "SCHED_FIFO failed: %s\n",
                strerror(sched_ret));
    } else {
        printf("Sensor Thread: SCHED_FIFO priority 50 enabled\n");
    }

#ifdef SMS_HOST_TEST

    unsigned int seed = (unsigned int)time(NULL);

    while (*args->running) {
        sensor_sample_t sample = {0};
        struct timespec now;

        clock_gettime(CLOCK_MONOTONIC, &now);

        sample.timestamp_ms =
            (uint64_t)now.tv_sec * 1000 +
            now.tv_nsec / 1000000;

        sample.temperature =
            15.0f + (rand_r(&seed) % 3001) / 100.0f;

        sample.humidity =
            30.0f + (rand_r(&seed) % 6001) / 100.0f;

        if (buffer_push(args->buffer, sample) != 0)
            break;

        struct timespec delay = {
            .tv_sec = args->interval_ms / 1000,
            .tv_nsec = (long)(args->interval_ms % 1000) * 1000000L
        };

        while (nanosleep(&delay, &delay) == -1)
            ;
    }

#else

    char data[128];
    int fd = open(args->device_path, O_RDONLY);

    if (fd < 0) {
        perror("open sensor device");
        *args->running = 0;
        return NULL;
    }

    unsigned int interval = args->interval_ms;

    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0)
        perror("SMS_SET_INTERVAL");

    while (*args->running) {
        ssize_t n = read(fd, data, sizeof(data) - 1);

        if (n <= 0) {
            if (*args->running)
                perror("read sensor");
            break;
        }

        data[n] = '\0';

        sensor_sample_t sample = {0};
        unsigned long long timestamp;

        if (sscanf(data, "%llu,%f,%f",
                   &timestamp,
                   &sample.temperature,
                   &sample.humidity) != 3) {
            fprintf(stderr, "Invalid sensor data: %s\n", data);
            continue;
        }

        sample.timestamp_ms = timestamp;

        if (buffer_push(args->buffer, sample) != 0)
            break;
    }

    close(fd);

#endif

    return NULL;
}
