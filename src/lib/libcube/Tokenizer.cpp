#include "Tokenizer.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;

Tokenizer::Tokenizer()
{
    token.type = TokenType::INVALID;
    token.col = 1;
    token.row = 1;
    eof = false;

    keywords.push_back("if");
    keywords.push_back("else");
    keywords.push_back("func");
    keywords.push_back("@");
    keywords.push_back("true");
    keywords.push_back("false");
    keywords.push_back("none");
    keywords.push_back("return");
    keywords.push_back("for");
    keywords.push_back("while");
    keywords.push_back("in");
    keywords.push_back("let");
    keywords.push_back("do");
    keywords.push_back("import");
    keywords.push_back("as");
    keywords.push_back("global");
    keywords.push_back("native");
    keywords.push_back("try");
    keywords.push_back("catch");
    keywords.push_back("class");
    keywords.push_back("new");

    operators.insert(".");
    operators.insert("+");
    operators.insert("++");
    operators.insert("-");
    operators.insert("--");
    operators.insert("*");
    operators.insert("/");
    operators.insert("%");
    operators.insert("!");
    operators.insert("=");
    operators.insert("==");
    operators.insert("<>");
    operators.insert(">=");
    operators.insert("<=");
    operators.insert(">");
    operators.insert("<");
    operators.insert("[");
    operators.insert("]");
    operators.insert("^");
    operators.insert("~");
    operators.insert("|");
    operators.insert("||");
    operators.insert("&");
    operators.insert("&&");
    operators.insert(":");
    operators.insert("**");
    operators.insert(">>");
    operators.insert("<<");
}

Tokenizer::~Tokenizer()
{

}

void Tokenizer::Fill(const string& src)
{
    input.Fill(src);
    token.col = 1;
    token.row = 1;
    token.type = TokenType::NONE;
    eof = false;
}

bool Tokenizer::Eof()
{
    bool eof = input.Eof();
    if(eof)
    {
        if(this->eof == false)
        {
            this->eof = true;
            eof = false;
        }
    }
    return eof;
}

Token& Tokenizer::Peek()
{
    return token;
}

Token& Tokenizer::Next()
{
    begin:

    if(Eof())
    {
        token.type = TokenType::INVALID;
        return token;
    }

    char c = SkipWhite();
    c = SkipComment();
    token.col = input.Col();
    token.row = input.Row();
    if(c == 0)
        token.type = TokenType::INVALID;
    else if(c == '\'' || c == '"')
        ReadString();
    else if(IsDigit(c))
        ReadNumber();
    else if(IsLetter(c) || c == '_' || c == '@')
        ReadName();
    else if(IsOperator(c))
        ReadOperator();
    else if(IsSymbol(c))
    {
        token.type = TokenType::SYMBOL;
        token._string = "";
        token._string += c;
        input.Next();
    }
    else
    {
        MakeError("Invalid character ", c);
        input.Next();
    }

    return token;
}

bool Tokenizer::IsDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool Tokenizer::IsLetter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Tokenizer::IsOperator(char c)
{
    return (
            c == '+' ||
            c == '-' ||
            c == '/' ||
            c == '*' ||
            c == '%' ||
            c == '!' ||
            c == '=' ||
            c == '>' ||
            c == '<' ||
            c == '[' ||
            c == ']' ||
            c == '^' ||
            c == '~' ||
            c == '|' ||
            c == '&' ||
            c == ':' ||
            c == '.'
    );
}

bool Tokenizer::IsSymbol(char c)
{
    return (
            c == '{' ||
            c == '}' ||
            c == '(' ||
            c == ')' ||
            c == ',' ||
            c == ';'
    );
}

char Tokenizer::SkipWhite()
{
    char c = input.Peek();
    while(c == 0 || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\b' || c == '\a')
    {
        c = input.Next();
        if(c == 0)
            break;
    }
    return c;
}

char Tokenizer::SkipComment()
{
    char c = input.Peek();
    while(c == '#')
    {
        c = input.Next();
        while(c != '\n')
        {
            c = input.Next();
        }
        c = input.Next();
        if(c == '\r')
        {
            c = input.Next();
        }
    }
    return c;
}

void Tokenizer::ReadString()
{
    string str = "";
    char endChar = input.Peek() == '\'' ? '\'' : '"';
    char c = input.Next();
    bool parse = false;
    while(!(c == endChar) || parse)
    {
        if(parse == false && c == '\\')
            parse = true;
        else if(parse == true)
        {
            if(c == 'a')
                str += "\a";
            else if(c == 'b')
                str += "\b";
            else if(c == 'n')
                str += "\n";
            else if(c == 'v')
                str += "\v";
            else if(c == 'r')
                str += "\r";
            else if(c == 't')
                str += "\t";
            else if(c == '\'' && endChar == '\'')
                str += "\'";
            else if(c == '"' && endChar == '"')
                str += "\"";
            else
            {
                str += "\\";
                str += c;
            }
            parse = false;
        }
        else
            str += c;
        c = input.Next();
        if(input.Eof())
        {
            MakeError("Could not find the end of the string");
            return;
        }
    }
    input.Next();
    token.type = TokenType::STRING;
    token._string = str;
}

void Tokenizer::ReadNumber()
{
    stringstream ss;
    double base = 0;
    int expoent = 0;
    int count = 0;
    bool hasExpoent = false;
    bool hasPoint = false;
    bool hasSignal = false;
    
    char c = input.Peek();
    ss << c;
    count++;
    int afterDigit = 0;

    c = input.Next();
    do
    {
        if(IsDigit(c))
        {
            ss << c;
            count++;
            if(hasPoint)
                afterDigit++;
        }
        else if((c == '+' || c == '-') && hasExpoent && !hasSignal)
        {
            ss << c;
            hasSignal = true;
        }
        else if(c == '.' && !hasPoint && !hasExpoent)
        {
            ss << c;
            hasPoint = true;
        }
        else if((c == 'e' || c == 'E') && !hasExpoent)
        {
            if(hasPoint && afterDigit == 0)
                break;
            if(count > 0)
                ss >> base;
            else
                base = 0;
            count = 0;
            ss = stringstream();
            hasExpoent = true;
        }
        else
            break;
        c = input.Next();
    }
    while(!input.Eof());
    if(hasPoint && afterDigit == 0)
        input.Back();
    if(hasExpoent)
    {
        if(count > 0)
            ss >> expoent;
        else
            expoent = 0;
    }
    else
    {
        if(count > 0)
            ss >> base;
        else
            base = 0;
    }

    token.type = TokenType::NUMBER;
    token._number = base*pow(10, expoent);
}

void Tokenizer::ReadName()
{
    string str = "";
    char c = input.Peek();
    str += c;

    c = input.Next();
    while(IsLetter(c) || c == '_' || IsDigit(c))
    {
        str += c;
        c = input.Next();
        if(input.Eof())
            break;
    }
    token._string = str;
    if(find(keywords.begin(), keywords.end(), str) != keywords.end())
        token.type = TokenType::KEYWORD;
    else    
        token.type = TokenType::VARIABLE;
}

void Tokenizer::ReadOperator()
{
    string str = "";
    char c = input.Peek();
    str += c;

    c = input.Next();
    while(IsOperator(c) && IsOperator(str+c))
    {
        str += c;
        c = input.Next();
        if(input.Eof())
            break;
    }
    token._string = str;
    token.type = TokenType::OPERATOR;
}

void Tokenizer::MakeError(const string& msg, char c)
{
    stringstream ss;
    ss << msg;
    if(c > 0)
        ss << c;
    ss << " [Line " << input.Row() << ", Column: " << input.Col() << "]";
    token._string = ss.str();
    token.type = TokenType::ERROR;
}

bool Tokenizer::IsOperator(const std::string& op)
{
    return operators.count(op) > 0;
}