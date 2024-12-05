#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_LOG_BUFFER 4096
#define LOG_QUEUE_SIZE 1024

typedef struct {
    char *message;
    size_t length;
} log_entry_t;

typedef struct {
    log_entry_t entries[LOG_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} log_queue_t;

static struct {
    FILE *log_file;
    log_level_t level;
    pthread_t writer_thread;
    log_queue_t queue;
    volatile int running;
    pthread_mutex_t file_mutex;
} logger = {
    .level = LOG_INFO,  // Default level
    .running = 0
};

static const char *level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
    COLOR_WHITE, COLOR_CYAN, COLOR_GREEN, COLOR_YELLOW, COLOR_RED, COLOR_MAGENTA
};

static void *log_writer_thread(void *arg);

int logger_init(const char *log_file_path) {
    // Get log level from environment
    const char *env_level = getenv("LOG_LEVEL");
    if (env_level) {
        if (strcasecmp(env_level, "TRACE") == 0) logger.level = LOG_TRACE;
        else if (strcasecmp(env_level, "DEBUG") == 0) logger.level = LOG_DEBUG;
        else if (strcasecmp(env_level, "INFO") == 0) logger.level = LOG_INFO;
        else if (strcasecmp(env_level, "WARN") == 0) logger.level = LOG_WARN;
        else if (strcasecmp(env_level, "ERROR") == 0) logger.level = LOG_ERROR;
        else if (strcasecmp(env_level, "FATAL") == 0) logger.level = LOG_FATAL;
    }

    logger.log_file = fopen(log_file_path, "a");
    if (!logger.log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", strerror(errno));
        return -1;
    }

    // Initialize mutexes and condition variables
    pthread_mutex_init(&logger.queue.mutex, NULL);
    pthread_mutex_init(&logger.file_mutex, NULL);
    pthread_cond_init(&logger.queue.not_empty, NULL);
    pthread_cond_init(&logger.queue.not_full, NULL);

    logger.queue.head = 0;
    logger.queue.tail = 0;
    logger.queue.count = 0;
    logger.running = 1;

    // Start writer thread
    if (pthread_create(&logger.writer_thread, NULL, log_writer_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create logger thread\n");
        fclose(logger.log_file);
        return -1;
    }

    return 0;
}

void logger_cleanup(void) {
    logger.running = 0;
    pthread_cond_signal(&logger.queue.not_empty);
    pthread_join(logger.writer_thread, NULL);

    pthread_mutex_destroy(&logger.queue.mutex);
    pthread_mutex_destroy(&logger.file_mutex);
    pthread_cond_destroy(&logger.queue.not_empty);
    pthread_cond_destroy(&logger.queue.not_full);

    if (logger.log_file) {
        fclose(logger.log_file);
    }
}

void logger_set_level(log_level_t level) {
    logger.level = level;
}

static void enqueue_log(const char *message, size_t length) {
    pthread_mutex_lock(&logger.queue.mutex);

    while (logger.queue.count >= LOG_QUEUE_SIZE && logger.running) {
        pthread_cond_wait(&logger.queue.not_full, &logger.queue.mutex);
    }

    if (!logger.running) {
        pthread_mutex_unlock(&logger.queue.mutex);
        return;
    }

    log_entry_t *entry = &logger.queue.entries[logger.queue.tail];
    entry->message = malloc(length + 1);
    if (entry->message) {
        memcpy(entry->message, message, length);
        entry->message[length] = '\0';
        entry->length = length;

        logger.queue.tail = (logger.queue.tail + 1) % LOG_QUEUE_SIZE;
        logger.queue.count++;

        pthread_cond_signal(&logger.queue.not_empty);
    }

    pthread_mutex_unlock(&logger.queue.mutex);
}

static void *log_writer_thread(void *arg) {
    (void)arg;

    while (logger.running || logger.queue.count > 0) {
        pthread_mutex_lock(&logger.queue.mutex);

        while (logger.queue.count == 0 && logger.running) {
            pthread_cond_wait(&logger.queue.not_empty, &logger.queue.mutex);
        }

        if (logger.queue.count == 0 && !logger.running) {
            pthread_mutex_unlock(&logger.queue.mutex);
            break;
        }

        log_entry_t *entry = &logger.queue.entries[logger.queue.head];
        char *message = entry->message;
        size_t length = entry->length;

        logger.queue.head = (logger.queue.head + 1) % LOG_QUEUE_SIZE;
        logger.queue.count--;

        pthread_cond_signal(&logger.queue.not_full);
        pthread_mutex_unlock(&logger.queue.mutex);

        if (message) {
            pthread_mutex_lock(&logger.file_mutex);
            fwrite(message, 1, length, logger.log_file);
            fflush(logger.log_file);
            pthread_mutex_unlock(&logger.file_mutex);

            free(message);
        }
    }

    return NULL;
}

static void log_message(log_level_t level, const char *file, int line, const char *fmt, va_list args) {
    if (level < logger.level) return;

    char buffer[MAX_LOG_BUFFER];
    int offset = 0;
    struct timeval tv;
    struct tm *tm_info;

    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    // Add timestamp
    offset += strftime(buffer + offset, sizeof(buffer) - offset,
                      "[%Y-%m-%d %H:%M:%S", tm_info);
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      ".%03ld] ", tv.tv_usec / 1000);

    // Add level and color
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "%s%-5s%s ", level_colors[level], level_strings[level], COLOR_RESET);

    // Add file and line
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "(%s:%d) ", file, line);

    // Add message
    offset += vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, args);

    // Add newline if not present
    if (offset > 0 && buffer[offset - 1] != '\n') {
        buffer[offset++] = '\n';
        buffer[offset] = '\0';
    }

    enqueue_log(buffer, offset);
}

void log_trace(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_TRACE, file, line, fmt, args);
    va_end(args);
}

void log_debug(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_DEBUG, file, line, fmt, args);
    va_end(args);
}

void log_info(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_INFO, file, line, fmt, args);
    va_end(args);
}

void log_warn(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_WARN, file, line, fmt, args);
    va_end(args);
}

void log_error(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_ERROR, file, line, fmt, args);
    va_end(args);
}

void log_fatal(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_FATAL, file, line, fmt, args);
    va_end(args);
}