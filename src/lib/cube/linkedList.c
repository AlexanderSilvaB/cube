#include "linkedList.h"
#include "memory.h"

linked_list* linked_list_create()
{
    linked_list *llist = ALLOCATE(linked_list, 1);
    llist->data = NULL;
    llist->next = NULL;
    llist->previous = NULL;
}

void linked_list_destroy_intern(linked_list *llist, bool freeData)
{
    if(freeData)
    {
        free(llist->data);
        llist->data = NULL;
    }
    if(llist->next != NULL)
    {
        linked_list_destroy_intern((linked_list*)llist->next, freeData);
    }
    llist->previous = NULL;
    FREE(linked_list, llist);
}

void linked_list_destroy(linked_list *llist, bool freeData)
{
    linked_list_reset(&llist);
    linked_list_destroy_intern(llist, freeData);
}

void linked_list_add(linked_list *llist, void* data)
{
    if(llist->next == NULL)
    {
        llist->data = data;
        llist->next = linked_list_create();
        ((linked_list*)llist->next)->previous = llist;
    }
    else
    {
        linked_list_add((linked_list*)llist->next, data);   
    }
}

bool linked_list_next(linked_list **llist)
{
    if( (*llist)->next != NULL)
    {
        *llist = (linked_list*)((*llist)->next);
        return true;
    }
    return false;
}

bool linked_list_previous(linked_list **llist)
{
    if( (*llist)->previous != NULL)
    {
        *llist = (linked_list*)((*llist)->previous);
        return true;
    }
    return false;
}

void linked_list_reset(linked_list **llist)
{
    while(linked_list_previous(llist));
}

void* linked_list_get(linked_list *llist)
{
    return llist->data;
}
