#include "Interpreter.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <time.h> 

using namespace std;

Interpreter::Interpreter()
{
    env = new Env();
    exitCode = 0;
    needBreak = false;
    exit = false;

    colors["black"] = 30;
    colors["red"] = 31;
    colors["green"] = 32;
    colors["yellow"] = 33;
    colors["blue"] = 34;
    colors["magenta"] = 35;
    colors["cyan"] = 36;
    colors["white"] = 37;
}

Interpreter::~Interpreter()
{

}

int Interpreter::ExitCode()
{
    return exitCode;
}

bool Interpreter::NeedBreak()
{
    return needBreak;
}

bool Interpreter::Evaluate(const string& src, bool interactive)
{
    exitCode = 0;
    needBreak = false;
    Node root = parser.Parse(src);
    if(root->type == NodeType::ERROR)
    {
        cout << root->ToString() << endl;
        exitCode = -1;
        exit = !interactive;
    }
    else
    {
        Var* var = Evaluate(root, env);
        if(!var)
        {
            cout << "Error: Undefined error" << endl;
            exitCode = -1;
            exit = !interactive;
        }
        else if(var->IsType(VarType::ERROR))
        {
            cout << "Error: " << var << endl;
            exitCode = -1;
            exit = !interactive;
        }
        else
        {
            env->def("ans", var);
        }
        if(var && !var->Stored())
            delete var;
    }

    return !exit;
}

Var* Interpreter::Evaluate(Node node, Env* env)
{
    Var* var = NULL;
    if(!node)
    {
        var = MKVAR();
        return var;
    }
    switch(node->type)
    {
        case NodeType::IGNORE:
            var = MKVAR();
            break;
        case NodeType::NONE:
            var = MKVAR();
            var->SetType(VarType::NONE);
            break;
        case NodeType::BOOL:
            var = MKVAR();
            *var = node->_bool;
            break;
        case NodeType::NUMBER:
            var = MKVAR();
            *var = node->_number;
            break;
        case NodeType::STRING:
            var = MKVAR();
            *var = node->_string;
            break;
        case NodeType::ARRAY:
        {
            var = MKVAR();
            VarArray _array(node->nodes.size());
            bool error = false;
            for(int i = 0; i < node->nodes.size(); i++)
            {
                Var* v = Evaluate(node->nodes[i], env);
                if(!v)
                {
                    MakeError(var, "Invalid array value", node->nodes[i]);
                    error = true;
                    break;
                }
                if(v->IsType(VarType::ERROR))
                {
                    var = v;
                    error = true;
                }
                _array[i] = *v;
                if(!v->Stored())
                    delete v;
            }
            if(!error)
            {
                *var = _array;
            }
        }
            break;
        case NodeType::DICT:
        {
            var = MKVAR();
            VarDict _dict;
            bool error = false;
            for(int i = 0; i < node->nodes.size(); i++)
            {
                Var* v = Evaluate(node->nodes[i]->right, env);
                if(!v)
                {
                    MakeError(var, "Invalid dict value", node->nodes[i]->right);
                    error = true;
                    break;
                }
                if(v->IsType(VarType::ERROR))
                {
                    var = v;
                    error = true;
                }
                _dict[node->nodes[i]->left->_string] = *v;
                if(!v->Stored())
                    delete v;
            }
            if(!error)
            {
                *var = _dict;
            }
        }
            break;
        case NodeType::FUNCTION:
            var = MKVAR();
            *var = node;
            var = env->set(node->_string, var);
            break;
        case NodeType::LAMBDA:
            var = MKVAR();
            *var = node;
            break;
        case NodeType::VARIABLE:
            var = env->get(node->_string);
            break;
        case NodeType::RETURN:
            var = Evaluate(node->body, env);
            if(var)
            {
                if(!var->IsType(VarType::ERROR))
                    var->Return(true);
            }
            break;
        case NodeType::ASSIGN:
        {
            if(node->left->type == NodeType::VARIABLE)
            {
                Var* value = Evaluate(node->right, env);
                if(!value->IsType(VarType::ERROR))
                    var = env->set(node->left->_string, value);
                else  
                    var = value;
            }
            else if(node->left->type == NodeType::INDEX)
            {
                MakeError(var, "Index not implemented ", node->left);
            }
            else
            {
                MakeError(var, "Cannot assign to ", node->left);
            }
        }
            break;
        case NodeType::BINARY:
        {
            if(node->_string == "&&")
            {
                Var *left = Evaluate(node->left, env);
                if(left->IsFalse())
                {
                    if(!left->Stored())
                        delete left;
                    var = MKVAR();
                    *var = false;
                }
                else
                {
                    var = ApplyOperator(node->_string, left, Evaluate(node->middle, env), Evaluate(node->right, env), env);    
                }
            }
            else if(node->_string == ".")
            {
                Var *left = Evaluate(node->left, env);
                if(!left->IsType(VarType::DICT))
                {
                    var = MKVAR();
                    stringstream ss;
                    ss << "Cannot apply the operator '.' to '" << left->TypeName() << "'";
                    MakeError(var, ss.str(), node);
                    if(!left->Stored())
                        delete left;
                }
                else
                {
                    VarDict &dict = left->Dict();
                    if(node->right->type != NodeType::VARIABLE)
                    {
                        var = MKVAR();
                        stringstream ss;
                        ss << "Could not apply the operator '.' to '" << node->right->ToString() << "'";
                        MakeError(var, ss.str(), node);
                    }
                    else
                    {
                        for(VarDict::iterator it = dict.begin(); it != dict.end(); it++)
                        {
                            if(it->first == node->right->_string)
                            {
                                var = MKVAR();
                                *var = it->second;
                            }
                        }
                        if(var == NULL)
                        {
                            var = MKVAR();
                            stringstream ss;
                            ss << "Member '" << node->right->_string << "' not found";
                            MakeError(var, ss.str(), node);
                        }
                    }
                }
            }
            else
                var = ApplyOperator(node->_string, Evaluate(node->left, env), Evaluate(node->middle, env), Evaluate(node->right, env), env);
        }
            break;
        case NodeType::INDEX:
        {
            var = MKVAR();
            Var* left = Evaluate(node->left, env);
            VarArray index;
            for(int i = 0; i < node->nodes.size(); i++)
            {
                Var* v = Evaluate(node->nodes[i], env);
                if(v->IsType(VarType::ARRAY))
                {
                    VarArray &A = v->Array();
                    for(int j = 0; j < A.size(); j++)
                        index.push_back(A[j]);
                }
                else
                    index.push_back(*v);
                if(!v->Stored())
                    delete v;
            }
            Var in;
            in = index;
            *var = left->operator[](in);
            if(!left->Stored())
                delete left;
        }
            break;
        case NodeType::CALL:
        {
            bool process = true;
            if(node->func->type = NodeType::VARIABLE)
            {
                vector<Var*> args(node->nodes.size());
                for(int i = 0; i < node->nodes.size(); i++)
                {
                    if(node->func->_string == "color" && IsColor(node->nodes[i]->_string))
                    {
                        args[i] = MKVAR();
                        *args[i] = node->nodes[i]->_string;
                    }
                    else
                    {
                        args[i] = Evaluate(node->nodes[i], env);
                        if(args[i]->IsType(VarType::ERROR))
                        {
                            return args[i];
                        }
                    }
                }
                var = Call(node->func->_string, args, env);
            }
            else
            {
                var = MKVAR();
                MakeError(var, "Cannot call this like a function", node->func);
            }
        }
            break;
        case NodeType::FOR:
        {
            var = MKVAR();
            if(node->nodes.size() == 0 || node->nodes.size() > 3)
            {
                MakeError(var, "Invalid for arguments for 'for' loop", node);
            }
            else
            {
                Node in;
                Node start;
                Node cond;
                Node inc;
                if(node->nodes.size() == 1 && node->nodes[0]->type == NodeType::BINARY && node->nodes[0]->_string == "in")
                {
                    in = node->nodes[0];
                }
                else
                {
                    start = node->nodes[0];
                    if(node->nodes.size() > 1)
                        cond = node->nodes[1];
                    if(node->nodes.size() > 2)
                        inc = node->nodes[2];
                }
                Env *forEnv = env->extend();
                bool valid = true;
                Var *inVar = NULL;
                int inIndex = 0;
                Var parts;
                if(in)
                {
                    if(in->left->type != NodeType::VARIABLE)
                    {
                        MakeError(var, "Invalid type for 'in' operator");
                        valid = false;
                    }
                    else
                    {
                        Var *right = Evaluate(in->right, forEnv);
                        parts = right->split();
                        if(parts.IsType(VarType::ERROR))
                        {
                            *var = parts;
                            valid = false;
                        }
                        else
                        {
                            if(parts.Array().size() > 0)
                            {
                                inVar = &parts.Array()[inIndex];
                                forEnv->set(in->left->_string, inVar);
                                inIndex++;
                            }
                            else
                            {
                                var->SetType(VarType::IGNORE);
                                valid = false;
                            }
                        }
                    }
                }
                else
                {
                    Var *st = Evaluate(start, forEnv);
                    if(st->IsType(VarType::ERROR))
                    {
                        *var = *st;
                        valid = false;
                    }
                    else
                    {
                        if(cond)
                        {
                            Var *cd = Evaluate(cond, forEnv);
                            if(cd->IsType(VarType::ERROR))
                            {
                                *var = *cd;
                                valid = false;
                            }
                            else if(cd->IsFalse())
                            {
                                var->SetType(VarType::IGNORE);
                                valid = false;
                            }
                        }
                    }
                }
                if(valid)
                {
                    do
                    {
                        var = Evaluate(node->body, forEnv);
                        if(var->IsType(VarType::ERROR))
                        {
                            break;
                        }
                        if(in)
                        {
                            if(inIndex < parts.Array().size())
                            {
                                inVar = &parts.Array()[inIndex];
                                forEnv->set(in->left->_string, inVar);
                                inIndex++;
                            }
                            else
                            {
                                valid = false;
                            }
                        }
                        else
                        {
                            if(inc)
                            {
                                Var *ic = Evaluate(inc, forEnv);
                                if(ic->IsType(VarType::ERROR))
                                {
                                    *var = *ic;
                                    valid = false;
                                }
                            }

                            if(cond)
                            {
                                Var *cd = Evaluate(cond, forEnv);
                                if(cd->IsType(VarType::ERROR))
                                {
                                    *var = *cd;
                                    valid = false;
                                }
                                else if(cd->IsFalse())
                                {
                                    var->SetType(VarType::IGNORE);
                                    valid = false;
                                }
                            }
                        }
                    }while(valid);
                }
                delete forEnv;
            }
        }
            break;
        case NodeType::WHILE:
        {
            Env *whileEnv = env->extend();
            Var *cond = Evaluate(node->cond, whileEnv);
            if(cond->IsFalse())
            {
                *var = *cond;
            }
            while(!cond->IsFalse())
            {
                if(var != NULL && !var->Stored())
                    delete var;
                var = Evaluate(node->body, whileEnv);
                if(var->IsType(VarType::ERROR))
                {
                    break;
                }
                if(cond != NULL && !cond->Stored())
                    delete cond;
                cond = Evaluate(node->cond, whileEnv);
                if(cond->IsType(VarType::ERROR))
                {
                    *var = *cond;
                    break;
                }
            }
            delete whileEnv;
        }
            break;
        case NodeType::DOWHILE:
        {
            Env *whileEnv = env->extend();
            Var *cond = NULL;
            do
            {
                if(var != NULL && !var->Stored())
                    delete var;
                var = Evaluate(node->body, whileEnv);
                if(var->IsType(VarType::ERROR))
                {
                    break;
                }
                if(cond != NULL && !cond->Stored())
                    delete cond;
                cond = Evaluate(node->cond, whileEnv);
                if(cond->IsType(VarType::ERROR))
                {
                    *var = *cond;
                    break;
                }
            }while(!cond->IsFalse());
            delete whileEnv;
        }
            break;
        case NodeType::LET:
        {
            Env *letEnv = env->extend();
            bool valid = true;
            for(int i = 0; i < node->nodes.size(); i++)
            {
                var = Evaluate(node->nodes[i], letEnv);
                if(var->IsType(VarType::ERROR))
                {
                    valid = false;
                    break;
                }
            }
            if(valid)
            {
                var = Evaluate(node->body, letEnv);
            }
            delete letEnv;
        }
            break;
        case NodeType::CONTEXT:
        {
            for(int i = 0; i < node->nodes.size(); i++)
            {
                if(var && !var->Stored())
                {
                    delete var;
                    var = NULL;
                }
                var = Evaluate(node->nodes[i], env);
                if(var && var->IsType(VarType::ERROR))
                {
                    return var;
                }
                if(var && var->Returned())
                {
                    var->Return(false);
                    return var;
                }
            }
        }
            break;
        default:
            MakeError(var, "Could not evaluate", node);
            break;
    }
    return var;
}

void Interpreter::MakeError(Var* var, const string& text)
{
    if(!var)
        var = MKVAR();
    stringstream ss;
    ss << text;
    var->Error(ss.str());
}

void Interpreter::MakeError(Var* var, const string& text, Node &node)
{
    if(!var)
        var = MKVAR();
    stringstream ss;
    ss << text << " [ " << node->ToString() << " ]";
    var->Error(ss.str());
}

Var* Interpreter::ApplyOperator(const string& op, Var* left, Var* middle, Var* right, Env *env)
{
    Var* res = MKVAR();
    if(op == "==")
    {
        *res = *left == *right;
    }
    else if(op == "!=")
    {
        *res = *left != *right;
    }
    else if(op == ">")
    {
        *res = *left > *right;
    }
    else if(op == "<")
    {
        *res = *left < *right;
    }
    else if(op == ">=")
    {
        *res = *left >= *right;
    }
    else if(op == "<=")
    {
        *res = *left <= *right;
    }
    else if(op == "||")
    {
        *res = *left || *right;
    }
    else if(op == "&&")
    {
        *res = *left && *right;
    }
    else if(op == "|")
    {
        *res = *left | *right;
    }
    else if(op == "&")
    {
        *res = *left & *right;
    }
    else if(op == ">>")
    {
        *res = *left >> *right;
    }
    else if(op == "<<")
    {
        *res = *left << *right;
    }
    else if(op == "^")
    {
        *res = *left ^ *right;
    }
    else if(op == "~")
    {
        *res = ~ *right;
    }
    else if(op == "!")
    {
        *res = ! *right;
    }
    else if(op == "%")
    {
        *res = *left % *right;
    }
    else if(op == "**")
    {
        *res = left->pow(*right);
    }
    else if(op == "+")
    {
        if(left->IsType(VarType::IGNORE))
            *res = *right;
        else
            *res = *left + *right;
    }
    else if(op == "-")
    {
        Var zero;
        zero = -1.0;
        if(left->IsType(VarType::IGNORE))
            *res = zero * *right;
        else
            *res = *left - *right;
    }
    else if(op == "/")
    {
        *res = *left / *right;
    }
    else if(op == "*")
    {
        *res = *left * *right;
    }
    else if(op == "++")
    {
        Var inc;
        inc = (double)1;
        if(!left->IsType(VarType::IGNORE))
        {
            *res = *left;
            *left = *left + inc;
        }
        else
        {
            *right = *right + inc;
            *res = *right;
        }
    }
    else if(op == "--")
    {
        Var dec;
        dec = (double)1;
        if(!left->IsType(VarType::IGNORE))
        {
            *res = *left;
            *left = *left - dec;
        }
        else
        {
            *right = *right - dec;
            *res = *right;
        }
    }
    else if(op == ":")
    {
        if(!left->IsType(VarType::NUMBER))
        {
            stringstream ss;
            ss << "Invalid range start '" << left << "'";
            MakeError(res, ss.str());
        }
        else if(!right->IsType(VarType::NUMBER))
        {
            stringstream ss;
            ss << "Invalid range end '" << right << "'";
            MakeError(res, ss.str());
        }
        else
        {
            if(!middle->IsType(VarType::IGNORE) && !middle->IsType(VarType::NUMBER))
            {
                stringstream ss;
                ss << "Invalid range middle '" << middle << "'";
                MakeError(res, ss.str());
            }
            else
            {
                double start = left->Number();
                double step = 1;
                double end = right->Number();

                if(!middle->IsType(VarType::IGNORE))
                    step = middle->Number();
                else
                {
                    if(start > end)
                        step = -1;
                }

                VarArray _array;
                for(; start <= end; start += step)
                {
                    Var item;
                    item = start;
                    _array.push_back(item);
                }
                *res = _array;
            }
        }
    }
    else
    {
        stringstream ss;
        ss << "Invalid operator '" << op << "'";
        MakeError(res, ss.str());
    }

    if(!left->Stored())
        delete left;
    if(!middle->Stored())
        delete middle;
    if(!right->Stored())
        delete right;

    return res;
}

void Interpreter::ReplaceString(std::string& subject, const std::string& search,
                          const std::string& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

Var* Interpreter::Call(const std::string& func, std::vector<Var*>& args, Env *env)
{
    Var* res = MKVAR();
    if(func == "print" || func == "println")
    {
        for(int i = 0; i < args.size(); i++)
        {
            res = args[i];
            string str = args[i]->ToString();
            cout << str;
            if(i < args.size()-1 && str[0] != '\033')
                cout << " ";
        }
        cout << "\033[0m";
        if(func == "println")
            cout << endl;
        else
            needBreak = true;
    }
    else if(func == "color")
    {
        stringstream ss;
        ss << "\033[0;";
        if(args.size() > 0)
        {
            if(args[0]->IsType(VarType::STRING) && IsColor(args[0]->String()))
            {
                ss << colors[args[0]->String()];
            }
            else if(args[0]->IsType(VarType::NUMBER))
            {
                ss << (int)args[0]->Number();
            }
            else
            {
                MakeError(res, "Invalid foreground color");
                return res;
            }
        }
        if(args.size() > 1)
        {
            if(args[1]->IsType(VarType::STRING) && IsColor(args[1]->String()))
            {
                ss << ";" << (colors[args[1]->String()] + 10);
            }
            else if(args[1]->IsType(VarType::NUMBER))
            {
                ss << ";" << (int)args[1]->Number();
            }
            else
            {
                MakeError(res, "Invalid background color");
                return res;
            }
        }
        ss << "m";
        *res = ss.str();
    }
    else if(func == "date")
    {
        string format = "%c";
        if(args.size() > 0)
        {
            if(args[0]->IsType(VarType::STRING))
            {
                format = args[0]->String();
            }
            else
            {
                MakeError(res, "Invalid date format");
                return res;
            }
        }
        ReplaceString(format, "YYYY", "%Y");
        ReplaceString(format, "YY", "%y");
        ReplaceString(format, "MM", "%m");
        ReplaceString(format, "mm", "%M");
        ReplaceString(format, "ss", "%S");
        ReplaceString(format, "hh", "%I");
        ReplaceString(format, "HH", "%H");
        ReplaceString(format, "dd", "%d");
        ReplaceString(format, "D", "%e");

        time_t rawtime;
        struct tm * timeinfo;
        char buffer [256];
        time (&rawtime);
        timeinfo = localtime (&rawtime);
        strftime (buffer, 256,format.c_str(),timeinfo);
        string date(buffer);
        *res = date;
    }
    else if(func == "env")
    {
        cout << env->toString();
    }
    else if(func == "exit")
    {
        exitCode = 0;
        exit = true;
        if(args.size() > 0)
        {
            if(args[0]->IsType(VarType::NUMBER))
                exitCode = (int)args[0]->Number();
        }
        res->Return(true);
    }
    else
    {
        Var* var = env->get(func);
        if(var->IsType(VarType::ERROR))
        {
            delete res;
            return var;
        }
        if(!var->IsType(VarType::FUNC))
        {
            delete var;
            stringstream ss;
            ss << "'" << func << "' is not callable";
            MakeError(res, ss.str());
        }
        else
        {
            Env *funcEnv = env->extend();
            Node _func = var->Func();
            VarArray _array(args.size());
            for(int i = 0; i < _func->vars.size(); i++)
            {
                if(i < args.size())
                {
                    funcEnv->def(_func->vars[i], args[i]);
                    _array[i] = *args[i];
                }
                else
                {
                    Var *none = MKVAR();
                    none->SetType(VarType::NONE);
                    funcEnv->def(_func->vars[i], none);
                    delete none;
                }
            }
            Var *_args = MKVAR();
            *_args = _array;
            funcEnv->def("args", _args);
            Var *_res = Evaluate(_func->body, funcEnv);
            *res = *_res;
            delete funcEnv;
        }
    }
    return res;
}

bool Interpreter::IsColor(const std::string& color)
{
    map<string, int>::iterator it = colors.find(color);
    return it != colors.end();
}