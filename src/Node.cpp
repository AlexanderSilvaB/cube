#include "Node.h"

using namespace std;

string makeSpaces(int spaces)
{
    stringstream ss;
    for(int i = 0; i < spaces; i++)
        ss << '\t';
    return ss.str();
}

string Node_st::ToString(int spaces)
{
    stringstream ss;
    ss << makeSpaces(spaces);
    switch(type)
    {
        case NodeType::IGNORE:
            ss << "Ignore";
            break;
        case NodeType::ERROR:
            ss << "Error: " << _string;
            break;
        case NodeType::NONE:
            ss << "None";
            break;
        case NodeType::VARIABLE:
            ss << "Variable: " << _string;
            break;
        case NodeType::BOOL:
            ss << "Bool: " << (_bool ? "true" : "false");
            break;
        case NodeType::NUMBER:
            ss << "Number: " << _number;
            break;
        case NodeType::STRING:
            ss << "String: " << _string;
            break;
        case NodeType::ARRAY:
            ss << "Array: " << endl;
            ss << makeSpaces(spaces) << "[" << endl;
            for(int i = 0; i < nodes.size(); i++)
            {
                ss << nodes[i]->ToString(spaces + 1);
            }
            ss << makeSpaces(spaces) << "]" << endl;
            break;
         case NodeType::INDEX:
            ss << "Index: " << endl;
            ss << left->ToString(spaces + 1);
            ss << makeSpaces(spaces+1) << "[" << endl;
            for(int i = 0; i < nodes.size(); i++)
            {
                ss << nodes[i]->ToString(spaces + 2);
            }
            ss << makeSpaces(spaces+1) << "]" << endl;
            break;
        case NodeType::RETURN:
            ss << "Return: " << endl;
            ss << body->ToString(spaces + 1);
            break;
        case NodeType::FUNCTION:
            ss << "Function: " << _string << endl;
            ss << makeSpaces(spaces + 1);
            ss << "Vars: ";
            for(int i = 0; i < vars.size(); i++)
            {
                ss << vars[i];
                if(i < vars.size()-1)
                    ss << ", ";
            }
            ss << endl;
            ss << makeSpaces(spaces + 1);
            ss << "Body: " << endl;
            ss << body->ToString(spaces + 2);
            break;
        case NodeType::LAMBDA:
            ss << "Lambda: " << endl;
            ss << makeSpaces(spaces + 1);
            ss << "Vars: ";
            for(int i = 0; i < vars.size(); i++)
            {
                ss << vars[i];
                if(i < vars.size()-1)
                    ss << ", ";
            }
            ss << endl;
            ss << makeSpaces(spaces + 1);
            ss << "Body: " << endl;
            ss << body->ToString(spaces + 2);
            break;
        case NodeType::LET:
            ss << "Let: " << endl;
            ss << makeSpaces(spaces + 1);
            ss << "Vars: " << endl;
            for(int i = 0; i < nodes.size(); i++)
            {
                ss << nodes[i]->ToString(spaces + 2);
            }
            ss << makeSpaces(spaces + 1);
            ss << "Body: " << endl;
            ss << body->ToString(spaces + 2);
            break;
        case NodeType::CALL:
            ss << "Call: " << endl;
            ss << func->ToString(spaces + 1);
            ss << makeSpaces(spaces + 1);
            ss << "Args: " << endl;
            for(int i = 0; i < nodes.size(); i++)
            {
                ss << nodes[i]->ToString(spaces + 2);
            }
            break;
        case NodeType::ASSIGN:
            ss << "Assign: " << endl;
            ss << left->ToString(spaces + 1);
            ss << right->ToString(spaces + 1);
            break;
        case NodeType::BINARY:
            ss << "Binary: " << _string << endl;
            if(left)
                ss << left->ToString(spaces + 1);
            if(middle)
                ss << middle->ToString(spaces + 1);
            if(right)
                ss << right->ToString(spaces + 1);
            break;
        case NodeType::IF:
            ss << "If: " << endl;
            ss << cond->ToString(spaces + 1);
            ss << then->ToString(spaces + 1);
            if(contr)
            {
                ss << contr->ToString(spaces + 1);
            }
            break;
        case NodeType::WHILE:
            ss << "While: " << endl;
            ss << cond->ToString(spaces + 1);
            ss << body->ToString(spaces + 1);
            break;
        case NodeType::DOWHILE:
            ss << "Do while: " << endl;
            ss << cond->ToString(spaces + 1);
            ss << body->ToString(spaces + 1);
            break;
        case NodeType::FOR:
            ss << "For: " << endl;
            ss << makeSpaces(spaces + 1);
            ss << "Args: " << endl;
            for(int i = 0; i < nodes.size(); i++)
            {
                ss << nodes[i]->ToString(spaces + 2);
            }
            ss << makeSpaces(spaces + 1);
            ss << "Body: " << endl;
            ss << body->ToString(spaces + 2);
            break;
        case NodeType::CONTEXT:
            ss << "Context: " << endl;
            ss << makeSpaces(spaces) << "{" << endl;
            for(int i = 0; i < nodes.size(); i++)
            {
                ss << nodes[i]->ToString(spaces + 1);
                if(i < nodes.size()-1)
                    ss << endl;
            }
            ss << makeSpaces(spaces) << "}" << endl;
            break;
        default:
            ss << "Unknown";
    }
    ss << endl;

    return ss.str();
}