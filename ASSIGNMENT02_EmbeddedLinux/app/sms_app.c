#include "circular_buffer.h"
#include "sensor_thread.h"
#include "processor_thread.h"
#include "alert_thread.h"
#include "logger_thread.h"
#include "sms_messages.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define DEFAULT_INTERVAL 500
#define DEFAULT_TEMP_THRESHOLD 40.0f
#define DEFAULT_HUMID_THRESHOLD 80.0f
#define DEFAULT_DURATION 60

static volatile sig_atomic_t running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

static void print_usage(const char *name)
{
    printf("Usage: %s [-i ms] [-t temperature] [-h humidity] "
           "[-d seconds] [-v]\n", name);
}

int main(int argc, char *argv[])
{
    unsigned int interval = DEFAULT_INTERVAL;
    float temp_threshold = DEFAULT_TEMP_THRESHOLD;
    float humid_threshold = DEFAULT_HUMID_THRESHOLD;
    unsigned int duration = DEFAULT_DURATION;
    int verbose = 0;
    int option;

    while ((option = getopt(argc, argv, "i:t:h:d:v")) != -1) {
        switch (option) {
        case 'i':
            interval = (unsigned int)strtoul(optarg, NULL, 10);
            break;
        case 't':
            temp_threshold = strtof(optarg, NULL);
            break;
        case 'h':
            humid_threshold = strtof(optarg, NULL);
            break;
        case 'd':
            duration = (unsigned int)strtoul(optarg, NULL, 10);
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (interval == 0 || duration == 0) {
        fprintf(stderr, "Invalid interval or duration\n");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct mq_attr alarm_attr = {0};
    alarm_attr.mq_maxmsg = 10;
    alarm_attr.mq_msgsize = sizeof(sms_alarm_t);

    struct mq_attr log_attr = {0};
    log_attr.mq_maxmsg = 10;
    log_attr.mq_msgsize = sizeof(sms_log_message_t);

    mq_unlink(SMS_ALARM_QUEUE);
    mq_unlink(SMS_LOG_QUEUE);

    mqd_t alarm_queue = mq_open(SMS_ALARM_QUEUE,
                                O_CREAT | O_RDWR, 0600, &alarm_attr);

    if (alarm_queue == (mqd_t)-1) {
        perror("mq_open alarm");
        return EXIT_FAILURE;
    }

    mqd_t log_queue = mq_open(SMS_LOG_QUEUE,
                              O_CREAT | O_RDWR, 0600, &log_attr);

    if (log_queue == (mqd_t)-1) {
        perror("mq_open log");
        mq_close(alarm_queue);
        mq_unlink(SMS_ALARM_QUEUE);
        return EXIT_FAILURE;
    }

    circular_buffer_t buffer;

    if (buffer_init(&buffer) != 0) {
        fprintf(stderr, "Buffer initialization failed\n");
        mq_close(alarm_queue);
        mq_close(log_queue);
        mq_unlink(SMS_ALARM_QUEUE);
        mq_unlink(SMS_LOG_QUEUE);
        return EXIT_FAILURE;
    }

    sensor_thread_args_t sensor_args = {
        .buffer = &buffer,
        .device_path = "/dev/sms_sensor",
        .interval_ms = interval,
        .running = &running
    };

    processor_thread_args_t processor_args = {
        .buffer = &buffer,
        .alarm_queue = alarm_queue,
        .log_queue = log_queue,
        .temperature_threshold = temp_threshold,
        .humidity_threshold = humid_threshold
    };

    alert_thread_args_t alert_args = {
        .alarm_queue = alarm_queue,
        .running = &running
    };

    logger_thread_args_t logger_args = {
        .log_queue = log_queue,
        .running = &running
    };

    pthread_t sensor_tid;
    pthread_t processor_tid;
    pthread_t alert_tid;
    pthread_t logger_tid;

    if (pthread_create(&alert_tid, NULL, alert_thread, &alert_args) != 0 ||
        pthread_create(&logger_tid, NULL, logger_thread, &logger_args) != 0 ||
        pthread_create(&processor_tid, NULL, processor_thread,
                       &processor_args) != 0 ||
        pthread_create(&sensor_tid, NULL, sensor_thread, &sensor_args) != 0) {
        fprintf(stderr, "Failed to create threads\n");
        running = 0;
        buffer_stop(&buffer);
        return EXIT_FAILURE;
    }

    if (verbose) {
        printf("SMS started: interval=%u ms, temperature=%.1f C, "
               "humidity=%.1f%%, duration=%u s\n",
               interval, temp_threshold, humid_threshold, duration);
    }

    for (unsigned int elapsed = 0;
         elapsed < duration && running;
         elapsed++)
        sleep(1);

    running = 0;
    buffer_stop(&buffer);

    pthread_join(sensor_tid, NULL);
    pthread_join(processor_tid, NULL);
    pthread_join(alert_tid, NULL);
    pthread_join(logger_tid, NULL);

    printf("\nSMS stopped.\n");
    printf("Produced: %lu\n", buffer.total_produced);
    printf("Consumed: %lu\n", buffer.total_consumed);
    printf("Overflow: %lu\n", buffer.overflow_count);

    buffer_destroy(&buffer);

    mq_close(alarm_queue);
    mq_close(log_queue);
    mq_unlink(SMS_ALARM_QUEUE);
    mq_unlink(SMS_LOG_QUEUE);

    return EXIT_SUCCESS;
}
