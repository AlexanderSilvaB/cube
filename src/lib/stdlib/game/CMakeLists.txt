include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC_ENABLED_BY_DEFAULT ON CACHE BOOL "" FORCE)
set(SDL_STATIC_PIC ON CACHE BOOL "" FORCE) 

add_subdirectory("SDL2-2.0.12")

set(SRC game.cpp 
        image.cpp 
        display.cpp
        events.cpp
        drawing.cpp
        shapes.cpp)

include_directories("SDL2-2.0.12/include")

add_library(game SHARED ${SRC})
target_link_libraries(game SDL2-static)
set_target_properties(game PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../stdlib/libs/)
set_target_properties(game PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../stdlib/libs/)




