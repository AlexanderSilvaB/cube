include_directories(..)

set(SRC 
    Input.cpp
    Node.cpp
    Parser.cpp
    Tokenizer.cpp
    Env.cpp
    Var.cpp
    Loader.cpp
    Interpreter.cpp)

add_library(libcube ${SRC})
target_link_libraries(libcube dl)