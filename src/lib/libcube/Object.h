#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "Node.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

enum ObjectTypes
{
    OBJECT, NONE, BOOL, NUMBER, STRING, ARRAY, DICT, FUNC, DEF, CLASS, CLASSOBJ, LIB, EXCEPTION
};

class Object;
typedef std::vector<std::string> StringArray;
typedef std::vector<Object*> Array;
typedef std::unordered_map<std::string, Object*> Map;
typedef std::function< bool ( Object*, Object*, Array& ) > ObjectFunction;
typedef std::unordered_map< ObjectTypes, std::unordered_map<std::string, ObjectFunction > > FunctionMap;
typedef std::unordered_map< ObjectTypes, std::unordered_map<std::string, Object* > > ExtensionMap;


typedef struct
{
    bool _bool;
    double _number;
    std::string _string;
    Map map;
    Array array;
    StringArray names;
    Node node;
    void *handler;
    std::string _returnType;
}Value;

class Object
{
    private:
        bool marked;
        bool saved;
        bool returned;
        Object* next;

        ObjectTypes type;
        Value value;
        static FunctionMap globalFunctions;
        static ExtensionMap globalExtensions;
        Map localFunctions;

        void clearInternal();
    public:
        Object();
        virtual ~Object();

        void setNext(Object* obj);
        Object* getNext();
        void mark();
        void unmark();
        bool isMarked();

        void save();
        void release();
        bool isSaved();

        void isReturned(bool returned);
        bool isReturned();

        // Built-ins
        bool is(ObjectTypes type);
        ObjectTypes getType();
        std::string getTypeName();
        std::string printable();
        bool isFalse();
        bool isTrue();
        bool isCallable();
        static ObjectTypes getTypeByName(const std::string& typeName);
        static void registerGlobal(ObjectTypes type, const std::string& name, ObjectFunction func);
        static void registerGlobal(ObjectTypes type, const std::string& name, Object* func);
        void registerLocal(const std::string& name, Object* func);
        bool call(Object* obj, const std::string& name, Array args = Array());

        // Internals
        Array& array();
        std::string str();
        double number();
        void* handler();
        Node func();

        // Convert methods
        void from(Object* obj);
        void toObject();
        void toNone();
        void toBool(bool value);
        void toNumber(double value);
        void toString(const std::string& value);
        void toException(const std::string& value);
        void toArray(const Array& array);
        void toDict(Map& map);
        void toFunc(Node node);
        void toFuncDef(void* handler, const std::string& name, const std::string& ret, StringArray& vars);

        void convertTo(ObjectTypes type);
        void toArray(Object* obj);
        void toDict(Object* obj);
        void toFunc(Object* obj);
        void toFuncDef(Object* obj);
        void toClass(Object* obj);
        void toClassObj(Object* obj);
        void toLib(Object* obj);
        void toException(Object* obj);

        void split(Object* obj);
};

#endif
