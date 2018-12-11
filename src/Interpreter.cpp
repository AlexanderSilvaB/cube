#include "Interpreter.h"
#include <iostream>

using namespace std;

Interpreter::Interpreter()
{
    env = SEnv(new Env());
    exitCode = 0;
}

Interpreter::~Interpreter()
{

}

int Interpreter::ExitCode()
{
    return exitCode;
}

bool Interpreter::Evaluate(const string& src)
{
    Node root = parser.Parse(src);
    if(root->type == NodeType::ERROR)
    {
        cout << root->ToString() << endl;
        exitCode = -1;
    }
    else
    {
        SVar var = Evaluate(root, env);
        if(var.empty() || var->IsType(VarType::ERROR))
            exitCode = -1;
        else
            exitCode = 0;
        if(var.empty())
        {
            cout << "Error: Undefined error" << endl;
        }
        else if(var->IsType(VarType::ERROR))
        {
            cout << "Error: " << var << endl;
        }
    }

    return exitCode == 0;
}

SVar Interpreter::Evaluate(Node node, SEnv env)
{
    SVar var = MKVAR();
    cout << node->ToString() << endl;
    switch(node->type)
    {
        case NodeType::NONE:
            var->SetType(VarType::NONE);
            break;
        case NodeType::BOOL:
            *var = node->_bool;
            break;
        case NodeType::NUMBER:
            *var = node->_number;
            break;
        case NodeType::STRING:
            *var = node->_string;
            break;
        case NodeType::VARIABLE:
            var = env->get(node->_string);
            break;
        case NodeType::ASSIGN:
        {
            if(node->left->type == NodeType::VARIABLE)
            {
                SVar value = Evaluate(node->right, env);
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
        case NodeType::CONTEXT:
        {
            for(int i = 0; i < node->nodes.size(); i++)
                var = Evaluate(node->nodes[i], env);
        }
            break;
        default:
            MakeError(var, "Could not evaluate", node);
            break;
    }
    return var;
}

void Interpreter::MakeError(SVar &var, const string& text, Node &node)
{
    stringstream ss;
    ss << text << " [ " << node->ToString() << " ]";
    var->Error(ss.str());
}