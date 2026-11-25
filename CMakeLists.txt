cmake_minimum_required(VERSION 3.20)
project(three-in-a-row-solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(solver src/solver.cpp)
target_compile_options(solver PRIVATE -Wall -Wextra)
