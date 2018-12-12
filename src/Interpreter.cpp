#include "Interpreter.h"
#include <iostream>

using namespace std;

Interpreter::Interpreter()
{
    env = new Env();
    exitCode = 0;
    needBreak = false;
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
    needBreak = false;
    Node root = parser.Parse(src);
    if(root->type == NodeType::ERROR)
    {
        cout << root->ToString() << endl;
        exitCode = interactive ? 0 : -1;
    }
    else
    {
        Var* var = Evaluate(root, env);
        if(!var)
        {
            cout << "Error: Undefined error" << endl;
            exitCode = interactive ? 0 : -1;
        }
        else if(var->IsType(VarType::ERROR))
        {
            cout << "Error: " << var << endl;
            exitCode = interactive ? 0 : -1;
        }
        else
        {
            exitCode = 0;
        }
        if(var && !var->Stored())
            delete var;
    }

    return exitCode == 0;
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
        case NodeType::VARIABLE:
            var = env->get(node->_string);
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
            var = ApplyOperator(node->_string, Evaluate(node->left, env), Evaluate(node->middle, env), Evaluate(node->right, env), env);
            break;
        case NodeType::CALL:
        {
            bool process = true;
            if(node->func->type = NodeType::VARIABLE)
            {
                vector<Var*> args(node->nodes.size());
                for(int i = 0; i < node->nodes.size(); i++)
                {
                    args[i] = Evaluate(node->nodes[i], env);
                    if(args[i]->IsType(VarType::ERROR))
                    {
                        return args[i];
                    }
                }
                var = Call(node->func->_string, args, env);
            }
            else
            {
                MakeError(var, "Cannot call this like a function", node->func);
            }
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
    if(op == "+")
    {
        *res = *left + *right;
    }
    else if(op == "-")
    {
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
    else
    {
        stringstream ss;
        ss << "Invalid operator '" << op << "'";
        MakeError(res, ss.str());
    }

    return res;
}

Var* Interpreter::Call(const std::string& func, std::vector<Var*>& args, Env *env)
{
    Var* res = MKVAR();
    if(func == "print" || func == "println")
    {
        for(int i = 0; i < args.size(); i++)
        {
            cout << args[i]->ToString();
            if(i < args.size()-1)
                cout << " ";
        }
        if(func == "println")
            cout << endl;
        else
            needBreak = true;
    }
    else if(func == "env")
    {
        cout << env->toString();
    }
    return res;
}