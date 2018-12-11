#include "Var.h"
#include <sstream>

using namespace std;

Var::Var()
{
    type = VarType::NONE;
}

Var::~Var()
{

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
    return *this;
}

Var& Var::operator=(SVar& value)
{
    this->type = value->type;
    this->_bool = value->_bool;
    this->_number = value->_number;
    this->_string = value->_string;
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

std::ostream& operator<<(std::ostream& stream, SVar& var) 
{
    if(!var.empty())
        stream << var->ToString();
    return stream;
 }