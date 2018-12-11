#ifndef _VAR_H_
#define _VAR_H_

#include <string>
#include "SP.h"

namespace VarType
{
    enum Types {ERROR, BOOL, NUMBER, STRING, NONE};
}

class Var;
typedef SP<Var> SVar;

class Var
{
    private:
        VarType::Types type;
        bool _bool;
        double _number;
        std::string _string;
    
    public:
        Var();
        ~Var();

        VarType::Types Type();
        bool IsType(VarType::Types type);
        Var& SetType(VarType::Types type);

        void Error(const std::string& text);

        Var& operator=(const Var& value);
        Var& operator=(SVar& value);
        Var& operator=(const bool& value);
        Var& operator=(const double& value);
        Var& operator=(const std::string& value);

        std::string ToString();
};


#define MKVAR() SVar(new Var())

std::ostream& operator<< (std::ostream& stream, Var& var);
std::ostream& operator<< (std::ostream& stream, SVar& var);

#endif