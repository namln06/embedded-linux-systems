#include "circular_buffer.h"

#include <stdlib.h>
#include <string.h>

int buffer_init(circular_buffer_t *buffer)
{
    if (!buffer)
        return -1;

    memset(buffer, 0, sizeof(*buffer));

    buffer->data = calloc(BUFFER_CAPACITY, sizeof(sensor_sample_t));

    if (!buffer->data)
        return -1;

    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        free(buffer->data);
        buffer->data = NULL;
        return -1;
    }

    if (pthread_cond_init(&buffer->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->data);
        buffer->data = NULL;
        return -1;
    }

    if (pthread_cond_init(&buffer->not_full, NULL) != 0) {
        pthread_cond_destroy(&buffer->not_empty);
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->data);
        buffer->data = NULL;
        return -1;
    }

    return 0;
}

void buffer_destroy(circular_buffer_t *buffer)
{
    if (!buffer)
        return;

    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);
    pthread_mutex_destroy(&buffer->mutex);

    free(buffer->data);
    buffer->data = NULL;
}

int buffer_push(circular_buffer_t *buffer, sensor_sample_t sample)
{
    pthread_mutex_lock(&buffer->mutex);

    if (buffer->current_size == BUFFER_CAPACITY)
        buffer->overflow_count++;

    while (buffer->current_size == BUFFER_CAPACITY && !buffer->stopped)
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);

    if (buffer->stopped) {
        pthread_mutex_unlock(&buffer->mutex);
        return -1;
    }

    buffer->data[buffer->head] = sample;
    buffer->head = (buffer->head + 1) % BUFFER_CAPACITY;
    buffer->current_size++;
    buffer->total_produced++;

    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);

    return 0;
}

int buffer_pop(circular_buffer_t *buffer, sensor_sample_t *sample)
{
    pthread_mutex_lock(&buffer->mutex);

    while (buffer->current_size == 0 && !buffer->stopped)
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);

    if (buffer->current_size == 0 && buffer->stopped) {
        pthread_mutex_unlock(&buffer->mutex);
        return -1;
    }

    *sample = buffer->data[buffer->tail];
    buffer->tail = (buffer->tail + 1) % BUFFER_CAPACITY;
    buffer->current_size--;
    buffer->total_consumed++;

    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);

    return 0;
}

void buffer_stop(circular_buffer_t *buffer)
{
    pthread_mutex_lock(&buffer->mutex);

    buffer->stopped = 1;

    pthread_cond_broadcast(&buffer->not_empty);
    pthread_cond_broadcast(&buffer->not_full);

    pthread_mutex_unlock(&buffer->mutex);
}
