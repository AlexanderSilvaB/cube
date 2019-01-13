#include "Var.h"
#include "Env.h"
#include <sstream>
#include <iostream>
#include <cmath>

using namespace std;

void ReplaceString(std::string& subject, const std::string& search, const std::string& replace);
double fatorial(const double x);

Var::Var()
{
    type = VarType::IGNORE;
    stored = false;
    ret = false;
    _env = NULL;
    _handler =  NULL;
    _ref = NULL;
}

Var::~Var()
{

}

void Var::Destroy()
{
    if(_env != NULL)
        delete _env;
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

void Var::ClearRef()
{
    _ref = NULL;
}

VarType::Types Var::Type()
{
    return type;
}

string Var::TypeName()
{
    switch(type)
    {
        case VarType::IGNORE:
            return "ignore";
            break;
        case VarType::NONE:
            return "none";
            break;
        case VarType::BOOL:
            return "bool";
            break;
        case VarType::NUMBER:
            return "number";
            break;
        case VarType::STRING:
            return "string";
            break;
        case VarType::ARRAY:
            return "array";
            break;
        case VarType::DICT:
            return "dict";
            break;
        case VarType::LIB:
            return "lib";
            break;
        case VarType::FUNC:
            return "func";
            break;
        case VarType::CLASS:
            if(_counter == 0)
                return "class";
            else
                return "object";
            break;
        case VarType::ERROR:
            return "error";
            break;
        default:
            return "unknown";
            break;
    }
    return "unknown";
}

bool Var::IsType(VarType::Types type)
{
    return this->type == type;
}

bool Var::IsFalse()
{
    return (this->type == VarType::NONE || 
            this->type == VarType::IGNORE || 
            this->type == VarType::ERROR || 
            (this->type == VarType::BOOL && this->_bool == false) ||
            (this->type == VarType::NUMBER && this->_number == 0) ||
            (this->type == VarType::STRING && this->_string.size() == 0) ||
            (this->type == VarType::ARRAY && this->_array.size() == 0) ||
            (this->type == VarType::DICT && this->_dict.size() == 0));
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

Var& Var::operator=(Env* env)
{
    this->type = VarType::LIB;
    this->_dict = env->Vars();
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
    this->_env = value._env;
    this->_counter = value._counter;
    this->_handler = value._handler;
    this->_return = value._return;
    this->_names = value._names;
    if(_ref != NULL)
        _ref->operator=(value);
    else
        this->_ref = value._ref;
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

Var& Var::operator=(const VarArray& value)
{
    this->type = VarType::ARRAY;
    this->_array = value;
    return *this;
}

Var& Var::operator=(const VarDict& value)
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

Var* Var::Clone()
{
    Var *v = new Var();
    *v = *this;
    v->_counter = rand();
    v->_env = _env->copy();
}

int Var::Counter()
{
    return _counter;
}

void Var::ToClass(const string &name)
{
    this->type = VarType::CLASS;
    this->_string = name;
    this->_counter = 0;
    if(this->_env == NULL)
        this->_env = new Env();
}

void Var::ToNative(const std::string &name, void *handler)
{
    this->type = VarType::NATIVE;
    this->_string = name;
    this->_handler = handler;
}

void Var::ToFuncDef(void* handler, const std::string& name, const std::string& ret, NamesArray& vars)
{
    this->type = VarType::FUNC_DEF;
    this->_string = name;
    this->_return = ret;
    this->_names = vars;
    this->_handler = handler;
}

void Var::SetInitial()
{
    this->_counter = 0;
}

Node Var::Func()
{
    return _func;
}

string& Var::String()
{
    return _string;
}

double& Var::Number()
{
    return _number;
}

bool& Var::Bool()
{
    return _bool;
}

VarArray& Var::Array()
{
    return _array;
}

VarDict& Var::Dict()
{
    return _dict;
}

Env *Var::Context()
{
    return _env;
}

void* Var::Handler()
{
    return _handler;
}

Var Var::AsBool()
{
    Var var;
    switch(type)
    {
        case VarType::BOOL:
            var = Bool();
            break;
        case VarType::NUMBER:
            var = Number() != 0;
            break;
        case VarType::STRING:
            var = String() == "true";
            break;
        case VarType::ARRAY:
            var = Array().size() > 0;
            break;
        case VarType::DICT:
            var = Dict().size() > 0;
            break;
        default:
            var = false;
            break;
    }
    return var;
}

Var Var::AsNumber()
{
    Var var;
    switch(type)
    {
        case VarType::BOOL:
            var = Bool() ? 1.0 : 0.0;
            break;
        case VarType::NUMBER:
            var = Number();
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << String();
            double v = 0;
            ss >> v;
            var = v;
        }
            break;
        case VarType::ARRAY:
            var = (double)Array().size();
            break;
        case VarType::DICT:
            var = (double)Dict().size();
            break;
        default:
            var = 0.0;
            break;
    }
    return var;
}

Var Var::AsString()
{
    Var var;
    switch(type)
    {
        case VarType::BOOL:
        {
            string v;
            if(Bool())
                v = "true";
            else
                v = "false";
            var = v;
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << Number();
            var = ss.str();
        }
            break;
        case VarType::STRING:
        {
            var = String();
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            VarArray &A = Array();
            for(int i = 0; i < A.size(); i++)
            {
                Var a = A[i].AsString();
                ss << a.String();
            }
            var = ss.str();
        }
            break;
        case VarType::DICT:
            var = ToString();
            break;
        default:
            var = ToString();
            break;
    }
    return var;
}

Var Var::AsArray()
{
    Var var;
    var = split();
    if(var.IsType(VarType::ERROR))
    {
        VarArray _array(1);
        _array[0] = *this;
        var = _array;
    }
    return var;
}

bool Var::operator==(Var &other)
{
    if(type != other.type)
        return false;
    switch(type)
    {
        case VarType::IGNORE:
            return true;
            break;
        case VarType::NONE:
            return true;
            break;
        case VarType::BOOL:
            return _bool == other._bool;
            break;
        case VarType::NUMBER:
            return _number == other._number;
            break;
        case VarType::STRING:
            return _string == other._string;
            break;
        case VarType::ARRAY:
        {
            if(_array.size() != other._array.size())
                return false;
            for(int i = 0; i < _array.size(); i++)
            {
                if(_array[i] != other._array[i])
                    return false;
            }
            return true;
        }
            break;
        case VarType::DICT:
        {
            if(_dict.size() != other._dict.size())
                return false;
            VarDict::iterator it1 = _dict.begin();
            VarDict::iterator it2 = other._dict.begin();
            for(; it1 != _dict.end(); it1++, it2++)
            {
                if(it1->first != it2->first || it1->second != it2->second)
                    return false;
            }
            return true;
        }
            break;
        case VarType::FUNC:
            return _func.get() == other._func.get();
            break;
        case VarType::ERROR:
            return _string == other._string;
            break;
        default:
            return false;
            break;
    }
    return false;
}

bool Var::operator!=( Var& other)
{
    return !this->operator==(other);
}

bool Var::operator>(Var &other)
{
    if(type != other.type)
        return false;
    switch(type)
    {
        case VarType::IGNORE:
            return false;
            break;
        case VarType::NONE:
            return false;
            break;
        case VarType::BOOL:
            return _bool > other._bool;
            break;
        case VarType::NUMBER:
            return _number > other._number;
            break;
        case VarType::STRING:
            return _string.size() > other._string.size();
            break;
        case VarType::ARRAY:
            return _array.size() > other._array.size();
            break;
        case VarType::DICT:
            return _dict.size() > other._dict.size();
            break;
        case VarType::FUNC:
            return false;
            break;
        case VarType::ERROR:
            return false;
            break;
        default:
            return false;
            break;
    }
    return false;
}

bool Var::operator<(Var &other)
{
    if(type != other.type)
        return false;
    switch(type)
    {
        case VarType::IGNORE:
            return false;
            break;
        case VarType::NONE:
            return false;
            break;
        case VarType::BOOL:
            return _bool < other._bool;
            break;
        case VarType::NUMBER:
            return _number < other._number;
            break;
        case VarType::STRING:
            return _string.size() < other._string.size();
            break;
        case VarType::ARRAY:
            return _array.size() < other._array.size();
            break;
        case VarType::DICT:
            return _dict.size() < other._dict.size();
            break;
        case VarType::FUNC:
            return false;
            break;
        case VarType::ERROR:
            return false;
            break;
        default:
            return false;
            break;
    }
    return false;
}

bool Var::operator>=(Var &other)
{
    if(type != other.type)
        return false;
    switch(type)
    {
        case VarType::IGNORE:
            return true;
            break;
        case VarType::NONE:
            return true;
            break;
        case VarType::BOOL:
            return _bool >= other._bool;
            break;
        case VarType::NUMBER:
            return _number >= other._number;
            break;
        case VarType::STRING:
            return _string.size() >= other._string.size();
            break;
        case VarType::ARRAY:
            return _array.size() >= other._array.size();
            break;
        case VarType::DICT:
            return _dict.size() >= other._dict.size();
            break;
        case VarType::FUNC:
            return false;
            break;
        case VarType::ERROR:
            return false;
            break;
        default:
            return false;
            break;
    }
    return false;
}

bool Var::operator<=(Var &other)
{
    if(type != other.type)
        return false;
    switch(type)
    {
        case VarType::IGNORE:
            return true;
            break;
        case VarType::NONE:
            return true;
            break;
        case VarType::BOOL:
            return _bool <= other._bool;
            break;
        case VarType::NUMBER:
            return _number <= other._number;
            break;
        case VarType::STRING:
            return _string.size() <= other._string.size();
            break;
        case VarType::ARRAY:
            return _array.size() <= other._array.size();
            break;
        case VarType::DICT:
            return _dict.size() <= other._dict.size();
            break;
        case VarType::FUNC:
            return false;
            break;
        case VarType::ERROR:
            return false;
            break;
        default:
            return false;
            break;
    }
    return false;
}

bool Var::operator&&(Var &other)
{
    return !IsFalse() && !other.IsFalse();
}

bool Var::operator||(Var &other)
{
    return !IsFalse() || !other.IsFalse();
}

Var Var::operator+( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() + other;
            break;
        case VarType::NUMBER:
            res = Number() + other;
            break;
        case VarType::STRING:
            res = String() + other;
            break;
        case VarType::ARRAY:
            res = Array() + other;
            break;
        case VarType::DICT:
            res = Dict() + other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator+(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a + b.Bool());
            break;
        case VarType::NUMBER:
            res = a + b.Number();
            break;
        case VarType::STRING:
            res = (a ? "true" : "false") + b.String();
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator+(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = a + b.Bool();
            break;
        case VarType::NUMBER:
            res = a + b.Number();
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << a;
            res = ss.str() + b.String();
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator+(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = a + (b.Bool() ? "true" : "false");
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << b.Number();
            res = a + ss.str();
        }
            break;
        case VarType::STRING:
            res = a + b.String();
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator+(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            Var B;
            B = b.Bool();
            VarArray _array;
            _array.reserve(a.size() + 1);
            _array.insert( _array.end(), a.begin(), a.end() );
            _array.insert( _array.end(), B );
            res = _array;
        }
            break;
        case VarType::NUMBER:
       {
            Var B;
            B = b.Number();
            VarArray _array;
            _array.reserve(a.size() + 1);
            _array.insert( _array.end(), a.begin(), a.end() );
            _array.insert( _array.end(), B );
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            Var B;
            B = b.String();
            VarArray _array;
            _array.reserve(a.size() + 1);
            _array.insert( _array.end(), a.begin(), a.end() );
            _array.insert( _array.end(), B );
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array;
            _array.reserve(a.size() + B.size());
            _array.insert( _array.end(), a.begin(), a.end() );
            _array.insert( _array.end(), B.begin(), B.end() );
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator+(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            VarDict& B = b.Dict();
            VarDict _dict = (VarDict)a;
            _dict.insert( B.begin(), B.end() );
            res = _dict;
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '+' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator-( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() - other;
            break;
        case VarType::NUMBER:
            res = Number() - other;
            break;
        case VarType::STRING:
            res = String() - other;
            break;
        case VarType::ARRAY:
            res = Array() - other;
            break;
        case VarType::DICT:
            res = Dict() - other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator-(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a - b.Bool());
            break;
        case VarType::NUMBER:
            res = a - b.Number();
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator-(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = a - b.Bool();
            break;
        case VarType::NUMBER:
            res = a - b.Number();
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator-(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            string A = a;
            ReplaceString(A, (b.Bool() ? "true" : "false"), "");
            res = A;
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << b.Number();
            string A = a;
            ReplaceString(A, ss.str(), "");
            res = A;
        }
            break;
        case VarType::STRING:
        {
            string A = a;
            ReplaceString(A, b.String(), "");
            res = A;
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator-(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array = a;
            for(VarArray::iterator it = _array.begin(); it != _array.end();)
            {
                if(it->IsType(VarType::BOOL) && it->Bool() == b.Bool())
                    it = _array.erase(it);
                else
                    it++;
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array = a;
            for(VarArray::iterator it = _array.begin(); it != _array.end();)
            {
                if(it->IsType(VarType::NUMBER) && it->Number() == b.Number())
                    it = _array.erase(it);
                else
                    it++;
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array = a;
            for(VarArray::iterator it = _array.begin(); it != _array.end();)
            {
                if(it->IsType(VarType::STRING) && it->String() == b.String())
                    it = _array.erase(it);
                else
                    it++;
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray _array = a;
            VarArray& B = b.Array();
            for(VarArray::iterator it = _array.begin(); it != _array.end();)
            {
                bool removed = false;
                for(int i = 0; i < B.size(); i++)
                {
                    if(it->operator==(B[i]))
                    {
                        removed = true;
                        break;
                    }    
                }
                if(removed)
                    it = _array.erase(it);
                else
                    it++;
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator-(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            VarDict& B = b.Dict();
            VarDict _dict = (VarDict)a;
            for(VarDict::iterator it1 = _dict.begin(); it1 != _dict.end();)
            {
                bool removed = false;
                for(VarDict::iterator it2 = B.begin(); it2 != B.end();it2++)
                {
                    if(it1->first == it2->first && it1->second == it2->second)
                    {
                        removed = true;
                        break;
                    }
                }
                if(removed)
                    it1 = _dict.erase(it1);
                else
                    it1++;
            }
            res = _dict;
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator/( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() / other;
            break;
        case VarType::NUMBER:
            res = Number() / other;
            break;
        case VarType::STRING:
            res = String() / other;
            break;
        case VarType::ARRAY:
            res = Array() / other;
            break;
        case VarType::DICT:
            res = Dict() / other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator/(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a / b.Bool());
            break;
        case VarType::NUMBER:
            res = a / b.Number();
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator/(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            if(b.Number() == 0)
            {
                res.Error("Division by zero");
            }
            else
                res = a / b.Number();
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator/(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            int sz = a.size() / b.Number();
            res = a.substr(0, sz);
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator/(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] / b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(a.size() != B.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '/' to arrays with diferent sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] / B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator/(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '/' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '-' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator*( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() * other;
            break;
        case VarType::NUMBER:
            res = Number() * other;
            break;
        case VarType::STRING:
            res = String() * other;
            break;
        case VarType::ARRAY:
            res = Array() * other;
            break;
        case VarType::DICT:
            res = Dict() * other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator*(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a * b.Bool());
            break;
        case VarType::NUMBER:
            res = a * b.Number();
            break;
        case VarType::STRING:
            res = (a ? b.String() : "");
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a * B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator*(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = a * b.Bool();
            break;
        case VarType::NUMBER:
            res = a * b.Number();
            break;
        case VarType::STRING:
        {
            stringstream ss;
            for(int i = 0; i < a; i++)
                ss << b.String();
            res = ss.str();
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a * B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator*(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (b.Bool() ? a : "");
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            for(int i = 0; i < b.Number(); i++)
                ss << a;
            res = ss.str();
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a * B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator*(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] * b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] * b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] * b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(B.size() != a.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '*' to arrays with different sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] * B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator*(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '*' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator|( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() | other;
            break;
        case VarType::NUMBER:
            res = Number() | other;
            break;
        case VarType::STRING:
            res = String() | other;
            break;
        case VarType::ARRAY:
            res = Array() | other;
            break;
        case VarType::DICT:
            res = Dict() | other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator|(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a | b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a | (int)(b.Number()));
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a | B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator|(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (double)((int)a | b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a | (int)b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a | B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator|(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator|(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] | b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] | b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] | b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(B.size() != a.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '|' to arrays with different sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] | B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator|(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '|' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator&( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() & other;
            break;
        case VarType::NUMBER:
            res = Number() & other;
            break;
        case VarType::STRING:
            res = String() & other;
            break;
        case VarType::ARRAY:
            res = Array() & other;
            break;
        case VarType::DICT:
            res = Dict() & other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator&(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a & b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a & (int)(b.Number()));
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a & B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator&(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (double)((int)a & b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a & (int)b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a & B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator&(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator&(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] & b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] & b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] & b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(B.size() != a.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '&' to arrays with different sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] & B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator&(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '&' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator>>( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() >> other;
            break;
        case VarType::NUMBER:
            res = Number() >> other;
            break;
        case VarType::STRING:
            res = String() >> other;
            break;
        case VarType::ARRAY:
            res = Array() >> other;
            break;
        case VarType::DICT:
            res = Dict() >> other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator>>(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a >> b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a >> (int)(b.Number()));
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a >> B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator>>(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (double)((int)a >> b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a >> (int)b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a >> B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator>>(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator>>(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] >> b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] >> b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] >> b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(B.size() != a.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '>>' to arrays with different sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] >> B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator>>(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '>>' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator<<( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() << other;
            break;
        case VarType::NUMBER:
            res = Number() << other;
            break;
        case VarType::STRING:
            res = String() << other;
            break;
        case VarType::ARRAY:
            res = Array() << other;
            break;
        case VarType::DICT:
            res = Dict() << other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator<<(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a << b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a << (int)(b.Number()));
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a << B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator<<(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (double)((int)a << b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a << (int)b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a << B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator<<(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator<<(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] << b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] << b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] << b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(B.size() != a.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '<<' to arrays with different sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] << B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator<<(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '<<' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator~()
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = (bool)(~Bool());
            break;
        case VarType::NUMBER:
            res = (double)(~(int)Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '~' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = ~B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '~' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '~' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator^( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() ^ other;
            break;
        case VarType::NUMBER:
            res = Number() ^ other;
            break;
        case VarType::STRING:
            res = String() ^ other;
            break;
        case VarType::ARRAY:
            res = Array() ^ other;
            break;
        case VarType::DICT:
            res = Dict() ^ other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator^(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a ^ b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a ^ (int)(b.Number()));
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a ^ B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator^(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (double)((int)a ^ b.Bool());
            break;
        case VarType::NUMBER:
            res = (double)((int)a ^ (int)b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a ^ B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator^(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator^(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] ^ b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] ^ b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] ^ b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(B.size() != a.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '^' to arrays with different sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] ^ B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator^(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '^' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator!()
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = (double)1;
            break;
        case VarType::NUMBER:
            res = fatorial(Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '!' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = !B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '!' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '!' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator%( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = Bool() % other;
            break;
        case VarType::NUMBER:
            res = Number() % other;
            break;
        case VarType::STRING:
            res = String() % other;
            break;
        case VarType::ARRAY:
            res = Array() % other;
            break;
        case VarType::DICT:
            res = Dict() % other;
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator%(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = (bool)(a % b.Bool());
            break;
        case VarType::NUMBER:
            res = fmod(a, b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a % B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator%(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = fmod(a, b.Bool());
            break;
        case VarType::NUMBER:
            res = fmod(a, b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a % B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator%(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = a % B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator%(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] % b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] % b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] % b;
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(B.size() != a.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '%' to arrays with different sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = (Var)a[i] % B[i];
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var operator%(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '%' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::pow( Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
            res = pow(Bool(), other);
            break;
        case VarType::NUMBER:
            res = pow(Number(), other);
            break;
        case VarType::STRING:
            res = pow(String(), other);
            break;
        case VarType::ARRAY:
            res = (Array(), other);
            break;
        case VarType::DICT:
            res = (Dict(), other);
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::pow(const bool& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = std::pow(a ? 1.0:0.0, b.Bool() ? 1.0:0.0);
            break;
        case VarType::NUMBER:
            res = std::pow(a ? 1.0:0.0, b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = pow(a, B[i]);
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'bool' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::pow(const double& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
            res = std::pow(a, b.Bool() ? 1.0 : 0.0);
            break;
        case VarType::NUMBER:
            res = std::pow(a, b.Number());
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = pow(a, B[i]);
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'number' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::pow(const string& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            VarArray _array(B.size());
            for(int i = 0; i < B.size(); i++)
            {
                _array[i] = pow(a, B[i]);
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'string' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::pow(const VarArray& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = ((Var)a[i]).pow(b);
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::NUMBER:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = ((Var)a[i]).pow(b);
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = ((Var)a[i]).pow(b);
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            VarArray& B = b.Array();
            if(B.size() != a.size())
            {
                stringstream ss;
                ss << "Cannot apply the operator '**' to arrays with different sizes";
                res.Error(ss.str());
                return res;
            }
            VarArray _array(a.size());
            for(int i = 0; i < a.size(); i++)
            {
                _array[i] = ((Var)a[i]).pow(B[i]);
                if(_array[i].IsType(VarType::ERROR))
                    return _array[i];
            }
            res = _array;
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'array' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::pow(const VarDict& a, Var& b) 
{
    Var res;
    switch(b.Type())
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
       {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::ARRAY:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::DICT:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '**' to 'dict' and '" << b.TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::operator[](Var& other)
{
    Var res;
    switch(type)
    {
        case VarType::BOOL:
        {
            stringstream ss;
            ss << "Cannot apply the operator '[]' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::NUMBER:
        {
            stringstream ss;
            ss << "Cannot apply the operator '[]' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
        case VarType::STRING:
        {
            VarArray index;
            bool valid = true;
            if(!other.IsType(VarType::BOOL) && !other.IsType(VarType::NUMBER) && !other.IsType(VarType::ARRAY))
            {
                stringstream ss;
                ss << "Cannot apply the operator '[]' to 'string' and '" << other.TypeName() << "'";
                res.Error(ss.str());
                valid = false;
            }
            else
            {
                if(other.IsType(VarType::BOOL))
                {
                    Var in;
                    in = other.Bool() ? 1.0 : 0.0;
                    index.push_back(in);
                }
                else if(other.IsType(VarType::NUMBER))
                    index.push_back(other);
                else
                {
                    VarArray &indexes = other.Array();
                    for(int i = 0; i < indexes.size(); i++)
                    {
                        if(!indexes[i].IsType(VarType::BOOL) && !indexes[i].IsType(VarType::NUMBER))
                        {
                            stringstream ss;
                            ss << "Cannot apply the operator '[]' to 'string' and '" << indexes[i].TypeName() << "'";
                            res.Error(ss.str());
                            valid = false;
                            break;
                        }
                        else
                        {
                            if(indexes[i].IsType(VarType::BOOL))
                            {
                                Var in;
                                in = indexes[i].Bool() ? 1.0 : 0.0;
                                index.push_back(in);
                            }
                            else if(indexes[i].IsType(VarType::NUMBER))
                                index.push_back(indexes[i]);
                        }
                    }
                }
            }
            if(valid)
            {
                if(index.size() == 0)
                    res = "";
                else
                {
                    stringstream data;
                    string &str = String();
                    valid = true;
                    for(int i = 0; i < index.size(); i++)
                    {
                        int in = index[i].Number();
                        if(in >= 0)
                        {
                            if(in < str.size())
                            {
                                data << str[in];
                            }
                            else
                            {
                                stringstream ss;
                                ss << "Index '" << in << "' out of bounds";
                                res.Error(ss.str());
                                valid = false;
                                break;
                            }
                        }
                        else
                        {
                            int in_ = in + str.size();
                            if(in_ >= 0 && in_ < str.size())
                            {
                                data << str[in_];
                            }
                            else
                            {
                                stringstream ss;
                                ss << "Index '" << in << "' out of bounds";
                                res.Error(ss.str());
                                valid = false;
                                break;
                            }
                        }
                    }
                    if(valid)
                    {
                        res = data.str();
                    }
                }
            }
        }
            break;
        case VarType::ARRAY:
        {
            VarArray index;
            bool valid = true;
            if(!other.IsType(VarType::BOOL) && !other.IsType(VarType::NUMBER) && !other.IsType(VarType::ARRAY))
            {
                stringstream ss;
                ss << "Cannot apply the operator '[]' to 'string' and '" << other.TypeName() << "'";
                res.Error(ss.str());
                valid = false;
            }
            else
            {
                if(other.IsType(VarType::BOOL))
                {
                    Var in;
                    in = other.Bool() ? 1.0 : 0.0;
                    index.push_back(in);
                }
                else if(other.IsType(VarType::NUMBER))
                    index.push_back(other);
                else
                {
                    VarArray &indexes = other.Array();
                    for(int i = 0; i < indexes.size(); i++)
                    {
                        if(!indexes[i].IsType(VarType::BOOL) && !indexes[i].IsType(VarType::NUMBER))
                        {
                            stringstream ss;
                            ss << "Cannot apply the operator '[]' to 'string' and '" << indexes[i].TypeName() << "'";
                            res.Error(ss.str());
                            valid = false;
                            break;
                        }
                        else
                        {
                            if(indexes[i].IsType(VarType::BOOL))
                            {
                                Var in;
                                in = indexes[i].Bool() ? 1.0 : 0.0;
                                index.push_back(in);
                            }
                            else if(indexes[i].IsType(VarType::NUMBER))
                                index.push_back(indexes[i]);
                        }
                    }
                }
            }
            if(valid)
            {
                if(index.size() == 0)
                    res = index;
                else
                {
                    VarArray data;
                    VarArray &value = Array();
                    valid = true;
                    for(int i = 0; i < index.size(); i++)
                    {
                        int in = index[i].Number();
                        if(in >= 0)
                        {
                            if(in < value.size())
                            {
                                data.push_back(value[in]);
                                data[data.size()-1]._ref = &value[in];
                            }
                            else
                            {
                                stringstream ss;
                                ss << "Index '" << in << "' out of bounds";
                                res.Error(ss.str());
                                valid = false;
                                break;
                            }
                        }
                        else
                        {
                            int in_ = in + value.size();
                            if(in_ >= 0 && in_ < value.size())
                            {
                                data.push_back(value[in_]);
                                data[data.size()-1]._ref = &value[in_];
                            }
                            else
                            {
                                stringstream ss;
                                ss << "Index '" << in << "' out of bounds";
                                res.Error(ss.str());
                                valid = false;
                                break;
                            }
                        }
                    }
                    if(valid)
                    {
                        if(data.size() == 1)
                            res = data[0];
                        else
                            res = data;
                    }
                }
            }
        }
            break;
        case VarType::DICT:
        {
            vector<string> index;
            bool valid = true;
            if(!other.IsType(VarType::STRING) && !other.IsType(VarType::ARRAY))
            {
                stringstream ss;
                ss << "Cannot apply the operator '[]' to 'dict' and '" << other.TypeName() << "'";
                res.Error(ss.str());
                valid = false;
            }
            else
            {
                if(other.IsType(VarType::STRING))
                {
                    index.push_back(other.String());
                }
                else
                {
                    VarArray &indexes = other.Array();
                    for(int i = 0; i < indexes.size(); i++)
                    {
                        if(!indexes[i].IsType(VarType::STRING))
                        {
                            stringstream ss;
                            ss << "Cannot apply the operator '[]' to 'dict' and '" << indexes[i].TypeName() << "'";
                            res.Error(ss.str());
                            valid = false;
                            break;
                        }
                        else
                        {
                            index.push_back(indexes[i].String());
                        }
                    }
                }
            }
            if(valid)
            {
                if(index.size() == 0)
                    res.SetType(VarType::NONE);
                else
                {
                    VarDict data;
                    VarDict &value = Dict();
                    valid = true;
                    for(int i = 0; i < index.size(); i++)
                    {
                        string &name = index[i];
                        VarDict::iterator it = value.find(name);
                        if(it != value.end())
                        {
                            data[it->first] = it->second;
                        }
                        else
                        {
                            stringstream ss;
                            ss << "Index '" << name << "' does not exists";
                            res.Error(ss.str());
                            valid = false;
                            break;
                        }
                    }
                    if(valid)
                    {
                        if(data.size() == 1)
                            res = data.begin()->second;
                        else
                            res = data;
                    }
                }
            }
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator '[]' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

Var Var::split()
{
    Var res;
    switch(Type())
    {
        case VarType::BOOL:
        {
            VarArray _array(1);
            _array[0] = Bool();
            res = _array;
        }
            break;
        case VarType::NUMBER:
       {
            VarArray _array(1);
            _array[0] = Number();
            res = _array;
        }
            break;
        case VarType::STRING:
        {
            string &str = String();
            VarArray _array(str.size());
            for(int i = 0; i < str.size(); i++)
            {
                string s = "";
                s = s + str[i];
                _array[i] = s;
            }
            res = _array;
        }
            break;
        case VarType::ARRAY:
        {
            return *this;
        }
            break;
        case VarType::DICT:
        {
            VarArray _array(1);
            _array[0] = Dict();
            res = _array;
        }
            break;
        case VarType::ERROR:
        {
            VarArray _array(1);
            _array[0].Error(String());
            res = _array;
        }
            break;
        case VarType::NONE:
        {
            VarArray _array(1);
            _array[0].SetType(VarType::NONE);
            res = _array;
        }
            break;
        case VarType::FUNC:
        {
            VarArray _array(1);
            _array[0] = Func();
            res = _array;
        }
            break;
        default:
        {
            stringstream ss;
            ss << "Cannot apply the operator 'in' to '" << TypeName() << "'";
            res.Error(ss.str());
        }
            break;
    }
    return res;
}

int Var::Size()
{
    switch(Type())
    {
        case VarType::BOOL:
            return 1;
            break;
        case VarType::NUMBER:
            return 1;
            break;
        case VarType::STRING:
            return String().size();
            break;
        case VarType::ARRAY:
            return Array().size();
            break;
        case VarType::DICT:
            return Dict().size();
            break;
        case VarType::ERROR:
            return 1;
            break;
        default:
            return 1;
            break;
    }
    return 1;
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
            for(VarDict::iterator it = _dict.begin(); it != _dict.end(); it++, i++)
            {
                ss << it->first << " = " << it->second;
                if(i < _dict.size()-1)
                    ss << ", ";
            }
            ss << "]";
        }
            break;
        case VarType::LIB:
        {
            int i = 0;
            ss << "{";
            for(VarDict::iterator it = _dict.begin(); it != _dict.end(); it++, i++)
            {
                ss << it->first << " = " << it->second;
                if(i < _dict.size()-1)
                    ss << ", ";
            }
            ss << "}";
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
        case VarType::FUNC_DEF:
        {
            ss << "func " << _string << "( ";
            for(int i = 0; i < _names.size(); i++)
            {
                ss << _names[i];
                if(i < _names.size()-1)
                    ss << ", ";
            }
            ss << " ) : " << _return;
        }
            break;
        case VarType::CLASS:
        {
            if(_counter == 0)
                ss << "class ";
            else
                ss << "object ";
            //ss << _string << " ( " << _counter << " )";
            ss << _string;
        }
            break;
        case VarType::NATIVE:
            ss << "native " << _string << endl;
            for(int i = 0; i < _array.size(); i++)
            {
                ss << "  " << _array[i];
                if(i < _array.size()-1)
                    ss << endl;
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

 void ReplaceString(std::string& subject, const std::string& search,
                          const std::string& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

double fatorial(const double x) 
{
    if( x <= 2 )
        return 1.0*x;
    return fatorial(x-1) * x;
}