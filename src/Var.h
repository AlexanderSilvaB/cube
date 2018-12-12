#ifndef _VAR_H_
#define _VAR_H_

#include "Node.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace VarType
{
    enum Types {IGNORE, ERROR, BOOL, NUMBER, STRING, ARRAY, DICT, FUNC, NONE};
}

class Var;
typedef std::vector<Var> Array;
typedef std::map<std::string, Var> Dict;

class Var
{
    private:
        VarType::Types type;
        bool _bool;
        double _number;
        std::string _string;
        Array _array;
        Dict _dict;
        Node _func;

        bool ret;
        bool stored;
    
    public:
        Var();
        ~Var();

        VarType::Types Type();
        bool IsType(VarType::Types type);
        Var& SetType(VarType::Types type);

        void Error(const std::string& text);
        void Store();
        bool Stored();
        void Return(bool ret);
        bool Returned();

        Var& operator=(const Var& value);
        Var& operator=(const bool& value);
        Var& operator=(const double& value);
        Var& operator=(const std::string& value);
        Var& operator=(const Array& value);
        Var& operator=(const Dict& value);
        Var& operator=(Node value);

        Node Func();
        std::string String();
        double Number();
        bool Bool();

        Var operator+(const Var& other);
        Var operator-(const Var& other);
        Var operator/(const Var& other);
        Var operator*(const Var& other);

        std::string ToString();
};


//#define MKVAR() SVar(new Var())
#define MKVAR() new Var()

std::ostream& operator<< (std::ostream& stream, Var& var);
std::ostream& operator<< (std::ostream& stream, Var* var);

#endif