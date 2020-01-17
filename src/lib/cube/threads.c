#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "threads.h"

int thread_create(void *(*entryPoint) (void *), void *data)
{
    #ifdef WIN32
        HANDLE thread; //Thread handle
        DWORD lpId;
    #else
        pthread_t thread;
    #endif

    int code = 0;
#ifdef WIN32
    thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)entryPoint,(LPVOID) data,0,(LPDWORD) &lpId);
    if(thread == NULL) code = 1;
#else
    code = pthread_create( &thread, NULL, entryPoint, data);
#endif

    if(code == 1)
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