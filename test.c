/*
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "c-logger.h"
#ifndef __cplusplus
    #define __USE_GNU
#endif
#include <pthread.h>

#define THREAD_POOL_SIZE 3
#define THREAD_LOOP_COUNT 3

void func(void)
{
    warning("----------------------------------------------------");
}

void* thread(void* a)
{
    int i = 0;

    for (i = 0; i < THREAD_LOOP_COUNT; i++) {
        info("12345678901234567890123456789012345678901234567890:%d", i);

        func();
    }
    sleep(5);

    return NULL;
}

/*
 * 
 */
int main(int argc, char** argv)
{
    int i = 0;
    pthread_t t1;
/*
    c_logger_init(C_LOGGER_INFO, "t.xt");
*/
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
