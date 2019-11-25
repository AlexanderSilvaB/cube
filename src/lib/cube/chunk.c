#include <stdlib.h>

#include "chunk.h"
#include "memory.h"  

void initChunk(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) 
{                      
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);  
    freeValueArray(&chunk->constants);
    initChunk(chunk);                                 
}  

void writeChunk(Chunk* chunk, uint8_t byte, int line)
{
    if(chunk->capacity < chunk->count + 1)
    {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(chunk->lines, int, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;     
    chunk->lines[chunk->count] = line;          
    chunk->count++;    
}

void writeConstant(Chunk* chunk, Value value, int line)
{
    int constant = addConstant(chunk, value);
    uint8_t* bytes = (uint8_t*)&constant;
    writeChunk(chunk, OP_CONSTANT_LONG, line);
    writeChunk(chunk, bytes[0], line);
    writeChunk(chunk, bytes[1], line);
    writeChunk(chunk, bytes[2], line);
}

int addConstant(Chunk* chunk, Value value)
{
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}