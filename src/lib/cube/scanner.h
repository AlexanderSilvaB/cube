#ifndef CLOX_scanner_h
#define CLOX_scanner_h

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
    TOKEN_NONE,
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
    TOKEN_AS,
    TOKEN_NATIVE,
    TOKEN_WITH,
    TOKEN_ASYNC,
    TOKEN_AWAIT,
    TOKEN_ABORT,
    TOKEN_TRY,
    TOKEN_CATCH,

    TOKEN_DOC,
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

void initScanner(const char *source);
void backTrack();
Token scanToken();
bool isOperator(TokenType type);

#endif
