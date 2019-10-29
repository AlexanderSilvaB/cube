#include "VM.h"
#include "Object.h"
#include <iostream>
#include <cassert>
#include <algorithm>

using namespace std;

#define vm_assert(expr, message) assert(expr && message);

Object* VM::first = NULL;
vector<Object*> VM::stack;
int VM::numObjs = 0;
int VM::maxObjs = GC_THRESHOLD;

void VM::Init()
{
    stack.clear();
    first = NULL;
    numObjs = 0;
    maxObjs = GC_THRESHOLD;
}

void VM::Destroy()
{
    stack.clear();
    gc();
}

Object* VM::create()
{
    // if(numObjs == maxObjs)
    //    gc();

    Object* obj = new Object();
    obj->setNext(first);
    first = obj;

    numObjs++;

    return obj;
}

void VM::push(Object* obj)
{
    vm_assert(stack.size() < STACK_MAX, "Stack overflow!");
    stack.push_back(obj);
}

Object* VM::pop()
{
    vm_assert(stack.size() > 0, "Stack underflow!");
    Object* obj = stack.back();
    stack.pop_back();
    return obj;
}

void VM::release(Object* obj)
{
    if(obj->isSaved())
        return;
    vector<Object*>::iterator it = std::find(stack.begin(), stack.end(), obj);
    if(it != stack.end())
        stack.erase(it);
}

void VM::markAll()
{
    for(int i = 0; i < stack.size(); i++)
    {
        stack[i]->mark();
    }
}

void VM::sweep()
{
    Object* parent = NULL;
    Object* object = first;
    while(object)
    {
        if(!(object)->isMarked())
        {
            // cout << "Not marked" << endl;
            Object* unreached = object;
            object = unreached->getNext();
            if(parent != NULL)
                parent->setNext(object);

            if(unreached == first)
            {
                first = object;
            }

            // cout << unreached->printable() << endl;
            // cout << unreached << endl;
            delete unreached;
            numObjs--;
            // cout << "deleted" << endl;
        }
        else
        {
            // cout << "unmark" << endl;
            (object)->unmark();
            Object* reached = (object)->getNext();
            parent = object;
            object = reached;
            // cout << "skiped" << endl;
        }
    }
}

void VM::gc()
{
    int oldNumObjects = numObjs;

    markAll();
    sweep();

    maxObjs = numObjs * 2;
    if(maxObjs == 0)
        maxObjs = GC_THRESHOLD;
    cout << "Collected " << (oldNumObjects - numObjs) << " objs, " << numObjs << " remaining." << endl;
}