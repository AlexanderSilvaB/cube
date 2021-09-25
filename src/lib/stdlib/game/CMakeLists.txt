include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

find_package(SDL2)
if (${SDL2_FOUND})
	set(SRC game.cpp 
			image.cpp 
			display.cpp
			events.cpp
			drawing.cpp
			shapes.cpp
			audio.cpp
			audio.c)
        
	include_directories(${SDL2_INCLUDE_DIRS})

	add_library(libgame SHARED ${SRC})
	target_link_libraries(libgame ${SDL2_LIBRARIES})
	set_target_properties(libgame PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
	set_target_properties(libgame PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)

else()
    message("Couldn't find SDL2. Not building game library")
endif()
