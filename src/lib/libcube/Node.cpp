#include "Node.h"
#include <iostream>

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
        case NodeType::DICT:
            ss << "Dict: " << endl;
            ss << makeSpaces(spaces) << "[" << endl;
            for(int i = 0; i < nodes.size(); i++)
            {
                ss << makeSpaces(spaces + 1) << nodes[i]->left->_string << " = " << nodes[i]->right->ToString(0);
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
        case NodeType::FUNCTION_DEF:
            ss << "Function definition: " << _string << endl;
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
            ss << "Return: " << _nick;
            break;
        case NodeType::EXTENSION:
            ss << "Extension: " << _nick << "." << _string << endl;
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
         case NodeType::TRY:
            ss << "Try: " << endl;
            ss << makeSpaces(spaces + 1) << "Body: " << endl;
            ss << body->ToString(spaces + 1);
            if(contr)
            {
                ss << makeSpaces(spaces + 1) << "Catch";
                if(_string.length() > 0)
                    ss << ": " << _string << endl;
                else
                    ss << endl;
                ss << contr->ToString(spaces + 1);
            }
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
            if(createNew)
                ss << "new ";
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
        case NodeType::IMPORT:
            ss << "Import";
            if(_bool)
                ss << "(native)";
            ss << ": " << endl;
            for(int i = 0; i < nodes.size(); i++)
            {
                ss << makeSpaces(spaces + 1);
                ss << nodes[i]->_string;
                if(nodes[i]->_nick.size() > 0)
                    ss << " as " << nodes[i]->_nick;
                ss << endl;
            }
            break;
        case NodeType::CLASS:
            ss << "Class " << _string;
            if(vars.size() > 0)
            {
                ss << " : ";
                for(int i = 0; i < vars.size(); i++)
                {
                    ss << vars[i];
                    if(i < vars.size()-1)
                        ss << ", ";
                }
            }
            ss << endl;
            ss << body->ToString(spaces + 1);
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

vector<char> Node_st::Serialize()
{
    vector<char> data;
    binary bin;

    serialize(bin, data, type, int);

    switch(type)
    {
        case NodeType::IGNORE:
            break;
        case NodeType::ERROR:
            serializeS(bin, data, _string);
            break;
        case NodeType::NONE:
            break;
        case NodeType::VARIABLE:
            serializeS(bin, data, _string);
            break;
        case NodeType::BOOL:
            serialize(bin, data, _bool, bool);
            break;
        case NodeType::NUMBER:
            serialize(bin, data, _number, double);
            break;
        case NodeType::STRING:
            serializeS(bin, data, _string);
            break;
        case NodeType::ARRAY:
            serialize(bin, data, nodes.size(), int);
            for(int i = 0; i < nodes.size(); i++)
            {
                vector<char> nodeData = nodes[i]->Serialize();
                serializeV(data, nodeData);
            }
            break;
        case NodeType::DICT:
            serialize(bin, data, nodes.size(), int);
            for(int i = 0; i < nodes.size(); i++)
            {
                vector<char> leftData = nodes[i]->left->Serialize();
                serializeV(data, leftData);
                vector<char> nodeData = nodes[i]->right->Serialize();
                serializeV(data, nodeData);
            }
            break;
         case NodeType::INDEX:
         {
            vector<char> leftData = left->Serialize();
            serializeV(data, leftData);
            serialize(bin, data, nodes.size(), int);
            for(int i = 0; i < nodes.size(); i++)
            {
                vector<char> nodeData = nodes[i]->Serialize();
                serializeV(data, nodeData);
            }
         }
            break;
        case NodeType::RETURN:
        {
            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
        case NodeType::FUNCTION:
        {
            serializeS(bin, data, _string);
            serialize(bin, data, vars.size(), int);
            for(int i = 0; i < vars.size(); i++)
            {
                serializeS(bin, data, vars[i]);
            }
            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
        case NodeType::FUNCTION_DEF:
        {
            serializeS(bin, data, _string);
            serialize(bin, data, vars.size(), int);
            for(int i = 0; i < vars.size(); i++)
            {
                serializeS(bin, data, vars[i]);
            }
            serializeS(bin, data, _nick);
        }
            break;
        case NodeType::EXTENSION:
        {
            serializeS(bin, data, _nick);
            serializeS(bin, data, _string);
            serialize(bin, data, vars.size(), int);
            for(int i = 0; i < vars.size(); i++)
            {
                serializeS(bin, data, vars[i]);
            }
            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
         case NodeType::TRY:
         {
            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
            if(contr)
            {
                serialize(bin, data, true, bool);
                serializeS(bin, data, _string); 
                vector<char> contrData = contr->Serialize();
                serializeV(data, contrData);
            }
            else
            {
                serialize(bin, data, false, bool);
            }
         }
            break;
        case NodeType::LAMBDA:
        {
            serialize(bin, data, vars.size(), int);
            for(int i = 0; i < vars.size(); i++)
            {
                serializeS(bin, data, vars[i]);
            }
            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
        case NodeType::LET:
        {
            serialize(bin, data, nodes.size(), int);
            for(int i = 0; i < nodes.size(); i++)
            {
                vector<char> nodeData = nodes[i]->Serialize();
                serializeV(data, nodeData);
            }
            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
        case NodeType::CALL:
        {
            vector<char> funcData = func->Serialize();
            serialize(bin, data, createNew, bool);
            serializeV(data, funcData);

            serialize(bin, data, nodes.size(), int);
            for(int i = 0; i < nodes.size(); i++)
            {
                vector<char> nodeData = nodes[i]->Serialize();
                serializeV(data, nodeData);
            }
        }
            break;
        case NodeType::ASSIGN:
        {
            vector<char> leftData = left->Serialize();
            serializeV(data, leftData);

            vector<char> rightData = right->Serialize();
            serializeV(data, rightData);
        }
            break;
        case NodeType::BINARY:
        {
            serializeS(bin, data, _string);
            serialize(bin, data, left, bool);
            serialize(bin, data, middle, bool);
            serialize(bin, data, right, bool);
            if(left)
            {
                vector<char> leftData = left->Serialize();
                serializeV(data, leftData);
            }
            if(middle)
            {
                vector<char> middleData = middle->Serialize();
                serializeV(data, middleData);
            }
            if(right)
            {
                vector<char> rightData = right->Serialize();
                serializeV(data, rightData);
            }
        }
            break;
        case NodeType::IF:
        {
            vector<char> condData = cond->Serialize();
            serializeV(data, condData);

            vector<char> thenData = then->Serialize();
            serializeV(data, thenData);

            if(contr)
            {
                serialize(bin, data, true, bool);
                vector<char> contrData = contr->Serialize();
                serializeV(data, contrData);
            }
            else
            {
                serialize(bin, data, false, bool);
            }
        }
            break;
        case NodeType::WHILE:
        {
            vector<char> condData = cond->Serialize();
            serializeV(data, condData);

            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
        case NodeType::DOWHILE:
        {
            vector<char> condData = cond->Serialize();
            serializeV(data, condData);

            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
        case NodeType::FOR:
        {
            serialize(bin, data, nodes.size(), int);
            for(int i = 0; i < nodes.size(); i++)
            {
                vector<char> nodeData = nodes[i]->Serialize();
                serializeV(data, nodeData);
            }
            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
        case NodeType::IMPORT:
        {
            serialize(bin, data, _bool, bool);
            serialize(bin, data, nodes.size(), int);
            for(int i = 0; i < nodes.size(); i++)
            {
                vector<char> nodeData = nodes[i]->Serialize();
                serializeV(data, nodeData);
                serializeS(bin, data, nodes[i]->_string);
                serializeS(bin, data, nodes[i]->_nick);
            }
        }
            break;
        case NodeType::CLASS:
        {
            serializeS(bin, data, _string);
            serialize(bin, data, vars.size(), int);
            for(int i = 0; i < vars.size(); i++)
            {
                serializeS(bin, data, vars[i]);
            }
            vector<char> bodyData = body->Serialize();
            serializeV(data, bodyData);
        }
            break;
        case NodeType::CONTEXT:
        {
            serialize(bin, data, nodes.size(), int);
            for(int i = 0; i < nodes.size(); i++)
            {
                vector<char> nodeData = nodes[i]->Serialize();
                serializeV(data, nodeData);
            }
        }
            break;
        default:
        {
            serialize(bin, data, -1, int);
        }
    }
    return data;
}

void Node_st::Deserialize(std::vector<char>& data)
{
    binary bin;

    deserialize(bin, data, type, int, NodeType::Types);

    switch(type)
    {
        case NodeType::IGNORE:
            break;
        case NodeType::ERROR:
            deserializeS(bin, data, _string);
            break;
        case NodeType::NONE:
            break;
        case NodeType::VARIABLE:
            deserializeS(bin, data, _string);
            break;
        case NodeType::BOOL:
            deserialize(bin, data, _bool, bool, bool);
            break;
        case NodeType::NUMBER:
            deserialize(bin, data, _number, double, double);
            break;
        case NodeType::STRING:
            deserializeS(bin, data, _string);
            break;
        case NodeType::ARRAY:
        {
            int sz;
            deserialize(bin, data, sz, int, int);
            nodes.resize(sz);
            for(int i = 0; i < nodes.size(); i++)
            {
                nodes[i] = MKNODE();
                nodes[i]->Deserialize(data);
            }
        }
            break;
        case NodeType::DICT:
        {
            int sz;
            deserialize(bin, data, sz, int, int);
            nodes.resize(sz);
            for(int i = 0; i < nodes.size(); i++)
            {
                nodes[i] = MKNODE();
                nodes[i]->left = MKNODE();
                nodes[i]->right = MKNODE();
                
                nodes[i]->left->Deserialize(data);
                nodes[i]->right->Deserialize(data);
            }
        }
            break;
         case NodeType::INDEX:
         {
            left = MKNODE();
            left->Deserialize(data);
            int sz;
            deserialize(bin, data, sz, int, int);
            nodes.resize(sz);
            for(int i = 0; i < nodes.size(); i++)
            {
                nodes[i] = MKNODE();
                nodes[i]->Deserialize(data);
            }
         }
            break;
        case NodeType::RETURN:
        {
            body = MKNODE();
            body->Deserialize(data);
        }
            break;
        case NodeType::FUNCTION:
        {
            deserializeS(bin, data, _string);
            int sz;
            deserialize(bin, data, sz, int, int);
            vars.resize(sz);
            for(int i = 0; i < vars.size(); i++)
            {
                deserializeS(bin, data, vars[i]);
            }
            body = MKNODE();
            body->Deserialize(data);
        }
            break;
        case NodeType::FUNCTION_DEF:
        {
            deserializeS(bin, data, _string);
            int sz;
            deserialize(bin, data, sz, int, int);
            vars.resize(sz);
            for(int i = 0; i < vars.size(); i++)
            {
                deserializeS(bin, data, vars[i]);
            }
            deserializeS(bin, data, _nick);
        }
            break;
        case NodeType::EXTENSION:
        {
            deserializeS(bin, data, _nick);
            deserializeS(bin, data, _string);
            int sz;
            deserialize(bin, data, sz, int, int);
            vars.resize(sz);
            for(int i = 0; i < vars.size(); i++)
            {
                deserializeS(bin, data, vars[i]);
            }
            body = MKNODE();
            body->Deserialize(data);
        }
            break;
         case NodeType::TRY:
         {
            body = MKNODE();
            body->Deserialize(data);
            bool hasContr;
            deserialize(bin, data, hasContr, bool, bool);
            if(hasContr)
            {
                deserializeS(bin, data, _string); 
                contr = MKNODE();
                contr->Deserialize(data);
            }
         }
            break;
        case NodeType::LAMBDA:
        {
            int sz;
            deserialize(bin, data, sz, int, int);
            vars.resize(sz);
            for(int i = 0; i < vars.size(); i++)
            {
                deserializeS(bin, data, vars[i]);
            }
            body = MKNODE();
            body->Deserialize(data);
        }
            break;
        case NodeType::LET:
        {
            int sz;
            deserialize(bin, data, sz, int, int);
            nodes.resize(sz);
            for(int i = 0; i < nodes.size(); i++)
            {
                nodes[i] = MKNODE();
                nodes[i]->Deserialize(data);
            }
            body = MKNODE();
            body->Deserialize(data);
        }
            break;
        case NodeType::CALL:
        {
            deserialize(bin, data, createNew, bool, bool);

            func = MKNODE();
            func->Deserialize(data);

            int sz;
            deserialize(bin, data, sz, int, int);
            nodes.resize(sz);
            for(int i = 0; i < nodes.size(); i++)
            {
                nodes[i] = MKNODE();
                nodes[i]->Deserialize(data);
            }
        }
            break;
        case NodeType::ASSIGN:
        {
            left = MKNODE();
            left->Deserialize(data);

            right = MKNODE();
            right->Deserialize(data);
        }
            break;
        case NodeType::BINARY:
        {
            deserializeS(bin, data, _string);
            bool hasLeft, hasMiddle, hasRight;
            deserialize(bin, data, hasLeft, bool, bool);
            deserialize(bin, data, hasMiddle, bool, bool);
            deserialize(bin, data, hasRight, bool, bool);

            if(hasLeft)
            {
                left = MKNODE();
                left->Deserialize(data);
            }
            if(hasMiddle)
            {
                middle = MKNODE();
                middle->Deserialize(data);
            }
            if(hasRight)
            {
                right = MKNODE();
                right->Deserialize(data);
            }
        }
            break;
        case NodeType::IF:
        {
            cond = MKNODE();
            cond->Deserialize(data);

            then = MKNODE();
            then->Deserialize(data);

            bool hasContr;
            deserialize(bin, data, hasContr, bool, bool);

            if(hasContr)
            {
                contr = MKNODE();
                contr->Deserialize(data);
            }
        }
            break;
        case NodeType::WHILE:
        {
            cond = MKNODE();
            cond->Deserialize(data);

            body = MKNODE();
            body->Deserialize(data);
        }
            break;
        case NodeType::DOWHILE:
        {
            cond = MKNODE();
            cond->Deserialize(data);

            body = MKNODE();
            body->Deserialize(data);
        }
            break;
        case NodeType::FOR:
        {
            int sz;
            deserialize(bin, data, sz, int, int);
            nodes.resize(sz);
            for(int i = 0; i < nodes.size(); i++)
            {
                nodes[i] = MKNODE();
                nodes[i]->Deserialize(data);
            }

            body = MKNODE();
            body->Deserialize(data);
        }
            break;
        case NodeType::IMPORT:
        {
            deserialize(bin, data, _bool, bool, bool);
            int sz;
            deserialize(bin, data, sz, int, int);
            nodes.resize(sz);
            for(int i = 0; i < nodes.size(); i++)
            {
                nodes[i] = MKNODE();
                nodes[i]->Deserialize(data);
                deserializeS(bin, data, nodes[i]->_string);
                deserializeS(bin, data, nodes[i]->_nick);
            }
        }
            break;
        case NodeType::CLASS:
        {
            deserializeS(bin, data, _string);
            int sz;
            deserialize(bin, data, sz, int, int);
            vars.resize(sz);
            for(int i = 0; i < vars.size(); i++)
            {
                deserializeS(bin, data, vars[i]);
            }
            body = MKNODE();
            body->Deserialize(data);
        }
            break;
        case NodeType::CONTEXT:
        {
            int sz;
            deserialize(bin, data, sz, int, int);
            nodes.resize(sz);
            for(int i = 0; i < nodes.size(); i++)
            {
                nodes[i] = MKNODE();
                nodes[i]->Deserialize(data);
            }
        }
            break;
        default:
        {
            int df;
            deserialize(bin, data, df, int, int);
        }
    }    
}