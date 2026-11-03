#ifndef LOGGER_THREAD_H
#include <signal.h>
#define LOGGER_THREAD_H

#include <mqueue.h>

typedef struct {
    mqd_t log_queue;
    volatile sig_atomic_t *running;
} logger_thread_args_t;

void *logger_thread(void *arg);

#endif
