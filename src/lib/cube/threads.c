#ifdef WIN32
#include <windows.h>
#else
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#endif

#include "threads.h"
#include <stdio.h>

void threads_init()
{
#ifdef WIN32
#else

    int policy;
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    if (pthread_attr_getschedpolicy(&attr, &policy) != 0)
        fprintf(stderr, "Unable to get policy.\n");
    else
    {
        if (policy == SCHED_OTHER)
            printf("SCHED_OTHER\n");
        else if (policy == SCHED_RR)
            printf("SCHED_RR\n");
        else if (policy == SCHED_FIFO)
            printf("SCHED_FIFO\n");
    }

    if (pthread_attr_setschedpolicy(&attr, SCHED_RR) != 0)
        fprintf(stderr, "Unable to set policy.\n");

#endif
}

int thread_create(void *(*entryPoint)(void *), void *data)
{
#ifdef WIN32
    HANDLE thread; // Thread handle
    DWORD lpId;
#else
    pthread_t thread;
#endif

    int code = 0;
#ifdef WIN32
    thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)entryPoint, (LPVOID)data, 0, (LPDWORD)&lpId);
    if (thread == NULL)
        code = 1;
#else
    code = pthread_create(&thread, NULL, entryPoint, data);
#endif

    if (code == 1)
        return 0;

    int id;
#ifdef WIN32
    id = GetThreadId(thread);
#else
    id = thread;
#endif
    return id;
}

int thread_id()
{
    int id;
#ifdef WIN32
    id = GetCurrentThreadId();
#else
    id = pthread_self();
#endif
    return id;
}

void thread_yield()
{
#ifdef WIN32
    SwitchToThread();
#else
    pthread_yield();
#endif
}