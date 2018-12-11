cmake_minimum_required(VERSION 2.8)
project(Bot)

include_directories(src)

file(GLOB_RECURSE SRC
    "src/*.cpp"
    "src/*.h")

add_executable(${PROJECT_NAME} ${SRC})