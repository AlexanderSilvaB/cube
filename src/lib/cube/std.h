#ifndef _CUBE_STD_H_
#define _CUBE_STD_H_

#include "common.h"
#include "value.h"
#include "object.h"
#include "linkedList.h"

extern linked_list *stdFnList;

typedef struct
{
    const char *name;
    NativeFn fn;
}std_fn;

void initStd();
void destroyStd();

#endif
