#ifndef _VAR_H_
#define _VAR_H_

#include "Node.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace VarType
{
    enum Types {IGNORE, ERROR, BOOL, NUMBER, STRING, ARRAY, DICT, LIB, FUNC, CLASS, NATIVE, FUNC_DEF, NONE};
}

class Var;
typedef std::vector<std::string> NamesArray;
typedef std::vector<Var> VarArray;
typedef std::map<std::string, Var> VarDict;

class Env;

class Var
{
    private:
        VarType::Types type;
        bool _bool;
        double _number;
        std::string _string, _return;
        VarArray _array;
        VarDict _dict;
        NamesArray _names;
        Node _func;
        int _counter;
        std::shared_ptr<Env> _env;
        void* _handler;
        Var *_ref;

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
        void Destroy();

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
        void ClearRef();
        std::string ReturnType();

        Var& operator=(std::shared_ptr<Env> env);
        Var& operator=(const Var& value);
        Var& operator=(const bool& value);
        Var& operator=(const double& value);
        Var& operator=(const std::string& value);
        Var& operator=(const VarArray& value);
        Var& operator=(const VarDict& value);
        Var& operator=(Node value);

        int Counter();
        void ToClass(const std::string &name);
        void ToNative(const std::string &name, void *handler);
        void ToFuncDef(void* handler, const std::string& name, const std::string& ret, NamesArray& vars);
        void SetInitial();

        Var* Clone();

        Node Func();
        std::string& String();
        double& Number();
        bool& Bool();
        VarArray& Array();
        VarDict& Dict();
        std::shared_ptr<Env> Context();
        void* Handler();
        NamesArray& Names();

        Var AsBool();
        Var AsNumber();
        Var AsString();
        Var AsArray();

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

        int Size();

        std::string ToString();
};


//#define MKVAR() SVar(new Var())
#define MKVAR() new Var()

std::ostream& operator<< (std::ostream& stream, Var& var);
std::ostream& operator<< (std::ostream& stream, Var* var);

#endif