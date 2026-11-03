#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <pthread.h>
#include <stdint.h>

#define BUFFER_CAPACITY 64

typedef struct {
    uint64_t timestamp_ms;
    float temperature;
    float humidity;
} sensor_sample_t;

typedef struct {
    sensor_sample_t *data;

    int head;
    int tail;
    int current_size;

    unsigned long total_produced;
    unsigned long total_consumed;
    unsigned long overflow_count;

    int stopped;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} circular_buffer_t;

int buffer_init(circular_buffer_t *buffer);
void buffer_destroy(circular_buffer_t *buffer);

int buffer_push(circular_buffer_t *buffer, sensor_sample_t sample);
int buffer_pop(circular_buffer_t *buffer, sensor_sample_t *sample);

void buffer_stop(circular_buffer_t *buffer);

#endif
