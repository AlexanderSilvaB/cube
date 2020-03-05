#ifndef _CUBE_STD_H_
#define _CUBE_STD_H_

#include "common.h"
#include "linkedList.h"
#include "object.h"
#include "value.h"


extern linked_list *stdFnList;

typedef struct
{
    const char *name;
    NativeFn fn;
} std_fn;

void initStd();
void destroyStd();

#endif
