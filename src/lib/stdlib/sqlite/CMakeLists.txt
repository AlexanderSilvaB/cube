include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package (SQLite3)
if (SQLITE3_FOUND)
        set(SRC sqlite.cpp)

        include_directories(${SQLITE3_INCLUDE_DIRS})
        
        add_library(sqlite SHARED ${SRC})
        target_link_libraries(sqlite ${SQLITE3_LIBRARIES})
        set_target_properties(sqlite PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
        set_target_properties(sqlite PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
else(SQLITE3_FOUND)
	MESSAGE("Couldn't find libSQLite3. Not building SQLite3 library")
endif (SQLITE3_FOUND)




