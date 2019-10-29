include_directories(..)

set(SRC 
    Input.cpp
    Node.cpp
    Parser.cpp
    Tokenizer.cpp
    Object.cpp
    Env.cpp
    VM.cpp
    ICube.cpp)

add_library(libcube ${SRC})
target_link_libraries(libcube dl)