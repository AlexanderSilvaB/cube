include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package (Cairo)
if (Cairo_FOUND)
        set(SRC cairo.c)

        include_directories(${Cairo_INCLUDE_DIRS})
        
        add_library(cairo SHARED ${SRC})
        target_link_libraries(cairo ${Cairo_LIBRARIES})
        set_target_properties(cairo PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
        set_target_properties(cairo PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
else()
	MESSAGE("Couldn't find libCairo. Not building cairo library")
endif (Cairo_FOUND)




