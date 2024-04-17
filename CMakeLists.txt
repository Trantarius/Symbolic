cmake_minimum_required(VERSION 3.10)
project(Symbolic)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "-O0 -g3")

file(GLOB_RECURSE sources src/**.cpp)

add_executable(Symbolic ${sources})
