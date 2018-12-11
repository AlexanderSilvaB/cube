#include "Input.h"
#include <iostream>

using namespace std;

Input::Input()
{
    src = "";
    pos = 0;
    row = 1;
    col = 1;
    started = false;
}

Input::~Input()
{
    
}

void Input::Fill(const string& src)
{
    this->src = src;
    pos = 0;
    row = 1;
    col = 1;
    started = false;
}

bool Input::Eof()
{
    if(src.size() == 0)
        return true;
    if(!started)
        return false;
    return pos >= src.size();
}

char Input::Peek()
{
    if(Eof() || !started)
        return 0;
    return src[pos];
}

char Input::Next()
{
    if(Eof())
        return 0;
    if(started)
        pos++;
    else
    {
        started = true;
        pos = 0;
    }
    char c = src[pos];
    col++;
    if(c == '\n')
    {
        row++;
        col = 1;
    }
    return c;
}

size_t Input::Col()
{
    return col;
}

size_t Input::Row()
{
    return row;
}