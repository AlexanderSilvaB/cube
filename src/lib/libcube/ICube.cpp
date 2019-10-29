#include "ICube.h"
#include <iostream>
#include <algorithm>

using namespace std;

ICube::ICube()
{
    rootEnv = make_shared<Env>();
    exit = false;
    exitCode = 0;
    canPrint = true;

    directFunctions.push_back("color");
    directFunctions.push_back("del");
}

ICube::~ICube()
{

}

int ICube::getExitCode()
{
    return exitCode;
}

bool ICube::run(const string& src, bool cmd)
{
    Node root = parser.Parse(src);

    canPrint = true;
    Object* obj = run(root, rootEnv);
    if(cmd)
    {   
        if(canPrint)
            cout << obj->printable();
        cout << endl;
    }
    VM::release(obj);

    VM::gc();

    return !exit;
}

void ICube::allowPrint(bool allow)
{
    canPrint = allow;
}

Object* ICube::run(Node node, EnvPtr env, Object* caller)
{
    if(!node)
    {
        Object* obj = VM::create();
        VM::push(obj);
        obj->toException("Invalid node");
        return obj;
    }
    
    switch(node->type)
    {
        case NodeType::CONTEXT:
        {
            allowPrint(false);
            Object* obj = VM::create();
            VM::push(obj);
            for(int i = 0; i < node->nodes.size(); i++)
            {
                node->nodes[i]->parent = node;
                node->nodes[i]->parentIndex = i;
                VM::release(obj);
                obj = run(node->nodes[i], env);
                if(obj->isReturned())
                    break;
            }
            return obj;
        }
        break;
        case NodeType::OBJECT:
        {
            Object* obj = VM::create();
            VM::push(obj);
            obj->toObject();
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::NONE:
        {
            Object* obj = VM::create();
            VM::push(obj);
            obj->toNone();
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::BOOL:
        {
            Object* obj = VM::create();
            VM::push(obj);
            obj->toBool(node->_bool);
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::NUMBER:
        {
            Object* obj = VM::create();
            VM::push(obj);
            obj->toNumber(node->_number);
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::STRING:
        {
            Object* obj = VM::create();
            VM::push(obj);
            obj->toString(node->_string);
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::ARRAY:
        {
            Object* obj = VM::create();
            VM::push(obj);

            obj->toObject();

            Array array(node->nodes.size(), NULL);
            int i = 0;
            for(i = 0; i < node->nodes.size(); i++)
            {
                node->nodes[i]->parent = node;
                node->nodes[i]->parentIndex = i;
                array[i] = run(node->nodes[i], env);
                if(array[i]->is(ObjectTypes::EXCEPTION))
                {
                    obj->from(array[i]);
                    break;
                }
            }

            if(!obj->is(ObjectTypes::EXCEPTION))
                obj->toArray(array);
                

            for(i = 0; i < array.size(); i++)
            {
                if(array[i] != NULL)
                    VM::release(array[i]);
            }
            
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::DICT:
        {
            Object* obj = VM::create();
            VM::push(obj);

            obj->toObject();

            Map map;
            int i = 0;
            for(i = 0; i < node->nodes.size(); i++)
            {
                node->nodes[i]->parent = node;
                node->nodes[i]->parentIndex = i;
                map[ node->nodes[i]->left->_string ] = run(node->nodes[i]->right, env);
                if(map[ node->nodes[i]->left->_string ]->is(ObjectTypes::EXCEPTION))
                {
                    obj->from( map[ node->nodes[i]->left->_string ] );
                    break;
                }
            }

            if(!obj->is(ObjectTypes::EXCEPTION))
                obj->toDict(map);

            for(Map::iterator it = map.begin(); it != map.end(); it++)
                VM::release(it->second);
            
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::FUNCTION:
        {
            Object* obj = VM::create();
            VM::push(obj);
            obj->toFunc(node);
            env->set(node->_string, obj, true);

            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::LAMBDA:
        {
            Object* obj = VM::create();
            VM::push(obj);
            obj->toFunc(node);

            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::EXTENSION:
        {
            Object* obj = VM::create();
            VM::push(obj);
            ObjectTypes type = Object::getTypeByName(node->_nick);
            string name = node->_string;
            obj->toFunc(node);
            Object::registerGlobal(type, name, obj);

            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::FUNCTION_DEF:
        {
            if(caller == NULL)
            {
                Object* obj = VM::create();
                VM::push(obj);
                obj->toException("Cannot create a function definition in this context (just native context)");
                allowPrint(true);
                return obj;
            }
            else
            {
                Object* obj = VM::create();
                VM::push(obj);
                StringArray names(node->vars.size());
                for(int i = 0; i < node->vars.size(); i++)
                    names[i] = node->vars[i];
                obj->toFuncDef(caller->handler(), node->_string, node->_nick, names);
                caller->array().push_back(obj);
                allowPrint(true);
                return caller;
            }
        }
        break;
        case NodeType::VARIABLE:
        {
            Object* obj = env->get(node->_string);
            if(obj == NULL)
            {
                obj = VM::create();
                VM::push(obj);
                obj->toException("Undefined variable '" + node->_string + "'");
            }
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::RETURN:
        {
            Object* obj = run(node->body, env);
            if(!obj->is(ObjectTypes::EXCEPTION))
                obj->isReturned(true);
            return obj;
        }
        break;
        case NodeType::ASSIGN:
        {
            Object* obj = VM::create();
            VM::push(obj);
            if(node->left->type == NodeType::VARIABLE)
            {
                Object* value = run(node->right, env);
                if(!value->is(ObjectTypes::EXCEPTION))
                {
                    if(value->isSaved())
                    {
                        Object *newValue = VM::create();
                        VM::push(newValue);
                        newValue->from(value);
                        value = newValue;
                    }
                    env->set(node->left->_string, value, false);
                    VM::release(obj);
                    obj = value;
                }
                else  
                {
                    obj->from(value);
                }
            }
            obj->isReturned(false);
            return obj;
        }
        break;
        case NodeType::IF:
        {
            Object* obj = VM::create();
            VM::push(obj);

            Object *cond = run(node->cond, env);
            if(cond->is(ObjectTypes::EXCEPTION))
            {
                obj->from(cond);
                VM::release(cond);
            }
            else if(cond->isTrue())
            {
                VM::release(obj);
                obj = run(node->then, env);
            }
            else if(node->contr)
            {
                VM::release(obj);
                obj = run(node->contr, env);
            }
            allowPrint(false);
            return obj;
        }
        break;
        case NodeType::TRY:
        {
            Object* obj = VM::create();
            VM::push(obj);

            EnvPtr tryEnv = env->copy();
            Object *body = run(node->body, tryEnv);
            if(!body->is(ObjectTypes::EXCEPTION))
            {
                env->paste(tryEnv);
                VM::release(obj);
                obj = body;
            }
            else if(node->contr)
            {
                tryEnv = env->copy();
                if(node->_string.size() > 0)
                    tryEnv->set(node->_string, body, true);
                Object *contr = run(node->contr, tryEnv);
                if(contr->is(ObjectTypes::EXCEPTION))
                {
                    VM::release(obj);
                    obj = contr;
                }
                VM::release(body);
            }
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::LET:
        {
            Object* obj = VM::create();
            VM::push(obj);


            EnvPtr letEnv = env->extend();
            bool valid = true;
            for(int i = 0; i < node->nodes.size(); i++)
            {
                VM::release(obj);
                obj = run(node->nodes[i], letEnv);
                if(obj->is(ObjectTypes::EXCEPTION))
                {
                    valid = false;
                    break;
                }
            }
            if(valid)
            {
                VM::release(obj);
                obj = run(node->body, letEnv);
            }
            allowPrint(true);
            return obj;
        }
        break;
        case NodeType::WHILE:
        {
            Object* obj = VM::create();
            VM::push(obj);

            EnvPtr whileEnv = env->extend();
            Object* cond = run(node->cond, whileEnv);
            if(cond->isFalse())
            {
                VM::release(obj);
                obj = cond;
            }
            while(cond->isTrue())
            {
                VM::release(obj);
                obj = run(node->body, whileEnv);
                if(obj->is(ObjectTypes::EXCEPTION))
                {
                    break;
                }

                VM::release(cond);
                cond = run(node->cond, whileEnv);
                if(cond->is(ObjectTypes::EXCEPTION))
                {
                    VM::release(obj);
                    obj = cond;
                    break;
                }
            }
            allowPrint(false);
            return obj;
        }
        break;
        case NodeType::DOWHILE:
        {
            Object* obj = VM::create();
            VM::push(obj);

            EnvPtr whileEnv = env->extend();
            Object *cond = NULL;
            do
            {
                VM::release(obj);
                obj = run(node->body, whileEnv);
                if(obj->is(ObjectTypes::EXCEPTION))
                {
                    break;
                }
                if(cond != NULL)
                    VM::release(cond);
                cond = run(node->cond, whileEnv);
                if(cond->is(ObjectTypes::EXCEPTION))
                {
                    VM::release(obj);
                    obj = cond;
                    break;
                }
            }while(cond->isTrue());
            allowPrint(false);
            return obj;
        }
        break;
        case NodeType::FOR:
        {
            Object* obj = VM::create();
            VM::push(obj);

            if(node->nodes.size() == 0 || node->nodes.size() > 3)
            {
                obj->toException("Invalid for arguments for 'for' loop");
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
                EnvPtr forEnv = env->extend();
                bool valid = true;
                Object *inVar = NULL;
                int inIndex = 0;
                Object *parts = NULL;
                if(in)
                {
                    if(in->left->type != NodeType::VARIABLE)
                    {
                        obj->toException("Invalid type for 'in' operator");
                        valid = false;
                    }
                    else
                    {
                        Object *right = run(in->right, forEnv);
                        if(right->is(ObjectTypes::EXCEPTION))
                        {
                            VM::release(obj);
                            obj = right;
                            valid = false;
                        }
                        else
                        {
                            parts = VM::create();
                            VM::push(parts);
                            right->split(parts);

                            if(parts->array().size() > 0)
                            {
                                inVar = parts->array()[inIndex];
                                forEnv->set(in->left->_string, inVar, true);
                                inIndex++;
                            }
                            else
                            {
                                VM::release(parts);
                                obj->toObject();
                                valid = false;
                            }
                        }
                    }
                }
                else
                {
                    Object *st = run(start, forEnv);
                    if(st->is(ObjectTypes::EXCEPTION))
                    {
                        VM::release(obj);
                        obj = st;
                        valid = false;
                    }
                    else
                    {
                        if(cond)
                        {
                            Object *cd = run(cond, forEnv);
                            if(cd->is(ObjectTypes::EXCEPTION))
                            {
                                VM::release(obj);
                                obj = cd;
                                valid = false;
                            }
                            else if(cd->isFalse())
                            {
                                VM::release(cd);
                                obj->toObject();
                                valid = false;
                            }
                        }
                    }
                }
                if(valid)
                {
                    do
                    {
                        VM::release(obj);
                        obj = run(node->body, forEnv);
                        if(obj->is(ObjectTypes::EXCEPTION))
                        {
                            break;
                        }
                        if(in)
                        {
                            if(inIndex < parts->array().size())
                            {
                                inVar = parts->array()[inIndex];
                                forEnv->set(in->left->_string, inVar, true);
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
                                Object *ic = run(inc, forEnv);
                                if(ic->is(ObjectTypes::EXCEPTION))
                                {
                                    VM::release(obj);
                                    obj = ic;
                                    valid = false;
                                }
                            }

                            if(cond)
                            {
                                Object *cd = run(cond, forEnv);
                                if(cd->is(ObjectTypes::EXCEPTION))
                                {
                                    VM::release(obj);
                                    obj = cd;
                                    valid = false;
                                }
                                else if(cd->isFalse())
                                {
                                    //var->SetType(VarType::IGNORE);
                                    valid = false;
                                }
                            }
                        }
                    }while(valid);
                }

                if(parts != NULL)
                    VM::release(parts);
            }
            allowPrint(false);
            return obj;
        }
        break;
        case NodeType::CALL:
        {
            Object* obj = VM::create();
            VM::push(obj);
            
            if(node->func->type == NodeType::VARIABLE)
            {
                Array args(node->nodes.size(), NULL);
                bool valid = true;
                for(int i = 0; i < node->nodes.size(); i++)
                {
                    if(find(directFunctions.begin(), directFunctions.end(), node->func->_string) != directFunctions.end())
                    {
                        args[i] = VM::create();
                        args[i]->toString(node->nodes[i]->_string);
                    }
                    else
                    {
                        args[i] = run(node->nodes[i], env);
                        if(args[i]->is(ObjectTypes::EXCEPTION))
                        {
                            obj->from(args[i]);
                            valid = false;
                            break;
                        }
                    }
                }

                if(valid)
                {
                    VM::release(obj);
                    obj = call(node->func->_string, args, env, caller, node->createNew);
                }
                
                for(int i = 0; i < args.size(); i++)
                {
                    if(args[i] != NULL)
                        VM::release(args[i]);
                }
            }
            else
            {
                obj->toException("Cannot call this like a function [" + node->func->ToString() + "]");
            }
            return obj;
        }
        break;
        default:
            cout << "Unsuported node: " << endl;
            cout << node->ToString() << endl;
            Object* obj = VM::create();
            VM::push(obj);
            return obj;
            break;
    }

    return NULL;
}

Object* ICube::callCallable(Object* obj, Array args)
{
    if(!obj->isCallable())
        return obj;
    
}

Object* ICube::call(const std::string& func, Array& args, EnvPtr env, Object* caller, bool createObj)
{
    Object* res = VM::create();
    VM::push(res);
    if(func == "print" || func == "println")
    {
        bool success;
        Object* obj = VM::create();
        VM::push(obj);
        for(int i = 0; i < args.size(); i++)
        {
            res->from(args[i]);
            string str = "";
            success = args[i]->call(obj, "toString");
            if(success)
            {
                if(obj->isCallable())
                {
                    Object* result = callCallable(obj);
                    str = result->printable();
                    VM::release(result);
                }
                else
                {
                    str = obj->printable();
                }
            }
            else
                str = args[i]->printable();
            cout << str;
            if(i < args.size()-1 && str[0] != '\033')
                cout << " ";
        }
        if(func == "println")
        {
            cout << "\033[0m" << endl;
        }
        VM::release(obj);
        allowPrint(false);
    }
    else if(func == "input")
    {
        for(int i = 0; i < args.size(); i++)
        {
            cout << args[i]->printable();
            if(i < args.size()-1)
                cout << " ";
        }
        string str;
        getline(cin, str);
        res->toString(str);
        allowPrint(true);
    }
    else if(func == "del")
    {
        for(int i = 0; i < args.size(); i++)
        {
            env->del(args[i]->str());
        }
        res->toNone();
        allowPrint(false);
    }
    // else if(func == "color")
    // {
    //     stringstream ss;
    //     ss << "\033[0;";
    //     if(args.size() > 0)
    //     {
    //         if(args[0]->IsType(VarType::STRING) && IsColor(args[0]->String()))
    //         {
    //             ss << colors[args[0]->String()];
    //         }
    //         else if(args[0]->IsType(VarType::NUMBER))
    //         {
    //             ss << (int)args[0]->Number();
    //         }
    //         else
    //         {
    //             MakeError(res, "Invalid foreground color");
    //             return res;
    //         }
    //     }
    //     if(args.size() > 1)
    //     {
    //         if(args[1]->IsType(VarType::STRING) && IsColor(args[1]->String()))
    //         {
    //             ss << ";" << (colors[args[1]->String()] + 10);
    //         }
    //         else if(args[1]->IsType(VarType::NUMBER))
    //         {
    //             ss << ";" << (int)args[1]->Number();
    //         }
    //         else
    //         {
    //             MakeError(res, "Invalid background color");
    //             return res;
    //         }
    //     }
    //     ss << "m";
    //     *res = ss.str();
    // }
    // else if(func == "date")
    // {
    //     string format = "%c";
    //     if(args.size() > 0)
    //     {
    //         if(args[0]->IsType(VarType::STRING))
    //         {
    //             format = args[0]->String();
    //         }
    //         else
    //         {
    //             MakeError(res, "Invalid date format");
    //             return res;
    //         }
    //     }
    //     ReplaceString(format, "YYYY", "%Y");
    //     ReplaceString(format, "YY", "%y");
    //     ReplaceString(format, "MM", "%m");
    //     ReplaceString(format, "mm", "%M");
    //     ReplaceString(format, "ss", "%S");
    //     ReplaceString(format, "hh", "%I");
    //     ReplaceString(format, "HH", "%H");
    //     ReplaceString(format, "dd", "%d");
    //     ReplaceString(format, "D", "%e");

    //     time_t rawtime;
    //     struct tm * timeinfo;
    //     char buffer [256];
    //     time (&rawtime);
    //     timeinfo = localtime (&rawtime);
    //     strftime (buffer, 256,format.c_str(),timeinfo);
    //     string date(buffer);
    //     *res = date;
    // }
    else if(func == "env")
    {
        env->toDict(res);
        allowPrint(true);
    }
    else if(func == "type")
    {
        if(args.size() == 0)
            res->toNone();
        else if(args.size() == 1)
        {
            res->toString(args[0]->getTypeName());
        }
        else
        {
            Array array(args.size());
            for(int i = 0; i < args.size(); i++)
            {
                array[i] = VM::create();
                array[i]->toString(args[i]->getTypeName());
            }
            res->toArray(array);
        }
        allowPrint(true);
    }
    // else if(func == "bool")
    // {
    //     if(args.size() == 0)
    //     {
    //         res->operator=(false);
    //     }
    //     else if(args.size() == 1)
    //     {
    //         Var* v = CallInternOrReturn("toBool", args[0]);
    //         *res = v->AsBool();
    //     }
    //     else
    //     {
    //         VarArray _array(args.size());
    //         for(int i = 0; i < args.size(); i++)
    //         {
    //             Var* v = CallInternOrReturn("toBool", args[i]);
    //             _array[i] = v->AsBool();
    //         }
    //         *res = _array;
    //     }
    // }
    // else if(func == "number")
    // {
    //     if(args.size() == 0)
    //     {
    //         res->operator=(0.0);
    //     }
    //     else if(args.size() == 1)
    //     {
    //         Var* v = CallInternOrReturn("toNumber", args[0]);
    //         *res = v->AsNumber();
    //     }
    //     else
    //     {
    //         VarArray _array(args.size());
    //         for(int i = 0; i < args.size(); i++)
    //         {
    //             Var* v = CallInternOrReturn("toNumber", args[i]);
    //             _array[i] = v->AsNumber();
    //         }
    //         *res = _array;
    //     }
    // }
    // else if(func == "string")
    // {
    //     if(args.size() == 0)
    //     {
    //         string str = "";
    //         res->operator=(str);
    //     }
    //     else if(args.size() == 1)
    //     {
    //         Var* v = CallInternOrReturn("toString", args[0]);
    //         *res = v->AsString();
    //     }
    //     else
    //     {
    //         VarArray _array(args.size());
    //         for(int i = 0; i < args.size(); i++)
    //         {
    //             Var* v = CallInternOrReturn("toString", args[i]);
    //             _array[i] = v->AsString();
    //         }
    //         *res = _array;
    //     }
    // }
    // else if(func == "array")
    // {
    //     if(args.size() == 0)
    //     {
    //         VarArray _array;
    //         res->operator=(_array);
    //     }
    //     else if(args.size() == 1)
    //     {
    //         Var* v = CallInternOrReturn("toArray", args[0]);
    //         *res = v->AsArray();
    //     }
    //     else
    //     {
    //         VarArray _array(args.size());
    //         for(int i = 0; i < args.size(); i++)
    //         {
    //             Var* v = CallInternOrReturn("toArray", args[i]);
    //             _array[i] = v->AsArray();
    //         }
    //         *res = _array;
    //     }
    // }
    // else if(func == "int")
    // {
    //     if(args.size() == 0)
    //     {
    //         res->operator=(0.0);
    //     }
    //     else if(args.size() == 1)
    //     {
    //         Var* v = CallInternOrReturn("toNumber", args[0]);
    //         *res = v->AsNumber();
    //         *res = (double)((int)res->Number());
    //     }
    //     else
    //     {
    //         VarArray _array(args.size());
    //         for(int i = 0; i < args.size(); i++)
    //         {
    //             Var* v = CallInternOrReturn("toNumber", args[i]);
    //             _array[i] = v->AsNumber();
    //             _array[i] = (double)((int)_array[i].Number());
    //         }
    //         *res = _array;
    //     }
    // }
    // else if(func == "len")
    // {
    //     if(args.size() == 0)
    //     {
    //         res->operator=(0.0);
    //     }
    //     else if(args.size() == 1)
    //     {
    //         Var* v = CallInternOrReturn("len", args[0]);
    //         if(v == args[0])
    //             *res = (double)args[0]->Size();
    //         else
    //             *res = v->AsNumber();
    //     }
    //     else
    //     {
    //         VarArray _array(args.size());
    //         for(int i = 0; i < args.size(); i++)
    //         {
    //             Var* v = CallInternOrReturn("len", args[i]);
    //             if(v == args[i])
    //                 _array[i] = (double)args[i]->Size();
    //             else
    //                 _array[i] = v->AsNumber();
    //         }
    //         *res = _array;
    //     }
    // }
    else if(func == "exit")
    {
        exitCode = 0;
        exit = true;
        if(args.size() > 0)
        {
            if(args[0]->is(ObjectTypes::NUMBER))
                exitCode = (int)args[0]->number();
        }
        res->isReturned(true);
        allowPrint(false);
    }
    // else if(func == "instr")
    // {
    //     printInstr = true;
    // }
    // else
    // {
    //     Var* var = NULL;
    //     if(!createObj)
    //         var = env->get(func);
    //     else
    //         var = env->get(func, VarType::CLASS);
    //     if(var->IsType(VarType::ERROR))
    //     {
    //         delete res;
    //         return var;
    //     }
    //     if(!var->IsType(VarType::FUNC) && !var->IsType(VarType::CLASS))
    //     {
    //         delete var;
    //         stringstream ss;
    //         ss << "'" << func << "' is not callable";
    //         MakeError(res, ss.str());
    //     }
    //     else if(var->IsType(VarType::CLASS))
    //     {
    //         if(var->Counter() != 0)
    //         {
    //             MakeError(res, "Objects cannot be called like classes");
    //         }
    //         else
    //         {
    //             Var *obj = var->Clone();
    //             var = obj;
    //             var->Context()->setParent(env);
    //             if(var->Context()->get(func)->IsType(VarType::FUNC))
    //             {
    //                 Var *res = Call(func, args, var->Context(), var);
    //                 if(res->IsType(VarType::ERROR))
    //                 {
    //                     *var = *res;
    //                 }
    //             }
    //             res = var;
    //         }
    //     }
    //     else
    //     {
    //         EnvPtr funcEnv = env->extend();
    //         if(caller != NULL)
    //         {
    //             funcEnv->def("this", caller);
    //         }
    //         Node _func = var->Func();
    //         VarArray _array(args.size());
    //         int limit = max(_func->vars.size(), args.size());
    //         for(int i = 0; i < limit; i++)
    //         {
    //             if(i < args.size())
    //             {
    //                 if(i < _func->vars.size())
    //                     funcEnv->def(_func->vars[i], args[i]);
    //                 _array[i] = *args[i];
    //             }
    //             else
    //             {
    //                 Var *none = MKVAR();
    //                 none->SetType(VarType::NONE);
    //                 funcEnv->def(_func->vars[i], none);
    //                 delete none;
    //             }
    //         }
    //         Var *_args = MKVAR();
    //         *_args = _array;
    //         funcEnv->def("args", _args);
    //         Var *_res = Evaluate(_func->body, funcEnv);
    //         *res = *_res;
    //     }
    // }
    return res;
}