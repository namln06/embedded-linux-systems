#ifndef PROCESSOR_THREAD_H
#define PROCESSOR_THREAD_H

#include "circular_buffer.h"
#include <mqueue.h>

typedef struct {
    circular_buffer_t *buffer;
    mqd_t alarm_queue;
    mqd_t log_queue;
    float temperature_threshold;
    float humidity_threshold;
} processor_thread_args_t;

void *processor_thread(void *arg);

#endif
