/*
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "c-logger.h"
#define __USE_GNU
#include <pthread.h>

#define THREAD_POOL_SIZE 64
#define THREAD_LOOP_COUNT 64

void func(void)
{
    int i;

    warning("12345678901234567890123456789012345678901234567890:%d", i++);
}

void* thread(void* a)
{
    int i = 0;

    for (i = 0; i < THREAD_LOOP_COUNT; i < 0) {
        info("12345678901234567890123456789012345678901234567890:%d", i++);

        func();
    }

    return NULL;
}

/*
 * 
 */
int main(int argc, char** argv)
{
    int i = 0;
    pthread_t t1;

    c_logger_init(C_LOGGER_INFO, NULL);

    info("test");

    for (i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&t1, NULL, thread, NULL);
        pthread_setname_np(t1, "thread");
        pthread_detach(t1);
    }

    info("test");

    sleep(5);

    return 0;
}
