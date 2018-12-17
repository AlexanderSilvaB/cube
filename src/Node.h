#ifndef _NODE_H_
#define _NODE_H_

#include <string>
#include <sstream>
#include <vector>
#include <memory>

namespace NodeType
{
    enum Types
    {
        IGNORE, ERROR, NONE, VARIABLE, BOOL, NUMBER, STRING, ARRAY, DICT, INDEX, ASSIGN, BINARY, RETURN, LET, LAMBDA, FUNCTION, EXTENSION, IF, FOR, WHILE, DOWHILE, CONTEXT, CALL, IMPORT, TRY
    };
}

struct Node_st
{   
    NodeType::Types type;
    std::string _string, _nick;
    double _number;   
    bool _bool;
    std::vector<std::shared_ptr<struct Node_st> > nodes;
    std::vector<std::string> vars;
    std::shared_ptr<struct Node_st> body;
    std::shared_ptr<struct Node_st> func;
    std::shared_ptr<struct Node_st> cond;
    std::shared_ptr<struct Node_st> then;
    std::shared_ptr<struct Node_st> contr;
    std::shared_ptr<struct Node_st> left, middle, right;


    std::string ToString(int spaces = 0);
};

typedef std::shared_ptr<struct Node_st> Node;
#define MKNODE() Node(new struct Node_st())

#endif