#ifndef _NODE_H_
#define _NODE_H_

#include <string>
#include <sstream>
#include <vector>
#include "SP.h"

namespace NodeType
{
    enum Types
    {
        IGNORE, ERROR, NONE, VARIABLE, BOOL, NUMBER, STRING, ARRAY, INDEX, ASSIGN, BINARY, RETURN, LET, LAMBDA, FUNCTION, IF, FOR, WHILE, DOWHILE, CONTEXT, CALL
    };
}

struct Node_st
{   
    NodeType::Types type;
    std::string _string;
    double _number;   
    bool _bool;
    std::vector<SP<struct Node_st> > nodes;
    std::vector<std::string> vars;
    SP<struct Node_st> body;
    SP<struct Node_st> func;
    SP<struct Node_st> cond;
    SP<struct Node_st> then;
    SP<struct Node_st> contr;
    SP<struct Node_st> left, middle, right;


    std::string ToString(int spaces = 0);
};

typedef SP<struct Node_st> Node;
#define MKNODE() Node(new struct Node_st())

#endif