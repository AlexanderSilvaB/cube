#include "debug.h"
#include "object.h"
#include "value.h"
#include <stdio.h>
#include <string.h>

void disassembleChunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset)
{
    uint16_t constant = (uint16_t)(chunk->code[offset + 1] << 8);
    constant |= chunk->code[offset + 2];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

static int invokeInstruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t argCount = chunk->code[offset + 1];
    uint16_t constant = (uint16_t)(chunk->code[offset + 2] << 8);
    constant |= chunk->code[offset + 3];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 4;
}

static int extensionInstruction(const char *name, Chunk *chunk, int offset)
{
    uint16_t type = (uint16_t)(chunk->code[offset + 1] << 8);
    type |= chunk->code[offset + 2];
    uint16_t fn = (uint16_t)(chunk->code[offset + 3] << 8);
    fn |= chunk->code[offset + 4];
    printf("%-16s %4d %4d '", name, type, fn);
    printValue(chunk->constants.values[type]);
    printf("' '");
    printValue(chunk->constants.values[fn]);
    printf("'\n");
    return offset + 5;
}

static int simpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int shortInstruction(const char *name, Chunk *chunk, int offset)
{
    uint16_t slot = (uint16_t)(chunk->code[offset + 1] << 8);
    slot |= chunk->code[offset + 2];
    printf("%-16s %4d\n", name, slot);
    return offset + 3; // [debug]
}

static int byteInstruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2; // [debug]
}

static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int disassembleInstruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);
    int line = getLine(chunk, offset);
    if (offset > 0 && line == getLine(chunk, offset - 1))
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", line);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_STRING:
            return constantInstruction("OP_STRING", chunk, offset);
        case OP_NULL:
            return simpleInstruction("OP_NULL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_EXPAND:
            return simpleInstruction("OP_EXPAND", offset);
        case OP_NEW_LIST:
            return simpleInstruction("OP_NEW_LIST", offset);
        case OP_ADD_LIST:
            return simpleInstruction("OP_ADD_LIST", offset);
        case OP_NEW_DICT:
            return simpleInstruction("OP_NEW_DICT", offset);
        case OP_ADD_DICT:
            return simpleInstruction("OP_ADD_DICT", offset);
        case OP_SUBSCRIPT:
            return simpleInstruction("OP_SUBSCRIPT", offset);
        case OP_SUBSCRIPT_ASSIGN:
            return simpleInstruction("OP_SUBSCRIPT_ASSIGN", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_REPL_POP:
            return simpleInstruction("OP_REPL_POP", offset);
        case OP_GET_LOCAL:
            return shortInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return shortInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL_FORCED:
            return constantInstruction("OP_DEFINE_GLOBAL_FORCED", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_UPVALUE:
            return shortInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return shortInstruction("OP_SET_UPVALUE", chunk, offset);
        case OP_GET_PROPERTY:
            return constantInstruction("OP_GET_PROPERTY", chunk, offset);
        case OP_GET_PROPERTY_NO_POP:
            return constantInstruction("OP_GET_PROPERTY_NO_POP", chunk, offset);
        case OP_SET_PROPERTY:
            return constantInstruction("OP_SET_PROPERTY", chunk, offset);
        case OP_GET_SUPER:
            return constantInstruction("OP_GET_SUPER", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_INC:
            return simpleInstruction("OP_INC", offset);
        case OP_DEC:
            return simpleInstruction("OP_DEC", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_MOD:
            return simpleInstruction("OP_MOD", offset);
        case OP_POW:
            return simpleInstruction("OP_POW", offset);
        case OP_IN:
            return simpleInstruction("OP_IN", offset);
        case OP_IS:
            return simpleInstruction("OP_IS", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_SHIFT_LEFT:
            return simpleInstruction("OP_SHIFT_LEFT", offset);
        case OP_SHIFT_RIGHT:
            return simpleInstruction("OP_SHIFT_RIGHT", offset);
        case OP_BINARY_AND:
            return simpleInstruction("OP_BINARY_AND", offset);
        case OP_BINARY_OR:
            return simpleInstruction("OP_BINARY_OR", offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_BREAK:
            return jumpInstruction("OP_BREAK", 1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_INVOKE:
            return invokeInstruction("OP_INVOKE", chunk, offset);
        case OP_SUPER:
            return invokeInstruction("OP_SUPER_", chunk, offset);
        case OP_CLOSURE: {
            offset++;
            uint16_t constant = (uint16_t)(chunk->code[offset++] << 8);
            constant |= chunk->code[offset++];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            printValue(chunk->constants.values[constant]);
            printf("\n");

            ObjFunction *function = AS_FUNCTION(chunk->constants.values[constant]);
            for (int j = 0; j < function->upvalueCount; j++)
            {
                int isLocal = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf("%04d      |                     %s %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
            }

            return offset;
        }

        case OP_CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_IMPORT:
            return simpleInstruction("OP_IMPORT", offset);
        case OP_INCLUDE:
            return simpleInstruction("OP_INCLUDE", offset);
        case OP_FROM_PACKAGE:
            return simpleInstruction("OP_FROM_PACKAGE", offset);
        case OP_REMOVE_VAR:
            return simpleInstruction("OP_REMOVE_VAR", offset);
        case OP_REQUIRE:
            return simpleInstruction("OP_REQUIRE", offset);
        case OP_CLASS:
            return constantInstruction("OP_CLASS", chunk, offset);
        case OP_ENUM:
            return constantInstruction("OP_ENUM", chunk, offset);
        case OP_INHERIT:
            return simpleInstruction("OP_INHERIT", offset);
        case OP_METHOD:
            return constantInstruction("OP_METHOD", chunk, offset);
        case OP_PROPERTY:
            return constantInstruction("OP_PROPERTY", chunk, offset);
        case OP_EXTENSION:
            return extensionInstruction("OP_EXTENSION", chunk, offset);
        case OP_FILE:
            return simpleInstruction("OP_FILE", offset);
        case OP_ASYNC:
            return simpleInstruction("OP_ASYNC", offset);
        case OP_AWAIT:
            return simpleInstruction("OP_AWAIT", offset);
        case OP_ABORT:
            return simpleInstruction("OP_ABORT", offset);
        case OP_TRY:
            return jumpInstruction("OP_TRY", 1, chunk, offset);
        case OP_CLOSE_TRY:
            return jumpInstruction("OP_CLOSE_TRY", 1, chunk, offset);
        case OP_NATIVE_FUNC:
            return simpleInstruction("OP_NATIVE_FUNC", offset);
        case OP_NATIVE_STRUCT:
            return simpleInstruction("OP_NATIVE_STRUCT", offset);
        case OP_NATIVE:
            return constantInstruction("OP_NATIVE", chunk, offset);
        case OP_PASS:
            return simpleInstruction("OP_PASS", offset);
        case OP_SECURE_START:
            return simpleInstruction("OP_SECURE_START", offset);
        case OP_SECURE_END:
            return simpleInstruction("OP_SECURE_END", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
