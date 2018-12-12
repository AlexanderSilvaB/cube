#include "Var.h"
#include <sstream>
#include <iostream>

using namespace std;

Var::Var()
{
    type = VarType::IGNORE;
    stored = false;
    ret = false;
}

Var::~Var()
{

}

void Var::Store()
{
    stored = true;
}

bool Var::Stored()
{
    return stored;
}

void Var::Return(bool ret)
{
    this->ret = ret;
}

bool Var::Returned()
{
    return ret;
}

VarType::Types Var::Type()
{
    return type;
}

bool Var::IsType(VarType::Types type)
{
    return this->type == type;
}

void Var::Error(const string& text)
{
    this->_string = text;
    this->type = VarType::ERROR;
}

Var& Var::SetType(VarType::Types type)
{
    this->type = type;
    return *this;
}

Var& Var::operator=(const Var& value)
{
    this->type = value.type;
    this->_bool = value._bool;
    this->_number = value._number;
    this->_string = value._string;
    this->_array = value._array;
    this->_dict = value._dict;
    this->_func = value._func;
    return *this;
}

Var& Var::operator=(const bool& value)
{
    this->type = VarType::BOOL;
    this->_bool = value;
    return *this;
}

Var& Var::operator=(const double& value)
{
    this->type = VarType::NUMBER;
    this->_number = value;
    return *this;
}

Var& Var::operator=(const string& value)
{
    this->type = VarType::STRING;
    this->_string = value;
    return *this;
}

Var& Var::operator=(const Array& value)
{
    this->type = VarType::ARRAY;
    this->_array = value;
    return *this;
}

Var& Var::operator=(const Dict& value)
{
    this->type = VarType::DICT;
    this->_dict = value;
    return *this;
}

Var& Var::operator=(Node value)
{
    this->type = VarType::FUNC;
    this->_func = value;
    return *this;
}

Node Var::Func()
{
    return _func;
}

string Var::String()
{
    return _string;
}

double Var::Number()
{
    return _number;
}

bool Var::Bool()
{
    return _bool;
}

Var Var::operator+(const Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            break;
        case VarType::NUMBER:
            break;
        case VarType::STRING:
            break;
    }
    return res;
}

Var Var::operator-(const Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            break;
        case VarType::NUMBER:
            break;
        case VarType::STRING:
            break;
    }
    return res;
}

Var Var::operator/(const Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            break;
        case VarType::NUMBER:
            break;
        case VarType::STRING:
            break;
    }
    return res;
}

Var Var::operator*(const Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            break;
        case VarType::NUMBER:
            break;
        case VarType::STRING:
            break;
    }
    return res;
}

std::string Var::ToString()
{
    stringstream ss;
    switch(type)
    {
        case VarType::NONE:
            ss << "none";
            break;
        case VarType::BOOL:
            ss << (_bool ? "true" : "false");
            break;
        case VarType::NUMBER:
            ss << _number;
            break;
        case VarType::STRING:
            ss << _string;
            break;
        case VarType::ARRAY:
            ss << "[";
            for(int i = 0; i < _array.size(); i++)
            {
                ss << _array[i];
                if(i < _array.size()-1)
                    ss << ", ";
            }
            ss << "]";
            break;
        case VarType::DICT:
        {
            int i = 0;
            ss << "[";
            for(Dict::iterator it = _dict.begin(); it != _dict.end(); it++, i++)
            {
                ss << it->first << " = " << it->second;
                if(i < _dict.size()-1)
                    ss << ", ";
            }
            ss << "]";
        }
            break;
        case VarType::FUNC:
        {
            if(_func->type == NodeType::LAMBDA)
                ss << "@(";
            else
                ss << "func(";
            for(int i = 0; i < _func->vars.size(); i++)
            {
                ss << _func->vars[i];
                if(i < _func->vars.size()-1)
                    ss << ", ";
            }
            ss << ")";
        }
            break;
        case VarType::ERROR:
            ss << _string;
            break;
        default:
            break;
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& stream, Var& var) 
{
    stream << var.ToString();
    return stream;
 }

std::ostream& operator<<(std::ostream& stream, Var* var) 
{
    if(var)
        stream << var->ToString();
    return stream;
 }