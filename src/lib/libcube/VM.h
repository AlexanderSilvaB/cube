#ifndef _VM_H_
#define _VM_H_

#include <vector>

class Object;

#define STACK_MAX 4096
#define GC_THRESHOLD 16

class VM
{
    private:
        static Object* first;
        static std::vector<Object*> stack;
        static int numObjs;
        static int maxObjs;
    public:
        static void Init();
        static void Destroy();

        static Object* create();
        static void push(Object* obj);
        static Object* pop();
        static void release(Object* obj);
        static void gc();

    private:
        static void markAll();
        static void sweep();
};

#endif
