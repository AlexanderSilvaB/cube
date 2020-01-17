#ifndef CLOX_chunk_h
#define CLOX_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
  OP_CONSTANT,
  OP_NONE,
  OP_TRUE,
  OP_FALSE,
  OP_EXPAND,
  OP_NEW_LIST,
  OP_ADD_LIST,
  OP_NEW_DICT,
  OP_ADD_DICT,
  OP_SUBSCRIPT,
  OP_SUBSCRIPT_ASSIGN,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_PROPERTY,
  OP_GET_PROPERTY_NO_POP,
  OP_SET_PROPERTY,
  OP_GET_SUPER,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_INC,
  OP_DEC,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MOD,
  OP_POW,
  OP_IN,
  OP_IS,
  OP_NOT,
  OP_NEGATE,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_BREAK,
  OP_DUP,
  OP_IMPORT,
  OP_REQUIRE,
  OP_CALL,
  OP_INVOKE,
  OP_SUPER,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
  OP_CLASS,
  OP_INHERIT,
  OP_METHOD,
  OP_PROPERTY,
  OP_FILE,
  OP_ASYNC,
  OP_AWAIT,
  OP_ABORT,
  OP_TRY,
  OP_CLOSE_TRY,
  OP_NATIVE_FUNC,
  OP_NATIVE,
  OP_REPL_POP,
  OP_TEST
} OpCode;

typedef struct {
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
  LineStart* lines;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
int getLine(Chunk* chunk, int instruction);

#endif
