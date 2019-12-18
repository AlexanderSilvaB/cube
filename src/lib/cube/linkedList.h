#ifndef _CUBE_LINKED_LIST_H_
#define _CUBE_LINKED_LIST_H_

#include "common.h"

typedef struct
{
    void *data;
    void *next;
    void *previous;
}linked_list;

linked_list* linked_list_create();
void linked_list_destroy(linked_list *list, bool freeData);
void linked_list_add(linked_list *llist, void* data);
bool linked_list_next(linked_list **llist);
bool linked_list_previous(linked_list **llist);
void linked_list_reset(linked_list **llist);
void* linked_list_get(linked_list *llist);


#endif
