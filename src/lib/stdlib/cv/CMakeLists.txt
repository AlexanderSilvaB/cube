find_package(cube)
if(${cube_FOUND})
    include_directories(${cube_INCLUDE_DIRS})

    find_package( OpenCV)
    if(${OpenCV_FOUND})
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
        add_definitions(-DWIN_EXPORT)

        set(SRC libcv.cpp highgui.cpp image.cpp imgproc.cpp)

        add_library(cv SHARED ${SRC})
        target_link_libraries(cv ${OpenCV_LIBS})

        set_target_properties(cv PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
        set_target_properties(cv PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
    else()
        message("OpenCV not found: Not building cv for cube")
    endif()
else()
    message("Cube not found: Not building cv for cube")
endif()