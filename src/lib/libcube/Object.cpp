#include "Object.h"
#include "VM.h"
#include <sstream>
#include <iostream>
#include <cmath>

using namespace std;

FunctionMap Object::globalFunctions;
ExtensionMap Object::globalExtensions;

Object::Object()
{
    marked = false;
    next = NULL;
    saved = false;
    returned = false;

    this->type = OBJECT;
}

Object::~Object()
{

}

void Object::save()
{
    saved = true;
}

void Object::release()
{
    saved = false;
}

bool Object::isSaved()
{
    return saved;
}

void Object::isReturned(bool returned)
{
    this->returned = returned;
}

bool Object::isReturned()
{
    return this->returned;
}

void Object::setNext(Object* obj)
{
    next = obj;
}

Object* Object::getNext()
{
    return next;
}

void Object::mark()
{
    if(marked)
        return;

    marked = true;
    for(int i = 0; i < value.array.size(); i++)
    {
        value.array[i]->mark();
    }
    for(Map::iterator it = value.map.begin(); it != value.map.end(); it++)
    {
        it->second->mark();
    }
}

void Object::unmark()
{
    if(!marked)
        return;

    marked = false;
    for(int i = 0; i < value.array.size(); i++)
    {
        value.array[i]->unmark();
    }
    for(Map::iterator it = value.map.begin(); it != value.map.end(); it++)
    {
        it->second->unmark();
    }
}

bool Object::isMarked()
{
    return marked;
}

// Built-ins
bool Object::is(ObjectTypes type)
{
    return type == this->type;
}

ObjectTypes Object::getType()
{
    return type;
}

string Object::getTypeName()
{
    string typeName = "object";
    switch(type)
    {
        case OBJECT:
            typeName = "object";
            break;
        case NONE:
            typeName = "none";
            break;
        case BOOL:
            typeName = "bool";
            break;
        case NUMBER:
            typeName = "number";
            break;
        case STRING:
            typeName = "string";
            break;
        case ARRAY:
            typeName = "array";
            break;
        case DICT:
            typeName = "dict";
            break;
        case FUNC:
            typeName = "func";
            break;
        case DEF:
            typeName = "def";
            break;
        case CLASS:
            typeName = "class";
            break;
        case CLASSOBJ:
            typeName = "object(" + value._string + ")";
            break;
        case LIB:
            typeName = "lib";
            break;
        case EXCEPTION:
            typeName = "exception";
            break;
        default:
            break;
    }
    return typeName;
}

ObjectTypes Object::getTypeByName(const std::string& typeName)
{
    ObjectTypes type = OBJECT;
    if(typeName == "object")
        type = OBJECT;
    else if(typeName == "none")
        type = NONE;
    else if(typeName == "bool")
        type = BOOL;
    else if(typeName == "number")
        type = NUMBER;
    else if(typeName == "string")
        type = STRING;
    else if(typeName == "array")
        type = ARRAY;
    else if(typeName == "dict")
        type = DICT;
    else if(typeName == "func")
        type = FUNC;
    else if(typeName == "def")
        type = DEF;
    else if(typeName == "class")
        type = CLASS;
    else if(typeName == "lib")
        type = LIB;
    else if(typeName == "exception")
        type = EXCEPTION;
    return type;
}

bool Object::isFalse()
{
    return (this->type == ObjectTypes::NONE || 
        this->type == ObjectTypes::OBJECT || 
        this->type == ObjectTypes::EXCEPTION || 
        (this->type == ObjectTypes::BOOL && this->value._bool == false) ||
        (this->type == ObjectTypes::NUMBER && this->value._number == 0) ||
        (this->type == ObjectTypes::STRING && this->value._string.size() == 0) ||
        (this->type == ObjectTypes::ARRAY && this->value.array.size() == 0) ||
        (this->type == ObjectTypes::DICT && this->value.map.size() == 0));
}

bool Object::isTrue()
{
    return !isFalse();
}

bool Object::isCallable()
{
    // In the future this check can be improved
    if(is(FUNC))
        return true;
    return false;
}

void Object::registerGlobal(ObjectTypes type, const std::string& name, ObjectFunction func)
{
    globalFunctions[type][name] = func;
}

void Object::registerGlobal(ObjectTypes type, const std::string& name, Object* func)
{
    globalExtensions[type][name] = func;
    func->save();
}

void Object::registerLocal(const std::string& name, Object* func)
{
    localFunctions[name] = func;
    func->save();
}

string Object::printable()
{
    stringstream ss;
    switch(type)
    {
        case OBJECT:
            ss << "object";
            break;
        case NONE:
            ss << "none";
            break;
        case BOOL:
            ss << (value._bool ? "true" : "false");
            break;
        case NUMBER:
            ss << value._number;
            break;
        case STRING:
            ss << value._string;
            break;
        case ARRAY:
            ss << "[";
            for(int i = 0; i < value.array.size(); i++)
            {
                ss << value.array[i]->printable();
                if(i < value.array.size()-1)
                    ss << ", ";
            }
            ss << "]";
            break;
        case DICT:
        {
            int i = 0; 
            ss << "[";
            for(Map::iterator it = value.map.begin(); it != value.map.end(); it++, i++)
            {
                ss << it->first << " = " << it->second->printable();
                if(i < value.map.size()-1)
                    ss << ", ";
            }
            ss << "]";
        }
            break;
        case FUNC:
            ss << "func(" << value._string << ")";
            break;
        case DEF:
            ss << "def(" << value._string << ")";
            break;
        case CLASS:
            ss << "class(" << value._string << ")";
            break;
        case CLASSOBJ:
            ss << "object(" + value._string << ")";
            break;
        case LIB:
            ss << "lib(" << value._string << ")";
            break;
        case EXCEPTION:
            ss << "exception(" << value._string << ")";
            break;
        default:
            ss << "unknown()" << endl;
            break;
    }
    return ss.str();
}

bool Object::call(Object* obj, const string& name, Array args)
{
    if(localFunctions.find(name) != localFunctions.end())
    {
        obj->from(localFunctions[name]);
        return true;
    }
    else if(globalFunctions[type].find(name) != globalFunctions[type].end())
    {
        return globalFunctions[type][name](obj, this, args);
    }
    return false;
}

// Internals
string Object::str()
{
    return value._string;
}

double Object::number()
{
    return value._number;
}

Array& Object::array()
{
    return value.array;
}

void* Object::handler()
{
    return value.handler;
}

// Convert methods
void Object::from(Object* obj)
{
    switch(obj->type)
    {
        case OBJECT:
            this->toObject();
            break;
        case NONE:
            this->toNone();
            break;
        case BOOL:
            this->toBool(obj->value._bool);
            break;
        case NUMBER:
            this->toNumber(obj->value._number);
            break;
        case STRING:
            this->toString(obj->value._string);
            break;
        case EXCEPTION:
            this->toException(obj->value._string);
            break;
        case ARRAY:
            this->toArray(obj->value.array);
            break;
        case DICT:
            this->toDict(obj->value.map);
            break;
        case FUNC:
            this->toFunc(obj->value.node);
            break;
        default:
            this->toObject();
            break;
    }
}

void Object::clearInternal()
{
    value.array.clear();
    value.map.clear();
}

void Object::toObject()
{
    this->type = OBJECT;
    clearInternal();
}

void Object::toNone()
{
    this->type = NONE;
    clearInternal();
}

void Object::toBool(bool value)
{
    this->type = BOOL;
    this->value._bool = value;
    clearInternal();
}

void Object::toNumber(double value)
{
    this->type = NUMBER;
    this->value._number = value;
    clearInternal();
}

void Object::toString(const std::string& value)
{
    this->type = STRING;
    this->value._string = value;
    clearInternal();
}

void Object::toException(const std::string& value)
{
    this->type = EXCEPTION;
    this->value._string = value;
    clearInternal();
}

void Object::toArray(const Array& array)
{
    this->type = ARRAY;
    clearInternal();
    this->value.array.resize(array.size());
    for(int i = 0; i < array.size(); i++)
    {
        this->value.array[i] = VM::create();
        this->value.array[i]->from(array[i]);
    }
}

void Object::toDict(Map& map)
{
    this->type = DICT;
    clearInternal();
    for(Map::iterator it = map.begin(); it != map.end(); it++)
    {
        this->value.map[ it->first ] = VM::create();
        this->value.map[ it->first ]->from(it->second);
    }
}

void Object::toFunc(Node node)
{
    this->type = FUNC;
    clearInternal();
    this->value.node = node;
}

void Object::toFuncDef(void* handler, const std::string& name, const std::string& ret, StringArray& vars)
{
    this->type = DEF;
    clearInternal();
    this->value._string = name;
    this->value._returnType = ret;
    this->value.names = vars;
    this->value.handler = handler;
}

void Object::split(Object* obj)
{
    Array array;
    switch(type)
    {
        case BOOL:
        {
            array.resize(1);
            array[0] = VM::create();
            array[0]->toBool(value._bool);
        }
            break;
        case NUMBER:
       {
            array.resize(1);
            array[0] = VM::create();
            array[0]->toNumber(value._number);
        }
            break;
        case STRING:
        {
            string str = value._string;
            array.resize(str.size());
            for(int i = 0; i < str.size(); i++)
            {
                stringstream ss;
                ss << str[i];
                array[i] = VM::create();
                array[i]->toString(ss.str());
            }
        }
            break;
        case ARRAY:
        {
            array = value.array;
        }
            break;
        case DICT:
        {
            array.resize(value.map.size());
            int i = 0;
            for(Map::iterator it = value.map.begin(); it != value.map.end(); it++, i++)
            {
                array[i] = VM::create();
                array[i]->toString(it->first);
            }
        }
            break;
        default:
        {
            array.resize(1);
            array[0] = VM::create();
            array[0]->from(this);
        }
            break;
    }
    obj->toArray(array);
}