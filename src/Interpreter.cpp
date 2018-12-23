#include "Interpreter.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <time.h>
#include <streambuf>
#include <iostream> 
#include <fstream>

using namespace std;

Interpreter::Interpreter()
{
    env = new Env();
    loader = new Loader();
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
    delete env;
}

int Interpreter::ExitCode()
{
    return exitCode;
}

bool Interpreter::NeedBreak()
{
    return needBreak;
}

vector<char> Interpreter::Compile(const string& src)
{
    vector<char> data;
    Node root = parser.Parse(src);   
    if(root->type == NodeType::ERROR)
    {
        cout << root->ToString() << endl;
        exitCode = -1;
        return data;
    }
    const char *begin = "!<CUBE>";
    data.insert(data.end(), begin, begin + 7);

    vector<char> rootData = root->Serialize();
    serializeV(data, rootData);
    exitCode = 0;
    return data;
}

bool Interpreter::Evaluate(const string& src, bool interactive)
{
    exitCode = 0;
    needBreak = false;
    Node root;

    if(src.size() > 7 && src.substr(0, 7) == "!<CUBE>")
    {
        root = MKNODE();
        string byteCode = src.substr(7);
        vector<char> data;
        data.insert(data.end(), byteCode.begin(), byteCode.end());
        root->Deserialize(data);
    }
    else
        root = parser.Parse(src);
    if(root->type == NodeType::ERROR)
    {
        cout << root->ToString() << endl;
        exitCode = -1;
        exit = !interactive;
    }
    else
    {
        Var *var = MKVAR();
        string name = "";
        if(!interactive)
        {
            name = "__main__";
        }
        *var = name;
        env->set("__name__", var);
        var = Evaluate(root, env);
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

Var* Interpreter::Evaluate(Node node, Env* env, bool isClass, Var *caller)
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
            if(isClass)
                MakeError(var, "id type in class", node);
            else
                var->SetType(VarType::NONE);
            break;
        case NodeType::BOOL:
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
                *var = node->_bool;
            break;
        case NodeType::NUMBER:
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
                *var = node->_number;
            break;
        case NodeType::STRING:
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
                *var = node->_string;
            break;
        case NodeType::ARRAY:
        {
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
            {
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
        }
            break;
        case NodeType::DICT:
        {
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
            {
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
        }
            break;
        case NodeType::FUNCTION:
            var = MKVAR();
            *var = node;
            var = env->set(node->_string, var);
            break;
        case NodeType::FUNCTION_DEF:
        {
            if(caller == NULL)
            {
                var = MKVAR();
                MakeError(var, "Cannot create a function definition in this context (just native context)");
            }
            else
            {
                Var fn;
                NamesArray names(node->vars.size());
                for(int i = 0; i < node->vars.size(); i++)
                    names[i] = node->vars[i];
                fn.ToFuncDef(caller->Handler(), node->_string, node->_nick, names);
                caller->Array().push_back(fn);
                var = caller;
            }
        }
            break;
        case NodeType::EXTENSION:
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
            {
                *var = node;
                var = env->set(node->_nick+"."+node->_string, var);
            }
            break;
        case NodeType::LAMBDA:
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
                *var = node;
            break;
        case NodeType::VARIABLE:
            if(isClass)
            {
                var = MKVAR();
                var->SetType(VarType::NONE);
                var = env->def(node->_string, var);
            }
            else
            {
                var = env->get(node->_string);
            }
            break;
        case NodeType::RETURN:
            if(isClass)
            {
                var = MKVAR();
                MakeError(var, "id type in class", node);
            }
            else
            {
                var = Evaluate(node->body, env);
                if(var)
                {
                    if(!var->IsType(VarType::ERROR))
                        var->Return(true);
                }
            }
            break;
        case NodeType::ASSIGN:
        {
            var = MKVAR();
            if(node->left->type == NodeType::VARIABLE)
            {
                Var* value = Evaluate(node->right, env);
                if(!value->IsType(VarType::ERROR))
                {
                    if(value->IsType(VarType::CLASS) && value->Counter() == 0)
                    {
                        MakeError(var, "Cannot apply the operator '=' to an class", node->right);
                    }
                    else
                    {
                        var = env->set(node->left->_string, value);
                        var->ClearRef();
                    }
                }
                else  
                {
                    var = value;
                }
            }
            else if(node->left->type == NodeType::INDEX)
            {
                Var* value = Evaluate(node->right, env);
                if(!value->IsType(VarType::ERROR))
                {
                    if(value->IsType(VarType::CLASS) && value->Counter() == 0)
                    {
                        MakeError(var, "Cannot apply the operator '=' to an class", node->right);
                    }
                    else
                    {
                        var = Evaluate(node->left, env);
                        if(!var->IsType(VarType::ERROR))
                        {
                            *var = *value;
                        }
                    }
                }
                else  
                {
                    var = value;
                }
            }
            else
            {
                MakeError(var, "Cannot assign to ", node->left);
            }
        }
            break;
        case NodeType::BINARY:
        {
            if(isClass)
            {
                var = MKVAR();
                MakeError(var, "id type in class", node);
            }
            else
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
                    if(left->IsType(VarType::DICT) || left->IsType(VarType::LIB))
                    {
                        VarDict &dict = left->Dict();
                        if(node->right->type != NodeType::VARIABLE && node->right->type != NodeType::CALL)
                        {
                            var = MKVAR();
                            stringstream ss;
                            ss << "Could not apply the operator '.' to '" << node->right->ToString() << "'";
                            MakeError(var, ss.str(), node);
                        }
                        else if(node->right->type == NodeType::VARIABLE)
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
                        else
                        {
                            if(node->right->func->type = NodeType::VARIABLE)
                            {
                                Var *func = NULL;
                                Env* callEnv = env->extend();
                                for(VarDict::iterator it = dict.begin(); it != dict.end(); it++)
                                {
                                    callEnv->def(it->first, &it->second);
                                    if(it->first == node->right->func->_string)
                                    {
                                        func = &it->second;
                                    }
                                }
                                if(func == NULL)
                                {
                                    var = MKVAR();
                                    stringstream ss;
                                    ss << "'" << node->right->func->_string << "' does not name a function member"; 
                                    MakeError(var, ss.str(), node->right->func);
                                }
                                else
                                {
                                    bool valid = true;
                                    vector<Var*> args(node->right->nodes.size());
                                    for(int i = 0; i < node->right->nodes.size(); i++)
                                    {
                                        args[i] = Evaluate(node->right->nodes[i], env);
                                        if(args[i]->IsType(VarType::ERROR))
                                        {
                                            var = MKVAR();
                                            *var = args[i];
                                            valid = false;
                                        }
                                    }
                                    if(valid)
                                        var = Call(node->right->func->_string, args, callEnv);
                                }
                                delete callEnv;
                            }
                            else
                            {
                                var = MKVAR();
                                MakeError(var, "Cannot call this like a function", node->right->func);
                            }
                        }
                    }
                    else if(left->IsType(VarType::CLASS))
                    {
                        if(left->Counter() == 0)
                        {
                            var = MKVAR();
                            MakeError(var, "The call must be from an object", node->right);
                        }
                        else
                            var = Evaluate(node->right, left->Context(), false, left);
                    }
                    else if(left->IsType(VarType::NATIVE))
                    {
                        var = Evaluate(node->right, env, false, left);
                    }
                    else
                    {
                        if(node->right->type != NodeType::CALL)
                        {
                            var = MKVAR();
                            stringstream ss;
                            ss << "Could not apply the operator '.' to '" << node->right->ToString() << "'";
                            MakeError(var, ss.str(), node);
                            if(!left->Stored())
                                delete left;
                        }
                        else
                        {
                            string name = left->TypeName() + "." + node->right->func->_string;
                            Var *func = env->get(name);
                            if(func->IsType(VarType::ERROR))
                            {
                                var = MKVAR();
                                stringstream ss;
                                ss << "'" << left->TypeName() << "' does not have an extension called '" << node->right->func->_string << "'";
                                MakeError(var, ss.str(), node);
                                if(!left->Stored())
                                    delete left;
                            }
                            else
                            {
                                Env* callEnv = env->extend();
                                bool valid = true;
                                vector<Var*> args(node->right->nodes.size());
                                for(int i = 0; i < node->right->nodes.size(); i++)
                                {
                                    args[i] = Evaluate(node->right->nodes[i], env);
                                    if(args[i]->IsType(VarType::ERROR))
                                    {
                                        var = MKVAR();
                                        *var = args[i];
                                        valid = false;
                                    }
                                }
                                if(valid)
                                    var = Call(name, args, callEnv, left);
                                delete callEnv;
                                if(!left->Stored())
                                    delete left;
                            }
                        }
                    }
                }
                else
                {
                    bool valid = true;
                    Var *l = Evaluate(node->left, env);
                    if(l->IsType(VarType::ERROR))
                    {
                        var = MKVAR();
                        *var = *l;
                        valid = false;
                    }
                    Var *m = NULL;
                    if(valid)
                    {
                        m = Evaluate(node->middle, env);
                        if(m->IsType(VarType::ERROR))
                        {
                            var = MKVAR();
                            *var = *m;
                            valid = false;
                        }
                    }
                    Var *r = NULL;
                    if(valid)
                    {
                        r = Evaluate(node->right, env);
                        if(r->IsType(VarType::ERROR))
                        {
                            var = MKVAR();
                            *var = *r;
                            valid = false;
                        }
                    }
                    if(valid)
                        var = ApplyOperator(node->_string, l, m, r, env);
                }
            }
        }
            break;
        case NodeType::INDEX:
        {
            if(isClass)
            {
                var = MKVAR();
                MakeError(var, "id type in class", node);
            }
            else
            {
                var = MKVAR();
                Var* left = Evaluate(node->left, env);
                VarArray index;
                for(int i = 0; i < node->nodes.size(); i++)
                {
                    bool changeLeft = false;
                    bool changeRight = false;
                    if(node->nodes[i]->type == NodeType::BINARY && node->nodes[i]->_string == ":" )
                    {
                        if(node->nodes[i]->left->_string == "start")
                        {
                            node->nodes[i]->left->_number = 0;
                            node->nodes[i]->left->type = NodeType::NUMBER;
                            changeLeft = true;
                        }
                        if(node->nodes[i]->right->_string == "end")
                        {
                            node->nodes[i]->right->_number = left->Size()-1;
                            node->nodes[i]->right->type = NodeType::NUMBER;
                            changeRight = true;
                        }
                    }
                    Var* v = Evaluate(node->nodes[i], env);
                    if(changeLeft)
                    {
                        node->nodes[i]->left->type = NodeType::VARIABLE;
                    }
                    if(changeRight)
                    {
                        node->nodes[i]->right->type = NodeType::VARIABLE;
                    }
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
        }
            break;
        case NodeType::CALL:
        {
            if(isClass)
            {
                var = MKVAR();
                MakeError(var, "id type in class", node);
            }
            else
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
                    var = Call(node->func->_string, args, env, caller);
                }
                else
                {
                    var = MKVAR();
                    MakeError(var, "Cannot call this like a function", node->func);
                }
            }
        }
            break;
        case NodeType::IF:
        {
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
            {
                Var *cond = Evaluate(node->cond, env);
                if(cond->IsType(VarType::ERROR))
                {
                    *var = *cond;
                }
                else if(!cond->IsFalse())
                {
                    var = Evaluate(node->then, env);
                }
                else if(node->contr)
                {
                    var = Evaluate(node->contr, env);
                }
            }
        }
            break;
        case NodeType::TRY:
        {
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
            {
                Env* tryEnv = env->copy();
                Var *body = Evaluate(node->body, tryEnv);
                if(!body->IsType(VarType::ERROR))
                {
                    env->paste(tryEnv);
                    delete tryEnv;
                }
                else if(node->contr)
                {
                    delete tryEnv;
                    tryEnv = env->copy();
                    if(node->_string.size() > 0)
                        tryEnv->def(node->_string, body);
                    Var *contr = Evaluate(node->contr, tryEnv);
                    if(contr->IsType(VarType::ERROR))
                    {
                        *var = *contr;
                    }
                    delete tryEnv;
                }
            }
        }
            break;
        case NodeType::FOR:
        {
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
            {
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
                                    forEnv->def(in->left->_string, inVar);
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
                                    forEnv->def(in->left->_string, inVar);
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
        }
            break;
        case NodeType::WHILE:
        {
            if(isClass)
            {
                var = MKVAR();
                MakeError(var, "id type in class", node);
            }
            else
            {
                Env *whileEnv = env->extend();
                Var *cond = Evaluate(node->cond, whileEnv);
                if(cond->IsFalse())
                {
                    var = MKVAR();
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
        }
            break;
        case NodeType::DOWHILE:
        {
            if(isClass)
            {
                var = MKVAR();
                MakeError(var, "id type in class", node);
            }
            else
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
        }
            break;
        case NodeType::LET:
        {
            if(isClass)
            {
                var = MKVAR();
                MakeError(var, "id type in class", node);
            }
            else
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
        }
            break;
        case NodeType::CONTEXT:
        {
            bool native = false;
            Var *vNative = NULL;
            for(int i = 0; i < node->nodes.size(); i++)
            {
                if(var && !var->Stored())
                {
                    delete var;
                    var = NULL;
                }
                if(native)
                    var = Evaluate(node->nodes[i], env, isClass, vNative);
                else
                    var = Evaluate(node->nodes[i], env, isClass);
                if(var && var->IsType(VarType::ERROR))
                {
                    return var;
                }
                if(var && var->Returned() && !isClass)
                {
                    var->Return(false);
                    return var;
                }
                if(var && var->IsType(VarType::NATIVE))
                {
                    native = true;
                    vNative = var;
                }
            }
        }
            break;
        case NodeType::IMPORT:
        {
            var = MKVAR();
            if(isClass)
                MakeError(var, "id type in class", node);
            else
            {
                for(int i = 0; i < node->nodes.size(); i++)
                {
                    string name = GetName(node->nodes[i]->_string); 
                    bool global = false;
                    bool native = node->nodes[i]->_bool;
                    if(node->nodes[i]->_nick.length() > 0)
                    {
                        global = node->nodes[i]->_nick == "__global__";
                        name = node->nodes[i]->_nick;
                    }

                    if(native)
                    {
                        void *handler;
                        if(!loader->Load(node->nodes[i]->_string, &handler))
                        {
                            MakeError(var, "Could not load the native library");
                        }
                        else
                        {
                            var->ToNative(name, handler);
                            var = env->def(name, var);
                        }
                        break;
                    }
                    else
                    {
                        string src = OpenFile(node->nodes[i]->_string+".cube");
                        Node root = parser.Parse(src);
                        *var = GetName(node->nodes[i]->_string); 
                        if(global)
                        {
                            env->set("__name__", var);
                            var = Evaluate(root, env);
                        }
                        else
                        {
                            Env *libEnv = new Env();

                            libEnv->set("__name__", var);
                            Evaluate(root, libEnv);
                            
                            *var = libEnv;
                            env->def(name, var);
                            delete libEnv;
                        }
                    }
                }
                if(!var->IsType(VarType::NATIVE) && !var->IsType(VarType::ERROR))
                    var->SetType(VarType::IGNORE);
            }
        }
            break;
        case NodeType::CLASS:
            if(isClass)
            {
                var = MKVAR();
                MakeError(var, "id type in class", node);
            }
            else
                var = CreateClass(node, env);
            break;
        default:
            var = MKVAR();
            MakeError(var, "Could not evaluate", node);
            break;
    }
    return var;
}

std::string Interpreter::OpenFile(const std::string& fileName)
{
    string ret = "";
    ifstream t(fileName.c_str());
    if(!t.good())
    {
        for(set<string>::iterator it = paths.begin(); it != paths.end(); it++)
        {
            string path = (*it) + "/" + fileName;
            t = ifstream(path.c_str());
            if(t.good())
                break;
        }
    }

    if(t.good())
    {
        t.seekg(0, std::ios::end);   
        ret.reserve(t.tellg());
        t.seekg(0, std::ios::beg);
        ret.assign((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
        string folder = GetFolder(fileName);
        if(paths.count(folder) == 0)
            paths.insert(folder);
    }
    return ret;
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
        *res = ! *left;
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

Var* Interpreter::Call(const std::string& func, std::vector<Var*>& args, Env *env, Var* caller)
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
                cout << "";
        }
        if(func == "println")
        {
            cout << "\033[0m" << endl;
        }
        else
            needBreak = true;
    }
    else if(func == "input")
    {
        for(int i = 0; i < args.size(); i++)
        {
            cout << args[i]->String();
            if(i < args.size()-1)
                cout << "";
        }
        string str;
        getline(cin, str);
        *res = str;
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
    else if(func == "type")
    {
        if(args.size() == 0)
            res->SetType(VarType::NONE);
        else if(args.size() == 1)
        {
            *res = args[0]->TypeName();
        }
        else
        {
            VarArray _array(args.size());
            for(int i = 0; i < args.size(); i++)
            {
                _array[i] = args[i]->TypeName();
            }
            *res = _array;
        }
    }
    else if(func == "bool")
    {
        if(args.size() == 0)
        {
            res->operator=(false);
        }
        else if(args.size() == 1)
        {
            *res = args[0]->AsBool();
        }
        else
        {
            VarArray _array(args.size());
            for(int i = 0; i < args.size(); i++)
            {
                _array[i] = args[i]->AsBool();
            }
            *res = _array;
        }
    }
    else if(func == "number")
    {
        if(args.size() == 0)
        {
            res->operator=(0.0);
        }
        else if(args.size() == 1)
        {
            *res = args[0]->AsNumber();
        }
        else
        {
            VarArray _array(args.size());
            for(int i = 0; i < args.size(); i++)
            {
                _array[i] = args[i]->AsNumber();
            }
            *res = _array;
        }
    }
    else if(func == "string")
    {
        if(args.size() == 0)
        {
            string str = "";
            res->operator=(str);
        }
        else if(args.size() == 1)
        {
            *res = args[0]->AsString();
        }
        else
        {
            VarArray _array(args.size());
            for(int i = 0; i < args.size(); i++)
            {
                _array[i] = args[i]->AsString();
            }
            *res = _array;
        }
    }
    else if(func == "array")
    {
        if(args.size() == 0)
        {
            VarArray _array;
            res->operator=(_array);
        }
        else if(args.size() == 1)
        {
            *res = args[0]->AsArray();
        }
        else
        {
            VarArray _array(args.size());
            for(int i = 0; i < args.size(); i++)
            {
                _array[i] = args[i]->AsArray();
            }
            *res = _array;
        }
    }
    else if(func == "int")
    {
        if(args.size() == 0)
        {
            res->operator=(0.0);
        }
        else if(args.size() == 1)
        {
            *res = args[0]->AsNumber();
            *res = (double)((int)res->Number());
        }
        else
        {
            VarArray _array(args.size());
            for(int i = 0; i < args.size(); i++)
            {
                _array[i] = args[i]->AsNumber();
                _array[i] = (double)((int)_array[i].Number());
            }
            *res = _array;
        }
    }
    else if(func == "len")
    {
        if(args.size() == 0)
        {
            res->operator=(0.0);
        }
        else if(args.size() == 1)
        {
            *res = (double)args[0]->Size();
        }
        else
        {
            VarArray _array(args.size());
            for(int i = 0; i < args.size(); i++)
            {
                _array[i] = (double)args[i]->Size();
            }
            *res = _array;
        }
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
        if(!var->IsType(VarType::FUNC) && !var->IsType(VarType::CLASS))
        {
            delete var;
            stringstream ss;
            ss << "'" << func << "' is not callable";
            MakeError(res, ss.str());
        }
        else if(var->IsType(VarType::CLASS))
        {
            if(var->Counter() != 0)
            {
                MakeError(res, "Objects cannot be called like classes");
            }
            else
            {
                Var *obj = var->Clone();
                var = obj;
                if(var->Context()->get(func)->IsType(VarType::FUNC))
                {
                    Var *res = Call(func, args, var->Context(), var);
                    if(res->IsType(VarType::ERROR))
                    {
                        *var = *res;
                    }
                }
                res = var;
            }
        }
        else
        {
            Env *funcEnv = env->extend();
            if(caller != NULL)
            {
                funcEnv->def("this", caller);
            }
            Node _func = var->Func();
            VarArray _array(args.size());
            int limit = max(_func->vars.size(), args.size());
            for(int i = 0; i < limit; i++)
            {
                if(i < args.size())
                {
                    if(i < _func->vars.size())
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

string Interpreter::GetFolder(const string& path)
{
    int i = path.size()-1;
    while(i >= 0)
    {
        if(path[i] == '\\' || path[i] == '/')
        {
            break;
        }
        i--;
    }
    if(i >= 0)
        return path.substr(0, i);
    return "";       
}

string Interpreter::GetName(const string& path)
{
    int i = path.size()-1;
    while(i >= 0)
    {
        if(path[i] == '\\' || path[i] == '/')
        {
            break;
        }
        i--;
    }
    if(i >= 0)
        return path.substr(i, path.length()-i);
    return path;       
}

Var* Interpreter::CreateClass(Node node, Env* env)
{
    Var* var = MKVAR();
    if(env->exists(node->_string))
    {
        MakeError(var, "Class name already defined", node);
        return var;
    }
    
    var->ToClass(node->_string);

    for(int i = 0; i < node->vars.size(); i++)
    {
        Var *in = env->get(node->vars[i]);
        if(!in || in->IsType(VarType::ERROR))
        {
            MakeError(var, "Undefined class '"+node->vars[i]+"'", node);
            return var;
        }
        else if(!in->IsType(VarType::CLASS) && in->Counter() != 0)
        {
            MakeError(var, "The name '"+node->vars[i]+"' is not a class", node);
            return var;
        }
        else
        {
            var->Context()->paste(in->Context());
        }
    }

    Var *res = Evaluate(node->body, var->Context(), true);
    if(res->IsType(VarType::ERROR))
    {
        delete var;
        return res;
    }

    var = env->def(node->_string, var);
    var->SetInitial();
    return var;
}