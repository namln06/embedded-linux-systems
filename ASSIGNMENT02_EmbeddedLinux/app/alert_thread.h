#ifndef ALERT_THREAD_H
#include <signal.h>
#define ALERT_THREAD_H

#include <mqueue.h>

typedef struct {
    mqd_t alarm_queue;
    volatile sig_atomic_t *running;
} alert_thread_args_t;

void *alert_thread(void *arg);

#endif
