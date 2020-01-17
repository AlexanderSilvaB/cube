#ifndef _CUBE_THREADS_H_
#define _CUBE_THREADS_H_

int thread_create(void *(*entryPoint) (void *), void *data);
int thread_id();

#endif
