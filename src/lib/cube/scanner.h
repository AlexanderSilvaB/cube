#ifndef CLOX_scanner_h
#define CLOX_scanner_h

#include "linkedList.h"

typedef enum
{
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_EXPAND_IN,
    TOKEN_EXPAND_EX,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_TIL,

    TOKEN_DOT_PLUS,
    TOKEN_DOT_MINUS,
    TOKEN_DOT_STAR,
    TOKEN_DOT_SLASH,
    TOKEN_DOT_POW,
    TOKEN_DOT_PERCENT,

    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_PLUS_EQUALS,
    TOKEN_MINUS_EQUALS,
    TOKEN_MULTIPLY_EQUALS,
    TOKEN_DIVIDE_EQUALS,

    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_PERCENT,
    TOKEN_POW,

    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT,
    TOKEN_BINARY_AND,
    TOKEN_BINARY_OR,
    TOKEN_NULLABLE,
    TOKEN_QUESTION,
    TOKEN_SWAP,

    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_BYTE,

    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_STATIC,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUNC,
    TOKEN_LAMBDA,
    TOKEN_IF,
    TOKEN_NULL,
    TOKEN_OR,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_GLOBAL,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_IN,
    TOKEN_NOT_IN,
    TOKEN_IS,
    TOKEN_CONTINUE,
    TOKEN_BREAK,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_NAN,
    TOKEN_INF,
    TOKEN_ENUM,

    TOKEN_IMPORT,
    TOKEN_REQUIRE,
    TOKEN_INCLUDE,
    TOKEN_AS,
    TOKEN_NATIVE,
    TOKEN_STRUCT,
    TOKEN_WITH,
    TOKEN_ASYNC,
    TOKEN_AWAIT,
    TOKEN_ABORT,
    TOKEN_SUSPEND,
    TOKEN_RESUME,
    TOKEN_TRY,
    TOKEN_CATCH,
    TOKEN_ASSERT,
    TOKEN_SECURE,

    TOKEN_DOC,
    TOKEN_PASS,
    TOKEN_CUBE,
    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct
{
    TokenType type;
    const char *start;
    int length;
    int line;
} Token;

typedef struct
{
    char word[16];
    TokenType key;
} Keyword;

typedef struct
{
    const char *start;
    const char *current;
    int line;
    linked_list *keywords;
} Scanner;

void initScanner(Scanner *scanner, const char *source);
void backTrack(Scanner *scanner);
Token scanToken(Scanner *scanner);
Token getArguments(Scanner *scanner, int n);
bool isOperator(TokenType type);
bool isAlpha(char c);
bool isDigit(char c);
bool setLanguage(Scanner *scanner, const char *languageSource);

#endif
