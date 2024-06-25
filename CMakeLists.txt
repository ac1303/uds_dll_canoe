cmake_minimum_required(VERSION 3.25)
project(DLLTest)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)
link_directories(lib)

add_library(DLLTest SHARED
        include/vector/CCL/CCL_VC_x86.def
        src/main.cpp src/main.h)
target_link_libraries(DLLTest SQLiteCpp sqlite3)
