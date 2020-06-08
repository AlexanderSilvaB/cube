include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

find_package (CURL)
if (CURL_FOUND)
        set(SRC curl.c)

        include_directories(${CURL_INCLUDE_DIRS})
        
        add_library(curl SHARED ${SRC})
        target_link_libraries(curl ${CURL_LIBRARIES})
        set_target_properties(curl PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
        set_target_properties(curl PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
else()
	MESSAGE("Couldn't find libCurl. Not building curl library")
endif (CURL_FOUND)




