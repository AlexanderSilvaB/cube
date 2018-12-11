#ifndef _TOKEN_H_
#define _TOKEN_H_

#include <string>
#include <sstream>

namespace TokenType
{
    enum Types
    {
        INVALID, ERROR, NONE, OPERATOR, SYMBOL, VARIABLE, KEYWORD, NUMBER, STRING
    };
}

typedef struct
{   
    TokenType::Types type;
    int col, row;
    std::string _string;
    double _number;

    std::string ToString()
    {
        std::stringstream ss;
        switch(type)
        {
            case TokenType::INVALID:
                ss << "Invalid";
                break;
            case TokenType::ERROR:
                ss << "Error: " << _string;
                break;
            case TokenType::NONE:
                ss << "None";
                break;
            case TokenType::OPERATOR:
                ss << "Operator: " << _string;
                break;
            case TokenType::SYMBOL:
                ss << "Symbol: " << _string;
                break;
            case TokenType::VARIABLE:
                ss << "Variable: " << _string;
                break;
            case TokenType::KEYWORD:
                ss << "Keyword: " << _string;
                break;
            case TokenType::NUMBER:
                ss << "Number: " << _number;
                break;
            case TokenType::STRING:
                ss << "String: " << _string;
                break;
            default:
                ss << "Unknown";
        }
        return ss.str();
    }
}Token;

#endif