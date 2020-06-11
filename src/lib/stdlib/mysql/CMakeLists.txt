include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package (MySQL)
if (MYSQL_FOUND)
        set(SRC mysql.cpp)

        include_directories(${MYSQL_INCLUDE_DIRS})
        
        add_library(mysql SHARED ${SRC})
        target_link_libraries(mysql ${MYSQL_LIBRARIES})
        set_target_properties(mysql PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
        set_target_properties(mysql PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
else(MYSQL_FOUND)
	MESSAGE("Couldn't find libMySQL. Not building MySQL library")
endif (MYSQL_FOUND)




