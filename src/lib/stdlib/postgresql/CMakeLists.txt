include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

find_package (PostgreSQL)

if (PostgreSQL_FOUND)
        set(SRC postgresql.cpp)

        include_directories(${PostgreSQL_INCLUDE_DIRS})
        
        add_library(postgresql SHARED ${SRC})
        target_link_libraries(postgresql ${PostgreSQL_LIBRARIES})
        set_target_properties(postgresql PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
        set_target_properties(postgresql PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
else(PostgreSQLFOUND)
	MESSAGE("Couldn't find libpq. Not building PostgreSQL library")
endif (PostgreSQL_FOUND)




