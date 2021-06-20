
cmake_minimum_required(VERSION 3.2)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

PROJECT(cpp_base)

add_executable(${PROJECT_NAME} main.cpp point.cpp)