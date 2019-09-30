#include "VM.h"
#include "Var.h"
#include <iostream>
#include <cassert>

using namespace std;

#define vm_assert(expr, message) assert(expr && message);

Var* VM::first = NULL;
int VM::stackSize = 0;
Var* VM::stack[STACK_MAX];
int VM::numVars = 0;
int VM::maxVars = GC_THRESHOLD;

void VM::Init()
{
    stackSize = 0;
    first = NULL;
    numVars = 0;
    maxVars = GC_THRESHOLD;
}

void VM::Destroy()
{
    stackSize = 0;
    gc();
}

Var* VM::create()
{
    //if(numVars == maxVars)
    //    gc();

    Var* var = new Var();
    var->setNext(first);
    first = var;

    numVars++;
    
    return var;
}

Var* VM::createAndPush()
{
    Var* var = create();
    push(var);
    return var;
}

void VM::push(Var* var)
{
    vm_assert(stackSize < STACK_MAX, "Stack overflow!");
    stack[stackSize++] = var;
}

Var* VM::pop()
{
    vm_assert(stackSize > 0, "Stack underflow!");
    return stack[--stackSize];
}

void VM::markAll()
{
    for(int i = 0; i < stackSize; i++)
    {
        stack[i]->mark();
    }
}

void VM::sweep()
{
    Var** object = &first;
    while(*object)
    {
        if(!(*object)->isMarked())
        {
            Var* unreached = *object;
            *object = unreached->getNext();
            delete unreached;
            numVars--;
        }
        else
        {
            (*object)->unmark();
            Var* reached = (*object)->getNext();
            object = &reached;
        }
    }
}

void VM::gc()
{
    int oldNumVars = numVars;

    markAll();
    sweep();

    maxVars = numVars * 2;
    if(maxVars == 0)
        maxVars = GC_THRESHOLD;
    cout << "Collected " << (oldNumVars - numVars) << " vars, " << numVars << " remaining." << endl;
}