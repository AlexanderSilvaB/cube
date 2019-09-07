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
        IGNORE = 1, ERROR, NONE, VARIABLE, BOOL, NUMBER, STRING, ARRAY, DICT, INDEX, ASSIGN, BINARY, RETURN, LET, LAMBDA, FUNCTION, FUNCTION_DEF, EXTENSION, IF, FOR, WHILE, DOWHILE, CONTEXT, CALL, IMPORT, TRY, CLASS, SPAWN
    };
}

struct Node_st
{   
    bool createNew;
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
    std::vector<char> Serialize();
    void Deserialize(std::vector<char>& data);

    Node_st()
    {
        createNew = false;
    }
};

typedef union
{
    bool _bool;
    int _int;
    double _double;
    char bytes[sizeof(double)];
}binary;

typedef std::shared_ptr<struct Node_st> Node;
#define MKNODE() Node(new struct Node_st())

#define serialize(bin, data, value, type) bin._ ## type = (type)value, data.insert(data.end(), bin.bytes, bin.bytes + sizeof(type))
#define serializeS(bin, data, str) {serialize(bin, data, str.length(), int); const char *s = str.c_str(); data.insert(data.end(), s, s+str.length());}
#define serializeV(data, other) data.insert( data.end(), other.begin(), other.end() )

#define deserialize(bin, data, name, type, cast) bin._ ## type = *(type*)data.data(), name = (cast)bin._ ## type, data.erase(data.begin(), data.begin() + sizeof(type))
#define deserializeS(bin, data, str) {int sz; deserialize(bin, data, sz, int, int); string s(data.data(), sz); str = s; data.erase(data.begin(), data.begin() + sz);}

#endif