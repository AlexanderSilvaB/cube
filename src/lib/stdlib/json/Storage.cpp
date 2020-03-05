#include "Storage.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>

#if defined _MSC_VER
#include <direct.h>
#endif

using namespace std;

static int own_mkdir(const char *dir, int mode)
{
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
#ifdef _WIN32
#if defined _MSC_VER
            _mkdir(tmp);
#else
            mkdir(tmp);
#endif
#else
            mkdir(tmp, mode);
#endif
            *p = '/';
        }
    }
#ifdef _WIN32
#if defined _MSC_VER
    int rc = _mkdir(tmp);
#else
    int rc = mkdir(tmp);
#endif
#else
    int rc = mkdir(tmp, mode);
#endif
    return rc;
}

// Value
Value::Value()
{
    valid = true;
    type = OBJECT;
}

Value::Value(bool value)
{
    valid = true;
    type = BOOL;
    boolValue = value;
}

Value::Value(int value)
{
    valid = true;
    type = INT;
    intValue = value;
    floatValue = (float)value;
}

Value::Value(float value)
{
    valid = true;
    type = FLOAT;
    floatValue = value;
    intValue = (int)value;
}

Value::Value(string value)
{
    valid = true;
    type = STRING;
    stringValue = value;
}

Value::Value(vector<Value> &value)
{
    valid = true;
    type = ARRAY;
    arrayValue = value;
}

Value::Value(map<string, Value> &value)
{
    valid = true;
    type = OBJECT;
    objectValue = value;
}

Value::Value(const Value &value)
{
    valid = true;
    this->operator=(value);
}

Value::Types Value::Type()
{
    return type;
}

bool Value::Bool()
{
    return boolValue;
}

int Value::Int()
{
    return intValue;
}

float Value::Float()
{
    return floatValue;
}

string &Value::String()
{
    return stringValue;
}

vector<Value> &Value::Array()
{
    return arrayValue;
}

map<string, Value> &Value::Object()
{
    return objectValue;
}

void Value::Add(Value &value)
{
    type = ARRAY;
    arrayValue.push_back(value);
}

int Value::Size()
{
    if (type == ARRAY)
        return arrayValue.size();
    else if (type == OBJECT)
        return objectValue.size();
    else if (type == STRING)
        return stringValue.size();
    return -1;
}

bool Value::Parse(string text)
{
    int i = 0;
    this->operator=(parse(text, &i));
    return this->valid;
}

Value Value::parse(string &text, int *j)
{
    trim(text, j);
    Value value;
    if (*j >= text.size())
    {
        value.valid = false;
        return value;
    }
    switch (text[*j])
    {
        case '{':
            value = parseObject(text, j);
            break;
        case '"':
            value = parseString(text, j);
            break;
        case '[':
            value = parseArray(text, j);
            break;
        case 't':
        case 'f':
            value = parseBool(text, j);
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '+':
        case '-':
            value = parseNumber(text, j);
            break;
        default:
            value.valid = false;
            break;
    }
    return value;
}

Value Value::parseBool(string &text, int *i)
{
    trim(text, i);
    Value value;
    if (text[*i] == 't')
    {
        if (text.size() >= *i + 3)
        {
            if (text[*i + 1] == 'r' && text[*i + 2] == 'u' && text[*i + 3] == 'e')
                value = true;
            else
                value.valid = false;
        }
        else
            value.valid = false;
        *i = *i + 4;
    }
    else
    {
        if (text.size() >= *i + 4)
        {
            if (text[*i + 1] == 'a' && text[*i + 2] == 'l' && text[*i + 3] == 's' && text[*i + 4] == 'e')
                value = false;
            else
                value.valid = false;
        }
        else
            value.valid = false;
        *i = *i + 5;
    }
    return value;
}

Value Value::parseString(string &text, int *i)
{
    trim(text, i);
    Value value;
    stringstream ss;
    *i = *i + 1;
    char c;
    bool found = false;
    while (*i < text.size())
    {
        c = text[*i];
        if (c == '"')
        {
            found = true;
            break;
        }
        ss << c;
        *i = *i + 1;
    }
    if (found)
    {
        *i = *i + 1;
        value = ss.str();
    }
    else
        value.valid = false;
    return value;
}

Value Value::parseNumber(string &text, int *i)
{
    trim(text, i);
    Value value;
    stringstream ss;
    char c;
    bool found = false, isFloat = false;
    while (*i < text.size())
    {
        c = text[*i];
        if (c == '.')
        {
            if (!isFloat)
            {
                ss << c;
                isFloat = true;
            }
            else
                break;
        }
        else if ((c >= '0' && c <= '9') || (c == '-' || c == '+'))
        {
            ss << c;
        }
        else if (c == ' ' || c == ']' || c == '}' || c == ',')
        {
            found = true;
            break;
        }
        else
            break;
        *i = *i + 1;
    }
    if (found)
    {
        if (isFloat)
        {
            float v;
            ss >> v;
            value = v;
        }
        else
        {
            int v;
            ss >> v;
            value = v;
        }
    }
    else
    {
        value.valid = false;
    }
    return value;
}

Value Value::parseArray(string &text, int *i)
{
    trim(text, i);
    Value value;
    value.type = ARRAY;
    char c;
    bool done = false;
    *i = *i + 1;
    while (*i < text.size())
    {
        Value v;
        trim(text, i);
        if (text[*i] != ']')
        {
            v = parse(text, i);
            if (!v.valid)
                break;
            value.Add(v);
            trim(text, i);
        }
        if (*i >= text.size())
            break;
        if (text[*i] == ',')
        {
            *i = *i + 1;
            continue;
        }
        else if (text[*i] == ']')
        {
            *i = *i + 1;
            done = true;
            break;
        }
        else
        {
            break;
        }
    }
    if (!done)
    {
        value.valid = false;
    }
    return value;
}

Value Value::parseObject(string &text, int *i)
{
    trim(text, i);
    Value value;
    value.type = OBJECT;
    char c;
    bool done = false;
    *i = *i + 1;
    while (*i < text.size())
    {
        Value key = parseString(text, i);
        if (!key.valid)
            break;
        trim(text, i);
        if (*i >= text.size())
            break;
        if (text[*i] != ':')
            break;
        *i = *i + 1;
        Value v = parse(text, i);
        if (!v.valid)
            break;
        value[key.stringValue] = v;
        trim(text, i);
        if (*i >= text.size())
            break;
        if (text[*i] == ',')
        {
            *i = *i + 1;
            continue;
        }
        else if (text[*i] == '}')
        {
            *i = *i + 1;
            done = true;
            break;
        }
        else
        {
            break;
        }
    }
    if (!done)
    {
        value.valid = false;
    }
    return value;
}

void Value::trim(std::string &text, int *i)
{
    while (*i < text.size())
    {
        if (!(text[*i] == ' ' || text[*i] == '\n' || text[*i] == '\t' || text[*i] == '\r' || text[*i] == '\b'))
            break;
        *i = *i + 1;
    }
}

string Value::ToString()
{
    stringstream ss;
    switch (type)
    {
        case Value::OBJECT: {
            ss << "{ ";
            int size = objectValue.size();
            int i = 0;
            for (map<string, Value>::iterator it = objectValue.begin(); it != objectValue.end(); it++)
            {
                ss << "\"" << it->first << "\" : ";
                ss << it->second.ToString();
                if (i < size - 1)
                    ss << ", ";
                i++;
            }
            ss << " }";
        }
        break;
        case Value::ARRAY: {
            ss << "[ ";
            for (int i = 0; i < arrayValue.size(); i++)
            {
                ss << arrayValue[i].ToString();
                if (i < arrayValue.size() - 1)
                    ss << ", ";
            }
            ss << " ]";
        }
        break;
        case Value::STRING:
            ss << "\"" << stringValue << "\"";
            break;
        case Value::BOOL:
            ss << ((boolValue ? "true" : "false"));
            break;
        case Value::INT:
            ss << intValue;
            break;
        case Value::FLOAT:
            ss << floatValue;
            break;
        default:
            throw;
            break;
    }
    return ss.str();
}

Value &Value::operator[](const char *key)
{
    if (type != OBJECT)
        throw;
    return objectValue[string(key)];
}

Value &Value::operator[](string key)
{
    if (type != OBJECT)
        throw;
    return objectValue[key];
}

Value &Value::operator[](int i)
{
    if (type != ARRAY)
        throw;
    return arrayValue[i];
}

void Value::operator=(const bool &value)
{
    type = BOOL;
    boolValue = value;
}

void Value::operator=(const int &value)
{
    type = INT;
    intValue = value;
    floatValue = (float)value;
}

void Value::operator=(const float &value)
{
    type = FLOAT;
    floatValue = value;
    intValue = (int)value;
}

void Value::operator=(const string &value)
{
    type = STRING;
    stringValue = value;
}

void Value::operator=(const vector<Value> &value)
{
    type = ARRAY;
    arrayValue = value;
}

void Value::operator=(const map<string, Value> &value)
{
    type = OBJECT;
    objectValue = value;
}

void Value::operator=(const Value &value)
{
    type = value.type;
    switch (type)
    {
        case BOOL:
            boolValue = value.boolValue;
            break;
        case INT:
            intValue = value.intValue;
            break;
        case FLOAT:
            floatValue = value.floatValue;
            break;
        case STRING:
            stringValue = value.stringValue;
            break;
        case ARRAY:
            arrayValue = value.arrayValue;
            break;
        case OBJECT:
            objectValue = value.objectValue;
            break;
        default:
            throw;
            break;
    }
}

Value::operator bool()
{
    return boolValue;
}

Value::operator int()
{
    return intValue;
}

Value::operator float()
{
    return floatValue;
}

Value::operator std::string()
{
    return stringValue;
}

Value::operator std::vector<Value>()
{
    return arrayValue;
}

Value::operator std::map<std::string, Value>()
{
    return objectValue;
}

ostream &operator<<(ostream &os, Value &value)
{
    switch (value.Type())
    {
        case Value::OBJECT: {
            os << "{ ";
            int size = value.Object().size();
            int i = 0;
            for (map<string, Value>::iterator it = value.Object().begin(); it != value.Object().end(); it++)
            {
                os << "\"" << it->first << "\" : ";
                os << it->second;
                if (i < size - 1)
                    os << ", ";
                i++;
            }
            os << " }";
        }
        break;
        case Value::ARRAY: {
            os << "[ ";
            for (int i = 0; i < value.Array().size(); i++)
            {
                os << value[i];
                if (i < value.Array().size() - 1)
                    os << ", ";
            }
            os << " ]";
        }
        break;
        case Value::STRING:
            os << "\"" << value.String() << "\"";
            break;
        case Value::BOOL:
            os << ((value.Bool() ? "true" : "false"));
            break;
        case Value::INT:
            os << value.Int();
            break;
        case Value::FLOAT:
            os << value.Float();
            break;
        default:
            throw;
            break;
    }
    return os;
}

Value tmp;
Value &Value::Default(bool v)
{
    if (type == OBJECT && objectValue.size() == 0)
    {
        tmp = Value(v);
        return tmp;
    }
    return *this;
}

Value &Value::Default(int v)
{
    if (type == OBJECT && objectValue.size() == 0)
    {
        tmp = Value(v);
        return tmp;
    }
    return *this;
}

Value &Value::Default(float v)
{
    if (type == OBJECT && objectValue.size() == 0)
    {
        tmp = Value(v);
        return tmp;
    }
    return *this;
}

Value &Value::Default(std::string v)
{
    if (type == OBJECT && objectValue.size() == 0)
    {
        tmp = Value(v);
        return tmp;
    }
    return *this;
}

Value &Value::Default(std::vector<Value> &v)
{
    if (type == OBJECT && objectValue.size() == 0)
    {
        tmp = Value(v);
        return tmp;
    }
    return *this;
}

Value &Value::Default(std::map<std::string, Value> &v)
{
    if (type == OBJECT && objectValue.size() == 0)
    {
        tmp = Value(v);
        return tmp;
    }
    return *this;
}

Value &Value::Default(Value &v)
{
    if (type == OBJECT && objectValue.size() == 0)
    {
        return v;
    }
    return *this;
}

// Storage
Storage::Storage()
{
}

Storage::Storage(const string fileName)
{
    Open(fileName);
}

Storage::Storage(const string path, const string fileName)
{
    Open(path, fileName);
}

bool Storage::IsOpened()
{
    return isOpened;
}

bool Storage::Open(const string fileName)
{
    if (IsOpened())
    {
        data = Value();
        isOpened = false;
    }

    string path = FixPath(fileName);

    stringstream ss;
    string folder = path.substr(0, path.find_last_of("\\/"));
    if (path.find('/') != string::npos)
    {
        own_mkdir(folder.c_str(), 777);
    }

    this->fileName = path;
    isOpened = true;

    Read();
    return isOpened;
}

bool Storage::Open(const string path, const string fileName)
{
    return Open(SFILE(path, fileName));
}

void Storage::Save()
{
    if (IsOpened())
        Write();
    data = Value();
    isOpened = false;
}

void Storage::Read()
{
    if (!IsOpened())
        return;

    std::ifstream file(fileName.c_str());
    if (!file.is_open())
        return;

    std::stringstream buffer;
    buffer << file.rdbuf();
    data.Parse(buffer.str());

    file.close();
}

void Storage::Write()
{
    if (!IsOpened())
        return;

    std::ofstream file(fileName.c_str());
    file << data;
    file.close();
}

int Storage::Size()
{
    return data.Size();
}

Value &Storage::Data()
{
    return data;
}

Value &Storage::operator[](int i)
{
    return data[i];
}

Value &Storage::operator[](const char *key)
{
    return data[key];
}

Value &Storage::operator[](string key)
{
    return data[key];
}

void Storage::operator=(const Value &value)
{
    data = value;
}

void Storage::operator=(const Storage &storage)
{
    data = storage.data;
}

string Storage::FixPath(string path)
{
    char *homeEnv = getenv("HOME");
    if (homeEnv != NULL)
    {
        std::string home = homeEnv;
        size_t found = path.find("~");
        if (found != std::string::npos)
        {
            path.replace(found, 1, home);
        }
    }
    return path;
}

ostream &operator<<(ostream &os, Storage &value)
{
    cout << value.Data() << endl;
    return os;
}