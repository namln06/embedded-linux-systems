#include "logger_thread.h"
#include "sms_messages.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define LOG_FILE "/tmp/sms.log"
#define MAX_LOG_SIZE (1024 * 1024)

static void rotate_log(FILE **file)
{
    struct stat st;

    if (stat(LOG_FILE, &st) != 0 || st.st_size < MAX_LOG_SIZE)
        return;

    fclose(*file);

    unlink(LOG_FILE ".1");

    if (rename(LOG_FILE, LOG_FILE ".1") != 0)
        perror("rename log");

    *file = fopen(LOG_FILE, "a");

    if (*file == NULL)
        perror("open new log");
}

void *logger_thread(void *arg)
{
    logger_thread_args_t *args = arg;
    sms_log_message_t message;

    FILE *file = fopen(LOG_FILE, "a");

    if (file == NULL) {
        perror("fopen sms.log");
        return NULL;
    }

    time_t last_flush = time(NULL);

    while (*args->running) {
        struct timespec timeout;

        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        ssize_t n = mq_timedreceive(args->log_queue,
                                    (char *)&message,
                                    sizeof(message),
                                    NULL, &timeout);

        if (n >= 0) {
            message.text[sizeof(message.text) - 1] = '\0';

            fprintf(file, "%s\n", message.text);
            rotate_log(&file);

            if (file == NULL)
                return NULL;
        } else if (errno != ETIMEDOUT && errno != EINTR) {
            perror("mq_timedreceive");
            break;
        }

        time_t now = time(NULL);

        if (now - last_flush >= 5) {
            fflush(file);
            last_flush = now;
        }
    }

    fflush(file);
    fclose(file);

    return NULL;
}
