#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "gc.h"
#include "memory.h"
#include "mempool.h"
#include "packer.h"
#include "parser.h"
#include "scanner.h"
#include "util.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

static char *initString = "<CUBE>";
GlobalCompiler *gbcpl = NULL;

void initGlobalCompiler()
{
    GlobalCompiler *current = (GlobalCompiler *)mp_malloc(sizeof(GlobalCompiler));
    current->current = NULL;
    current->currentClass = NULL;
    current->staticMethod = false;
    current->currentDoc = NULL;
    current->loopInCount = 0;
    current->compilingToFile = false;
    current->tempId = 0;
    current->innermostLoopStart = -1;
    current->innermostLoopScopeDepth = 0;
    current->currentBreak = NULL;
    memset(&current->scanner, '\0', sizeof(Scanner));
    memset(&current->parser, '\0', sizeof(Parser));
    current->previous = gbcpl;
    gbcpl = current;
}

void freeGlobalCompiler()
{
    GlobalCompiler *current = gbcpl;
    gbcpl = current->previous;
    mp_free(current);
}

static Chunk *currentChunk()
{
    return &gbcpl->current->function->chunk;
}

static void errorAt(Token *token, const char *message)
{
    if (gbcpl->parser.panicMode)
        return;
    gbcpl->parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // Nothing.
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    gbcpl->parser.hadError = true;
}

static void error(const char *message)
{
    errorAt(&gbcpl->parser.previous, message);
}

static void errorAtCurrent(const char *message)
{
    errorAt(&gbcpl->parser.current, message);
}

static void advance()
{
    gbcpl->parser.previous = gbcpl->parser.current;

    for (;;)
    {
        gbcpl->parser.current = scanToken(&gbcpl->scanner);
        if (gbcpl->parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(gbcpl->parser.current.start);
    }
}

static void consume(TokenType type, const char *message)
{
    if (gbcpl->parser.current.type == type)
    {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type)
{
    return gbcpl->parser.current.type == type;
}

static bool match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, gbcpl->parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

static void emitShort(uint8_t op, uint16_t value)
{
    emitByte(op);
    emitByte((value >> 8) & 0xFF);
    emitByte(value & 0xFF);
}

static void emitShortAlone(uint16_t value)
{
    emitByte((value >> 8) & 0xFF);
    emitByte(value & 0xFF);
}

static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX)
        error("Loop body too large.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitBreak()
{
    BreakPoint *bp = (BreakPoint *)mp_malloc(sizeof(BreakPoint));
    bp->point = emitJump(OP_BREAK);
    bp->next = NULL;

    if (gbcpl->currentBreak == NULL)
        gbcpl->currentBreak = bp;
    else
    {
        bp->next = gbcpl->currentBreak;
        gbcpl->currentBreak = bp;
    }
}

static void emitReturn()
{
    if (gbcpl->current->type == TYPE_INITIALIZER)
    {
        emitShort(OP_GET_LOCAL, 0);
    }
    else
    {
        emitByte(OP_NULL);
    }

    emitByte(OP_RETURN);
}

static uint16_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT16_MAX)
    {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint16_t)constant;
}

static void emitConstant(Value value)
{
    emitShort(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset)
{
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX)
    {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

static void patchBreak()
{
    while (gbcpl->currentBreak != NULL)
    {
        patchJump(gbcpl->currentBreak->point);
        BreakPoint *next = gbcpl->currentBreak->next;
        mp_free(gbcpl->currentBreak);
        gbcpl->currentBreak = next;
    }
}

static void initDoc()
{
    gbcpl->currentDoc = NULL;
}

static void endDoc()
{
    Documentation *old;
    while (gbcpl->currentDoc != NULL)
    {
        free(gbcpl->currentDoc->doc);
        old = gbcpl->currentDoc;
        gbcpl->currentDoc = old->next;
        free(old);
    }
}

static Documentation *getDoc()
{
    return gbcpl->currentDoc;
}

static void initCompiler(Compiler *compiler, FunctionType type)
{
    compiler->enclosing = gbcpl->current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->loopDepth = 0;
    compiler->function = newFunction(type == TYPE_STATIC);
    if (gbcpl->current != NULL)
        compiler->path = gbcpl->current->path;
    else
        compiler->path = "<null>";
    gbcpl->current = compiler;

    if (type != TYPE_SCRIPT && type != TYPE_EVAL)
    {
        gbcpl->current->function->name = copyString(gbcpl->parser.previous.start, gbcpl->parser.previous.length);
    }

    compiler->locals = mp_malloc(sizeof(Local) * UINT16_COUNT);
    compiler->upvalues = mp_malloc(sizeof(Upvalue) * UINT16_COUNT);

    Local *local = &gbcpl->current->locals[gbcpl->current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type == TYPE_INITIALIZER || type == TYPE_METHOD || type == TYPE_EXTENSION)
    {
        local->name.start = "this";
        local->name.length = 4;
    }
    else
    {
        local->name.start = "";
        local->name.length = 0;
    }
}

static ObjFunction *endCompiler()
{
    emitReturn();
    ObjFunction *fn = gbcpl->current->function;
    if (gbcpl->current->path == NULL)
        fn->path = NULL;
    else
    {
        char *path = mp_malloc(strlen(gbcpl->current->path) + 1);
        strcpy(path, gbcpl->current->path);
        fn->path = path;
    }

#ifdef DEBUG_PRINT_CODE
    if (!gbcpl->parser.hadError)
    {
        disassembleChunk(currentChunk(), fn->name != NULL ? fn->name->chars : "<script>");
    }
#endif

    mp_free(gbcpl->current->locals);
    mp_free(gbcpl->current->upvalues);

    gbcpl->current = gbcpl->current->enclosing;
    return fn;
}

static void beginScope()
{
    gbcpl->current->scopeDepth++;
}

static void endScope()
{
    gbcpl->current->scopeDepth--;

    while (gbcpl->current->localCount > 0 &&
           gbcpl->current->locals[gbcpl->current->localCount - 1].depth > gbcpl->current->scopeDepth)
    {
        if (gbcpl->current->locals[gbcpl->current->localCount - 1].isCaptured)
        {
            emitByte(OP_CLOSE_UPVALUE);
        }
        else
        {
            emitByte(OP_POP);
        }
        gbcpl->current->localCount--;
    }
}

static void expression();
static void statement();
static void declaration(bool checkEnd);
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint16_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b)
{
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name, bool inFunction)
{
    for (int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name))
        {
            if (!inFunction && local->depth == -1)
            {
                error("Cannot read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static int addUpvalue(Compiler *compiler, uint16_t index, bool isLocal)
{
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++)
    {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal)
        {
            return i;
        }
    }

    if (upvalueCount == UINT16_COUNT)
    {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name)
{
    if (compiler->enclosing == NULL)
        return -1;

    int local = resolveLocal(compiler->enclosing, name, true);
    if (local != -1)
    {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint16_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1)
    {
        return addUpvalue(compiler, (uint16_t)upvalue, false);
    }

    return -1;
}

static void addLocal(Token name)
{
    if (gbcpl->current->localCount == UINT16_COUNT)
    {
        error("Too many local variables in function.");
        return;
    }

    Local *local = &gbcpl->current->locals[gbcpl->current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

static uint16_t setVariablePop(Token name)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(gbcpl->current, &name, false);
    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else if ((arg = resolveUpvalue(gbcpl->current, &name)) != -1)
    {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    }
    else
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    emitShort(setOp, (uint16_t)arg);
    return (uint16_t)arg;
}

static uint16_t setVariable(Token name, Value value)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(gbcpl->current, &name, false);
    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else if ((arg = resolveUpvalue(gbcpl->current, &name)) != -1)
    {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    }
    else
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    emitConstant(value);
    emitShort(setOp, (uint16_t)arg);
    return (uint16_t)arg;
}

static uint16_t getVariable(Token name)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(gbcpl->current, &name, false);
    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else if ((arg = resolveUpvalue(gbcpl->current, &name)) != -1)
    {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    }
    else
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    emitShort(getOp, (uint16_t)arg);
    return (uint16_t)arg;
}

static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(gbcpl->current, &name, false);
    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else if ((arg = resolveUpvalue(gbcpl->current, &name)) != -1)
    {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    }
    else
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_PLUS_EQUALS))
    {
        namedVariable(name, false);
        expression();
        emitBytes(OP_ADD, OP_FALSE);
        emitShort(setOp, (uint16_t)arg);
    }
    else if (canAssign && match(TOKEN_MINUS_EQUALS))
    {
        namedVariable(name, false);
        expression();
        emitBytes(OP_SUBTRACT, OP_FALSE);
        emitShort(setOp, (uint16_t)arg);
    }
    else if (canAssign && match(TOKEN_MULTIPLY_EQUALS))
    {
        namedVariable(name, false);
        expression();
        emitBytes(OP_MULTIPLY, OP_FALSE);
        emitShort(setOp, (uint16_t)arg);
    }
    else if (canAssign && match(TOKEN_DIVIDE_EQUALS))
    {
        namedVariable(name, false);
        expression();
        emitBytes(OP_DIVIDE, OP_FALSE);
        emitShort(setOp, (uint16_t)arg);
    }
    else if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitShort(setOp, (uint16_t)arg);
    }
    else if (canAssign && match(TOKEN_INC))
    {
        namedVariable(name, false);
        emitByte(OP_INC);
        emitShort(setOp, (uint16_t)arg);
        emitConstant(NUMBER_VAL(1));
        emitBytes(OP_SUBTRACT, OP_FALSE);
    }
    else if (canAssign && match(TOKEN_DEC))
    {
        namedVariable(name, false);
        emitByte(OP_DEC);
        emitShort(setOp, (uint16_t)arg);
        emitConstant(NUMBER_VAL(1));
        emitBytes(OP_ADD, OP_FALSE);
    }
    else
    {
        emitShort(getOp, (uint16_t)arg);
    }
}

static void variable(bool canAssign)
{
    namedVariable(gbcpl->parser.previous, canAssign);
}

static Token syntheticToken(const char *text)
{
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

static void beginSyntheticCall(const char *name)
{
    Token tok = syntheticToken(name);
    getVariable(tok);
}

static void endSyntheticCall(uint8_t len)
{
    emitBytes(OP_CALL, len);
}

static void syntheticCall(const char *name, Value value)
{
    beginSyntheticCall(name);
    emitConstant(value);
    endSyntheticCall(1);
}

static void declareVariable()
{
    if (gbcpl->current->scopeDepth == 0)
        return;

    Token *name = &gbcpl->parser.previous;
    for (int i = gbcpl->current->localCount - 1; i >= 0; i--)
    {
        Local *local = &gbcpl->current->locals[i];
        if (local->depth != -1 && local->depth < gbcpl->current->scopeDepth)
        {
            break; // [negative]
        }

        if (identifiersEqual(name, &local->name))
        {
            error("Variable with this name already declared in this scope.");
        }
    }

    addLocal(*name);
}

static void declareNamedVariable(Token *name)
{
    if (gbcpl->current->scopeDepth == 0)
        return;

    for (int i = gbcpl->current->localCount - 1; i >= 0; i--)
    {
        Local *local = &gbcpl->current->locals[i];
        if (local->depth != -1 && local->depth < gbcpl->current->scopeDepth)
        {
            break; // [negative]
        }

        if (identifiersEqual(name, &local->name))
        {
            error("Variable with this name already declared in this scope.");
        }
    }

    addLocal(*name);
}

static uint16_t parseVariable(const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable();

    Token var = gbcpl->parser.previous;

    if (match(TOKEN_COLON))
    {
        consume(TOKEN_IDENTIFIER, "Only variable types allowed");
        Token type = gbcpl->parser.previous;
    }

    if (gbcpl->current->scopeDepth > 0)
        return 0;

    return identifierConstant(&var);
}

static void markInitialized()
{
    if (gbcpl->current->scopeDepth == 0)
        return;
    gbcpl->current->locals[gbcpl->current->localCount - 1].depth = gbcpl->current->scopeDepth;
}

static void defineVariable(uint16_t global)
{
    if (gbcpl->current->scopeDepth > 0)
    {
        markInitialized();
        return;
    }

    emitShort(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList()
{
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            expression();

            if (argCount == 255)
            {
                error("Cannot have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void and_(bool canAssign)
{
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void is(bool canAssign)
{
    if (match(TOKEN_BANG))
    {
        emitByte(OP_TRUE);
    }
    else
    {
        emitByte(OP_FALSE);
    }

    if (match(TOKEN_IDENTIFIER) || match(TOKEN_NULL) || match(TOKEN_FUNC) || match(TOKEN_CLASS) || match(TOKEN_ENUM))
    {
        ObjString *str = copyString(gbcpl->parser.previous.start, gbcpl->parser.previous.length);

        emitConstant(OBJ_VAL(str));
        emitByte(OP_IS);
    }
    else
        errorAtCurrent("Expected type name after 'is'.");
}

static void binary(bool canAssign)
{
    TokenType operatorType = gbcpl->parser.previous.type;

    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType)
    {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_SHIFT_LEFT:
            emitByte(OP_SHIFT_LEFT);
            break;
        case TOKEN_SHIFT_RIGHT:
            emitByte(OP_SHIFT_RIGHT);
            break;
        case TOKEN_BINARY_AND:
            emitByte(OP_BINARY_AND);
            break;
        case TOKEN_BINARY_OR:
            emitByte(OP_BINARY_OR);
            break;
        case TOKEN_PLUS:
        case TOKEN_DOT_PLUS:
            emitBytes(OP_ADD, operatorType == TOKEN_DOT_PLUS ? OP_TRUE : OP_FALSE);
            break;
        case TOKEN_MINUS:
        case TOKEN_DOT_MINUS:
            emitBytes(OP_SUBTRACT, operatorType == TOKEN_DOT_MINUS ? OP_TRUE : OP_FALSE);
            break;
        case TOKEN_STAR:
        case TOKEN_DOT_STAR:
            emitBytes(OP_MULTIPLY, operatorType == TOKEN_DOT_STAR ? OP_TRUE : OP_FALSE);
            break;
        case TOKEN_SLASH:
        case TOKEN_DOT_SLASH:
            emitBytes(OP_DIVIDE, operatorType == TOKEN_DOT_SLASH ? OP_TRUE : OP_FALSE);
            break;
        case TOKEN_PERCENT:
        case TOKEN_DOT_PERCENT:
            emitBytes(OP_MOD, operatorType == TOKEN_DOT_PERCENT ? OP_TRUE : OP_FALSE);
            break;
        case TOKEN_POW:
        case TOKEN_DOT_POW:
            emitBytes(OP_POW, operatorType == TOKEN_DOT_POW ? OP_TRUE : OP_FALSE);
            break;
        case TOKEN_IN:
            emitByte(OP_IN);
            break;
        default:
            return; // Unreachable.
    }
}

static void call(bool canAssign)
{
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void emitPreviousAsString()
{
    int len = gbcpl->parser.previous.length + 1;
    char *str = mp_malloc(sizeof(char) * len);
    strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
    str[gbcpl->parser.previous.length] = '\0';
    emitConstant(OBJ_VAL(copyString(str, strlen(str))));
    mp_free(str);
}

static char *getPreviousAsString()
{
    int len = gbcpl->parser.previous.length + 1;
    char *str = mp_malloc(sizeof(char) * len);
    strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
    str[gbcpl->parser.previous.length] = '\0';
    return str;
}

static void dot(bool canAssign)
{
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint16_t name = identifierConstant(&gbcpl->parser.previous);

    if (canAssign && match(TOKEN_PLUS_EQUALS))
    {
        emitShort(OP_GET_PROPERTY_NO_POP, name);
        expression();
        emitBytes(OP_ADD, OP_FALSE);
        emitShort(OP_SET_PROPERTY, name);
    }
    else if (canAssign && match(TOKEN_MINUS_EQUALS))
    {
        emitShort(OP_GET_PROPERTY_NO_POP, name);
        expression();
        emitBytes(OP_SUBTRACT, OP_FALSE);
        emitShort(OP_SET_PROPERTY, name);
    }
    else if (canAssign && match(TOKEN_MULTIPLY_EQUALS))
    {
        emitShort(OP_GET_PROPERTY_NO_POP, name);
        expression();
        emitBytes(OP_MULTIPLY, OP_FALSE);
        emitShort(OP_SET_PROPERTY, name);
    }
    else if (canAssign && match(TOKEN_DIVIDE_EQUALS))
    {
        emitShort(OP_GET_PROPERTY_NO_POP, name);
        expression();
        emitBytes(OP_DIVIDE, OP_FALSE);
        emitShort(OP_SET_PROPERTY, name);
    }
    else if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitShort(OP_SET_PROPERTY, name);
    }
    else if (match(TOKEN_LEFT_PAREN))
    {
        uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, argCount);
        emitShortAlone(name);
    }
    else
    {
        emitShort(OP_GET_PROPERTY, name);
    }
}

static void literal(bool canAssign)
{
    switch (gbcpl->parser.previous.type)
    {
        case TOKEN_FALSE:
            emitByte(OP_FALSE);
            break;
        case TOKEN_NULL:
            emitByte(OP_NULL);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE);
            break;
        default:
            return; // Unreachable.
    }
}

static void grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
    double value;
    if (gbcpl->parser.previous.type == TOKEN_NAN)
        value = NAN;
    else if (gbcpl->parser.previous.type == TOKEN_INF)
        value = INFINITY;
    else
        value = strtod(gbcpl->parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void byte(bool canAssign)
{
    char *string = NULL;

    size_t slength = gbcpl->parser.previous.length - 2;
    if ((slength % 2) != 0) // must be even
    {
        string = mp_malloc(slength + 1);
        memcpy(string, gbcpl->parser.previous.start + 2, slength);
        string[slength] = string[slength - 1];
        string[slength - 1] = '0';
        slength++;
    }
    else
    {
        string = mp_malloc(slength);
        memcpy(string, gbcpl->parser.previous.start + 2, slength);
    }

    size_t dlength = slength / 2;

    uint8_t *data = mp_malloc(dlength);
    memset(data, 0, dlength);

    size_t index = 0;
    while (index < slength)
    {
        char c = string[index];
        int value = 0;
        if (c >= '0' && c <= '9')
            value = (c - '0');
        else if (c >= 'A' && c <= 'F')
            value = (10 + (c - 'A'));
        else if (c >= 'a' && c <= 'f')
            value = (10 + (c - 'a'));
        else
        {
            errorAtCurrent("Invalid hex characters.");
            break;
        }

        data[(index / 2)] += value << (((index + 1) % 2) * 4);
        index++;
    }

    emitConstant(BYTES_VAL(data, dlength));
    mp_free(data);
    mp_free(string);
    // return data;
    // emitConstant(BYTES_VAL(gbcpl->parser.previous.start, gbcpl->parser.previous.length));
}

static void or_(bool canAssign)
{
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void string(bool canAssign)
{
    int num_id = 0;
    int slen = gbcpl->parser.previous.length;
    char *str = mp_malloc(sizeof(char) * slen);
    int j = 0;
    for (int i = 1; i < gbcpl->parser.previous.length - 1; i++)
    {
        if (gbcpl->parser.previous.start[i] == '\\' && i < gbcpl->parser.previous.length - 2)
        {
            if (gbcpl->parser.previous.start[i + 1] == 'n')
            {
                i++;
                str[j++] = '\n';
            }
            else if (gbcpl->parser.previous.start[i + 1] == 'r')
            {
                i++;
                str[j++] = '\r';
            }
            else if (gbcpl->parser.previous.start[i + 1] == 't')
            {
                i++;
                str[j++] = '\t';
            }
            else if (gbcpl->parser.previous.start[i + 1] == 'b')
            {
                i++;
                str[j++] = '\b';
            }
            else if (gbcpl->parser.previous.start[i + 1] == 'v')
            {
                i++;
                str[j++] = '\v';
            }
            else if (gbcpl->parser.previous.start[i + 1] == '$' && i < gbcpl->parser.previous.length - 3 &&
                     gbcpl->parser.previous.start[i + 2] == '{')
            {
                i++;
                str[j++] = '$';
                i++;
                str[j++] = '{';
            }
            else
            {
                str[j++] == '\\';
            }
        }
        else if (gbcpl->parser.previous.start[i] == '$' && i < gbcpl->parser.previous.length - 4 &&
                 gbcpl->parser.previous.start[i + 1] == '{')
        {
            str[j++] = gbcpl->parser.previous.start[i++];
            str[j++] = gbcpl->parser.previous.start[i++];
            int s = i;
            int n = 0;
            while (gbcpl->parser.previous.start[i] != '}' && i < gbcpl->parser.previous.length)
            {
                if (n == 0)
                {
                    if (!isAlpha(gbcpl->parser.previous.start[i]))
                    {
                        errorAtCurrent("Invalid identifier in string.");
                        return;
                    }
                }
                else
                {
                    if (!isAlpha(gbcpl->parser.previous.start[i]) && !isDigit(gbcpl->parser.previous.start[i]))
                    {
                        errorAtCurrent("Invalid identifier in string.");
                        return;
                    }
                }

                i++;
                n++;
            }
            if (i == gbcpl->parser.previous.length)
            {
                errorAtCurrent("Invalid identifier end in string.");
                return;
            }
            char *id = mp_malloc(sizeof(char) * (n + 1));
            memcpy(id, &gbcpl->parser.previous.start[s], n);
            id[n] = '\0';

            slen += 6;
            str = mp_realloc(str, slen);
            j += sprintf(str + j, "__%d__}", num_id++);

            getVariable(syntheticToken(id));

            mp_free(id);
            id = NULL;
        }
        else
            str[j++] = gbcpl->parser.previous.start[i];
    }

    str[j] = '\0';

    if (num_id == 0)
        emitConstant(STRING_VAL(str));
    else
    {
        emitConstant(NUMBER_VAL(num_id));
        emitShort(OP_STRING, makeConstant(STRING_VAL(str)));
    }

    mp_free(str);
}

static void list(bool canAssign)
{
    emitByte(OP_NEW_LIST);

    do
    {
        if (check(TOKEN_RIGHT_BRACKET))
            break;

        expression();
        emitByte(OP_ADD_LIST);
    } while (match(TOKEN_COMMA));

    consume(TOKEN_RIGHT_BRACKET, "Expected closing ']'");
}

static void dict(bool canAssign)
{
    emitByte(OP_NEW_DICT);

    do
    {
        if (check(TOKEN_RIGHT_BRACE))
            break;

        parsePrecedence(PREC_UNARY);
        consume(TOKEN_COLON, "Expected ':'");
        expression();
        emitByte(OP_ADD_DICT);
    } while (match(TOKEN_COMMA));

    consume(TOKEN_RIGHT_BRACE, "Expected closing '}'");
}

static void subscript(bool canAssign)
{
    expression();

    consume(TOKEN_RIGHT_BRACKET, "Expected closing ']'");

    if (match(TOKEN_EQUAL))
    {
        expression();
        emitByte(OP_SUBSCRIPT_ASSIGN);
    }
    else
        emitByte(OP_SUBSCRIPT);
}

static void pushSuperclass()
{
    if (gbcpl->currentClass == NULL)
        return;
    namedVariable(syntheticToken("super"), false);
}

static void super_(bool canAssign)
{
    if (gbcpl->currentClass == NULL)
    {
        error("Cannot use 'super' outside of a class.");
    }
    else if (!gbcpl->currentClass->hasSuperclass)
    {
        error("Cannot use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint16_t name = identifierConstant(&gbcpl->parser.previous);

    namedVariable(syntheticToken("this"), false);

    if (match(TOKEN_LEFT_PAREN))
    {
        uint8_t argCount = argumentList();

        pushSuperclass();
        emitBytes(OP_SUPER, argCount);
        emitShortAlone(name);
    }
    else
    {
        pushSuperclass();
        emitShort(OP_GET_SUPER, name);
    }
}

static void this_(bool canAssign)
{
    if (gbcpl->current->type == TYPE_EXTENSION)
    {
        variable(false);
    }
    else if (gbcpl->currentClass == NULL)
    {
        error("Cannot use 'this' outside of a class.");
    }
    else if (gbcpl->staticMethod)
        error("Cannot use 'this' inside a static method.");
    else
    {
        variable(false);
    }
}

static void unary(bool canAssign)
{
    TokenType operatorType = gbcpl->parser.previous.type;

    parsePrecedence(PREC_UNARY);
    switch (operatorType)
    {
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return; // Unreachable.
    }
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration(true);
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static uint16_t createSyntheticVariable(const char *name, Token *token)
{
    *token = syntheticToken(name);
    declareNamedVariable(token);
    uint16_t it;
    if (gbcpl->current->scopeDepth > 0)
        it = 0;
    else
        it = identifierConstant(token);
    return it;
}

static void function(FunctionType type)
{
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope(); // [no-end-scope]

    // Compile the parameter list.
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            gbcpl->current->function->arity++;
            if (gbcpl->current->function->arity > 255)
            {
                errorAtCurrent("Cannot have more than 255 parameters.");
            }

            uint16_t paramConstant = parseVariable("Expect parameter name.");
            // if (match(TOKEN_EQUAL))
            // {
            //   expression();
            // }
            // else
            // {
            //   emitConstant(NUMBER_VAL(gbcpl->current->function->arity));
            // }

            defineVariable(paramConstant);
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    // Create args
    Token argsToken;
    uint16_t args = createSyntheticVariable("args", &argsToken);
    defineVariable(args);
    Token argsInternToken = syntheticToken(vm.argsString);
    getVariable(argsInternToken);
    setVariablePop(argsToken);

    // The body.
    if (match(TOKEN_EQUAL))
    {
        if (!match(TOKEN_GREATER))
            errorAtCurrent("Only scoped '{}' and short '=>' functions allowed.");
        expression();
        match(TOKEN_SEMICOLON);
        emitByte(OP_RETURN);
    }
    else
    {
        consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        block();
    }

    // Check this out
    // endScope();

    if (type == TYPE_STATIC)
        gbcpl->staticMethod = false;

    // Create the function object.
    ObjFunction *fn = endCompiler();
    emitShort(OP_CLOSURE, makeConstant(OBJ_VAL(fn)));

    for (int i = 0; i < fn->upvalueCount; i++)
    {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitShortAlone(compiler.upvalues[i].index);
    }
}

static void lambda(bool canAssign)
{
    TokenType operatorType = gbcpl->parser.previous.type;
    // markInitialized();
    function(TYPE_FUNCTION);
}

static void expand(bool canAssign)
{
    bool ex = gbcpl->parser.previous.type == TOKEN_EXPAND_EX;
    parsePrecedence(PREC_UNARY);

    if (match(TOKEN_EXPAND_IN) || match(TOKEN_EXPAND_EX))
    {
        ex = gbcpl->parser.previous.type == TOKEN_EXPAND_EX;
        parsePrecedence(PREC_UNARY);
    }
    else
    {
        emitByte(OP_NULL);
    }

    emitByte(ex ? OP_TRUE : OP_FALSE);

    emitByte(OP_EXPAND);
}

static void prefix(bool canAssign)
{
    TokenType operatorType = gbcpl->parser.previous.type;
    Token cur = gbcpl->parser.current;
    consume(TOKEN_IDENTIFIER, "Expected variable");
    namedVariable(gbcpl->parser.previous, true);

    int arg;
    bool instance = false;

    if (match(TOKEN_DOT))
    {
        consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
        arg = identifierConstant(&gbcpl->parser.previous);
        emitShort(OP_GET_PROPERTY_NO_POP, arg);
        instance = true;
    }

    switch (operatorType)
    {
        case TOKEN_INC:
            emitByte(OP_INC);
            break;
        case TOKEN_DEC:
            emitByte(OP_DEC);
            break;
        default:
            return;
    }

    if (instance)
        emitShort(OP_SET_PROPERTY, arg);
    else
    {
        uint8_t setOp;
        // int arg = resolveLocal(current, &cur, false);
        int arg = resolveLocal(gbcpl->current, &cur, false);

        if (arg != -1)
            setOp = OP_SET_LOCAL;
        else if ((arg = resolveUpvalue(gbcpl->current, &cur)) != -1)
            setOp = OP_SET_UPVALUE;
        else
        {
            arg = identifierConstant(&cur);
            setOp = OP_SET_GLOBAL;
        }

        emitShort(setOp, (uint16_t)arg);
    }
}

static void static_(bool canAssign)
{
    if (gbcpl->currentClass == NULL)
        error("Cannot use 'static' outside of a class.");
}

static void require(bool canAssign)
{
    consume(TOKEN_LEFT_PAREN, "In 'require' a package name must be passed between parentesis.");
    uint8_t argCount = argumentList();
    if (argCount != 1)
        error("Required accepts only one parameter.");
    emitByte(OP_REQUIRE);
}

static void await(bool canAssign)
{
    consume(TOKEN_IDENTIFIER, "Expect a function call in await.");
    getVariable(gbcpl->parser.previous);
    emitByte(OP_AWAIT);
}

static void async(bool canAssign)
{
    consume(TOKEN_IDENTIFIER, "Expect a function call in async.");

    Compiler compiler;
    initCompiler(&compiler, TYPE_FUNCTION);
    beginScope(); // [no-end-scope]

    // Create args
    Token argsToken = syntheticToken("args");
    uint16_t args = identifierConstant(&argsToken);
    getVariable(syntheticToken(vm.argsString));
    defineVariable(args);

    namedVariable(gbcpl->parser.previous, false);

    consume(TOKEN_LEFT_PAREN, "Expect a function function call in async.");
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);

    emitByte(OP_RETURN);

    // Create the function object.
    ObjFunction *fn = endCompiler();
    emitShort(OP_CLOSURE, makeConstant(OBJ_VAL(fn)));

    for (int i = 0; i < fn->upvalueCount; i++)
    {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitShortAlone(compiler.upvalues[i].index);
    }

    emitByte(OP_ASYNC);
}

ParseRule rules[] = {
    {grouping, call, PREC_CALL},     // TOKEN_LEFT_PAREN
    {NULL, NULL, PREC_NULL},         // TOKEN_RIGHT_PAREN
    {dict, NULL, PREC_NULL},         // TOKEN_LEFT_BRACE [big]
    {NULL, NULL, PREC_NULL},         // TOKEN_RIGHT_BRACE
    {list, subscript, PREC_CALL},    // TOKEN_LEFT_BRACKET
    {NULL, NULL, PREC_NULL},         // TOKEN_RIGHT_BRACKET
    {NULL, NULL, PREC_NULL},         // TOKEN_COMMA
    {NULL, dot, PREC_CALL},          // TOKEN_DOT
    {NULL, expand, PREC_FACTOR},     // TOKEN_EXPAND_IN
    {NULL, expand, PREC_FACTOR},     // TOKEN_EXPAND_EX
    {unary, binary, PREC_TERM},      // TOKEN_MINUS
    {NULL, binary, PREC_TERM},       // TOKEN_PLUS
    {NULL, binary, PREC_TERM},       // TOKEN_DOT_PLUS
    {NULL, binary, PREC_TERM},       // TOKEN_DOT_MINUS
    {NULL, binary, PREC_TERM},       // TOKEN_DOT_STAR
    {NULL, binary, PREC_TERM},       // TOKEN_DOT_SLASH
    {NULL, binary, PREC_TERM},       // TOKEN_DOT_POW
    {NULL, binary, PREC_TERM},       // TOKEN_DOT_PERCENT
    {prefix, NULL, PREC_NULL},       // TOKEN_INC
    {prefix, NULL, PREC_NULL},       // TOKEN_DEC
    {NULL, NULL, PREC_NULL},         // TOKEN_PLUS_EQUALS
    {NULL, NULL, PREC_NULL},         // TOKEN_MINUS_EQUALS
    {NULL, NULL, PREC_NULL},         // TOKEN_MULTIPLY_EQUALS
    {NULL, NULL, PREC_NULL},         // TOKEN_DIVIDE_EQUALS
    {NULL, NULL, PREC_NULL},         // TOKEN_SEMICOLON
    {NULL, NULL, PREC_NULL},         // TOKEN_COLON
    {NULL, binary, PREC_FACTOR},     // TOKEN_SLASH
    {NULL, binary, PREC_FACTOR},     // TOKEN_STAR
    {NULL, binary, PREC_FACTOR},     // TOKEN_PERCENT
    {NULL, binary, PREC_FACTOR},     // TOKEN_POW
    {unary, NULL, PREC_NULL},        // TOKEN_BANG
    {NULL, binary, PREC_EQUALITY},   // TOKEN_BANG_EQUAL
    {NULL, NULL, PREC_NULL},         // TOKEN_EQUAL
    {NULL, binary, PREC_EQUALITY},   // TOKEN_EQUAL_EQUAL
    {NULL, binary, PREC_COMPARISON}, // TOKEN_GREATER
    {NULL, binary, PREC_COMPARISON}, // TOKEN_GREATER_EQUAL
    {NULL, binary, PREC_COMPARISON}, // TOKEN_LESS
    {NULL, binary, PREC_COMPARISON}, // TOKEN_LESS_EQUAL
    {NULL, binary, PREC_EQUALITY},   // TOKEN_SHIFT_LEFT
    {NULL, binary, PREC_EQUALITY},   // TOKEN_SHIFT_RIGHT
    {NULL, binary, PREC_EQUALITY},   // TOKEN_BINARY_AND
    {NULL, binary, PREC_EQUALITY},   // TOKEN_BINARY_OR
    {variable, NULL, PREC_NULL},     // TOKEN_IDENTIFIER
    {string, NULL, PREC_NULL},       // TOKEN_STRING
    {number, NULL, PREC_NULL},       // TOKEN_NUMBER
    {byte, NULL, PREC_NULL},         // TOKEN_BYTE
    {NULL, and_, PREC_AND},          // TOKEN_AND
    {NULL, NULL, PREC_NULL},         // TOKEN_CLASS
    {static_, NULL, PREC_NULL},      // TOKEN_STATIC
    {NULL, NULL, PREC_NULL},         // TOKEN_ELSE
    {literal, NULL, PREC_NULL},      // TOKEN_FALSE
    {NULL, NULL, PREC_NULL},         // TOKEN_FOR
    {NULL, NULL, PREC_NULL},         // TOKEN_FUNC
    {lambda, NULL, PREC_NULL},       // TOKEN_LAMBDA
    {NULL, NULL, PREC_NULL},         // TOKEN_IF
    {literal, NULL, PREC_NULL},      // TOKEN_NULL
    {NULL, or_, PREC_OR},            // TOKEN_OR
    {NULL, NULL, PREC_NULL},         // TOKEN_RETURN
    {super_, NULL, PREC_NULL},       // TOKEN_SUPER
    {this_, NULL, PREC_NULL},        // TOKEN_THIS
    {literal, NULL, PREC_NULL},      // TOKEN_TRUE
    {NULL, NULL, PREC_NULL},         // TOKEN_VAR
    {NULL, NULL, PREC_NULL},         // TOKEN_GLOBAL
    {NULL, NULL, PREC_NULL},         // TOKEN_WHILE
    {NULL, NULL, PREC_NULL},         // TOKEN_DO
    {NULL, binary, PREC_TERM},       // TOKEN_IN
    {NULL, is, PREC_TERM},           // TOKEN_IS
    {NULL, NULL, PREC_NULL},         // TOKEN_CONTINUE
    {NULL, NULL, PREC_NULL},         // TOKEN_BREAK
    {NULL, NULL, PREC_NULL},         // TOKEN_SWITCH
    {NULL, NULL, PREC_NULL},         // TOKEN_CASE
    {NULL, NULL, PREC_NULL},         // TOKEN_DEFAULT
    {number, NULL, PREC_NULL},       // TOKEN_NAN
    {number, NULL, PREC_NULL},       // TOKEN_INF
    {NULL, NULL, PREC_NULL},         // TOKEN_ENUM
    {NULL, NULL, PREC_NULL},         // TOKEN_IMPORT
    {require, NULL, PREC_NULL},      // TOKEN_REQUIRE
    {NULL, NULL, PREC_NULL},         // TOKEN_INCLUDE
    {NULL, NULL, PREC_NULL},         // TOKEN_AS
    {NULL, NULL, PREC_NULL},         // TOKEN_NATIVE
    {NULL, NULL, PREC_NULL},         // TOKEN_STRUCT
    {NULL, NULL, PREC_NULL},         // TOKEN_WITH
    {async, NULL, PREC_NULL},        // TOKEN_ASYNC
    {await, NULL, PREC_NULL},        // TOKEN_AWAIT
    {NULL, NULL, PREC_NULL},         // TOKEN_ABORT
    {NULL, NULL, PREC_NULL},         // TOKEN_TRY
    {NULL, NULL, PREC_NULL},         // TOKEN_CATCH
    {NULL, NULL, PREC_NULL},         // TOKEN_ASSERT
    {NULL, NULL, PREC_NULL},         // TOKEN_DOC
    {NULL, NULL, PREC_NULL},         // TOKEN_PASS
    {NULL, NULL, PREC_NULL},         // TOKEN_CUBE
    {NULL, NULL, PREC_NULL},         // TOKEN_ERROR
    {NULL, NULL, PREC_NULL},         // TOKEN_EOF
};

static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefixRule = getRule(gbcpl->parser.previous.type)->prefix;
    if (prefixRule == NULL)
    {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(gbcpl->parser.current.type)->precedence)
    {
        advance();
        ParseFn infixRule = getRule(gbcpl->parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL))
    {
        error("Invalid assignment target.");
    }
}

static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

static void method(bool isStatic)
{
    FunctionType type;

    if (isStatic)
    {
        type = TYPE_STATIC;
        gbcpl->staticMethod = true;
    }
    else
        type = TYPE_METHOD;

    consume(TOKEN_FUNC, "Expect a function declaration.");
    bool bracked = false;
    bool set = false;
    if (!check(TOKEN_IDENTIFIER) && !isOperator(gbcpl->parser.current.type))
    {
        if (match(TOKEN_LEFT_BRACKET))
        {
            if (match(TOKEN_RIGHT_BRACKET))
            {
                bracked = true;
            }
            else if (match(TOKEN_EQUAL))
            {
                if (match(TOKEN_RIGHT_BRACKET))
                {
                    bracked = true;
                    set = true;
                }
            }
        }
        if (!bracked)
            errorAtCurrent("Expect method name.");
    }
    if (!bracked)
        advance();

    uint16_t constant;
    if (!bracked)
        constant = identifierConstant(&gbcpl->parser.previous);
    else
    {
        Token br = syntheticToken(set ? "[=]" : "[]");
        constant = identifierConstant(&br);
    }

    // If the method is named "init", it's an initializer.
    if (gbcpl->parser.previous.length == 4 && memcmp(gbcpl->parser.previous.start, "init", 4) == 0)
    {
        type = TYPE_INITIALIZER;
    }

    function(type);

    emitShort(OP_METHOD, constant);
}

static void property(bool isStatic, Token *className)
{
    // uint16_t name = parseVariable("Expect variable name.");
    uint16_t name;

    while (true)
    {
        consume(TOKEN_IDENTIFIER, "Expect variable name.");
        name = identifierConstant(&gbcpl->parser.previous);
        // bool isLocal = (gbcpl->current->scopeDepth > 0);

        if (match(TOKEN_COLON))
        {
            consume(TOKEN_IDENTIFIER, "Only variable types allowed");
            Token type = gbcpl->parser.previous;
        }

        if (match(TOKEN_EQUAL))
        {
            expression();
        }
        else
        {
            emitByte(OP_NULL);
        }

        // emitByte(isLocal ?  OP_TRUE : OP_FALSE);
        emitByte(isStatic ? OP_TRUE : OP_FALSE);
        emitShort(OP_PROPERTY, name);
        if (!match(TOKEN_COMMA))
            break;
        else
            namedVariable(*className, false);
    }

    // consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    match(TOKEN_SEMICOLON);
}

static void methodOrProperty(Token *className)
{
    bool isStatic = false;
    if (match(TOKEN_STATIC))
    {
        isStatic = true;
    }

    if (check(TOKEN_FUNC))
    {
        method(isStatic);
    }
    else if (check(TOKEN_VAR))
    {
        consume(TOKEN_VAR, "Expected a variable declaration.");
        property(isStatic, className);
    }
    else
    {
        errorAtCurrent("Only variables and functions allowed inside a class.");
    }
}

static void nativeFunc()
{
    if (!match(TOKEN_VAR))
        consume(TOKEN_IDENTIFIER, "Expect type name.");
    int len = gbcpl->parser.previous.length + 1;
    char *str = mp_malloc(sizeof(char) * len);
    strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
    str[gbcpl->parser.previous.length] = '\0';
    emitConstant(OBJ_VAL(copyString(str, strlen(str))));
    mp_free(str);

    consume(TOKEN_IDENTIFIER, "Expect function name.");
    Token name = gbcpl->parser.previous;
    uint16_t nameConstant = identifierConstant(&gbcpl->parser.previous);
    declareVariable();

    // uint8_t name = parseVariable("Expect funcion name.");
    // Token tokName = gbcpl->parser.previous;

    // consume(TOKEN_IDENTIFIER, "Expect function name.");
    len = gbcpl->parser.previous.length + 1;
    str = mp_malloc(sizeof(char) * len);
    strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
    str[gbcpl->parser.previous.length] = '\0';
    emitConstant(OBJ_VAL(copyString(str, strlen(str))));
    mp_free(str);

    // Compile the parameter list.
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    int arity = 0;

    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            arity++;
            // if (arity > 10)
            // {
            //     errorAtCurrent("Cannot have more than 10 parameters.");
            // }

            if (!match(TOKEN_VAR))
            {
                if (!match(TOKEN_FUNC))
                    consume(TOKEN_IDENTIFIER, "Expect parameter type.");
            }
            len = gbcpl->parser.previous.length + 1;
            str = mp_malloc(sizeof(char) * len);
            strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
            str[gbcpl->parser.previous.length] = '\0';
            emitConstant(OBJ_VAL(copyString(str, strlen(str))));
            mp_free(str);

        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    // consume(TOKEN_SEMICOLON, "Expect ';' after native func declaration.");
    match(TOKEN_SEMICOLON);

    emitConstant(NUMBER_VAL(arity));

    emitByte(OP_NULL);
    defineVariable(nameConstant);

    emitByte(OP_NATIVE_FUNC);

    setVariablePop(name);
}

static void nativeStruct()
{
    if (!match(TOKEN_STRUCT))
        consume(TOKEN_STRUCT, "Expect struct type.");

    consume(TOKEN_IDENTIFIER, "Expect struct name.");
    Token name = gbcpl->parser.previous;
    uint16_t nameConstant = identifierConstant(&gbcpl->parser.previous);
    declareVariable();

    int len = gbcpl->parser.previous.length + 1;
    char *str = mp_malloc(sizeof(char) * len);
    strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
    str[gbcpl->parser.previous.length] = '\0';
    emitConstant(OBJ_VAL(copyString(str, strlen(str))));
    mp_free(str);

    // Compile the members list.
    consume(TOKEN_LEFT_BRACE, "Expect '{' after struct name.");
    int members = 0;

    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            members++;

            // Type
            if (!match(TOKEN_VAR))
            {
                if (!match(TOKEN_FUNC))
                    consume(TOKEN_IDENTIFIER, "Expect member type.");
            }
            len = gbcpl->parser.previous.length + 1;
            str = mp_malloc(sizeof(char) * len);
            strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
            str[gbcpl->parser.previous.length] = '\0';
            emitConstant(OBJ_VAL(copyString(str, strlen(str))));
            mp_free(str);

            // Name
            consume(TOKEN_IDENTIFIER, "Expect member name.");
            len = gbcpl->parser.previous.length + 1;
            str = mp_malloc(sizeof(char) * len);
            strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
            str[gbcpl->parser.previous.length] = '\0';
            emitConstant(OBJ_VAL(copyString(str, strlen(str))));
            mp_free(str);

            match(TOKEN_SEMICOLON);

            if (check(TOKEN_RIGHT_BRACE))
                break;

        } while (true);
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after parameters.");
    match(TOKEN_SEMICOLON);

    emitConstant(NUMBER_VAL(members));

    emitByte(OP_NULL);
    defineVariable(nameConstant);

    emitByte(OP_NATIVE_STRUCT);

    setVariablePop(name);
}

static void nativeFuncOrStruct()
{
    if (check(TOKEN_STRUCT))
        nativeStruct();
    else
        nativeFunc();
}

static void classDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = gbcpl->parser.previous;
    uint16_t nameConstant = identifierConstant(&gbcpl->parser.previous);
    declareVariable();

    emitShort(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.name = gbcpl->parser.previous;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = gbcpl->currentClass;
    gbcpl->currentClass = &classCompiler;

    if (match(TOKEN_COLON))
    {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");

        if (identifiersEqual(&className, &gbcpl->parser.previous))
        {
            error("A class cannot inherit from itself.");
        }

        classCompiler.hasSuperclass = true;

        beginScope();

        // Store the superclass in a local variable named "super".
        variable(false);
        addLocal(syntheticToken("super"));
        defineVariable(0);

        namedVariable(className, false);
        emitByte(OP_INHERIT);
    }

    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        namedVariable(className, false);
        methodOrProperty(&className);
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");

    if (classCompiler.hasSuperclass)
    {
        endScope();
    }

    gbcpl->currentClass = gbcpl->currentClass->enclosing;
}

static void enumMember(Token *enumName)
{
    uint16_t name;

    while (true)
    {
        consume(TOKEN_IDENTIFIER, "Expect member name.");
        name = identifierConstant(&gbcpl->parser.previous);

        if (match(TOKEN_EQUAL))
        {
            expression();
        }
        else
        {
            emitByte(OP_NULL);
        }

        emitByte(OP_FALSE);
        emitShort(OP_PROPERTY, name);
        if (!match(TOKEN_COMMA))
            break;
        else
            namedVariable(*enumName, false);
    }

    match(TOKEN_SEMICOLON);
}

static void enumDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect enum name.");
    Token enumName = gbcpl->parser.previous;
    uint16_t nameConstant = identifierConstant(&gbcpl->parser.previous);
    declareVariable();

    emitShort(OP_ENUM, nameConstant);
    defineVariable(nameConstant);

    consume(TOKEN_LEFT_BRACE, "Expect '{' before enum body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        namedVariable(enumName, false);
        enumMember(&enumName);
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after enum body.");
}

static void pathOrString(const char *extension, const char *errorIdentifier, const char *errorString)
{
    if (match(TOKEN_IDENTIFIER))
    {
        int len = gbcpl->parser.previous.length;
        int totalLen = len;
        char *str = ALLOCATE(char, len + 1);
        strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
        str[len] = '\0';

        while (true)
        {
            if (match(TOKEN_SLASH))
            {
                if (match(TOKEN_IDENTIFIER))
                {
                    totalLen += gbcpl->parser.previous.length + 1;
                    str = GROW_ARRAY(str, char, len + 1, totalLen + 1);
                    str[len] = '/';
                    strncpy(str + (len + 1), gbcpl->parser.previous.start, gbcpl->parser.previous.length);
                    len = totalLen;
                    str[len] = '\0';
                }
                else
                {
                    errorAtCurrent(errorIdentifier);
                }
            }
            else
            {
                break;
            }
        }

        totalLen = len + strlen(extension);
        str = GROW_ARRAY(str, char, len + 1, totalLen + 1);
        len = totalLen;
        strcat(str, extension);

        emitConstant(OBJ_VAL(copyString(str, strlen(str))));
        FREE_ARRAY(char, str, totalLen + 1);
    }
    else
    {
        consume(TOKEN_STRING, errorString);
        emitConstant(OBJ_VAL(copyString(gbcpl->parser.previous.start + 1, gbcpl->parser.previous.length - 2)));
    }
}

static int pathOrStringNoEmit(char **p, const char *extension, const char *errorIdentifier, const char *errorString)
{
    if (match(TOKEN_IDENTIFIER))
    {
        int len = gbcpl->parser.previous.length;
        int totalLen = len;
        char *str = ALLOCATE(char, len + 1);
        strncpy(str, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
        str[len] = '\0';

        while (true)
        {
            if (match(TOKEN_SLASH))
            {
                if (match(TOKEN_IDENTIFIER))
                {
                    totalLen += gbcpl->parser.previous.length + 1;
                    str = GROW_ARRAY(str, char, len + 1, totalLen + 1);
                    str[len] = '/';
                    strncpy(str + (len + 1), gbcpl->parser.previous.start, gbcpl->parser.previous.length);
                    len = totalLen;
                    str[len] = '\0';
                }
                else
                {
                    errorAtCurrent(errorIdentifier);
                }
            }
            else
            {
                break;
            }
        }

        totalLen = len + strlen(extension);
        str = GROW_ARRAY(str, char, len + 1, totalLen + 1);
        len = totalLen;
        strcat(str, extension);

        *p = str;
        return totalLen + 1;
    }
    else
    {
        consume(TOKEN_STRING, errorString);
        char *str = ALLOCATE(char, gbcpl->parser.previous.length - 1);
        memcpy(str, gbcpl->parser.previous.start + 1, gbcpl->parser.previous.length - 2);
        str[gbcpl->parser.previous.length - 2] = '\0';

        *p = str;
        return gbcpl->parser.previous.length - 1;
    }
    *p = NULL;
    return 0;
}

static void nativeDeclaration()
{
    pathOrString(vm.nativeExtension, "Expect an identifier after slash in native.", "Expect string after native.");

    if (match(TOKEN_AS))
    {
        if (match(TOKEN_IDENTIFIER) || match(TOKEN_DEFAULT))
        {
            emitPreviousAsString();
        }
        else
        {
            consume(TOKEN_IDENTIFIER, "Expect identifier after as.");
        }
    }
    else
    {
        emitByte(OP_NULL);
    }

    int count = 0;
    consume(TOKEN_LEFT_BRACE, "Expect '{' before native body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        nativeFuncOrStruct();
        count++;
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after native body.");

    emitConstant(NUMBER_VAL(count));
    emitByte(OP_NATIVE);
}

static void cubeDeclaration()
{
    char id[32];
    char value[256];

    consume(TOKEN_LEFT_BRACE, "Expect '{' before cube body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        consume(TOKEN_IDENTIFIER, "Every cube setting must be an identifier.");
        int len = gbcpl->parser.previous.length;
        if (len > 31)
        {
            errorAt(&gbcpl->parser.previous, "Cube setting identifiers must be max 31 characters long.");
            break;
        }
        else
        {
            strncpy(id, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
            id[len] = '\0';
        }

        consume(TOKEN_EQUAL, "Every cube setting must set a value.");

        if (match(TOKEN_IDENTIFIER))
        {
            len = gbcpl->parser.previous.length;
            if (len > 255)
            {
                errorAt(&gbcpl->parser.previous, "Cube setting value must be max 255 characters long.");
                break;
            }
            else
            {
                strncpy(value, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
                value[len] = '\0';
            }
        }
        else if (match(TOKEN_STRING))
        {
            if (gbcpl->parser.previous.length > 255)
            {
                errorAt(&gbcpl->parser.previous, "Cube setting value must be max 255 characters long.");
                break;
            }
            else
            {
                memcpy(value, gbcpl->parser.previous.start + 1, gbcpl->parser.previous.length - 2);
                value[gbcpl->parser.previous.length - 2] = '\0';
            }
        }
        else if (match(TOKEN_NUMBER))
        {
            strncpy(value, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
            value[gbcpl->parser.previous.length] = '\0';
        }
        else if (match(TOKEN_NULL))
        {
            strncpy(value, gbcpl->parser.previous.start, gbcpl->parser.previous.length);
            value[gbcpl->parser.previous.length] = '\0';
        }
        else
        {
            errorAtCurrent("Invalid cube setting value.");
            break;
        }

        if (strcmp(id, "title") == 0)
        {
            strcpy(cube_bin_options.title, value);
        }
        else if (strcmp(id, "bin") == 0)
        {
            strcpy(cube_bin_options.binary, value);
        }
        else if (strcmp(id, "description") == 0)
        {
            strcpy(cube_bin_options.description, value);
        }
        else
        {
            printf("Invalid cube setting: %s = %s\n", id, value);
            errorAt(&gbcpl->parser.previous, "");
            break;
        }

        if (!match(TOKEN_COMMA))
            break;
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after cube body.");
}

static void funDeclaration()
{
    uint16_t global;
    uint16_t prop;
    if (!match(TOKEN_IDENTIFIER))
    {
        if (!match(TOKEN_FUNC))
        {
            if (!match(TOKEN_CLASS))
            {
                if (!match(TOKEN_ENUM))
                {
                    consume(TOKEN_IDENTIFIER, "Expect function name or type.");
                }
            }
        }
    }
    if (gbcpl->parser.current.type == TOKEN_DOT)
    {
        global = identifierConstant(&gbcpl->parser.previous);
        advance();
        consume(TOKEN_IDENTIFIER, "Expect function name");
        prop = identifierConstant(&gbcpl->parser.previous);

        function(TYPE_EXTENSION);

        emitShort(OP_EXTENSION, global);
        emitShortAlone(prop);
    }
    else
    {
        declareVariable();
        if (gbcpl->current->scopeDepth > 0)
            global = 0;
        else
            global = identifierConstant(&gbcpl->parser.previous);
        markInitialized();
        function(TYPE_FUNCTION);
        defineVariable(global);
    }
}

static void globalDeclaration(bool checkEnd)
{
    consume(TOKEN_VAR, "Only global variable declaration is valid.");

    uint16_t global;

    while (true)
    {
        consume(TOKEN_IDENTIFIER, "Expect variable name.");
        global = identifierConstant(&gbcpl->parser.previous);

        if (match(TOKEN_COLON))
        {
            consume(TOKEN_IDENTIFIER, "Only variable types allowed");
            Token type = gbcpl->parser.previous;
        }

        if (match(TOKEN_EQUAL))
        {
            expression();
        }
        else
        {
            emitByte(OP_NULL);
        }

        emitShort(OP_DEFINE_GLOBAL_FORCED, global);
        if (!match(TOKEN_COMMA))
            break;
    }

    // if (checkEnd)
    //   consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    match(TOKEN_SEMICOLON);
}

static void docDeclaration()
{
    Token tdoc = gbcpl->parser.previous;
    Documentation *doc = (Documentation *)mp_malloc(sizeof(Documentation));
    doc->id = 0;
    doc->line = -1;
    doc->doc = (char *)mp_malloc(sizeof(char) * (tdoc.length + 1));
    memcpy(doc->doc, tdoc.start, tdoc.length);
    doc->doc[tdoc.length] = '\0';
    doc->next = gbcpl->currentDoc;
    gbcpl->currentDoc = doc;
    if (doc->next != NULL)
        doc->id = doc->next->id + 1;
}

static void assertDeclaration()
{
    variable(false);
    Token args = getArguments(&gbcpl->scanner, 1);
    if (args.start == NULL)
        errorAtCurrent("assert must receive an expression.");
    else
    {
        emitConstant(OBJ_VAL(copyString(args.start, args.length)));

        consume(TOKEN_LEFT_PAREN, "assert must be called as a function");
        uint8_t argCount = argumentList() + 1;
        emitBytes(OP_CALL, argCount);
    }
}

static void varDeclaration(bool checkEnd)
{
    uint16_t global;

    while (true)
    {
        global = parseVariable("Expect variable name.");
        if (match(TOKEN_EQUAL))
        {
            expression();
        }
        else
        {
            emitByte(OP_NULL);
        }

        defineVariable(global);
        if (!match(TOKEN_COMMA))
            break;
    }

    // if (checkEnd)
    //   consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    match(TOKEN_SEMICOLON);
}

static void expressionStatement()
{
    expression();
    // consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    match(TOKEN_SEMICOLON);
    emitByte(OP_REPL_POP);
}

static void forStatement()
{
    beginScope();
    gbcpl->current->loopDepth++;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    bool in = false;
    uint16_t name;
    bool hasIndex = false;
    uint16_t it, var, cond;
    Token nameVar, loopVar, valVar, condVar;
    int exitJump = -1;
    int surroundingLoopStart;
    int surroundingLoopScopeDepth;

    if (match(TOKEN_SEMICOLON))
    {
        // No initializer.
    }
    else if (match(TOKEN_VAR))
    {
        name = parseVariable("Expect variable name.");
        nameVar = gbcpl->parser.previous;
        if (match(TOKEN_COMMA))
        {
            consume(TOKEN_IDENTIFIER, "Expected a index name.");
            hasIndex = true;
            loopVar = gbcpl->parser.previous;
        }

        if (match(TOKEN_EQUAL))
        {
            expression();
        }
        else
        {
            if (match(TOKEN_IN))
            {
                char *str__value = (char *)mp_malloc(sizeof(char) * 32);
                char *str__index = (char *)mp_malloc(sizeof(char) * 32);
                char *str__cond = (char *)mp_malloc(sizeof(char) * 32);
                str__value[0] = str__index[0] = str__cond[0] = '\0';
                sprintf(str__value, "__value%d", gbcpl->loopInCount);
                sprintf(str__index, "__index%d", gbcpl->loopInCount);
                sprintf(str__cond, "__cond%d", gbcpl->loopInCount);
                gbcpl->loopInCount++;

                emitByte(OP_NULL);
                defineVariable(name);

                var = createSyntheticVariable(str__value, &valVar);
                emitByte(OP_NULL);
                defineVariable(var);

                expression();
                setVariablePop(valVar);

                if (!hasIndex)
                    it = createSyntheticVariable(str__index, &loopVar);
                else
                {
                    declareNamedVariable(&loopVar);
                    if (gbcpl->current->scopeDepth > 0)
                        it = 0;
                    else
                        it = identifierConstant(&loopVar);
                }
                emitByte(OP_NULL);
                defineVariable(it);

                emitConstant(NUMBER_VAL(0));
                setVariablePop(loopVar);

                cond = createSyntheticVariable(str__cond, &condVar);
                emitByte(OP_NULL);
                defineVariable(cond);

                emitConstant(NUMBER_VAL(0));
                setVariablePop(condVar);

                /*
                getVariable(valVar);
                getVariable(loopVar);
                emitByte(OP_SUBSCRIPT);
                setVariablePop(nameVar);
                */

                in = true;
            }
            else
                emitByte(OP_NULL);
        }

        if (!in)
        {
            consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
            defineVariable(name);
        }
    }
    else
    {
        expressionStatement();
    }

    surroundingLoopStart = gbcpl->innermostLoopStart;
    surroundingLoopScopeDepth = gbcpl->innermostLoopScopeDepth;
    gbcpl->innermostLoopStart = currentChunk()->count;
    gbcpl->innermostLoopScopeDepth = gbcpl->current->scopeDepth;

    if (!in && !match(TOKEN_SEMICOLON))
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }
    else if (in)
    {
        beginSyntheticCall("len");
        getVariable(valVar);
        endSyntheticCall(1);
        setVariablePop(condVar);
        emitByte(OP_POP);

        getVariable(loopVar);
        getVariable(condVar);
        // emitConstant(NUMBER_VAL(5));
        emitByte(OP_LESS);
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }

    if (!in && !match(TOKEN_RIGHT_PAREN))
    {
        int bodyJump = emitJump(OP_JUMP);

        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(gbcpl->innermostLoopStart);
        gbcpl->innermostLoopStart = incrementStart;
        patchJump(bodyJump);
    }
    else if (in)
    {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;

        // Increment counter
        getVariable(loopVar);
        emitConstant(NUMBER_VAL(1));
        emitBytes(OP_ADD, OP_FALSE);
        setVariablePop(loopVar);
        emitByte(OP_POP);

        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(gbcpl->innermostLoopStart);
        gbcpl->innermostLoopStart = incrementStart;
        patchJump(bodyJump);

        // Set variable
        getVariable(valVar);
        getVariable(loopVar);
        emitByte(OP_SUBSCRIPT);
        setVariablePop(nameVar);
        emitByte(OP_POP);
    }

    statement();

    // Jump back to the beginning (or the increment).
    emitLoop(gbcpl->innermostLoopStart);

    patchBreak();
    if (exitJump != -1)
    {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition.
    }

    if (in)
    {
        setVariable(loopVar, NUMBER_VAL(0));
        setVariable(condVar, NUMBER_VAL(0));
        setVariable(valVar, NUMBER_VAL(0));

        emitByte(OP_POP);
        emitByte(OP_POP);
        emitByte(OP_POP);
        emitByte(OP_POP);
        emitByte(OP_POP);
        emitByte(OP_POP);
    }

    gbcpl->innermostLoopStart = surroundingLoopStart;
    gbcpl->innermostLoopScopeDepth = surroundingLoopScopeDepth;

    endScope();
    gbcpl->current->loopDepth--;
}

static void ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); // [paren]

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if (match(TOKEN_ELSE))
        statement();
    patchJump(elseJump);
}

static void withStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'with'.");
    expression();
    if (match(TOKEN_COMMA))
    {
        expression();
    }
    else
    {
        emitConstant(STRING_VAL("r"));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'with'.");

    beginScope();

    Token file = syntheticToken("file");
    Local *local = &gbcpl->current->locals[gbcpl->current->localCount++];
    local->depth = gbcpl->current->scopeDepth;
    local->isCaptured = false;
    local->name = file;

    emitByte(OP_FILE);
    setVariablePop(file);

    statement();

    getVariable(file);

    Token fn = syntheticToken("close");
    uint16_t name = identifierConstant(&fn);

    emitBytes(OP_INVOKE, 0);
    emitShortAlone(name);

    endScope();
}

static void returnStatement()
{
    if (gbcpl->current->type == TYPE_SCRIPT)
    {
        error("Cannot return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON))
    {
        emitReturn();
    }
    else
    {
        if (gbcpl->current->type == TYPE_INITIALIZER)
        {
            error("Cannot return a value from an initializer.");
        }

        expression();
        // consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        match(TOKEN_SEMICOLON);
        emitByte(OP_RETURN);
    }
}

static void includeStatement()
{
    char *fileName;
    int len = pathOrStringNoEmit(&fileName, vm.extension, "Expect an identifier after slash in include.",
                                 "Expect string after include.");

    if (len == 0 || fileName == NULL)
        return;

    char *s = NULL;
    char *strPath;
    if (!findAndReadFile(fileName, &strPath, &s))
    {
        mp_free(strPath);
        FREE_ARRAY(char, fileName, len);
        errorAtCurrent("Could not load the file for include.");
        return;
    }
    FREE_ARRAY(char, fileName, len);

    ObjFunction *fn = compile(s, strPath);
    if (fn == NULL)
    {
        mp_free(strPath);
        FREE_ARRAY(char, fileName, len);
        mp_free(s);
        errorAtCurrent("Could not compile the included file.");
        return;
    }
    mp_free(s);

    emitConstant(STRING_VAL(strPath));
    // mp_free(strPath);

    if (match(TOKEN_AS))
    {
        if (match(TOKEN_IDENTIFIER) || match(TOKEN_DEFAULT))
        {
            emitPreviousAsString();
        }
        else
        {
            consume(TOKEN_IDENTIFIER, "Expect identifier after as.");
        }
    }
    else
    {
        emitByte(OP_NULL);
    }

    // consume(TOKEN_SEMICOLON, "Expect ';' after import.");
    match(TOKEN_SEMICOLON);
    emitShort(OP_INCLUDE, makeConstant(OBJ_VAL(fn)));
}

static void importStatement()
{
    if (vm.forceInclude)
    {
        includeStatement();
        return;
    }
    pathOrString(vm.extension, "Expect an identifier after slash in import.", "Expect string after import.");
    bool forClause = false;
    Token module;
    char *str = NULL;

    if (match(TOKEN_AS))
    {
        if (match(TOKEN_IDENTIFIER) || match(TOKEN_DEFAULT))
        {
            emitPreviousAsString();
        }
        else
        {
            consume(TOKEN_IDENTIFIER, "Expect identifier after as.");
        }
    }
    else if (match(TOKEN_FOR))
    {
        forClause = true;
        str = mp_malloc(sizeof(char) * 64);
        sprintf(str, "__temp_package_%d", gbcpl->tempId++);
        emitConstant(OBJ_VAL(copyString(str, strlen(str))));
        emitByte(OP_IMPORT);

        module = syntheticToken(str);

        do
        {
            if (match(TOKEN_IDENTIFIER) || match(TOKEN_STAR))
            {
                getVariable(module);
                emitPreviousAsString();
                emitByte(OP_FROM_PACKAGE);
            }
            else
                errorAtCurrent("Variable name expected.");
        } while (match(TOKEN_COMMA));
    }
    else
    {
        emitByte(OP_NULL);
    }

    // consume(TOKEN_SEMICOLON, "Expect ';' after import.");
    match(TOKEN_SEMICOLON);
    if (!forClause)
        emitByte(OP_IMPORT);
    else
    {
        emitConstant(OBJ_VAL(copyString(str, strlen(str))));
        emitByte(OP_REMOVE_VAR);
        mp_free(str);
    }
}

static void abortStatement()
{
    consume(TOKEN_IDENTIFIER, "Expect a function call in abort.");
    getVariable(gbcpl->parser.previous);
    // consume(TOKEN_SEMICOLON, "Expect ';' after abort.");
    match(TOKEN_SEMICOLON);
    emitByte(OP_ABORT);
}

static void whileStatement()
{
    gbcpl->current->loopDepth++;

    int surroundingLoopStart = gbcpl->innermostLoopStart;
    int surroundingLoopScopeDepth = gbcpl->innermostLoopScopeDepth;
    gbcpl->innermostLoopStart = currentChunk()->count;
    gbcpl->innermostLoopScopeDepth = gbcpl->current->scopeDepth;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    statement();

    // Loop back to the start.
    emitLoop(gbcpl->innermostLoopStart);

    patchBreak();
    patchJump(exitJump);
    emitByte(OP_POP);

    gbcpl->innermostLoopStart = surroundingLoopStart;
    gbcpl->innermostLoopScopeDepth = surroundingLoopScopeDepth;

    gbcpl->current->loopDepth--;
}

static void doWhileStatement()
{
    gbcpl->current->loopDepth++;

    int surroundingLoopStart = gbcpl->innermostLoopStart;
    int surroundingLoopScopeDepth = gbcpl->innermostLoopScopeDepth;
    gbcpl->innermostLoopStart = currentChunk()->count;
    gbcpl->innermostLoopScopeDepth = gbcpl->current->scopeDepth;

    statement();

    consume(TOKEN_WHILE, "Expected 'while' after 'do' body.");
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    // consume(TOKEN_SEMICOLON, "Expected semicolon after 'do while'");
    match(TOKEN_SEMICOLON);

    int exitJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);

    // Loop back to the start.
    emitLoop(gbcpl->innermostLoopStart);

    patchBreak();
    patchJump(exitJump);
    emitByte(OP_POP);

    gbcpl->innermostLoopStart = surroundingLoopStart;
    gbcpl->innermostLoopScopeDepth = surroundingLoopScopeDepth;

    gbcpl->current->loopDepth--;
}

static void continueStatement()
{
    if (gbcpl->innermostLoopStart == -1)
        error("Cannot use 'continue' outside of a loop.");

    // consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'.");
    match(TOKEN_SEMICOLON);

    // Discard any locals created inside the loop.
    for (int i = gbcpl->current->localCount - 1;
         i >= 0 && gbcpl->current->locals[i].depth > gbcpl->innermostLoopScopeDepth; i--)
        emitByte(OP_POP);

    // Jump to top of current innermost loop.
    emitLoop(gbcpl->innermostLoopStart);
}

static void breakStatement()
{
    if (gbcpl->current->loopDepth == 0)
    {
        error("Cannot use 'break' outside of a loop.");
        return;
    }

    // consume(TOKEN_SEMICOLON, "Expected semicolon after break");
    match(TOKEN_SEMICOLON);
    emitBreak();
}

static void switchStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after value.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before switch cases.");

    int state = 0; // 0: before all cases, 1: before default, 2: after default.
    int caseEnds[MAX_CASES];
    int caseCount = 0;
    int previousCaseSkip = -1;

    while (!match(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        if (match(TOKEN_CASE) || match(TOKEN_DEFAULT))
        {
            TokenType caseType = gbcpl->parser.previous.type;

            if (state == 2)
            {
                error("Cannot have another case or default after the default case.");
            }

            if (state == 1)
            {
                // At the end of the previous case, jump over the others.
                caseEnds[caseCount++] = emitJump(OP_JUMP);

                // Patch its condition to jump to the next case (this one).
                patchJump(previousCaseSkip);
                emitByte(OP_POP);
            }

            if (caseType == TOKEN_CASE)
            {
                state = 1;

                // See if the case is equal to the value.
                emitByte(OP_DUP);
                // expression();
                parsePrecedence(PREC_UNARY);

                consume(TOKEN_COLON, "Expect ':' after case value.");

                emitByte(OP_EQUAL);
                previousCaseSkip = emitJump(OP_JUMP_IF_FALSE);

                // Pop the comparison result.
                emitByte(OP_POP);
            }
            else
            {
                state = 2;
                consume(TOKEN_COLON, "Expect ':' after default.");
                previousCaseSkip = -1;
            }
        }
        else
        {
            // Otherwise, it's a statement inside the current case.
            if (state == 0)
            {
                error("Cannot have statements before any case.");
            }
            statement();
        }
    }

    // If we ended without a default case, patch its condition jump.
    if (state == 1)
    {
        patchJump(previousCaseSkip);
        emitByte(OP_POP);
    }

    // Patch all the case jumps to the end.
    for (int i = 0; i < caseCount; i++)
    {
        patchJump(caseEnds[i]);
    }

    emitByte(OP_POP); // The switch value.
}

static void tryStatement()
{
    int catch = emitJump(OP_TRY);

    declaration(true);

    int end = emitJump(OP_CLOSE_TRY);
    patchJump(catch);

    if (match(TOKEN_CATCH))
    {
        emitByte(OP_TRUE);
        function(TYPE_FUNCTION);
        emitBytes(OP_CALL, 1);
    }
    else
    {
        emitByte(OP_FALSE);
    }

    patchJump(end);
}

static void synchronize()
{
    gbcpl->parser.panicMode = false;

    while (gbcpl->parser.current.type != TOKEN_EOF)
    {
        if (gbcpl->parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (gbcpl->parser.current.type)
        {
            case TOKEN_CLASS:
            case TOKEN_ENUM:
            case TOKEN_NATIVE:
            case TOKEN_STRUCT:
            case TOKEN_FUNC:
            case TOKEN_STATIC:
            case TOKEN_VAR:
            case TOKEN_GLOBAL:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
            case TOKEN_IMPORT:
            case TOKEN_INCLUDE:
            case TOKEN_BREAK:
            case TOKEN_WITH:
            case TOKEN_ABORT:
            case TOKEN_TRY:
            case TOKEN_CATCH:
            case TOKEN_DOC:
            case TOKEN_PASS:
            case TOKEN_CUBE:
                return;

            default:
                // Do nothing.
                ;
        }

        advance();
    }
}

static void declaration(bool checkEnd)
{
    if (gbcpl->currentDoc != NULL && gbcpl->currentDoc->line < 0)
        gbcpl->currentDoc->line = gbcpl->parser.current.line;

    if (match(TOKEN_CLASS))
    {
        classDeclaration();
    }
    else if (match(TOKEN_ENUM))
    {
        enumDeclaration();
    }
    else if (match(TOKEN_NATIVE))
    {
        nativeDeclaration();
    }
    else if (match(TOKEN_CUBE))
    {
        cubeDeclaration();
    }
    else if (match(TOKEN_FUNC))
    {
        funDeclaration();
    }
    else if (match(TOKEN_GLOBAL))
    {
        globalDeclaration(checkEnd);
    }
    else if (match(TOKEN_VAR))
    {
        varDeclaration(checkEnd);
    }
    else if (match(TOKEN_DOC))
    {
        docDeclaration();
    }
    else if (match(TOKEN_PASS))
    {
        emitByte(OP_TEST);
        match(TOKEN_SEMICOLON);
    }
    else if (match(TOKEN_ASSERT))
    {
        assertDeclaration();
    }
    else
    {
        statement();
    }

    if (gbcpl->parser.panicMode)
        synchronize();
}

static void statement()
{
    if (match(TOKEN_FOR))
    {
        forStatement();
    }
    else if (match(TOKEN_IF))
    {
        ifStatement();
    }
    else if (match(TOKEN_RETURN))
    {
        returnStatement();
    }
    else if (match(TOKEN_WITH))
    {
        withStatement();
    }
    else if (match(TOKEN_IMPORT))
    {
        importStatement();
    }
    else if (match(TOKEN_INCLUDE))
    {
        includeStatement();
    }
    else if (match(TOKEN_ABORT))
    {
        abortStatement();
    }
    else if (match(TOKEN_BREAK))
    {
        breakStatement();
    }
    else if (match(TOKEN_CONTINUE))
    {
        continueStatement();
    }
    else if (match(TOKEN_WHILE))
    {
        whileStatement();
    }
    else if (match(TOKEN_DO))
    {
        doWhileStatement();
    }
    else if (match(TOKEN_SWITCH))
    {
        switchStatement();
    }
    else if (match(TOKEN_TRY))
    {
        tryStatement();
    }
    else if (match(TOKEN_LEFT_BRACE))
    {
        Token previous = gbcpl->parser.previous;
        Token current = gbcpl->parser.current;
        if (check(TOKEN_STRING))
        {
            for (int i = 0; i < gbcpl->parser.current.length - gbcpl->parser.previous.length + 1; ++i)
                backTrack(&gbcpl->scanner);

            gbcpl->parser.current = previous;
            expressionStatement();
            return;
        }
        else if (check(TOKEN_RIGHT_BRACE))
        {
            advance();
            if (check(TOKEN_SEMICOLON))
            {
                backTrack(&gbcpl->scanner);
                backTrack(&gbcpl->scanner);
                gbcpl->parser.current = previous;
                expressionStatement();
                return;
            }
        }
        gbcpl->parser.current = current;

        beginScope();
        block();
        endScope();
    }
    else
    {
        expressionStatement();
    }
}

ObjFunction *compile(const char *source, const char *path)
{
    initGlobalCompiler();
    DISABLE_GC;
    ObjFunction *fn = NULL;
    if (memcmp(initString, source, strlen(initString)) == 0)
    {
        char *src = (char *)source + strlen(initString);

        uint32_t len = ((uint32_t *)src)[0];
        src += sizeof(uint32_t);

        uint32_t pos = 0;
        Value value = loadByteCode(src, &pos, len);
        if (IS_FUNCTION(value))
            fn = AS_FUNCTION(value);
    }
    else
    {
        initScanner(&gbcpl->scanner, source);
        Compiler compiler;

        initDoc();
        initCompiler(&compiler, TYPE_SCRIPT);
        compiler.path = path;

        gbcpl->parser.hadError = false;
        gbcpl->parser.panicMode = false;

        // Create args
        Token argsToken = syntheticToken("args");
        uint16_t args = identifierConstant(&argsToken);
        getVariable(syntheticToken(vm.argsString));
        defineVariable(args);

        // Parse the code
        advance();

        while (!match(TOKEN_EOF))
        {
            declaration(true);
        }

        fn = endCompiler();
        fn->doc = getDoc();
        if (gbcpl->parser.hadError)
            fn = NULL;
    }
    RESTORE_GC;
    freeGlobalCompiler();
    return fn;
}

ObjFunction *eval(const char *source)
{
    initGlobalCompiler();
    int len = strlen(source) + 24;
    char *code = ALLOCATE(char, len);
    strcpy(code, "return (");
    strcat(code, source);
    strcat(code, ");");

    ObjFunction *fn = NULL;

    initScanner(&gbcpl->scanner, code);
    Compiler compiler;

    initDoc();
    initCompiler(&compiler, TYPE_EVAL);

    gbcpl->parser.hadError = false;
    gbcpl->parser.panicMode = false;

    // Parse the code
    advance();

    while (!match(TOKEN_EOF))
    {
        declaration(true);
    }

    fn = endCompiler();
    fn->doc = getDoc();
    if (gbcpl->parser.hadError)
        fn = NULL;

    FREE_ARRAY(char, code, len);
    freeGlobalCompiler();
    return fn;
}

void markCompilerRoots()
{
    if (gbcpl == NULL)
        return;
    Compiler *compiler = gbcpl->current;
    while (compiler != NULL)
    {
        mark_object((Obj *)compiler->function);
        compiler = compiler->enclosing;
    }
}

bool writeByteCodeChunk(FILE *file, Chunk *chunk);

bool initByteCode(FILE *file)
{
    uint32_t sz = 0;
    if (fwrite(initString, sizeof(char), strlen(initString), file) != strlen(initString))
        return false;
    if (fwrite(&sz, sizeof(sz), 1, file) != 1)
        return false;
    return true;
}

bool writeByteCode(FILE *file, Value value)
{
#ifndef NAN_TAGGING
    uint32_t type = value.type;
    uint32_t objType = 0;

    if (IS_OBJ(value))
        objType = OBJ_TYPE(value);

    // char *typeName = valueType(value);
    // printf("Write: <%s> ", typeName);
    // printValue(value);
    // printf("\n");
    // mp_free(typeName);

    int i = 0;
    if (fwrite(&type, sizeof(type), 1, file) != 1)
        return false;
    if (fwrite(&objType, sizeof(objType), 1, file) != 1)
        return false;

    if (IS_BOOL(value))
    {
        if (fwrite(&value.as.boolean, sizeof(value.as.boolean), 1, file) != 1)
            return false;
    }
    else if (IS_NUMBER(value))
    {
        if (fwrite(&value.as.number, sizeof(value.as.number), 1, file) != 1)
            return false;
    }
    else if (IS_STRING(value))
    {
        ObjString *str = AS_STRING(value);
        if (fwrite(&str->length, sizeof(str->length), 1, file) != 1)
            return false;
        if (fwrite(str->chars, sizeof(char), str->length, file) != str->length)
            return false;
    }
    else if (IS_BYTES(value))
    {
        ObjBytes *bytes = AS_BYTES(value);
        if (fwrite(&bytes->length, sizeof(bytes->length), 1, file) != 1)
            return false;
        if (fwrite(bytes->bytes, sizeof(unsigned char), bytes->length, file) != bytes->length)
            return false;
    }
    else if (IS_LIST(value))
    {
        ObjList *list = AS_LIST(value);
        if (fwrite(&list->values.count, sizeof(list->values.count), 1, file) != 1)
            return false;
        for (i = 0; i < list->values.count; i++)
        {
            if (!writeByteCode(file, list->values.values[i]))
                return false;
        }
    }
    else if (IS_DICT(value))
    {
        ObjDict *dict = AS_DICT(value);
        if (fwrite(&dict->count, sizeof(dict->count), 1, file) != 1)
            return false;
        for (i = 0; i < dict->capacity; i++)
        {
            if (!dict->items[i])
                continue;

            Value key = STRING_VAL(dict->items[i]->key);

            if (!writeByteCode(file, key))
                return false;
            if (!writeByteCode(file, dict->items[i]->item))
                return false;
        }
    }
    else if (IS_NATIVE_FUNC(value))
    {
        printf("Invalid bytecode: NativeFunc (Valid only on runtime)\n");
        return false;
    }
    else if (IS_NATIVE_LIB(value))
    {
        printf("Invalid bytecode: NativeLib (Valid only on runtime)\n");
        return false;
    }
    else if (IS_NATIVE(value))
    {
        printf("Invalid bytecode: Native (Valid only on runtime)\n");
        return false;
    }
    else if (IS_INSTANCE(value))
    {
        printf("Invalid bytecode: Instance (Valid only on runtime)\n");
        return false;
    }
    else if (IS_FUNCTION(value))
    {
        ObjFunction *func = AS_FUNCTION(value);
        if (func->name == NULL)
        {
            Value name = STRING_VAL(vm.scriptName);
            if (!writeByteCode(file, name))
                return false;
        }
        else
        {
            if (!writeByteCode(file, OBJ_VAL(func->name)))
                return false;
        }
        if (fwrite(&func->arity, sizeof(func->arity), 1, file) != 1)
            return false;
        if (fwrite(&func->staticMethod, sizeof(func->staticMethod), 1, file) != 1)
            return false;
        if (fwrite(&func->upvalueCount, sizeof(func->upvalueCount), 1, file) != 1)
            return false;
        if (!writeByteCodeChunk(file, &func->chunk))
            return false;

        Documentation *doc = func->doc;
        uint32_t sz = 0;
        if (doc != NULL)
            sz = doc->id + 1;

        if (fwrite(&sz, sizeof(sz), 1, file) != 1)
            return false;

        while (doc != NULL)
        {
            if (fwrite(&doc->id, sizeof(doc->id), 1, file) != 1)
                return false;
            if (fwrite(&doc->line, sizeof(doc->line), 1, file) != 1)
                return false;

            sz = strlen(doc->doc);
            if (fwrite(&sz, sizeof(sz), 1, file) != 1)
                return false;

            if (fwrite(doc->doc, sizeof(char), sz, file) != sz)
                return false;

            doc = doc->next;
        }
    }
    else if (IS_CLOSURE(value))
    {
        ObjClosure *closure = AS_CLOSURE(value);
        if (!writeByteCode(file, OBJ_VAL(closure->function)))
            return false;
        if (fwrite(&closure->upvalueCount, sizeof(closure->upvalueCount), 1, file) != 1)
            return false;
        for (i = 0; i < closure->upvalueCount; i++)
        {
            if (!writeByteCode(file, OBJ_VAL(closure->upvalues[i])))
                return false;
        }
    }
    else if (IS_BOUND_METHOD(value))
    {
        printf("Invalid bytecode: Bound Method (Valid only on runtime)\n");
        return false;
    }
    else if (IS_CLASS(value))
    {
        ObjClass *klass = AS_CLASS(value);
        if (!writeByteCode(file, OBJ_VAL(klass->name)))
            return false;

        if (fwrite(&klass->fields.count, sizeof(klass->fields.count), 1, file) != 1)
            return false;
        i = 0;
        Entry entry;
        while (iterateTable(&klass->fields, &entry, &i))
        {
            if (entry.key == NULL)
                continue;

            if (!writeByteCode(file, OBJ_VAL(entry.key)))
                return false;

            if (!writeByteCode(file, entry.value))
                return false;
        }

        if (fwrite(&klass->methods.count, sizeof(klass->methods.count), 1, file) != 1)
            return false;
        i = 0;
        while (iterateTable(&klass->methods, &entry, &i))
        {
            if (entry.key == NULL)
                continue;

            if (!writeByteCode(file, OBJ_VAL(entry.key)))
                return false;

            if (!writeByteCode(file, entry.value))
                return false;
        }

        if (fwrite(&klass->staticFields.count, sizeof(klass->staticFields.count), 1, file) != 1)
            return false;
        i = 0;
        while (iterateTable(&klass->staticFields, &entry, &i))
        {
            if (entry.key == NULL)
                continue;

            if (!writeByteCode(file, OBJ_VAL(entry.key)))
                return false;

            if (!writeByteCode(file, entry.value))
                return false;
        }
    }
    else if (value.type == OBJ_UPVALUE)
    {
        /*
        ObjUpvalue *upvalue = (ObjUpvalue *)AS_OBJ(value);
        char valid;
        if (upvalue->location)
        {
          valid = 255;
          if (fwrite(&valid, sizeof(valid), 1, file) != 1)
            return false;
          if (!writeByteCode(file, *upvalue->location))
            return false;
        }
        else
        {
          valid = 250;
          if (fwrite(&valid, sizeof(valid), 1, file) != 1)
            return false;
        }

        if (!writeByteCode(file, upvalue->closed))
          return false;

        if (upvalue->next)
        {
          valid = 255;
          if (fwrite(&valid, sizeof(valid), 1, file) != 1)
            return false;
          if (!writeByteCode(file, OBJ_VAL(upvalue->next)))
            return false;
        }
        else
        {
          valid = 250;
          if (fwrite(&valid, sizeof(valid), 1, file) != 1)
            return false;
        }
        */
        printf("Invalid bytecode: UpValue (Valid only on runtime)\n");
        return false;
    }
#endif
    return true;
}

bool writeByteCodeChunk(FILE *file, Chunk *chunk)
{
    int i = 0;
    if (fwrite(&chunk->count, sizeof(chunk->count), 1, file) != 1)
        return false;
    if (fwrite(chunk->code, sizeof(uint8_t), chunk->count, file) != chunk->count)
        return false;
    if (fwrite(&chunk->lineCount, sizeof(chunk->lineCount), 1, file) != 1)
        return false;
    if (fwrite(chunk->lines, sizeof(LineStart), chunk->lineCount, file) != chunk->lineCount)
        return false;
    if (fwrite(&chunk->constants.count, sizeof(chunk->constants.count), 1, file) != 1)
        return false;
    for (i = 0; i < chunk->constants.count; i++)
    {
        if (!writeByteCode(file, chunk->constants.values[i]))
            return false;
    }
    return true;
}

bool finishByteCode(FILE *file)
{
    fseek(file, 0L, SEEK_END);
    uint32_t fileSize = ftell(file);
    rewind(file);

    uint32_t pos = strlen(initString);

    fseek(file, pos, SEEK_SET);
    if (fwrite(&fileSize, sizeof(fileSize), 1, file) != 1)
        return false;
    return true;
}

#define READ(type)                                                                                                     \
    ((type *)(source + *pos))[0];                                                                                      \
    *pos += sizeof(type)
#define READ_ARRAY(type, dest, len)                                                                                    \
    memcpy(dest, source + *pos, len * sizeof(type));                                                                   \
    *pos += sizeof(type) * len

void loadChunk(Chunk *chunk, const char *source, uint32_t *pos, uint32_t total);

Value loadByteCode(const char *source, uint32_t *pos, uint32_t total)
{
    Value value = NULL_VAL;

#ifndef NAN_TAGGING
    uint32_t type = READ(uint32_t);
    uint32_t objType = READ(uint32_t);
    // printf("Load: %d, %d\n", type, objType);

    int i = 0;

    if (type == VAL_NULL)
    {
        // null
    }
    else if (type == VAL_BOOL)
    {
        BOOL_TYPE v = READ(BOOL_TYPE);
        value = BOOL_VAL(v);
    }
    else if (type == VAL_NUMBER)
    {
        NUMBER_TYPE v = READ(NUMBER_TYPE);
        value = NUMBER_VAL(v);
    }
    else if (type == VAL_OBJ)
    {
        if (objType == OBJ_STRING)
        {
            int len = READ(int);
            char *str = ALLOCATE(char, len + 1);
            str[len] = '\0';
            READ_ARRAY(char, str, len);
            value = STRING_VAL(str);
            FREE_ARRAY(char, str, len + 1);
        }
        else if (objType == OBJ_BYTES)
        {
            int len = READ(int);
            unsigned char *bytes = ALLOCATE(unsigned char, len);
            READ_ARRAY(unsigned char, bytes, len);
            value = BYTES_VAL(bytes, len);
            FREE_ARRAY(unsigned char, bytes, len);
        }
        else if (objType == OBJ_LIST)
        {
            ObjList *list = initList();
            int len = READ(int);
            for (i = 0; i < len; i++)
            {
                Value obj = loadByteCode(source, pos, total);
                writeValueArray(&list->values, obj);
            }
            value = OBJ_VAL(list);
        }
        else if (objType == OBJ_DICT)
        {
            ObjDict *dict = initDict();
            int len = READ(int);
            for (i = 0; i < len; i++)
            {
                if (!dict->items[i])
                    continue;

                Value key = loadByteCode(source, pos, total);
                Value val = loadByteCode(source, pos, total);
                insertDict(dict, AS_CSTRING(key), val);
            }
            value = OBJ_VAL(list);
        }
        else if (objType == OBJ_NATIVE_FUNC)
        {
            printf("Invalid bytecode: Native (Valid only on runtime)\n");
        }
        else if (objType == OBJ_NATIVE_LIB)
        {
            printf("Invalid bytecode: Native (Valid only on runtime)\n");
        }
        else if (objType == OBJ_NATIVE)
        {
            printf("Invalid bytecode: Native (Valid only on runtime)\n");
        }
        else if (objType == OBJ_INSTANCE)
        {
            printf("Invalid bytecode: Instance (Valid only on runtime)\n");
        }
        else if (objType == OBJ_FUNCTION)
        {
            ObjFunction *func = newFunction(false);
            Value name = loadByteCode(source, pos, total);
            func->name = AS_STRING(name);
            func->arity = READ(int);
            func->staticMethod = READ(bool);
            func->upvalueCount = READ(int);
            loadChunk(&func->chunk, source, pos, total);

            uint32_t sz = READ(uint32_t);
            uint32_t len;
            while (sz > 0)
            {
                Documentation *doc = (Documentation *)mp_malloc(sizeof(Documentation));
                doc->id = READ(int);
                doc->line = READ(int);
                len = READ(uint32_t);
                doc->doc = (char *)mp_malloc(sizeof(char) * (len + 1));
                READ_ARRAY(char, doc->doc, len);
                doc->doc[len] = '\0';
                doc->next = func->doc;
                func->doc = doc;
                sz--;
            }

            value = OBJ_VAL(func);
        }
        else if (objType == OBJ_CLOSURE)
        {
            Value func = loadByteCode(source, pos, total);
            ObjClosure *closure = newClosure(AS_FUNCTION(func));
            closure->upvalueCount = READ(int);
            for (i = 0; i < closure->upvalueCount; i++)
            {
                Value val = loadByteCode(source, pos, total);
                closure->upvalues[i] = (ObjUpvalue *)AS_OBJ(val);
            }
        }
        else if (objType == OBJ_BOUND_METHOD)
        {
            printf("Invalid bytecode: Bound Method (Valid only on runtime)\n");
        }
        else if (objType == OBJ_CLASS)
        {
            Value name = loadByteCode(source, pos, total);
            ObjClass *klass = newClass(AS_STRING(name));

            int len = READ(int);
            for (i = 0; i < len; i++)
            {
                Value key = loadByteCode(source, pos, total);
                Value val = loadByteCode(source, pos, total);
                tableSet(&klass->fields, AS_STRING(key), val);
            }

            len = READ(int);
            for (i = 0; i < len; i++)
            {
                Value key = loadByteCode(source, pos, total);
                Value val = loadByteCode(source, pos, total);
                tableSet(&klass->methods, AS_STRING(key), val);
            }

            len = READ(int);
            for (i = 0; i < len; i++)
            {
                Value key = loadByteCode(source, pos, total);
                Value val = loadByteCode(source, pos, total);
                tableSet(&klass->staticFields, AS_STRING(key), val);
            }
        }
        else if (objType == OBJ_UPVALUE)
        {
            /*
            ObjUpvalue *upvalue = newUpvalue();
            char valid;
            if (upvalue->location)
            {
              valid = 255;
              if (fwrite(&valid, sizeof(valid), 1, file) != 1)
                return false;
              if (!writeByteCode(file, *upvalue->location))
                return false;
            }
            else
            {
              valid = 250;
              if (fwrite(&valid, sizeof(valid), 1, file) != 1)
                return false;
            }

            if (!writeByteCode(file, upvalue->closed))
              return false;

            if (upvalue->next)
            {
              valid = 255;
              if (fwrite(&valid, sizeof(valid), 1, file) != 1)
                return false;
              if (!writeByteCode(file, OBJ_VAL(upvalue->next)))
                return false;
            }
            else
            {
              valid = 250;
              if (fwrite(&valid, sizeof(valid), 1, file) != 1)
                return false;
            }
            */
            printf("Invalid bytecode: UpValue (Valid only on runtime)\n");
        }
    }
// printf("Value: ");
// printValue(value);
// printf("\n");
#endif
    return value;
}

void loadChunk(Chunk *chunk, const char *source, uint32_t *pos, uint32_t total)
{
    chunk->count = READ(int);
    chunk->capacity = chunk->count;
    chunk->code = GROW_ARRAY(chunk->code, uint8_t, 0, chunk->capacity);
    READ_ARRAY(uint8_t, chunk->code, chunk->count);

    chunk->lineCount = READ(int);
    chunk->lineCapacity = chunk->lineCount;
    chunk->lines = GROW_ARRAY(chunk->lines, LineStart, 0, chunk->lineCapacity);

    READ_ARRAY(LineStart, chunk->lines, chunk->lineCount);

    int i = 0;
    int len = READ(int);
    for (i = 0; i < len; i++)
    {
        Value value = loadByteCode(source, pos, total);
        writeValueArray(&chunk->constants, value);
    }
}