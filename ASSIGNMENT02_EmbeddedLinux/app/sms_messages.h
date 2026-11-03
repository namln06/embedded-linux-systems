#ifndef SMS_MESSAGES_H
#define SMS_MESSAGES_H

#include <stdint.h>

#define SMS_ALARM_QUEUE "/sms_alarm_queue"
#define SMS_LOG_QUEUE   "/sms_log_queue"

typedef struct {
    uint64_t timestamp_ms;
    int type;
    float value;
    float threshold;
} sms_alarm_t;

#define ALARM_TEMP_HIGH  1
#define ALARM_HUMID_HIGH 2
#define ALARM_TEMP_SPIKE 3

typedef struct {
    char text[256];
} sms_log_message_t;

#endif
