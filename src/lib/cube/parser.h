#ifndef _CUBE_PARSER_H_
#define _CUBE_PARSER_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "object.h"
#include "scanner.h"

#define MAX_CASES 256

typedef struct
{
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum
{
    PREC_NULL,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + - in is
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct
{
    Token name;
    int depth;
    bool isCaptured;
} Local;

typedef struct
{
    uint16_t index;
    bool isLocal;
} Upvalue;

typedef enum
{
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_EXTENSION,
    TYPE_STATIC,
    TYPE_SCRIPT,
    TYPE_EVAL
} FunctionType;

typedef struct Compiler
{
    struct Compiler *enclosing;
    ObjFunction *function;
    FunctionType type;

    // Local locals[UINT16_COUNT];
    Local *locals;
    int localCount;
    // Upvalue upvalues[UINT16_COUNT];
    Upvalue *upvalues;
    int scopeDepth;
    int loopDepth;

    const char *path;
} Compiler;

typedef struct ClassCompiler
{
    struct ClassCompiler *enclosing;

    Token name;
    bool hasSuperclass;
} ClassCompiler;

typedef struct BreakPoint_t
{
    int point;
    struct BreakPoint_t *next;
} BreakPoint;

typedef struct GlobalCompiler_t
{
    Scanner scanner;
    Parser parser;
    Compiler *current;
    ClassCompiler *currentClass;
    bool staticMethod;
    Documentation *currentDoc;
    int loopInCount;
    bool compilingToFile;
    int tempId;

    // Used for "continue" statements
    int innermostLoopStart;
    int innermostLoopScopeDepth;

    // Used for "break" statements
    BreakPoint *currentBreak;

    struct GlobalCompiler_t *previous;
} GlobalCompiler;

typedef struct GlobalTranspiler_t
{
    Scanner scanner;
    Parser parser;
    Compiler *current;
    ClassCompiler *currentClass;
    bool staticMethod;
    Documentation *currentDoc;
    int loopInCount;
    bool compilingToFile;
    int tempId;

    // Used for "continue" statements
    int innermostLoopStart;
    int innermostLoopScopeDepth;

    // Used for "break" statements
    BreakPoint *currentBreak;

    struct GlobalTranspiler_t *previous;
} GlobalTranspiler;

#endif
