#include <stdio.h>
#include <string.h>

#include "common.h"
#include "mempool.h"
#include "scanner.h"

Keyword *read_keyword(char **source);
TokenType get_token_type(char *key);

void initScanner(Scanner *scanner, const char *source)
{
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
    scanner->keywords = NULL;
}

bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool isHex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

static bool isAtEnd(Scanner *scanner)
{
    return *scanner->current == '\0';
}

static char advance(Scanner *scanner)
{
    scanner->current++;
    return scanner->current[-1];
}

static char peek(Scanner *scanner)
{
    return *scanner->current;
}

static char peekNext(Scanner *scanner)
{
    if (isAtEnd(scanner))
        return '\0';
    return scanner->current[1];
}

static char peekNextNext(Scanner *scanner)
{
    if (isAtEnd(scanner))
        return '\0';
    return scanner->current[2];
}

static bool match(Scanner *scanner, char expected)
{
    if (isAtEnd(scanner))
        return false;
    if (*scanner->current != expected)
        return false;

    scanner->current++;
    return true;
}

Token getArguments(Scanner *scanner, int n)
{
    Token args;
    args.length = 0;
    args.start = scanner->current;
    int folds = 0;
    int N = 0;
    while (args.start[args.length] != '\0')
    {
        if (args.start[args.length] == '(')
            folds++;
        else if (args.start[args.length] == ')')
        {
            if (folds > 0)
                folds--;
            else
            {
                break;
            }
        }
        else if (args.start[args.length] == ',' && folds == 0 && n > 0)
        {
            N++;
            if (n == N)
                break;
        }
        args.length++;
    }

    if (args.start[args.length] == '\0')
    {
        args.start = NULL;
        args.length = 0;
    }

    return args;
}

static Token makeToken(Scanner *scanner, TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;

    return token;
}

static Token errorToken(Scanner *scanner, const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner->line;

    return token;
}

static void skipWhitespace(Scanner *scanner, Token *doc)
{
    for (;;)
    {
        char c = peek(scanner);
        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;

            case '\n':
                scanner->line++;
                advance(scanner);
                break;

            case '#': {
                if (peekNext(scanner) == '!' && scanner->start == scanner->current)
                {
                    char *shebang = ((char *)scanner->current) + 2;

                    while (peek(scanner) != '\n' && !isAtEnd(scanner))
                        advance(scanner);

                    int len = scanner->current - shebang;
                }
                break;
            }

            case '/':
                if (peekNext(scanner) == '/')
                {
                    advance(scanner);
                    advance(scanner);
                    if (peek(scanner) == '?')
                    {
                        advance(scanner);
                        doc->type = TOKEN_DOC;
                        doc->start = scanner->current;
                        doc->line = scanner->line;
                    }

                    // A comment goes until the end of the line.
                    while (peek(scanner) != '\n' && !isAtEnd(scanner))
                        advance(scanner);

                    if (doc->type == TOKEN_DOC)
                        doc->length = scanner->current - doc->start;
                }
                else if (peekNext(scanner) == '*')
                {
                    // A block comment goes until */
                    advance(scanner);
                    advance(scanner);

                    if (peek(scanner) == '?')
                    {
                        advance(scanner);
                        doc->type = TOKEN_DOC;
                        doc->start = scanner->current;
                        doc->line = scanner->line;
                    }

                    bool done = peek(scanner) == '*' && peekNext(scanner) == '/';
                    while (!done && !isAtEnd(scanner))
                    {
                        advance(scanner);
                        done = peek(scanner) == '*' && peekNext(scanner) == '/';
                    }

                    if (doc->type == TOKEN_DOC)
                        doc->length = scanner->current - doc->start;

                    if (!isAtEnd(scanner))
                    {
                        advance(scanner);
                        advance(scanner);
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

static TokenType checkKeyword(Scanner *scanner, int start, int length, const char *rest, TokenType type)
{
    if (scanner->current - scanner->start == start + length && memcmp(scanner->start + start, rest, length) == 0)
    {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType(Scanner *scanner)
{
    if (scanner->keywords != NULL)
    {
        int L = (scanner->current - scanner->start);
        do
        {
            Keyword *keyword = linked_list_get(scanner->keywords);
            if (keyword != NULL)
            {
                int l = strlen(keyword->word);
                if (L == l && memcmp(scanner->start, keyword->word, l) == 0)
                {
                    return keyword->key;
                }
            }
        } while (linked_list_next(&scanner->keywords));
        linked_list_reset(&scanner->keywords);
    }

    switch (scanner->start[0])
    {
        case 'a':
            if (scanner->current - scanner->start > 1)
            {
                switch (scanner->start[1])
                {
                    case 'b':
                        return checkKeyword(scanner, 2, 3, "ort", TOKEN_ABORT);
                    case 's': {
                        if (scanner->current - scanner->start > 2)
                        {
                            switch (scanner->start[2])
                            {
                                case 'y':
                                    return checkKeyword(scanner, 3, 2, "nc", TOKEN_ASYNC);
                                case 's':
                                    return checkKeyword(scanner, 3, 3, "ert", TOKEN_ASSERT);
                            }
                        }
                        else
                        {
                            return checkKeyword(scanner, 2, 0, "", TOKEN_AS);
                        }
                    }
                    case 'n':
                        return checkKeyword(scanner, 2, 1, "d", TOKEN_AND);
                    case 'w':
                        return checkKeyword(scanner, 2, 3, "ait", TOKEN_AWAIT);
                }
            }
            break;
        case 'b':
            return checkKeyword(scanner, 1, 4, "reak", TOKEN_BREAK);
        case 'c':
            if (scanner->current - scanner->start > 1)
                switch (scanner->start[1])
                {
                    case 'a': {
                        if (scanner->current - scanner->start > 2)
                        {
                            switch (scanner->start[2])
                            {
                                case 's':
                                    return checkKeyword(scanner, 3, 1, "e", TOKEN_CASE);
                                case 't':
                                    return checkKeyword(scanner, 3, 2, "ch", TOKEN_CATCH);
                            }
                        }
                        break;
                    }
                    case 'l':
                        return checkKeyword(scanner, 2, 3, "ass", TOKEN_CLASS);
                    case 'o':
                        return checkKeyword(scanner, 2, 6, "ntinue", TOKEN_CONTINUE);
                    case 'u':
                        return checkKeyword(scanner, 2, 2, "be", TOKEN_CUBE);
                }
            break;
        case 'd':
            if (scanner->current - scanner->start > 1)
            {
                switch (scanner->start[1])
                {
                    case 'e':
                        return checkKeyword(scanner, 2, 5, "fault", TOKEN_DEFAULT);
                    case 'o':
                        return checkKeyword(scanner, 2, 0, "", TOKEN_DO);
                }
            }
            break;
        case 'e':
            if (scanner->current - scanner->start > 1)
            {
                switch (scanner->start[1])
                {
                    case 'l':
                        return checkKeyword(scanner, 2, 2, "se", TOKEN_ELSE);
                    case 'n':
                        return checkKeyword(scanner, 2, 2, "um", TOKEN_ENUM);
                }
            }
            break;
        case 'f':
            if (scanner->current - scanner->start > 1)
            {
                switch (scanner->start[1])
                {
                    case 'a':
                        return checkKeyword(scanner, 2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return checkKeyword(scanner, 2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return checkKeyword(scanner, 2, 2, "nc", TOKEN_FUNC);
                }
            }
            break;
        case 'g':
            return checkKeyword(scanner, 1, 5, "lobal", TOKEN_GLOBAL);
            break;
        case 'i':
            if (scanner->current - scanner->start > 1)
                switch (scanner->start[1])
                {
                    case 'f':
                        return checkKeyword(scanner, 2, 0, "", TOKEN_IF);
                    case 's':
                        return checkKeyword(scanner, 2, 0, "", TOKEN_IS);
                    case 'n': {
                        if (scanner->current - scanner->start > 2)
                        {
                            switch (scanner->start[2])
                            {
                                case 'f':
                                    return checkKeyword(scanner, 3, 0, "", TOKEN_INF);
                                case 'c':
                                    return checkKeyword(scanner, 3, 4, "lude", TOKEN_INCLUDE);
                            }
                        }
                        else
                        {
                            return checkKeyword(scanner, 2, 0, "", TOKEN_IN);
                        }
                    }
                    case 'm':
                        return checkKeyword(scanner, 2, 4, "port", TOKEN_IMPORT);
                }
            break;
        case 'n':
            if (scanner->current - scanner->start > 1)
            {
                switch (scanner->start[1])
                {
                    case 'u':
                        return checkKeyword(scanner, 2, 2, "ll", TOKEN_NULL);
                    case 'o': {
                        if (scanner->current - scanner->start > 2)
                        {
                            switch (scanner->start[2])
                            {
                                case 't':
                                    return checkKeyword(scanner, 3, 0, "", TOKEN_BANG);
                            }
                        }
                        break;
                    }
                    case 'a': {
                        if (scanner->current - scanner->start > 2)
                        {
                            switch (scanner->start[2])
                            {
                                case 'n':
                                    return checkKeyword(scanner, 3, 0, "", TOKEN_NAN);
                                case 't':
                                    return checkKeyword(scanner, 3, 3, "ive", TOKEN_NATIVE);
                            }
                        }
                        break;
                    }
                }
            }
            break;
        case 'o':
            return checkKeyword(scanner, 1, 1, "r", TOKEN_OR);
        case 'r':
            if (scanner->current - scanner->start > 1)
            {
                switch (scanner->start[1])
                {
                    case 'e': {
                        if (scanner->current - scanner->start > 2)
                        {
                            switch (scanner->start[2])
                            {
                                case 't':
                                    return checkKeyword(scanner, 3, 3, "urn", TOKEN_RETURN);
                                case 'q':
                                    return checkKeyword(scanner, 3, 4, "uire", TOKEN_REQUIRE);
                            }
                        }
                        break;
                    }
                }
            }
            break;
        case 's':
            if (scanner->current - scanner->start > 1)
            {
                switch (scanner->start[1])
                {
                    case 'u':
                        return checkKeyword(scanner, 2, 3, "per", TOKEN_SUPER);
                    case 't': {
                        if (scanner->current - scanner->start > 2)
                        {
                            switch (scanner->start[2])
                            {
                                case 'r':
                                    return checkKeyword(scanner, 3, 3, "uct", TOKEN_STRUCT);
                                case 'a':
                                    return checkKeyword(scanner, 3, 3, "tic", TOKEN_STATIC);
                            }
                        }
                        break;
                    }
                    case 'w':
                        return checkKeyword(scanner, 2, 4, "itch", TOKEN_SWITCH);
                }
            }
            break;
        case 't':
            if (scanner->current - scanner->start > 1)
            {
                switch (scanner->start[1])
                {
                    case 'h':
                        return checkKeyword(scanner, 2, 2, "is", TOKEN_THIS);
                    case 'r': {
                        if (scanner->current - scanner->start > 2)
                        {
                            switch (scanner->start[2])
                            {
                                case 'u':
                                    return checkKeyword(scanner, 3, 1, "e", TOKEN_TRUE);
                                case 'y':
                                    return checkKeyword(scanner, 3, 0, "", TOKEN_TRY);
                            }
                        }
                        break;
                    }
                }
            }
            break;
        case 'v':
            return checkKeyword(scanner, 1, 2, "ar", TOKEN_VAR);
        case 'w':
            if (scanner->current - scanner->start > 1)
                switch (scanner->start[1])
                {
                    case 'h':
                        return checkKeyword(scanner, 2, 3, "ile", TOKEN_WHILE);
                    case 'i':
                        return checkKeyword(scanner, 2, 2, "th", TOKEN_WITH);
                }
            break;
        case 'p':
            return checkKeyword(scanner, 1, 3, "ass", TOKEN_PASS);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier(Scanner *scanner)
{
    while (isAlpha(peek(scanner)) || isDigit(peek(scanner)))
        advance(scanner);

    return makeToken(scanner, identifierType(scanner));
}

static Token number(Scanner *scanner)
{
    while (isDigit(peek(scanner)))
        advance(scanner);

    // Look for a fractional part.
    if (peek(scanner) == '.' && isDigit(peekNext(scanner)))
    {
        // Consume the ".".
        advance(scanner);

        while (isDigit(peek(scanner)))
            advance(scanner);
    }
    else if (peek(scanner) == 'e' && isDigit(peekNext(scanner)))
    {
        // Consume the ".".
        advance(scanner);

        while (isDigit(peek(scanner)))
            advance(scanner);
    }
    else if (peek(scanner) == 'e' &&
             ((peekNext(scanner) == '-' || peekNext(scanner) == '+') && isDigit(peekNextNext(scanner))))
    {
        // Consume the ".".
        advance(scanner);
        advance(scanner);

        while (isDigit(peek(scanner)))
            advance(scanner);
    }
    else if (peek(scanner) == 'x' && isHex(peekNext(scanner)))
    {
        // consum the 'x'
        advance(scanner);

        while (isHex(peek(scanner)))
            advance(scanner);

        return makeToken(scanner, TOKEN_BYTE);
    }

    return makeToken(scanner, TOKEN_NUMBER);
}

static Token string(Scanner *scanner, char c)
{
    while (peek(scanner) != c && !isAtEnd(scanner))
    {
        if (peek(scanner) == '\n')
            scanner->line++;
        advance(scanner);
    }

    if (isAtEnd(scanner))
        return errorToken(scanner, "Unterminated string.");

    advance(scanner);
    return makeToken(scanner, TOKEN_STRING);
}

void backTrack(Scanner *scanner)
{
    scanner->current--;
}

Token scanToken(Scanner *scanner)
{
    Token doc;
    doc.type = TOKEN_ERROR;
    skipWhitespace(scanner, &doc);
    if (doc.type == TOKEN_DOC)
    {
        return doc;
    }

    scanner->start = scanner->current;

    if (isAtEnd(scanner))
        return makeToken(scanner, TOKEN_EOF);

    char c = advance(scanner);

    if (isAlpha(c))
        return identifier(scanner);
    if (isDigit(c))
        return number(scanner);

    switch (c)
    {
        case '(':
            return makeToken(scanner, TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(scanner, TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(scanner, TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(scanner, TOKEN_RIGHT_BRACE);
        case '[':
            return makeToken(scanner, TOKEN_LEFT_BRACKET);
        case ']':
            return makeToken(scanner, TOKEN_RIGHT_BRACKET);
        case ';':
            return makeToken(scanner, TOKEN_SEMICOLON);
        case ':':
            return makeToken(scanner, TOKEN_COLON);
        case ',':
            return makeToken(scanner, TOKEN_COMMA);
        case '.': {
            if (match(scanner, '.'))
            {
                if (match(scanner, '.'))
                    return makeToken(scanner, TOKEN_EXPAND_EX);
                else
                    return makeToken(scanner, TOKEN_EXPAND_IN);
            }
            else if (match(scanner, '+'))
                return makeToken(scanner, TOKEN_DOT_PLUS);
            else if (match(scanner, '-'))
                return makeToken(scanner, TOKEN_DOT_MINUS);
            else if (match(scanner, '*'))
                return makeToken(scanner, TOKEN_DOT_STAR);
            else if (match(scanner, '/'))
                return makeToken(scanner, TOKEN_DOT_SLASH);
            else if (match(scanner, '^'))
                return makeToken(scanner, TOKEN_DOT_POW);
            else if (match(scanner, '%'))
                return makeToken(scanner, TOKEN_DOT_PERCENT);
            else
                return makeToken(scanner, TOKEN_DOT);
        }
        case '%':
            return makeToken(scanner, TOKEN_PERCENT);
        case '-': {
            if (match(scanner, '-'))
                return makeToken(scanner, TOKEN_DEC);
            else if (match(scanner, '='))
                return makeToken(scanner, TOKEN_MINUS_EQUALS);
            else
                return makeToken(scanner, TOKEN_MINUS);
        }
        case '+': {
            if (match(scanner, '+'))
                return makeToken(scanner, TOKEN_INC);
            else if (match(scanner, '='))
                return makeToken(scanner, TOKEN_PLUS_EQUALS);
            else
                return makeToken(scanner, TOKEN_PLUS);
        }
        case '/': {
            if (match(scanner, '='))
                return makeToken(scanner, TOKEN_DIVIDE_EQUALS);
            else
                return makeToken(scanner, TOKEN_SLASH);
        }
        case '*': {
            if (match(scanner, '='))
                return makeToken(scanner, TOKEN_MULTIPLY_EQUALS);
            else
                return makeToken(scanner, TOKEN_STAR);
        }
        case '^':
            return makeToken(scanner, TOKEN_POW);
        case '!':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': {
            if (match(scanner, '<'))
            {
                return makeToken(scanner, TOKEN_SHIFT_LEFT);
            }
            else
                return makeToken(scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        }
        case '>': {
            if (match(scanner, '>'))
            {
                return makeToken(scanner, TOKEN_SHIFT_RIGHT);
            }
            else
                return makeToken(scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        }

        case '@':
            return makeToken(scanner, TOKEN_LAMBDA);

        case '&': {
            if (match(scanner, '&'))
                return makeToken(scanner, TOKEN_AND);
            else
                return makeToken(scanner, TOKEN_BINARY_AND);
        }

        case '|': {
            if (match(scanner, '|'))
                return makeToken(scanner, TOKEN_OR);
            else
                return makeToken(scanner, TOKEN_BINARY_OR);
        }

        case '"':
        case '\'':
            return string(scanner, c);
    }

    return errorToken(scanner, "Unexpected character.");
}

bool isOperator(TokenType type)
{
    return type == TOKEN_MINUS || type == TOKEN_PLUS || type == TOKEN_SLASH || type == TOKEN_STAR ||
           type == TOKEN_POW || type == TOKEN_BANG || type == TOKEN_BANG_EQUAL || type == TOKEN_EQUAL_EQUAL ||
           type == TOKEN_LESS || type == TOKEN_GREATER || type == TOKEN_LESS_EQUAL || type == TOKEN_GREATER_EQUAL ||
           type == TOKEN_SHIFT_LEFT || type == TOKEN_SHIFT_RIGHT || type == TOKEN_BINARY_AND ||
           type == TOKEN_BINARY_OR || type == TOKEN_INC || type == TOKEN_DEC;
}

bool setLanguage(Scanner *scanner, const char *languageSource)
{
    if (scanner->keywords != NULL)
    {
        linked_list_destroy(scanner->keywords, true);
    }
    scanner->keywords = NULL;

    scanner->keywords = linked_list_create();

    char *source = (char *)languageSource;
    Keyword *keyword = read_keyword(&source);
    while (keyword != NULL)
    {
        linked_list_add(scanner->keywords, keyword);
        keyword = read_keyword(&source);
    }

    return false;
}

Keyword *read_keyword(char **source)
{
    char key[32];
    char word[32];

    char *src = *source;
    int rc = sscanf(src, "%s %s\n", (char *)&key, (char *)&word);
    if (rc != 2)
        return NULL;

    int inc = strlen(key) + strlen(word) + 2;
    src += inc;

    *source = src;

    printf("%d: %s -> %s\n", rc, key, word);

    Keyword *keyword = mp_malloc(sizeof(Keyword));
    keyword->key = get_token_type(key);
    strcpy(keyword->word, word);
    return keyword;
}

TokenType get_token_type(char *key)
{
    if (strcmp(key, "if") == 0)
    {
        return TOKEN_IF;
    }
    else if (strcmp(key, "else") == 0)
    {
        return TOKEN_ELSE;
    }
    return TOKEN_ERROR;
}