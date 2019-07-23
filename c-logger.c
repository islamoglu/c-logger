/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include "c-logger.h"
#ifndef __cplusplus
    #define __USE_GNU
#endif
#include <pthread.h>

#define BOOM fprintf(stderr,"%s:%d:%s\n",__FUNCTION__,__LINE__,strerror(errno));
#define MAX_EVENTS 1
#define BUFF_SIZE 16384
#define THREAD_NAME_SIZE 256
#define DEFAULT_LOG_LEVEL C_LOGGER_INFO

int c_logger_level = DEFAULT_LOG_LEVEL;

const char* c_logger_str[C_LOGGER_LEVEL_MAX + 1] = {
    "MIN",
    "DEBUG",
    "TRACE",
    "INFO",
    "WARN",
    "ERROR",
    "MAX"
};

pthread_key_t logger_key;
pthread_once_t logger_key_once = PTHREAD_ONCE_INIT;

typedef void logger_deamon_data_t;

typedef struct
{
    int write_fd;
    int read_fd;
#ifdef C_LOGGER_THREAD_NAME
    char thread_name[THREAD_NAME_SIZE];
    pthread_t thread;
#endif
#ifdef C_LOGGER_THREAD_ID
    pid_t thread_id;
#endif
} logger_t;

typedef struct
{
    int epoll_fd;
    int log_fd;
    pthread_t deamon_thread;
    char* file_path;
    int logger_level;
} logger_global_t;

logger_global_t* logger_global = NULL;
pthread_mutex_t logger_global_lock;

int set_logger_level(int level)
{
    if (level <= C_LOGGER_LEVEL_MIN || level >= C_LOGGER_LEVEL_MAX) {
        return -1;
    }
    else {
        c_logger_level = level;
        logger_global->logger_level = level;
    }

    return 0;
}

void logger_destroy(void* _logger)
{
    logger_t* logger = (logger_t*) _logger;
    struct epoll_event event;

    if (epoll_ctl(logger_global->epoll_fd, EPOLL_CTL_DEL, logger->read_fd, &event) < 0) {
        BOOM;
    }

    close(logger->read_fd);
    close(logger->write_fd);

    free(logger);
}

static void make_logger_key(void)
{
    (void) pthread_key_create(&logger_key, logger_destroy);
}

int open_logger_stream(char* file_path)
{
    int fd = -1;

    if (file_path != NULL) {
        fd = open(logger_global->file_path, O_CREAT | O_WRONLY | O_APPEND, S_IWUSR | S_IRUSR);
    }

    if (fd < 0) {
        fd = fileno(stdout);
        if (fd < 0) {
            BOOM;
            return -1;
        }
        write(fd, "log file open error, stdout is going to use for logging\n",
              strlen("log file open error, stdout is going to use for logging\n"));
    }

    return fd;
}

void* logger_deamon_thread(void * data)
{
    int epoll_fd = logger_global->epoll_fd;
    int log_fd = logger_global->log_fd;
    char read_buffer[BUFF_SIZE];
    int event_count, i;
    int length = 0;
    size_t bytes_read = 0;
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, INT_MAX);

        for (i = 0; i < event_count; i++) {
            if (read(events[i].data.fd, (char*) &length, sizeof (int)) < 0) {
                continue;
            }

            bytes_read = read(events[i].data.fd, read_buffer, length);
            read_buffer[bytes_read] = '\0';
            write(log_fd, read_buffer, bytes_read);
        }
    }

    close(log_fd);
    close(epoll_fd);
}

int real_init_logger_deamon()
{
    int epoll_fd;
    int log_fd;

    log_fd = open_logger_stream(logger_global->file_path);
    if (log_fd < 0) {
        BOOM;
        return -1;
    }
    else {
        logger_global->log_fd = log_fd;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        BOOM;
        close(log_fd);
        return -1;
    }
    else {
        logger_global->epoll_fd = epoll_fd;
    }

    if (pthread_create(&(logger_global->deamon_thread), NULL, logger_deamon_thread, NULL) < 0) {
        BOOM;
        close(epoll_fd);
        close(log_fd);
        return -1;
    }
}

int check_and_init_logger_deamon(int level, const char* file_path)
{
    int rc = 0;

    if (logger_global == NULL) {
        pthread_mutex_lock(&logger_global_lock);

        if (logger_global != NULL) {
            pthread_mutex_unlock(&logger_global_lock);
            rc = 0;
        }
        else {
            logger_global = (logger_global_t*) calloc(1, sizeof (logger_global_t));

            if (level <= C_LOGGER_LEVEL_MIN || level >= C_LOGGER_LEVEL_MAX) {
                c_logger_level = DEFAULT_LOG_LEVEL;
            }
            else {
                c_logger_level = level;
                logger_global->logger_level = level;
            }

            if (file_path != NULL) {
                logger_global->file_path = strdup(file_path);
            }

            rc = real_init_logger_deamon();
            if (rc < 0) {
                BOOM;
            }

            pthread_mutex_unlock(&logger_global_lock);
        }
    }

    return rc;
}

void * init_logger(void)
{
    int pipes[2];
    struct epoll_event event;

    if (check_and_init_logger_deamon( DEFAULT_LOG_LEVEL , NULL ) < 0) {
        BOOM;
        return NULL;
    }

    logger_t* logger = (logger_t*) calloc(1, sizeof (logger_t));

#ifdef C_LOGGER_THREAD_NAME
    logger->thread = pthread_self();
    pthread_getname_np(logger->thread, logger->thread_name, THREAD_NAME_SIZE);
#endif
#ifdef C_LOGGER_THREAD_ID
    logger->thread_id = syscall(SYS_gettid);
#endif

    if (pipe(pipes) < 0) {
        BOOM;
        goto free;
    }

    logger->read_fd = pipes[0];
    logger->write_fd = pipes[1];

    event.events = EPOLLIN;
    event.data.fd = logger->read_fd;

    if (epoll_ctl(logger_global->epoll_fd, EPOLL_CTL_ADD, logger->read_fd, &event) < 0) {
        BOOM;
        goto close_pipes;
    }

    if (pthread_setspecific(logger_key, logger) < 0) {
        BOOM;
        goto remove_epoll;
    }

    return logger;

remove_epoll:
    epoll_ctl(logger_global->epoll_fd, EPOLL_CTL_DEL, logger->read_fd, &event);

close_pipes:
    close(pipes[0]);
    close(pipes[1]);

free:
    free(logger);

    return NULL;
}

void* get_logger(void)
{
    (void) pthread_once(&logger_key_once, make_logger_key);

    void* logger = pthread_getspecific(logger_key);

    if (logger == NULL) {
        logger = init_logger();
    }
    return logger;
}

int c_logger_init(int level, const char* log_file_path)
{
    if (check_and_init_logger_deamon(level, log_file_path) < 0) {
        BOOM;
        return -1;
    }

    if (get_logger() == NULL) {
        BOOM;
        return -1;
    }

    return 0;
}

void c_logger(c_logger_level_enum_t level,
#ifdef C_LOGGER_TIME
        struct timespec* time,
#endif
#ifdef C_LOGGER_FILE
        const char* file,
#endif
#ifdef C_LOGGER_FUNCTION
        const char* function,
#endif
#ifdef C_LOGGER_LINE
        int line,
#endif
        const char* format, ...)
{
    logger_t* logger = (logger_t*)get_logger();
    char write_buffer[BUFF_SIZE];
    int len = 0;

    if (logger) {
        va_list args;
        va_start(args, format);

#ifdef C_LOGGER_TIME
        len += snprintf(write_buffer + len, BUFF_SIZE - 1 - len, "[%lu.%lu]", time->tv_sec, time->tv_nsec);
#endif
#ifdef C_LOGGER_THREAD_NAME
        len += snprintf(write_buffer + len, BUFF_SIZE - 1 - len, "[%s]", logger->thread_name);
#endif        
#ifdef C_LOGGER_THREAD_ID
        len += snprintf(write_buffer + len, BUFF_SIZE - 1 - len, "[%d]", logger->thread_id);
#endif
#ifdef C_LOGGER_LEVEL_TEXT
        len += snprintf(write_buffer + len, BUFF_SIZE - 1 - len, "[%s]", c_logger_str[level]);
#endif
#ifdef C_LOGGER_FILE
        len += snprintf(write_buffer + len, BUFF_SIZE - 1 - len, "[%s]", file);
#endif
#ifdef C_LOGGER_FUNCTION
        len += snprintf(write_buffer + len, BUFF_SIZE - 1 - len, "[%s]", function);
#endif
#ifdef C_LOGGER_LINE
        len += snprintf(write_buffer + len, BUFF_SIZE - 1 - len, "[%d]", line);
#endif

        write_buffer[len] = ' ';
        len++;

        len += vsnprintf(write_buffer + len, BUFF_SIZE - 1 - len - sizeof (int), format, args);

        write_buffer[len] = '\n';
        len++;

        if (len > 0) {
            if (write(logger->write_fd, (char*) &len, sizeof (int)) < sizeof (int)) {
                BOOM;
                goto end;
            }
            if (write(logger->write_fd, write_buffer, len) < len) {
                BOOM;
            }
        }

end:
        va_end(args);
    }
}
