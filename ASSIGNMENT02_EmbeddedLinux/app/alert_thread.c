#include "alert_thread.h"
#include "sms_messages.h"

#include <stdio.h>
#include <errno.h>
#include <time.h>

void *alert_thread(void *arg)
{
    alert_thread_args_t *args = arg;
    sms_alarm_t alarm;

    unsigned long temp_high_count = 0;
    unsigned long humidity_high_count = 0;
    unsigned long temp_spike_count = 0;

    while (*args->running) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        ssize_t n = mq_timedreceive(args->alarm_queue,
                                    (char *)&alarm,
                                    sizeof(alarm),
                                    NULL, &timeout);

        if (n < 0) {
            if (errno == EINTR || errno == ETIMEDOUT)
                continue;

            perror("mq_timedreceive alarm");
            break;
        }

        const char *type_name;

        switch (alarm.type) {
        case ALARM_TEMP_HIGH:
            type_name = "TEMP_HIGH";
            temp_high_count++;
            break;

        case ALARM_HUMID_HIGH:
            type_name = "HUMID_HIGH";
            humidity_high_count++;
            break;

        case ALARM_TEMP_SPIKE:
            type_name = "TEMP_SPIKE";
            temp_spike_count++;
            break;

        default:
            type_name = "UNKNOWN";
            break;
        }

        fprintf(stderr,
                "[ALERT][%llu] %s: VALUE=%.2f THRESHOLD=%.2f\n",
                (unsigned long long)alarm.timestamp_ms,
                type_name,
                alarm.value,
                alarm.threshold);
    }

    fprintf(stderr,
            "\nAlert summary:\n"
            "Temperature high: %lu\n"
            "Humidity high: %lu\n"
            "Temperature spike: %lu\n",
            temp_high_count,
            humidity_high_count,
            temp_spike_count);

    return NULL;
}
