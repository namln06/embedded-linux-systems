#include "processor_thread.h"
#include "sms_messages.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define WINDOW_SIZE 10

static void send_alarm(mqd_t queue,
                       uint64_t timestamp,
                       int type,
                       float value,
                       float threshold)
{
    sms_alarm_t alarm;
    memset(&alarm, 0, sizeof(alarm));

    alarm.timestamp_ms = timestamp;
    alarm.type = type;
    alarm.value = value;
    alarm.threshold = threshold;

    if (mq_send(queue, (const char *)&alarm,
                sizeof(alarm), 0) == -1)
        perror("mq_send alarm");
}

static void send_log(mqd_t queue, const char *message)
{
    sms_log_message_t log_message = {0};

    snprintf(log_message.text, sizeof(log_message.text),
             "%s", message);

    if (mq_send(queue, (const char *)&log_message,
                sizeof(log_message), 0) == -1)
        perror("mq_send log");
}

void *processor_thread(void *arg)
{
    processor_thread_args_t *args = arg;

    float temperatures[WINDOW_SIZE] = {0};
    float humidities[WINDOW_SIZE] = {0};

    int count = 0;
    int index = 0;
    float previous_temperature = 0.0f;
    int has_previous = 0;

    sensor_sample_t sample;

    while (buffer_pop(args->buffer, &sample) == 0) {
        temperatures[index] = sample.temperature;
        humidities[index] = sample.humidity;

        index = (index + 1) % WINDOW_SIZE;

        if (count < WINDOW_SIZE)
            count++;

        float temp_sum = 0.0f;
        float humidity_sum = 0.0f;

        for (int i = 0; i < count; i++) {
            temp_sum += temperatures[i];
            humidity_sum += humidities[i];
        }

        float avg_temperature = temp_sum / count;
        float avg_humidity = humidity_sum / count;

        if (avg_temperature > args->temperature_threshold) {
            send_alarm(args->alarm_queue, sample.timestamp_ms,
                       ALARM_TEMP_HIGH, avg_temperature,
                       args->temperature_threshold);
        }

        if (avg_humidity > args->humidity_threshold) {
            send_alarm(args->alarm_queue, sample.timestamp_ms,
                       ALARM_HUMID_HIGH, avg_humidity,
                       args->humidity_threshold);
        }

        if (has_previous &&
            sample.temperature - previous_temperature > 5.0f) {
            send_alarm(args->alarm_queue, sample.timestamp_ms,
                       ALARM_TEMP_SPIKE, sample.temperature,
                       previous_temperature + 5.0f);
        }

        char log_text[256];

        snprintf(log_text, sizeof(log_text),
                 "[%llu] Temperature=%.2f C, Humidity=%.2f%%, "
                 "Average=%.2f C/%.2f%%",
                 (unsigned long long)sample.timestamp_ms,
                 sample.temperature,
                 sample.humidity,
                 avg_temperature,
                 avg_humidity);

        send_log(args->log_queue, log_text);

        previous_temperature = sample.temperature;
        has_previous = 1;
    }

    return NULL;
}
