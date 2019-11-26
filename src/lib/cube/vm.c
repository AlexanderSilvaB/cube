#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "common.h"
#include "object.h"
#include "memory.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"
#include "std.h"

VM vm;

static void resetStack()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

void runtimeError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);                  
    va_end(args);                                    
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--)
    {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        // -1 because the IP is sitting on the next instruction to be
        // executed.                                                 
        size_t instruction = frame->ip - function->chunk.code - 1;   
        fprintf(stderr, "[line %d] in ",                             
        function->chunk.lines[instruction]);                 
        if (function->name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", function->name->chars);
        }                                                            
    }

    resetStack();
}

static void defineNative(const char* name, NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM()
{
    resetStack();
    vm.objects = NULL;
    vm.listObjects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);
    vm.initString = copyString("init", 4);

    // STD
    defineNative("clock", clockNative);
    defineNative("time", timeNative);
    defineNative("exit", exitNative);
    defineNative("input", inputNative);
    defineNative("print", printNative);
    defineNative("println", printlnNative);
    defineNative("random", randomNative);
    defineNative("bool", boolNative);
    defineNative("num", numNative);
    defineNative("str", strNative);
    defineNative("ceil", ceilNative);
    defineNative("floor", floorNative);
    defineNative("round", roundNative);
    defineNative("pow", powNative);
    defineNative("exp", expNative);
    defineNative("len", lenNative);
}

void freeVM()
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    freeObjects();
    freeLists();
}

void push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

static bool call(ObjClosure* closure, int argCount)
{
    if (argCount != closure->function->arity)
    {
        runtimeError("Function '%s' expected %d arguments but got %d.",
                        closure->function->name->chars, closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX)
    {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;

    frame->slots = vm.stackTop - argCount - 1;           
    return true;
}

static bool callValue(Value callee, int argCount)
{
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
            case OBJ_BOUND_METHOD: 
            {
				ObjBoundMethod *bound = AS_BOUND_METHOD(callee);

				// Replace the bound method with the receiver so it's in the
				// right slot when the method is called.
				vm.stackTop[-argCount - 1] = bound->receiver;
				return call(bound->method, argCount);
			}
            case OBJ_CLASS: 
            {
				ObjClass *klass = AS_CLASS(callee);
				// Create the instance.
				vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));

				// Call the initializer, if there is one.
				Value initializer;
				if (tableGet(&klass->methods, vm.initString, &initializer))
                {
                    return call(AS_CLOSURE(initializer), argCount);
                }
				else if (argCount != 0) 
                {
					runtimeError("Expected 0 arguments but got %d.", argCount);
					return false;
				}

				return true;
			}
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);

            case OBJ_NATIVE:
            {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;                            
                push(result);                                           
                return true;                                            
            }

            default:
                // Non-callable object type.
                break;
        }
    }                                                    

    runtimeError("Can only call functions and classes.");
    return false;
}

static bool invokeFromClass(ObjClass *klass, ObjString *name, int argCount) 
{
	// Look for the method.
	Value method;
	if (!tableGet(&klass->methods, name, &method)) {
		runtimeError("Undefined property '%s'.", name->chars);
		return false;
	}

	return call(AS_CLOSURE(method), argCount);
}

static bool invoke(ObjString *name, int argCount) 
{
	Value receiver = peek(argCount);

	if (IS_CLASS(receiver)) 
    {
		ObjClass *instance = AS_CLASS(receiver);
		Value method;
		if (!tableGet(&instance->methods, name, &method)) 
        {
			runtimeError("Undefined property '%s'.", name->chars);
			return false;
		}

		if (!AS_CLOSURE(method)->function->staticMethod) 
        {
			runtimeError("'%s' is not static. Only static methods can be "
						 "invoked directly from a class.",
						 name->chars);
			return false;
		}

		return callValue(method, argCount);
	} 
    /*
    
    else if (IS_LIST(receiver))
		return listMethods(name->chars, argCount + 1);
	else if (IS_DICT(receiver))
		return dictMethods(name->chars, argCount + 1);
	else if (IS_STRING(receiver))
		return stringMethods(name->chars, argCount + 1);
	else if (IS_FILE(receiver))
		return fileMethods(name->chars, argCount + 1);
    */
	if (!IS_INSTANCE(receiver)) 
    {
		runtimeError("Only instances have methods.");
		return false;
	}

	ObjInstance *instance = AS_INSTANCE(receiver);

	// First look for a field which may shadow a method.
	Value value;
	if (tableGet(&instance->fields, name, &value)) 
    {
		vm.stackTop[-argCount] = value;
		return callValue(value, argCount);
	}

	return invokeFromClass(instance->klass, name, argCount);
}

static bool bindMethod(ObjClass *klass, ObjString *name) 
{
	Value method;
	if (!tableGet(&klass->methods, name, &method)) 
    {
		runtimeError("Undefined property '%s'.", name->chars);
		return false;
	}

	ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));
	pop(); // Instance.
	push(OBJ_VAL(bound));
	return true;
}


static ObjUpvalue* captureUpvalue(Value* local)
{
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;

    while (upvalue != NULL && upvalue->location > local)
    {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local)
        return upvalue;

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL)
    {
        vm.openUpvalues = createdUpvalue;
    }
    else
    {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value* last)
{
    while (vm.openUpvalues != NULL &&
            vm.openUpvalues->location >= last)
    {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static void defineMethod(ObjString *name) 
{
	Value method = peek(0);
	ObjClass *klass = AS_CLASS(peek(1));
	tableSet(&klass->methods, name, method);
	pop();
}

static void createClass(ObjString *name, ObjClass *superclass) 
{
	ObjClass *klass = newClass(name, superclass);
	push(OBJ_VAL(klass));

	// Inherit methods.
	if (superclass != NULL)
		tableAddAll(&superclass->methods, &klass->methods);
}

static bool isFalsey(Value value)
{
  return IS_NONE(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;            
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';                          

    ObjString* result = takeString(chars, length); 
    push(OBJ_VAL(result));
}

static InterpretResult run()
{
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

    #define READ_BYTE() (*frame->ip++)
    #define READ_SHORT() \
        (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
    #define READ_CONSTANT() \
        (frame->closure->function->chunk.constants.values[READ_BYTE()])
    #define READ_CONSTANT_LONG() (frame->closure->function->chunk.constants.values[(READ_BYTE() | (READ_BYTE() << 8) | (READ_BYTE() << 16))])

    #define READ_STRING() AS_STRING(READ_CONSTANT())

    #define BINARY_OP(valueType, op) \
        do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
        } while (false)

    for(;;)
    {
        #ifdef DEBUG_TRACE_EXECUTION
            printf("          ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) 
            {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }
            printf("\n");
            disassembleInstruction(&frame->closure->function->chunk,
                    (int)(frame->ip - frame->closure->function->chunk.code));
        #endif

        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
            case OP_CONSTANT:
            {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_CONSTANT_LONG:
            {
                Value constant = READ_CONSTANT_LONG();
                push(constant);
                break;
            }
            case OP_NONE: push(NONE_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;

            case OP_GET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }

            case OP_SET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }

            case OP_GET_GLOBAL:
            {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value))
                {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }

            case OP_DEFINE_GLOBAL:
            {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();                               
                break;
            }

            case OP_SET_GLOBAL:
            {
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0)))
                {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_GET_UPVALUE:
            {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;                                          
            }

            case OP_SET_UPVALUE:
            {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;                                              
            }

            OP_GET_PROPERTY: 
            {
                if (!IS_INSTANCE(peek(0))) 
                {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(0));
                ObjString *name = READ_STRING();
                Value value;
                if (tableGet(&instance->fields, name, &value)) 
                {
                    pop(); // Instance.
                    push(value);
                    break;
                }

                if (!bindMethod(instance->klass, name))
                    return INTERPRET_RUNTIME_ERROR;

                break;
            }
            OP_GET_PROPERTY_NO_POP: 
            {
                if (!IS_INSTANCE(peek(0))) 
                {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(0));
                ObjString *name = READ_STRING();
                Value value;
                if (tableGet(&instance->fields, name, &value)) 
                {
                    push(value);
                    break;
                }

                if (!bindMethod(instance->klass, name))
                    return INTERPRET_RUNTIME_ERROR;

                break;
            }
            OP_SET_PROPERTY: 
            {
                if (!IS_INSTANCE(peek(1))) 
                {
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);
                break;
            }
            OP_GET_SUPER: 
            {
                ObjString *name = READ_STRING();
                ObjClass *superclass = AS_CLASS(pop());

                if (!bindMethod(superclass, name))
                    return INTERPRET_RUNTIME_ERROR;
                break;
            }

            case OP_EQUAL:
            {
                Value b = pop();
                Value a = pop();                                
                push(BOOL_VAL(valuesEqual(a, b)));              
                break;
            }
            case OP_GREATER:    BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:       BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD:
            {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
                {
                    concatenate();
                }
                else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
                {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                }
                else if (IS_LIST(peek(0)) && IS_LIST(peek(1)))
                {
                    Value listToAddValue = pop();
                    Value listValue = pop();

                    ObjList *list = AS_LIST(listValue);
                    ObjList *listToAdd = AS_LIST(listToAddValue);

                    ObjList *result = initList();
                    for (int i = 0; i < list->values.count; ++i)
                        writeValueArray(&result->values, list->values.values[i]);
                    for (int i = 0; i < listToAdd->values.count; ++i)
                        writeValueArray(&result->values, listToAdd->values.values[i]);

                    push(OBJ_VAL(result));
			    }
                else
                {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }                                                              
                break;                                                         
            }
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, -); break;
            case OP_INC:
            {
                if (!IS_NUMBER(peek(0)))
                {
				    runtimeError("Operand must be a number.");
				    return INTERPRET_RUNTIME_ERROR;
                }

                push(NUMBER_VAL(AS_NUMBER(pop()) + 1));
                break;
            }
            case OP_DEC:
            {
                if (!IS_NUMBER(peek(0)))
                {
				    runtimeError("Operand must be a number.");
				    return INTERPRET_RUNTIME_ERROR;
                }

                push(NUMBER_VAL(AS_NUMBER(pop()) - 1));
                break;
            }
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /); break;
            case OP_MOD:
            {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());

                push(NUMBER_VAL(fmod(a, b)));
                break;
		    }
            case OP_NOT:        push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_NEGATE:
            {
                if (!IS_NUMBER(peek(0)))
                {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }

            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;                         
            }

            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0)))
                    frame->ip += offset;
                break;
            }

            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }

            case OP_BREAK:
            {
                break;
            }

            case OP_NEW_LIST:
            {
                ObjList *list = initList();
                push(OBJ_VAL(list));
                break;
            }
            case OP_ADD_LIST:
            {
                Value addValue = pop();
                Value listValue = pop();

                ObjList *list = AS_LIST(listValue);
                writeValueArray(&list->values, addValue);

                push(OBJ_VAL(list));
                break;
            }

            case OP_NEW_DICT:
            {
                ObjDict *dict = initDict();
                push(OBJ_VAL(dict));
                break;
            }
            case OP_ADD_DICT:
            {
                Value value = pop();
                Value key = pop();
                Value dictValue = pop();

                if (!IS_STRING(key))
                {
                    runtimeError("Dictionary key must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjDict *dict = AS_DICT(dictValue);
                char *keyString = AS_CSTRING(key);

                insertDict(dict, keyString, value);

                push(OBJ_VAL(dict));
                break;
            }
            case OP_SUBSCRIPT:
            {
                Value indexValue = pop();
                Value listValue = pop();

                if (!IS_NUMBER(indexValue))
                {
                    runtimeError("Array index must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjList *list = AS_LIST(listValue);
                int index = AS_NUMBER(indexValue);

                if (index < 0)
                    index = list->values.count + index;
                if (index >= 0 && index < list->values.count)
                {
                    push(list->values.values[index]);
                    break;
                }

                runtimeError("Array index out of bounds.");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_SUBSCRIPT_ASSIGN:
            {
                Value assignValue = pop();
                Value indexValue = pop();
                Value listValue = pop();

                if (!IS_NUMBER(indexValue))
                {
                    runtimeError("Array index must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjList *list = AS_LIST(listValue);
                int index = AS_NUMBER(indexValue);

                if (index < 0)
                    index = list->values.count + index;
                if (index >= 0 && index < list->values.count)
                {
                    list->values.values[index] = assignValue;
                    push(NONE_VAL);
                    break;
                }

                push(NONE_VAL);

                runtimeError("Array index out of bounds.");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_SUBSCRIPT_DICT:
            {
                Value indexValue = pop();
                Value dictValue = pop();

                if (!IS_STRING(indexValue))
                {
                    runtimeError("Dictionary key must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjDict *dict = AS_DICT(dictValue);
                char *key = AS_CSTRING(indexValue);

                push(searchDict(dict, key));

                break;
            }
            case OP_SUBSCRIPT_DICT_ASSIGN:
            {
                Value value = pop();
                Value key = pop();
                Value dictValue = pop();

                if (!IS_STRING(key))
                {
                    runtimeError("Dictionary key must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjDict *dict = AS_DICT(dictValue);
                char *keyString = AS_CSTRING(key);

                insertDict(dict, keyString, value);

                push(NONE_VAL);
                break;
            }

            case OP_CALL:
            {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OP_INVOKE:
            {
                int argCount = READ_BYTE();
                ObjString *method = READ_STRING();
                //int argCount = instruction - OP_INVOKE_0;

                if (!invoke(method, argCount))
                    return INTERPRET_RUNTIME_ERROR;

                frame = &vm.frames[vm.frameCount - 1];
                break;
		    }

            case OP_SUPER: 
            {
                int argCount = READ_BYTE();
                ObjString *method = READ_STRING();
                //int argCount = instruction - OP_SUPER_0;
                ObjClass *superclass = AS_CLASS(pop());

                if (!invokeFromClass(superclass, method, argCount))
                    return INTERPRET_RUNTIME_ERROR;

                frame = &vm.frames[vm.frameCount - 1];
                break;
		    }

            case OP_CLOSURE:
            {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);          
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++)
                {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();                                  
                    if (isLocal)
                    {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    }
                    else
                    {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;                                               
            }

            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;

            case OP_RETURN:
            {
                Value result = pop();

                closeUpvalues(frame->slots);

                vm.frameCount--;                      
                if (vm.frameCount == 0)
                {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);

                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OP_CLASS: 
                createClass(READ_STRING(), NULL);
                break;

            case OP_SUBCLASS: 
            {
                Value superclass = peek(0);
                if (!IS_CLASS(superclass)) 
                {
                    runtimeError("Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                createClass(READ_STRING(), AS_CLASS(superclass));
                break;
            }
            case OP_METHOD: 
                defineMethod(READ_STRING());
                break;
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef READ_CONSTANT_LONG
    #undef READ_SHORT
    #undef READ_STRING
    #undef BINARY_OP
}

InterpretResult interpret(const char* source, const char* path, bool justCompile)
{
    ObjFunction* function = compile(source);
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    if(path != NULL)
    {
        char* newPath = ALLOCATE(char, strlen(path) + 10);
        strcpy(newPath, path);
        char *x;
        x = strrchr(newPath, '.');
        strcpy(x, ".ccube");

        uint32_t hash = hashString(source, strlen(source));

        writeByteCode(newPath, function, hash);
    }

    if(justCompile)
        return INTERPRET_OK;

    push(OBJ_VAL(function));

    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    callValue(OBJ_VAL(closure), 0);

    InterpretResult result = run();
    printf("\n");

    return result;
}