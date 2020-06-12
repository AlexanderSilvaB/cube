#ifndef CLOX_chunk_h
#define CLOX_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
#define OPCODE(name) OP_##name,
#include "opcodes.h"
#undef OPCODE
} OpCode;

typedef struct
{
    int offset;
    int line;
} LineStart;

typedef struct
{
    int count;
    int capacity;
    uint8_t *code;
    ValueArray constants;
    int lineCount;
    int lineCapacity;
    LineStart *lines;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
int getLine(Chunk *chunk, int instruction);

#endif
