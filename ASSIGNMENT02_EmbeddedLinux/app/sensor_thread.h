#ifndef SENSOR_THREAD_H
#include <signal.h>
#define SENSOR_THREAD_H

#include "circular_buffer.h"

typedef struct {
    circular_buffer_t *buffer;
    const char *device_path;
    unsigned int interval_ms;
    volatile sig_atomic_t *running;
} sensor_thread_args_t;

void *sensor_thread(void *arg);

#endif
