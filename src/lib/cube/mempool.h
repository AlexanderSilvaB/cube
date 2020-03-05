#ifndef _MEMPOOL_MEMPOOL_H_
#define _MEMPOOL_MEMPOOL_H_

#include "mempool_config.h"
#include <stddef.h>

int mp_init();
void mp_destroy();
size_t mp_arenas();
size_t mp_capacity();
size_t mp_allocated();

void *mp_malloc(size_t size);
void *mp_calloc(size_t num, size_t nsize);
void *mp_realloc(void *ptr, size_t size);
void mp_free(void *ptr);

#if MP_OVERLOAD_STD == 1
#if MP_ENABLED == 1

#define malloc(a) mp_malloc(a)
#define calloc(a, b) mp_calloc(a, b)
#define realloc(a, b) mp_realloc(a, b)
#define free(a) mp_free(a)

#endif
#endif

#endif
