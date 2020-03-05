#include "mempool_config.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char ALIGN[16];
typedef union mp_header_t {
    struct
    {
        size_t size;
        uint8_t is_free;
        union mp_header_t *next;
    } s;
    ALIGN stub;
} mp_header;

typedef struct mp_arena_st
{
#if MP_STATIC_POOL != 0
    uint8_t data[MP_ARENA_SIZE];
#else
    uint8_t *data;
#endif
    size_t size;

    mp_header *head, *tail;

    struct mp_arena_st *next;
} mp_arena;

static mp_arena fixed_arena;
static mp_arena *arena = NULL;

int alloc_arena();
int grow_arena(mp_arena *arena, size_t minimum);
int init_arena(mp_arena *arena);
void free_arena(mp_arena *current);

void mp_free(void *ptr);

int mp_init()
{
#if MP_ENABLED == 0
    return 1;
#endif

#if MP_VERBOSE != 0
    printf("MemPool Init: %ld b\n", (size_t)MP_ARENA_SIZE);
#endif

    if (MP_ARENA_SIZE <= sizeof(mp_header))
        return 0;

    arena = &fixed_arena;
#if MP_STATIC_POOL == 0
    arena->data = NULL;
#endif

    if (!init_arena(arena))
        return 0;

    return 1;
}

void mp_destroy()
{
#if MP_ENABLED == 0
    return;
#endif

#if MP_VERBOSE != 0
    printf("MemPool Destroy\n");
#endif

#if MP_STATIC_POOL == 0
    free_arena(arena);
#endif
}

size_t mp_arenas()
{
    size_t size = 0;
    mp_arena *current = arena;
    while (current != NULL)
    {
        size += 1;
        current = current->next;
    }
#if MP_VERBOSE != 0
    printf("MemPool Arenas: %ld\n", size);
#endif
    return size;
}

size_t mp_capacity()
{
    size_t size = 0;
    mp_arena *current = arena;
    while (current != NULL)
    {
        size += current->size;
        current = current->next;
    }
#if MP_VERBOSE != 0
    printf("MemPool Capacity: %ld\n", size);
#endif
    return size;
}

size_t mp_allocated()
{
    size_t size = 0;
    mp_arena *current;
    mp_header *ptr;
    current = arena;
    while (current != NULL)
    {
        ptr = current->head;
        while (ptr != current->tail && !ptr->s.is_free)
        {
            size += ptr->s.size;
            ptr = ptr->s.next;
        }
        current = current->next;
    }
#if MP_VERBOSE != 0
    printf("MemPool Allocated: %ld\n", size);
#endif
    return size;
}

mp_header *mp_get_free_block(size_t size)
{
    mp_arena *current;
    mp_header *ptr;
    current = arena;
    while (current != NULL)
    {
        ptr = current->head;
        while (ptr != current->tail && (!ptr->s.is_free || ptr->s.size < size))
            ptr = ptr->s.next;
        if (ptr && ptr->s.size > 0)
            return ptr;
        current = current->next;
    }
    return NULL;
}

void *mp_malloc(size_t size)
{
#if MP_ENABLED == 0
    return malloc(size);
#endif

#if MP_VERBOSE != 0
    printf("MemPool Malloc: %ld\n", size);
#endif

    if (!size)
        return NULL;

    mp_arena *current;
    mp_header *ptr;

#if MP_ALLOW_REUSE != 0
    ptr = mp_get_free_block(size);
    if (ptr)
    {
        ptr->s.is_free = 0;
        return (void *)((uint8_t *)ptr + sizeof(mp_header));
    }
#endif

find_arena:
    current = arena;
    while (current->next != NULL)
        current = current->next;

    ptr = current->tail;
    while (!ptr->s.is_free)
        ptr = ptr->s.next;

    if (current->data + current->size - (uint8_t *)ptr + sizeof(mp_header) < size)
    {
#if MP_ALLOW_GROW_ARENA == 0
        if (!alloc_arena())
#else
        if (!grow_arena(current, current->size + size))
#endif
            return NULL;
        goto find_arena;
    }

    ptr->s.size = size;
    ptr->s.is_free = 0;
    ptr->s.next = (mp_header *)((uint8_t *)ptr + size + sizeof(mp_header));
    current->tail = ptr->s.next;
    current->tail->s.is_free = 1;
    current->tail->s.size = 0;
    return (void *)((uint8_t *)ptr + sizeof(mp_header));
}

void *mp_calloc(size_t num, size_t nsize)
{
#if MP_ENABLED == 0
    return calloc(num, nsize);
#endif

#if MP_VERBOSE != 0
    printf("MemPool Calloc: %ld\n", num * nsize);
#endif

    size_t size = num * nsize;
    void *ptr = mp_malloc(size);
    if (ptr == NULL)
        return NULL;
    memset(ptr, '\0', size);
    return ptr;
}

void *mp_realloc(void *ptr, size_t size)
{
#if MP_ENABLED == 0
    return realloc(ptr, size);
#endif

    if (!ptr)
        return mp_malloc(size);

    if (!size)
    {
        mp_free(ptr);
        return NULL;
    }

    mp_header *header = (mp_header *)ptr - 1;
    if (header->s.size >= size)
        return ptr;

#if MP_VERBOSE != 0
    printf("MemPool Realloc: %ld -> %ld\n", header->s.size, size);
#endif

    void *ret = mp_malloc(size);
    if (ret)
    {
        memcpy(ret, ptr, header->s.size);
        mp_free(ptr);
    }

    return ret;
}

void mp_free(void *ptr)
{
#if MP_ENABLED == 0
    return free(ptr);
#endif

#if MP_VERBOSE != 0
    printf("MemPool Free\n");
#endif

    if (ptr == NULL)
        return;
    mp_header *header = (mp_header *)((uint8_t *)ptr - sizeof(mp_header));
    header->s.is_free = 1;
}

// -------------------

int init_arena(mp_arena *arena)
{
#if MP_STATIC_POOL == 0
    if (arena->data != NULL)
        return 1;

    arena->data = (uint8_t *)malloc(sizeof(uint8_t) * MP_ARENA_SIZE);
    if (arena->data == NULL)
        return 0;
#endif

    memset(arena->data, '\0', MP_ARENA_SIZE);

    arena->size = MP_ARENA_SIZE;
    arena->next = NULL;
    arena->head = arena->tail = (mp_header *)arena->data;
    arena->tail->s.is_free = 1;
    arena->tail->s.size = 0;

#if MP_VERBOSE != 0
    printf("MemPool Init Arena: %ld\n", arena->size);
#endif

    return 1;
}

int alloc_arena()
{
#if MP_STATIC_POOL != 0
    return 0;
#endif

    mp_arena *current = arena;
    while (current->next != NULL)
        current = current->next;

    current->next = (mp_arena *)malloc(sizeof(mp_arena));
#if MP_STATIC_POOL == 0
    current->next->data = NULL;
#endif

#if MP_VERBOSE != 0
    printf("MemPool Alloc Arena\n");
#endif

    return init_arena(current->next);
}

int grow_arena(mp_arena *arena, size_t minimum)
{
#if MP_STATIC_POOL != 0
    return 0;
#else

#if MP_VERBOSE != 0
    size_t old_size = arena->size;
#endif

    size_t offset = (uint8_t *)arena->tail - arena->data;
    while (arena->size < minimum)
        arena->size *= MP_GROW_FACTOR;
    arena->data = (uint8_t *)realloc(arena->data, arena->size);
    if (arena->data == NULL)
        return 0;
    arena->head = (mp_header *)arena->data;
    arena->tail = (mp_header *)(arena->data + offset);

    mp_header *header = arena->head;
    while (header != arena->tail)
    {
        header->s.next = (mp_header *)((uint8_t *)header + header->s.size + sizeof(mp_header));
        header = header->s.next;
    }

#if MP_VERBOSE != 0
    printf("MemPool Grow Arena: %ld -> %ld\n", old_size, arena->size);
#endif

    return 1;
#endif
}

void free_arena(mp_arena *current)
{
    if (current == NULL)
        return;

    if (current->data != NULL)
        free(current->data);

#if MP_VERBOSE != 0
    printf("MemPool Free Arena\n");
#endif

#if MP_STATIC_POOL == 0
    current->data = NULL;
#endif
    current->head = current->tail = NULL;
    free_arena(current->next);
    current->next = NULL;
    if (current != arena)
    {
        free(current);
    }
}