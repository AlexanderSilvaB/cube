include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(ROS_PATH "/opt/ros/melodic/")

if(EXISTS ${ROS_PATH} AND IS_DIRECTORY ${ROS_PATH} AND FALSE)

    include_directories(${ROS_PATH}/include ${ROS_PATH}/lib)

    find_package(Eigen3 REQUIRED) 
    include_directories(${EIGEN3_INCLUDE_DIR})

    set(SRC ros.cpp Storage.cpp model.cpp)
    file(GLOB_RECURSE ros_type_introspection_SRC ros_type_introspection/src/*.cpp)

    include_directories(ros_type_introspection/include)

    add_library(ros SHARED ${SRC} ${ros_type_introspection_SRC})
    set_target_properties(ros PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
    set_target_properties(ros PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)

    target_link_libraries(ros
        -I${ROS_PATH}/include -L${ROS_PATH}/lib
        -lroscpp -lrostime -lrosconsole -lroscpp_serialization -ltf -llaser_geometry -lsensor_base -lmove_base -lactionlib -lsensor_range -lmessage_filters -lboost_system -lboost_thread -ltopic_tools -lurdf)

else()
  message("ROS not found: Not building ROS library")
endif()