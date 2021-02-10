#include "model.hpp"
#include <map>
#include <string>
#include <urdf/model.h>
#include <vector>

using namespace std;

cube_native_var *parseMaterial(const urdf::MaterialSharedPtr &mat);
cube_native_var *parseJoint(const urdf::JointSharedPtr &joint);

cube_native_var *parseVector3(const urdf::Vector3 &vec)
{
    cube_native_var *dict = NATIVE_DICT();

    ADD_NATIVE_DICT(dict, COPY_STR("x"), NATIVE_NUMBER(vec.x));
    ADD_NATIVE_DICT(dict, COPY_STR("y"), NATIVE_NUMBER(vec.y));
    ADD_NATIVE_DICT(dict, COPY_STR("z"), NATIVE_NUMBER(vec.z));

    return dict;
}

cube_native_var *parseRotation(const urdf::Rotation &rot)
{
    cube_native_var *dict = NATIVE_DICT();

    ADD_NATIVE_DICT(dict, COPY_STR("x"), NATIVE_NUMBER(rot.x));
    ADD_NATIVE_DICT(dict, COPY_STR("y"), NATIVE_NUMBER(rot.y));
    ADD_NATIVE_DICT(dict, COPY_STR("z"), NATIVE_NUMBER(rot.z));
    ADD_NATIVE_DICT(dict, COPY_STR("w"), NATIVE_NUMBER(rot.w));

    return dict;
}

cube_native_var *parsePose(const urdf::Pose &pose)
{
    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("position"), parseVector3(pose.position));
    ADD_NATIVE_DICT(dict, COPY_STR("rotation"), parseRotation(pose.rotation));
    return dict;
}

cube_native_var *parseInertial(const urdf::InertialSharedPtr &inertial)
{
    if (!inertial)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();

    ADD_NATIVE_DICT(dict, COPY_STR("origin"), parsePose(inertial->origin));
    ADD_NATIVE_DICT(dict, COPY_STR("mass"), NATIVE_NUMBER(inertial->mass));
    ADD_NATIVE_DICT(dict, COPY_STR("ixx"), NATIVE_NUMBER(inertial->ixx));
    ADD_NATIVE_DICT(dict, COPY_STR("ixy"), NATIVE_NUMBER(inertial->ixy));
    ADD_NATIVE_DICT(dict, COPY_STR("ixz"), NATIVE_NUMBER(inertial->ixz));
    ADD_NATIVE_DICT(dict, COPY_STR("iyy"), NATIVE_NUMBER(inertial->iyy));
    ADD_NATIVE_DICT(dict, COPY_STR("iyz"), NATIVE_NUMBER(inertial->iyz));
    ADD_NATIVE_DICT(dict, COPY_STR("izz"), NATIVE_NUMBER(inertial->izz));

    return dict;
}

cube_native_var *parseGeometry(const urdf::GeometrySharedPtr &geo)
{
    if (!geo)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();

    ADD_NATIVE_DICT(dict, COPY_STR("type"), NATIVE_NUMBER((int)geo->type));
    switch (geo->type)
    {
        case urdf::Geometry::SPHERE:
            ADD_NATIVE_DICT(dict, COPY_STR("radius"),
                            NATIVE_NUMBER(urdf::static_pointer_cast<urdf::Sphere>(geo)->radius));
            break;
        case urdf::Geometry::BOX:
            ADD_NATIVE_DICT(dict, COPY_STR("dim"), parseVector3(urdf::static_pointer_cast<urdf::Box>(geo)->dim));
            break;
        case urdf::Geometry::CYLINDER:
            ADD_NATIVE_DICT(dict, COPY_STR("length"),
                            NATIVE_NUMBER(urdf::static_pointer_cast<urdf::Cylinder>(geo)->length));
            ADD_NATIVE_DICT(dict, COPY_STR("radius"),
                            NATIVE_NUMBER(urdf::static_pointer_cast<urdf::Cylinder>(geo)->radius));
            break;
        case urdf::Geometry::MESH:
            ADD_NATIVE_DICT(dict, COPY_STR("filename"),
                            NATIVE_STRING_COPY(urdf::static_pointer_cast<urdf::Mesh>(geo)->filename.c_str()));
            ADD_NATIVE_DICT(dict, COPY_STR("scale"), parseVector3(urdf::static_pointer_cast<urdf::Mesh>(geo)->scale));
            break;
        default:
            break;
    }

    return dict;
}

cube_native_var *parseVisual(const urdf::VisualSharedPtr &visual, bool justName)
{
    if (!visual)
        return NATIVE_NULL();

    if (justName)
        return NATIVE_STRING_COPY(visual->material_name.c_str());

    cube_native_var *dict = NATIVE_DICT();

    ADD_NATIVE_DICT(dict, COPY_STR("origin"), parsePose(visual->origin));
    ADD_NATIVE_DICT(dict, COPY_STR("geometry"), parseGeometry(visual->geometry));
    ADD_NATIVE_DICT(dict, COPY_STR("material"), parseMaterial(visual->material));
    ADD_NATIVE_DICT(dict, COPY_STR("name"), NATIVE_STRING_COPY(visual->name.c_str()));

    return dict;
}

cube_native_var *parseCollision(const urdf::CollisionSharedPtr &coll, bool justName)
{
    if (!coll)
        return NATIVE_NULL();

    if (justName)
        return NATIVE_STRING_COPY(coll->name.c_str());

    cube_native_var *dict = NATIVE_DICT();

    ADD_NATIVE_DICT(dict, COPY_STR("origin"), parsePose(coll->origin));
    ADD_NATIVE_DICT(dict, COPY_STR("geometry"), parseGeometry(coll->geometry));
    ADD_NATIVE_DICT(dict, COPY_STR("name"), NATIVE_STRING_COPY(coll->name.c_str()));

    return dict;
}

cube_native_var *parseLink(const urdf::LinkSharedPtr &link)
{
    if (!link)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("name"), NATIVE_STRING_COPY(link->name.c_str()));
    ADD_NATIVE_DICT(dict, COPY_STR("inertial"), parseInertial(link->inertial));

    cube_native_var *collisionArrayList = NATIVE_LIST();
    auto collision_array = link->collision_array;
    for (int i = 0; i < collision_array.size(); i++)
    {
        ADD_NATIVE_LIST(collisionArrayList, parseCollision(collision_array[i], false));
        if (collision_array[i] == link->collision)
            ADD_NATIVE_DICT(dict, COPY_STR("collision"), NATIVE_NUMBER(i));
    }
    ADD_NATIVE_DICT(dict, COPY_STR("collision_array"), collisionArrayList);

    cube_native_var *visualArrayList = NATIVE_LIST();
    auto visual_array = link->visual_array;
    for (int i = 0; i < visual_array.size(); i++)
    {
        ADD_NATIVE_LIST(visualArrayList, parseVisual(visual_array[i], false));
        if (visual_array[i] == link->visual)
            ADD_NATIVE_DICT(dict, COPY_STR("visual"), NATIVE_NUMBER(i));
    }
    ADD_NATIVE_DICT(dict, COPY_STR("visual_array"), visualArrayList);

    if (!link->parent_joint)
        ADD_NATIVE_DICT(dict, COPY_STR("parent_joint"), NATIVE_NULL());
    else
        ADD_NATIVE_DICT(dict, COPY_STR("parent_joint"), NATIVE_STRING_COPY(link->parent_joint->name.c_str()));

    cube_native_var *childJointsList = NATIVE_LIST();
    auto child_joints = link->child_joints;
    for (int i = 0; i < child_joints.size(); i++)
    {
        ADD_NATIVE_LIST(childJointsList, parseJoint(child_joints[i]));
    }
    ADD_NATIVE_DICT(dict, COPY_STR("child_joints"), childJointsList);

    cube_native_var *childLinksList = NATIVE_LIST();
    auto child_links = link->child_links;
    for (int i = 0; i < child_links.size(); i++)
    {
        ADD_NATIVE_LIST(childLinksList, parseLink(child_links[i]));
    }
    ADD_NATIVE_DICT(dict, COPY_STR("child_links"), childLinksList);

    return dict;
}

cube_native_var *parseJointDynamics(const urdf::JointDynamicsSharedPtr &dyna)
{
    if (!dyna)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("damping"), NATIVE_NUMBER(dyna->damping));
    ADD_NATIVE_DICT(dict, COPY_STR("friction"), NATIVE_NUMBER(dyna->friction));

    return dict;
}

cube_native_var *parseJointLimits(const urdf::JointLimitsSharedPtr &limits)
{
    if (!limits)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("lower"), NATIVE_NUMBER(limits->lower));
    ADD_NATIVE_DICT(dict, COPY_STR("upper"), NATIVE_NUMBER(limits->upper));
    ADD_NATIVE_DICT(dict, COPY_STR("effort"), NATIVE_NUMBER(limits->effort));
    ADD_NATIVE_DICT(dict, COPY_STR("velocity"), NATIVE_NUMBER(limits->velocity));

    return dict;
}

cube_native_var *parseJointSafety(const urdf::JointSafetySharedPtr &safety)
{
    if (!safety)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("soft_upper_limit"), NATIVE_NUMBER(safety->soft_upper_limit));
    ADD_NATIVE_DICT(dict, COPY_STR("soft_lower_limit"), NATIVE_NUMBER(safety->soft_lower_limit));
    ADD_NATIVE_DICT(dict, COPY_STR("k_position"), NATIVE_NUMBER(safety->k_position));
    ADD_NATIVE_DICT(dict, COPY_STR("k_velocity"), NATIVE_NUMBER(safety->k_velocity));

    return dict;
}

cube_native_var *parseJointCalibration(const urdf::JointCalibrationSharedPtr &calib)
{
    if (!calib)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("reference_position"), NATIVE_NUMBER(calib->reference_position));

    if (calib->rising)
        ADD_NATIVE_DICT(dict, COPY_STR("rising"), NATIVE_NUMBER(*(calib->rising)));
    else
        ADD_NATIVE_DICT(dict, COPY_STR("rising"), NATIVE_NULL());

    if (calib->falling)
        ADD_NATIVE_DICT(dict, COPY_STR("falling"), NATIVE_NUMBER(*(calib->falling)));
    else
        ADD_NATIVE_DICT(dict, COPY_STR("falling"), NATIVE_NULL());

    return dict;
}

cube_native_var *parseJointMimic(const urdf::JointMimicSharedPtr &mimic)
{
    if (!mimic)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("offset"), NATIVE_NUMBER(mimic->offset));
    ADD_NATIVE_DICT(dict, COPY_STR("multiplier"), NATIVE_NUMBER(mimic->multiplier));
    ADD_NATIVE_DICT(dict, COPY_STR("joint_name"), NATIVE_STRING_COPY(mimic->joint_name.c_str()));

    return dict;
}

cube_native_var *parseJoint(const urdf::JointSharedPtr &joint)
{
    if (!joint)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("name"), NATIVE_STRING_COPY(joint->name.c_str()));
    ADD_NATIVE_DICT(dict, COPY_STR("type"), NATIVE_NUMBER((int)joint->type));
    ADD_NATIVE_DICT(dict, COPY_STR("axis"), parseVector3(joint->axis));
    ADD_NATIVE_DICT(dict, COPY_STR("child_link_name"), NATIVE_STRING_COPY(joint->child_link_name.c_str()));
    ADD_NATIVE_DICT(dict, COPY_STR("parent_link_name"), NATIVE_STRING_COPY(joint->parent_link_name.c_str()));
    ADD_NATIVE_DICT(dict, COPY_STR("parent_to_joint_origin_transform"),
                    parsePose(joint->parent_to_joint_origin_transform));
    ADD_NATIVE_DICT(dict, COPY_STR("dynamics"), parseJointDynamics(joint->dynamics));
    ADD_NATIVE_DICT(dict, COPY_STR("limits"), parseJointLimits(joint->limits));
    ADD_NATIVE_DICT(dict, COPY_STR("safety"), parseJointSafety(joint->safety));
    ADD_NATIVE_DICT(dict, COPY_STR("calibration"), parseJointCalibration(joint->calibration));
    ADD_NATIVE_DICT(dict, COPY_STR("mimic"), parseJointMimic(joint->mimic));

    return dict;
}

cube_native_var *parseColor(const urdf::Color &color)
{
    cube_native_var *dict = NATIVE_DICT();

    ADD_NATIVE_DICT(dict, COPY_STR("r"), NATIVE_NUMBER(color.r));
    ADD_NATIVE_DICT(dict, COPY_STR("g"), NATIVE_NUMBER(color.g));
    ADD_NATIVE_DICT(dict, COPY_STR("b"), NATIVE_NUMBER(color.b));
    ADD_NATIVE_DICT(dict, COPY_STR("a"), NATIVE_NUMBER(color.a));

    return dict;
}

cube_native_var *parseMaterial(const urdf::MaterialSharedPtr &mat)
{
    if (!mat)
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("name"), NATIVE_STRING_COPY(mat->name.c_str()));
    ADD_NATIVE_DICT(dict, COPY_STR("texture_filename"), NATIVE_STRING_COPY(mat->texture_filename.c_str()));
    ADD_NATIVE_DICT(dict, COPY_STR("color"), parseColor(mat->color));

    return dict;
}

cube_native_var *parseUrdf(const char *fileName)
{
    urdf::Model model;
    if (!model.initFile(fileName))
        return NATIVE_NULL();

    cube_native_var *dict = NATIVE_DICT();
    ADD_NATIVE_DICT(dict, COPY_STR("name"), NATIVE_STRING_COPY(model.name_.c_str()));
    ADD_NATIVE_DICT(dict, COPY_STR("root"), parseLink(model.root_link_));

    return dict;
}