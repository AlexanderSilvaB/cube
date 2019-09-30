#ifndef _VM_H_
#define _VM_H_

class Var;

#define STACK_MAX 4096
#define GC_THRESHOLD 256

class VM
{
    private:
        static Var* first;
        static int stackSize;
        static Var* stack[STACK_MAX];
        static int numVars;
        static int maxVars;
    public:
        static void Init();
        static void Destroy();

        static Var* create();
        static Var* createAndPush();
        static void push(Var* var);
        static Var* pop();
        static void gc();

    private:
        static void markAll();
        static void sweep();
};

// #define MKVAR() VM::createAndPush()
#define MKVAR() VM::create()
#define PUSH(var) VM::push(var)
#define POP() VM::pop()

#endif
