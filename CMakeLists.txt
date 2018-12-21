cmake_minimum_required(VERSION 2.8)
project(Cube)

include_directories(src)

file(GLOB_RECURSE SRC
    "src/*.cpp"
    "src/*.h")

add_executable(${PROJECT_NAME} ${SRC})