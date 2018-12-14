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
typedef std::vector<Var> VarArray;
typedef std::map<std::string, Var> VarDict;

class Var
{
    private:
        VarType::Types type;
        bool _bool;
        double _number;
        std::string _string;
        VarArray _array;
        VarDict _dict;
        Node _func;

        bool ret;
        bool stored;

        friend Var operator+(const bool& a, Var& b);
        friend Var operator+(const double& a, Var& b);
        friend Var operator+(const std::string& a, Var& b);
        friend Var operator+(const VarArray& a, Var& b);
        friend Var operator+(const VarDict& a, Var& b);

        friend Var operator-(const bool& a, Var& b);
        friend Var operator-(const double& a, Var& b);
        friend Var operator-(const std::string& a, Var& b);
        friend Var operator-(const VarArray& a, Var& b);
        friend Var operator-(const VarDict& a, Var& b);
    
        friend Var operator/(const bool& a, Var& b);
        friend Var operator/(const double& a, Var& b);
        friend Var operator/(const std::string& a, Var& b);
        friend Var operator/(const VarArray& a, Var& b);
        friend Var operator/(const VarDict& a, Var& b);

        friend Var operator*(const bool& a, Var& b);
        friend Var operator*(const double& a, Var& b);
        friend Var operator*(const std::string& a, Var& b);
        friend Var operator*(const VarArray& a, Var& b);
        friend Var operator*(const VarDict& a, Var& b);

        friend Var operator|(const bool& a, Var& b);
        friend Var operator|(const double& a, Var& b);
        friend Var operator|(const std::string& a, Var& b);
        friend Var operator|(const VarArray& a, Var& b);
        friend Var operator|(const VarDict& a, Var& b);

        friend Var operator&(const bool& a, Var& b);
        friend Var operator&(const double& a, Var& b);
        friend Var operator&(const std::string& a, Var& b);
        friend Var operator&(const VarArray& a, Var& b);
        friend Var operator&(const VarDict& a, Var& b);

        friend Var operator>>(const bool& a, Var& b);
        friend Var operator>>(const double& a, Var& b);
        friend Var operator>>(const std::string& a, Var& b);
        friend Var operator>>(const VarArray& a, Var& b);
        friend Var operator>>(const VarDict& a, Var& b);

        friend Var operator<<(const bool& a, Var& b);
        friend Var operator<<(const double& a, Var& b);
        friend Var operator<<(const std::string& a, Var& b);
        friend Var operator<<(const VarArray& a, Var& b);
        friend Var operator<<(const VarDict& a, Var& b);

        friend Var operator^(const bool& a, Var& b);
        friend Var operator^(const double& a, Var& b);
        friend Var operator^(const std::string& a, Var& b);
        friend Var operator^(const VarArray& a, Var& b);
        friend Var operator^(const VarDict& a, Var& b);

        friend Var operator%(const bool& a, Var& b);
        friend Var operator%(const double& a, Var& b);
        friend Var operator%(const std::string& a, Var& b);
        friend Var operator%(const VarArray& a, Var& b);
        friend Var operator%(const VarDict& a, Var& b);

        Var pow(const bool& a, Var& b);
        Var pow(const double& a, Var& b);
        Var pow(const std::string& a, Var& b);
        Var pow(const VarArray& a, Var& b);
        Var pow(const VarDict& a, Var& b);

    public:
        Var();
        ~Var();

        VarType::Types Type();
        std::string TypeName();
        bool IsType(VarType::Types type);
        Var& SetType(VarType::Types type);
        bool IsFalse();

        void Error(const std::string& text);
        void Store();
        bool Stored();
        void Return(bool ret);
        bool Returned();

        Var& operator=(const Var& value);
        Var& operator=(const bool& value);
        Var& operator=(const double& value);
        Var& operator=(const std::string& value);
        Var& operator=(const VarArray& value);
        Var& operator=(const VarDict& value);
        Var& operator=(Node value);

        Node Func();
        std::string& String();
        double& Number();
        bool& Bool();
        VarArray& Array();
        VarDict& Dict();

        bool operator==(Var& other);
        bool operator!=(Var& other);
        bool operator>(Var& other);
        bool operator<(Var& other);
        bool operator>=(Var& other);
        bool operator<=(Var& other);
        bool operator&&(Var& other);
        bool operator||(Var& other);
        bool in(Var& other);

        Var operator+(Var& other);
        Var operator-(Var& other);
        Var operator/(Var& other);
        Var operator*(Var& other);
        Var operator|(Var& other);
        Var operator&(Var& other);
        Var operator>>(Var& other);
        Var operator<<(Var& other);
        Var operator~();
        Var operator^(Var& other);
        Var operator!();
        Var operator%(Var& other);
        Var pow(Var& other);
        Var operator[](Var& other);
        Var split();

        std::string ToString();
};


//#define MKVAR() SVar(new Var())
#define MKVAR() new Var()

std::ostream& operator<< (std::ostream& stream, Var& var);
std::ostream& operator<< (std::ostream& stream, Var* var);

#endif