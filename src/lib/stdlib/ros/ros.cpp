#include "Storage.h"
#include "model.hpp"
#include "ros_type_introspection/ros_introspection.hpp"
#include <boost/bind.hpp>
#include <cube/cubeext.h>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <ros/ros.h>
#include <sstream>
#include <string>
#include <topic_tools/shape_shifter.h>
#include <vector>

using namespace std;
using topic_tools::ShapeShifter;
using namespace RosIntrospection;

Parser parser;
shared_ptr<ros::NodeHandle> nh;

int subsI = 0;
map<int, ros::Subscriber> subs;
map<int, std::string> sub_topics;
map<int, cube_native_var *> sub_callbacks;

int pubsI = 0;
map<int, ros::Publisher> pubs;
map<int, topic_tools::ShapeShifter> pub_shapes;

map<string, int> type_sizes = {{"bool", sizeof(uint8_t)},    {"int8", sizeof(int8_t)},     {"uint8", sizeof(uint8_t)},
                               {"int16", sizeof(int16_t)},   {"uint16", sizeof(uint16_t)}, {"int32", sizeof(int32_t)},
                               {"uint32", sizeof(uint32_t)}, {"int64", sizeof(int64_t)},   {"uint64", sizeof(uint64_t)},
                               {"float32", sizeof(float)},   {"float64", sizeof(double)},  {"string", sizeof(char)}};

void subs_callback(const ShapeShifter::ConstPtr &msg, int id)
{
    std::string topic = sub_topics[id];
    cube_native_var *fn = sub_callbacks[id];

    const std::string &datatype = msg->getDataType();
    const std::string &definition = msg->getMessageDefinition();

    parser.registerMessageDefinition(topic, RosIntrospection::ROSType(datatype), definition);

    std::vector<uint8_t> buffer;
    FlatMessage flat_container;
    RenamedValues renamed_value;

    // copy raw memory into the buffer
    int sz = msg->size();
    buffer.resize(sz);
    ros::serialization::OStream stream(buffer.data(), buffer.size());
    msg->write(stream);

    // deserialize and rename the vectors
    parser.deserializeIntoFlatContainer(topic, Span<uint8_t>(buffer), &flat_container, 100);
    parser.applyNameTransform(topic, flat_container, &renamed_value);

    stringstream ss;
    for (auto it : renamed_value)
    {
        const std::string &key = it.first;
        const Variant &value = it.second;
        ss << key << "=" << it.second.convert<double>() << "\n";
    }
    for (auto it : flat_container.name)
    {
        const std::string &key = it.first.toStdString();
        ss << key << "=" << it.second << "\n";
    }

    cube_native_var *blobs = NATIVE_DICT();
    for (auto it : flat_container.blob)
    {
        const std::string &key = it.first.toStdString();
        cube_native_var *blob = NATIVE_BYTES_COPY(it.second.size(), it.second.data());
        ADD_NATIVE_DICT(blobs, COPY_STR(key.c_str()), blob);
    }

    cube_native_var *args = NATIVE_LIST();
    ADD_NATIVE_LIST(args, NATIVE_STRING_COPY(ss.str().c_str()));
    ADD_NATIVE_LIST(args, blobs);
    CALL_NATIVE_FUNC(fn, args);
}

Value NativeToValue(cube_native_var *var)
{
    Value ret;
    if (IS_NATIVE_LIST(var))
    {
        vector<Value> values;
        values.resize(var->size);
        for (int i = 0; i < var->size; i++)
        {
            values[i] = NativeToValue(var->list[i]);
        }

        ret = Value(values);
    }
    else if (IS_NATIVE_DICT(var))
    {
        map<string, Value> values;

        for (int i = 0; i < var->size; i++)
        {
            Value value = NativeToValue(var->dict[i]);
            values[string(var->dict[i]->key)] = value;
        }
        ret = Value(values);
    }
    else
    {
        switch (NATIVE_TYPE(var))
        {
            case TYPE_VOID:
            case TYPE_NULL: {
            }
            break;
            case TYPE_BOOL: {
                ret = Value(AS_NATIVE_BOOL(var));
            }
            break;
            case TYPE_NUMBER: {
                ret = Value((float)AS_NATIVE_NUMBER(var));
            }
            break;
            case TYPE_STRING: {
                ret = Value(string(AS_NATIVE_STRING(var)));
            }
            break;
            case TYPE_BYTES: {
                cube_native_bytes bytes = AS_NATIVE_BYTES(var);
                ret = Value(bytes.bytes, bytes.length);
            }
            break;
            default:
                break;
        }
    }

    return ret;
}

int measureMessage(Value &info, Value &sizes, Value &value)
{
    int size = 0;
    if (info.Type() == Value::OBJECT)
    {
        ObjectValue &items = info.Object();
        for (ObjectValue::iterator it = items.begin(); it != items.end(); it++)
        {
            if (it->first.rfind("__", 0) == 0)
                continue;
            if (it->second.Type() == Value::OBJECT)
            {
                Value &innerSizes = sizes[it->first];
                Value &innerValue = value[it->first];
                if (innerValue.Type() == Value::ARRAY && innerSizes.Type() == Value::OBJECT)
                {
                    int sz = sizes["__" + it->first].Float();
                    if (sz < 0)
                        size += sizeof(uint32_t);
                    for (int i = 0; i < innerValue.Size(); i++)
                    {
                        size += measureMessage(it->second, innerSizes, innerValue[i]);
                    }
                }
                else
                    size += measureMessage(it->second, innerSizes, innerValue);
            }
            else
            {
                string type = it->second;
                if (type == "string")
                {
                    int sz = sizes[it->first].Float();
                    if (sz == 0)
                    {
                        size += sizeof(uint32_t);
                        string &str = value[it->first].String();
                        size += str.size();
                    }
                    else if (sz > 0)
                    {
                        ArrayValue &array = value[it->first].Array();
                        int i = 0;
                        for (; i < array.size() && i < sz; i++)
                        {
                            size += sizeof(uint32_t) + value[it->first].String().size();
                        }
                        for (; i < i < sz; i++)
                        {
                            size += sizeof(uint32_t) + 0;
                        }
                    }
                    else if (sz < 0)
                    {
                        ArrayValue &array = value[it->first].Array();
                        size += sizeof(uint32_t);
                        for (int i = 0; i < array.size(); i++)
                        {
                            size += sizeof(uint32_t) + value[it->first].String().size();
                        }
                    }
                }
                else
                {
                    int sz = sizes[it->first].Float();
                    if (sz > 0)
                        size += type_sizes[type] * sz;
                    else if (sz < 0)
                        size += sizeof(uint32_t) + type_sizes[type] * value[it->first].Size();
                    else
                        size += type_sizes[type];
                }
            }
        }
    }
    return size;
}

void serialize(ros::serialization::OStream &stream, Value &info, Value &sizes, Value &value)
{
    if (info.Type() == Value::OBJECT)
    {
        ArrayValue &order = info["__order"].Array();
        for (int i = 0; i < order.size(); i++)
        {
            string &name = order[i].String();
            Value &innerSizes = sizes[name];
            Value &innerValue = value[name];
            if (innerValue.Type() == Value::ARRAY && innerSizes.Type() == Value::OBJECT)
            {
                int sz = sizes["__" + name].Float();
                if (sz < 0)
                {
                    uint32_t _sz = innerValue.Size();
                    ros::serialization::serialize(stream, _sz);
                }
                for (int i = 0; i < innerValue.Size(); i++)
                {
                    serialize(stream, info[name], innerSizes, innerValue[i]);
                }
            }
            else
                serialize(stream, info[name], innerSizes, innerValue);
        }
    }
    else if (info.Type() == Value::STRING)
    {
        string &type = info.String();
        int size = sizes.Float();
        if (type == "string")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                for (; i < size && i < array.size(); i++)
                {
                    ros::serialization::serialize(stream, array[i].String());
                }
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, string(""));
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    ros::serialization::serialize(stream, array[i].String());
                }
            }
            else
                ros::serialization::serialize(stream, value.String());
        }
        else if (type == "bool")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                uint8_t v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                uint8_t v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                uint8_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "int8")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                int8_t v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                int8_t v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                int8_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "uint8")
        {
            if (size > 0)
            {
                if (value.Type() == Value::ARRAY)
                {
                    ArrayValue &array = value.Array();
                    int i = 0;
                    uint8_t v;
                    for (; i < size && i < array.size(); i++)
                    {
                        v = array[i].Float();
                        ros::serialization::serialize(stream, v);
                    }
                    v = 0;
                    for (; i < size; i++)
                    {
                        ros::serialization::serialize(stream, v);
                    }
                }
                else if (value.Type() == Value::BYTES)
                {
                    uint8_t *bytes = value.Bytes();
                    int i = 0;
                    uint8_t v;
                    for (; i < size && i < value.Size(); i++)
                    {
                        v = bytes[i];
                        ros::serialization::serialize(stream, v);
                    }
                    v = 0;
                    for (; i < size; i++)
                    {
                        ros::serialization::serialize(stream, v);
                    }
                }
            }
            else if (size < 0)
            {
                if (value.Type() == Value::ARRAY)
                {
                    ArrayValue &array = value.Array();
                    uint32_t sz = array.size();
                    uint8_t v;
                    ros::serialization::serialize(stream, sz);
                    for (int i = 0; i < array.size(); i++)
                    {
                        v = array[i].Float();
                        ros::serialization::serialize(stream, v);
                    }
                }
                else if (value.Type() == Value::BYTES)
                {
                    uint8_t *bytes = value.Bytes();
                    uint32_t sz = value.Size();
                    uint8_t v;
                    ros::serialization::serialize(stream, sz);
                    for (int i = 0; i < sz; i++)
                    {
                        v = bytes[i];
                        ros::serialization::serialize(stream, v);
                    }
                }
            }
            else
            {
                uint8_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "int16")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                int16_t v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                int16_t v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                int16_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "uint16")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                uint16_t v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                uint16_t v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                uint16_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "int32")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                int32_t v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                int32_t v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                int32_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "uint32")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                uint32_t v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                uint32_t v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                uint32_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "int64")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                int64_t v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                int64_t v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                int64_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "uint64")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                uint64_t v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                uint64_t v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                uint64_t v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "float32")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                float v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                float v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                float v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
        else if (type == "float64")
        {
            if (size > 0)
            {
                ArrayValue &array = value.Array();
                int i = 0;
                double v;
                for (; i < size && i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
                v = 0;
                for (; i < size; i++)
                {
                    ros::serialization::serialize(stream, v);
                }
            }
            else if (size < 0)
            {
                ArrayValue &array = value.Array();
                uint32_t sz = array.size();
                double v;
                ros::serialization::serialize(stream, sz);
                for (int i = 0; i < array.size(); i++)
                {
                    v = array[i].Float();
                    ros::serialization::serialize(stream, v);
                }
            }
            else
            {
                double v = value.Float();
                ros::serialization::serialize(stream, v);
            }
        }
    }
}

extern "C"
{
    EXPORTED void init(const char *name, bool anonymous, bool use_private)
    {
        int argc = 1;
        char *argv[] = {"cube"};
        if (anonymous)
            ros::init(argc, argv, name, ros::init_options::AnonymousName);
        else
            ros::init(argc, argv, name);

        if (use_private)
            nh = shared_ptr<ros::NodeHandle>(new ros::NodeHandle("~"));
        else
            nh = shared_ptr<ros::NodeHandle>(new ros::NodeHandle());
    }

    EXPORTED bool ok()
    {
        return ros::ok();
    }

    EXPORTED bool isShuttingDown()
    {
        return ros::isShuttingDown();
    }

    EXPORTED void spinOnce()
    {
        ros::spinOnce();
    }

    EXPORTED void logdebug(const char *text)
    {
        ROS_DEBUG("%s", text);
    }

    EXPORTED void loginfo(const char *text)
    {
        ROS_INFO("%s", text);
    }

    EXPORTED void logwarn(const char *text)
    {
        ROS_WARN("%s", text);
    }

    EXPORTED void logerror(const char *text)
    {
        ROS_ERROR("%s", text);
    }

    EXPORTED void logfatal(const char *text)
    {
        ROS_FATAL("%s", text);
    }

    EXPORTED int createSub(const char *topic, int queueSize, cube_native_var *fn)
    {
        int id = subsI;
        subsI++;

        sub_topics[id] = string(topic);
        sub_callbacks[id] = SAVE_FUNC(fn);

        boost::function<void(const ShapeShifter::ConstPtr &)> callback;
        callback = [id](const ShapeShifter::ConstPtr &msg) { subs_callback(msg, id); };
        subs[id] = nh->subscribe<topic_tools::ShapeShifter>(topic, queueSize, callback);
        return id;
    }

    EXPORTED void shutdownSub(int id)
    {
        if (subs.find(id) == subs.end())
            return;

        subs[id].shutdown();
    }

    EXPORTED int getNumPubsSub(int id)
    {
        if (subs.find(id) == subs.end())
            return 0;

        return subs[id].getNumPublishers();
    }

    EXPORTED int createPub(const char *topic, int queueSize, const char *md5, const char *type, const char *desc)
    {
        int id = pubsI;
        pubsI++;

        pub_shapes[id] = topic_tools::ShapeShifter();
        pub_shapes[id].morph(string(md5), string(type), string(desc), "");

        pubs[id] = pub_shapes[id].advertise(*nh, topic, queueSize);
        return id;
    }

    EXPORTED void shutdownPub(int id)
    {
        if (pubs.find(id) == pubs.end())
            return;

        pubs[id].shutdown();
    }

    EXPORTED int getNumSubsPub(int id)
    {
        if (pubs.find(id) == pubs.end())
            return 0;

        return pubs[id].getNumSubscribers();
    }

    EXPORTED bool publishMsg(int id, cube_native_var *data)
    {
        if (pubs.find(id) == pubs.end())
            return false;

        if (!IS_NATIVE_DICT(data))
            return false;

        map<string, Value> values;
        for (int i = 0; i < data->size; i++)
        {
            Value value = NativeToValue(data->dict[i]);
            values[string(data->dict[i]->key)] = value;
        }

        Value json = Value(values);
        // cout << json << endl;

        if (!json.Has("info") || !json.Has("size") || !json.Has("value"))
            return false;

        Value info = json["info"];
        Value size = json["size"];
        Value value = json["value"];

        int msgSize = measureMessage(info, size, value);

        std::vector<uint8_t> buffer;
        buffer.resize(msgSize);

        ros::serialization::OStream s(buffer.data(), buffer.size());
        serialize(s, info, size, value);

        ros::serialization::OStream stream(buffer.data(), buffer.size());
        pub_shapes[id].read(stream);
        pubs[id].publish(pub_shapes[id]);

        return true;
    }

    cube_native_var *getParam(const char *name)
    {
        string p_string;
        if (nh->getParam(name, p_string))
        {
            return NATIVE_STRING_COPY(p_string.c_str());
        }
        else
        {
            double p_double;
            if (nh->getParam(name, p_double))
            {
                return NATIVE_NUMBER(p_double);
            }
            else
            {
                float p_float;
                if (nh->getParam(name, p_float))
                {
                    return NATIVE_NUMBER(p_float);
                }
                else
                {
                    int p_int;
                    if (nh->getParam(name, p_int))
                    {
                        return NATIVE_NUMBER(p_int);
                    }
                    else
                    {
                        bool p_bool;
                        if (nh->getParam(name, p_bool))
                        {
                            return NATIVE_BOOL(p_bool);
                        }
                        else
                        {
                            return NATIVE_NULL();
                        }
                    }
                }
            }
        }
        return NATIVE_NULL();
    }

    bool setParam(const char *name, cube_native_var *val)
    {
        bool ok = false;
        switch (NATIVE_TYPE(val))
        {
            case TYPE_BOOL: {
                nh->setParam(name, AS_NATIVE_BOOL(val));
                ok = true;
            }
            break;
            case TYPE_NUMBER: {
                double v = AS_NATIVE_NUMBER(val);
                int vi = v;
                if (((double)vi) == v)
                    nh->setParam(name, vi);
                else
                    nh->setParam(name, v);
                ok = true;
            }
            break;
            case TYPE_STRING: {
                nh->setParam(name, string(AS_NATIVE_STRING(val)));
                ok = true;
            }
            break;
            default:
                ok = false;
                break;
        }
        return ok;
    }

    bool hasParam(const char *name)
    {
        return nh->hasParam(name);
    }

    bool deleteParam(const char *name)
    {
        return nh->deleteParam(name);
    }

    cube_native_var *searchParam(const char *name)
    {
        string key;
        if (nh->searchParam(name, key))
        {
            return NATIVE_STRING_COPY(key.c_str());
        }
        return NATIVE_NULL();
    }

    cube_native_var *getParamNames()
    {
        vector<string> keys;
        nh->getParamNames(keys);

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < keys.size(); i++)
        {
            ADD_NATIVE_LIST(list, NATIVE_STRING_COPY(keys[i].c_str()));
        }

        return list;
    }

    cube_native_var *loadUrdf(const char *fileName)
    {
        cube_native_var *dict = parseUrdf(fileName);
        return dict;
    }
}