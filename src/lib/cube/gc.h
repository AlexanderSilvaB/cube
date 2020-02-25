#ifndef _CUBE_GC_H_
#define _CUBE_GC_H_

#include "value.h"

void gc_maybe_collect();
void gc_collect();
void mark_value(Value val);
void mark_object(Obj *obj);

#endif
