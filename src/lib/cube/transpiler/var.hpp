#ifndef _CUBE_VAR_HPP_
#define _CUBE_VAR_HPP_

#include <map>
#include <string>
#include <vector>

using namespace std;

enum VarType
{
    NUL,
    BOOL,
    NUMBER,
    STRING,
    LIST,
    DICT
};

class Var
{
  public:
    enum VarType type;
    bool bo;
    double num;
    string str;
    vector<Var> list;
    map<Var, Var> dict;

    Var()
    {
        type = NUL;
    }

    Var(bool bo)
    {
        this->bo = bo;
    }

    Var(double num)
    {
        this->num = num;
    }

    Var(string str)
    {
        this->str = str;
    }

    virtual ~Var()
    {
    }
};

#endif
