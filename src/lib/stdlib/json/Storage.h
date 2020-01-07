#ifndef _RINOBOT_NAO_LIB_UTIL_STORAGE_H_
#define _RINOBOT_NAO_LIB_UTIL_STORAGE_H_

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#define BASE_FOLDER std::string("~/rinobot/")
#define DATA_FOLDER BASE_FOLDER+std::string("data/")
#define CONFIG_FOLDER BASE_FOLDER+std::string("config/")
#define BIN_FOLDER BASE_FOLDER+std::string("bin/")
#define LIBS_FOLDER BASE_FOLDER+std::string("libs/")

#define SFILE(path, fileName) path+fileName

class Value
{
    public:
        enum Types { BOOL, INT, FLOAT, STRING, ARRAY, OBJECT } ;
    private:
        bool valid;
        Types type;
        bool boolValue;
        int intValue;
        float floatValue;
        std::string stringValue;
        std::vector<Value> arrayValue;
        std::map<std::string, Value> objectValue;
        Value parse(std::string&, int*);
        Value parseBool(std::string&, int*);
        Value parseNumber(std::string&, int*);
        Value parseString(std::string&, int*);
        Value parseArray(std::string&, int*);
        Value parseObject(std::string&, int*);
        void trim(std::string&, int*);
    public:
        Value();
        Value(bool);
        Value(int);
        Value(float);
        Value(std::string);
        Value(std::vector<Value>&);
        Value(std::map<std::string, Value>&);
        Value(const Value&);

        bool IsValid();

        Types Type();
        bool Bool();
        int Int();
        float Float();
        std::string& String();
        std::vector<Value>& Array();
        std::map<std::string, Value>& Object();

        void Add(Value&);
        int Size();

        bool Parse(std::string);
        std::string ToString();

        Value &operator[](std::string);
        Value &operator[](const char*);
        Value &operator[](int);

        void operator=(const bool &value);
        void operator=(const int &value);
        void operator=(const float &value);
        void operator=(const std::string &value);
        void operator=(const std::vector<Value> &value);
        void operator=(const std::map<std::string, Value> &value);
        void operator=(const Value &value);
        
        Value &Default(bool);
        Value &Default(int);
        Value &Default(float);
        Value &Default(std::string);
        Value &Default(std::vector<Value>&);
        Value &Default(std::map<std::string, Value>&);
        Value &Default(Value&);

        operator bool();
        operator int();
        operator float();
        operator std::string();
        operator std::vector<Value>();
        operator std::map<std::string, Value>();
};

std::ostream &operator<<( std::ostream &output, Value &value );

class Storage
{
    private:
        bool isOpened;
        std::string fileName;
        Value data;
        void Read();
        void Write();
    public:
        Storage();
        Storage(const std::string fileName);
        Storage(const std::string path, const std::string fileName);

        bool IsOpened();

        bool Open(const std::string fileName);
        bool Open(const std::string path, const std::string fileName);
        void Save();

        Value &Data();
        int Size();
        void operator=(const Value &value);
        void operator=(const Storage &storage);
        Value &operator[](std::string);
        Value &operator[](const char *);
        Value &operator[](int);

        static std::string FixPath(std::string);
};

std::ostream &operator<<( std::ostream &output, Storage &value );

#endif