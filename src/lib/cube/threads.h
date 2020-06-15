#ifndef _CUBE_THREADS_H_
#define _CUBE_THREADS_H_

void threads_init();
int thread_create(void *(*entryPoint)(void *), void *data);
void thread_join(int id);
int thread_id();
void thread_yield();

#endif
