#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct
{
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;
void initScanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool isHex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}

static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static char peek()
{
    return *scanner.current;
}

static char peekNext()
{
    if (isAtEnd())
        return '\0';
    return scanner.current[1];
}

static char peekNextNext()
{
    if (isAtEnd())
        return '\0';
    return scanner.current[2];
}

static bool match(char expected)
{
    if (isAtEnd())
        return false;
    if (*scanner.current != expected)
        return false;

    scanner.current++;
    return true;
}

static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

static Token errorToken(const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;

    return token;
}

static void skipWhitespace(Token *doc)
{
    for (;;)
    {
        char c = peek();
        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;

            case '\n':
                scanner.line++;
                advance();
                break;

            case '#': {
                if (peekNext() == '!' && scanner.start == scanner.current)
                {
                    while (peek() != '\n' && !isAtEnd())
                        advance();
                }
                break;
            }

            case '/':
                if (peekNext() == '/')
                {
                    advance();
                    advance();
                    if (peek() == '?')
                    {
                        advance();
                        doc->type = TOKEN_DOC;
                        doc->start = scanner.current;
                        doc->line = scanner.line;
                    }

                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd())
                        advance();

                    if (doc->type == TOKEN_DOC)
                        doc->length = scanner.current - doc->start;
                }
                else if (peekNext() == '*')
                {
                    // A block comment goes until */
                    advance();
                    advance();

                    if (peek() == '?')
                    {
                        advance();
                        doc->type = TOKEN_DOC;
                        doc->start = scanner.current;
                        doc->line = scanner.line;
                    }

                    bool done = peek() == '*' && peekNext() == '/';
                    while (!done && !isAtEnd())
                    {
                        advance();
                        done = peek() == '*' && peekNext() == '/';
                    }

                    if (doc->type == TOKEN_DOC)
                        doc->length = scanner.current - doc->start;

                    if (!isAtEnd())
                    {
                        advance();
                        advance();
                    }
                }
                else
                {
                    return;
                }
                break;

            default:
                return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const char *rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0)
    {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType()
{
    switch (scanner.start[0])
    {
        case 'a':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'b':
                        return checkKeyword(2, 3, "ort", TOKEN_ABORT);
                    case 's': {
                        if (scanner.current - scanner.start > 2)
                        {
                            switch (scanner.start[2])
                            {
                                case 'y':
                                    return checkKeyword(3, 2, "nc", TOKEN_ASYNC);
                            }
                        }
                        else
                        {
                            return checkKeyword(2, 0, "", TOKEN_AS);
                        }
                    }
                    case 'n':
                        return checkKeyword(2, 1, "d", TOKEN_AND);
                    case 'w':
                        return checkKeyword(2, 3, "ait", TOKEN_AWAIT);
                }
            }
            break;
        case 'b':
            return checkKeyword(1, 4, "reak", TOKEN_BREAK);
        case 'c':
            if (scanner.current - scanner.start > 1)
                switch (scanner.start[1])
                {
                    case 'a': {
                        if (scanner.current - scanner.start > 2)
                        {
                            switch (scanner.start[2])
                            {
                                case 's':
                                    return checkKeyword(3, 1, "e", TOKEN_CASE);
                                case 't':
                                    return checkKeyword(3, 2, "ch", TOKEN_CATCH);
                            }
                        }
                        break;
                    }
                    case 'l':
                        return checkKeyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o':
                        return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
                }
            break;
        case 'd':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'e':
                        return checkKeyword(2, 5, "fault", TOKEN_DEFAULT);
                    case 'o':
                        return checkKeyword(2, 0, "", TOKEN_DO);
                }
            }
            break;
        case 'e':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'l':
                        return checkKeyword(2, 2, "se", TOKEN_ELSE);
                    case 'n':
                        return checkKeyword(2, 2, "um", TOKEN_ENUM);
                }
            }
            break;
        case 'f':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'a':
                        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return checkKeyword(2, 2, "nc", TOKEN_FUNC);
                }
            }
            break;
        case 'g':
            return checkKeyword(1, 5, "lobal", TOKEN_GLOBAL);
            break;
        case 'i':
            if (scanner.current - scanner.start > 1)
                switch (scanner.start[1])
                {
                    case 'f':
                        return checkKeyword(2, 0, "", TOKEN_IF);
                    case 's':
                        return checkKeyword(2, 0, "", TOKEN_IS);
                    case 'n': {
                        if (scanner.current - scanner.start > 2)
                        {
                            switch (scanner.start[2])
                            {
                                case 'f':
                                    return checkKeyword(3, 0, "", TOKEN_INF);
                            }
                        }
                        else
                        {
                            return checkKeyword(2, 0, "", TOKEN_IN);
                        }
                    }
                    case 'm':
                        return checkKeyword(2, 4, "port", TOKEN_IMPORT);
                }
            break;
        case 'n':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'u':
                        return checkKeyword(2, 2, "ll", TOKEN_NULL);
                    case 'o': {
                        if (scanner.current - scanner.start > 2)
                        {
                            switch (scanner.start[2])
                            {
                                case 't':
                                    return checkKeyword(3, 0, "", TOKEN_BANG);
                            }
                        }
                        break;
                    }
                    case 'a': {
                        if (scanner.current - scanner.start > 2)
                        {
                            switch (scanner.start[2])
                            {
                                case 'n':
                                    return checkKeyword(3, 0, "", TOKEN_NAN);
                                case 't':
                                    return checkKeyword(3, 3, "ive", TOKEN_NATIVE);
                            }
                        }
                        break;
                    }
                }
            }
            break;
        case 'o':
            return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'r':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'e': {
                        if (scanner.current - scanner.start > 2)
                        {
                            switch (scanner.start[2])
                            {
                                case 't':
                                    return checkKeyword(3, 3, "urn", TOKEN_RETURN);
                                case 'q':
                                    return checkKeyword(3, 4, "uire", TOKEN_REQUIRE);
                            }
                        }
                        break;
                    }
                }
            }
            break;
        case 's':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'u':
                        return checkKeyword(2, 3, "per", TOKEN_SUPER);
                    case 't':
                        return checkKeyword(2, 4, "atic", TOKEN_STATIC);
                    case 'w':
                        return checkKeyword(2, 4, "itch", TOKEN_SWITCH);
                }
            }
            break;
        case 't':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'h':
                        return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': {
                        if (scanner.current - scanner.start > 2)
                        {
                            switch (scanner.start[2])
                            {
                                case 'u':
                                    return checkKeyword(3, 1, "e", TOKEN_TRUE);
                                case 'y':
                                    return checkKeyword(3, 0, "", TOKEN_TRY);
                            }
                        }
                        break;
                    }
                }
            }
            break;
        case 'v':
            return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w':
            if (scanner.current - scanner.start > 1)
                switch (scanner.start[1])
                {
                    case 'h':
                        return checkKeyword(2, 3, "ile", TOKEN_WHILE);
                    case 'i':
                        return checkKeyword(2, 2, "th", TOKEN_WITH);
                }
            break;
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    while (isAlpha(peek()) || isDigit(peek()))
        advance();

    return makeToken(identifierType());
}

static Token number()
{
    while (isDigit(peek()))
        advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext()))
    {
        // Consume the ".".
        advance();

        while (isDigit(peek()))
            advance();
    }
    else if (peek() == 'e' && isDigit(peekNext()))
    {
        // Consume the ".".
        advance();

        while (isDigit(peek()))
            advance();
    }
    else if (peek() == 'e' && ((peekNext() == '-' || peekNext() == '+') && isDigit(peekNextNext())))
    {
        // Consume the ".".
        advance();
        advance();

        while (isDigit(peek()))
            advance();
    }
    else if (peek() == 'x' && isHex(peekNext()))
    {
        // consum the 'x'
        advance();

        while (isHex(peek()))
            advance();

        return makeToken(TOKEN_BYTE);
    }

    return makeToken(TOKEN_NUMBER);
}

static Token string(char c)
{
    while (peek() != c && !isAtEnd())
    {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string.");

    advance();
    return makeToken(TOKEN_STRING);
}

void backTrack()
{
    scanner.current--;
}

Token scanToken()
{
    Token doc;
    doc.type = TOKEN_ERROR;
    skipWhitespace(&doc);
    if (doc.type == TOKEN_DOC)
    {
        return doc;
    }

    scanner.start = scanner.current;

    if (isAtEnd())
        return makeToken(TOKEN_EOF);

    char c = advance();

    if (isAlpha(c))
        return identifier();
    if (isDigit(c))
        return number();

    switch (c)
    {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case '[':
            return makeToken(TOKEN_LEFT_BRACKET);
        case ']':
            return makeToken(TOKEN_RIGHT_BRACKET);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ':':
            return makeToken(TOKEN_COLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.': {
            if (match('.'))
            {
                if (match('.'))
                    return makeToken(TOKEN_EXPAND_EX);
                else
                    return makeToken(TOKEN_EXPAND_IN);
            }
            else if (match('+'))
                return makeToken(TOKEN_DOT_PLUS);
            else if (match('-'))
                return makeToken(TOKEN_DOT_MINUS);
            else if (match('*'))
                return makeToken(TOKEN_DOT_STAR);
            else if (match('/'))
                return makeToken(TOKEN_DOT_SLASH);
            else if (match('^'))
                return makeToken(TOKEN_DOT_POW);
            else if (match('%'))
                return makeToken(TOKEN_DOT_PERCENT);
            else
                return makeToken(TOKEN_DOT);
        }
        case '%':
            return makeToken(TOKEN_PERCENT);
        case '-': {
            if (match('-'))
                return makeToken(TOKEN_DEC);
            else if (match('='))
                return makeToken(TOKEN_MINUS_EQUALS);
            else
                return makeToken(TOKEN_MINUS);
        }
        case '+': {
            if (match('+'))
                return makeToken(TOKEN_INC);
            else if (match('='))
                return makeToken(TOKEN_PLUS_EQUALS);
            else
                return makeToken(TOKEN_PLUS);
        }
        case '/': {
            if (match('='))
                return makeToken(TOKEN_DIVIDE_EQUALS);
            else
                return makeToken(TOKEN_SLASH);
        }
        case '*': {
            if (match('='))
                return makeToken(TOKEN_MULTIPLY_EQUALS);
            else
                return makeToken(TOKEN_STAR);
        }
        case '^':
            return makeToken(TOKEN_POW);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': {
            if (match('<'))
            {
                return makeToken(TOKEN_SHIFT_LEFT);
            }
            else
                return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        }
        case '>': {
            if (match('>'))
            {
                return makeToken(TOKEN_SHIFT_RIGHT);
            }
            else
                return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        }

        case '@':
            return makeToken(TOKEN_LAMBDA);

        case '"':
        case '\'':
            return string(c);
    }

    return errorToken("Unexpected character.");
}

bool isOperator(TokenType type)
{
    return type == TOKEN_MINUS || type == TOKEN_PLUS || type == TOKEN_SLASH || type == TOKEN_STAR ||
           type == TOKEN_POW || type == TOKEN_BANG || type == TOKEN_BANG_EQUAL || type == TOKEN_EQUAL_EQUAL ||
           type == TOKEN_LESS || type == TOKEN_GREATER || type == TOKEN_LESS_EQUAL || type == TOKEN_GREATER_EQUAL;
}