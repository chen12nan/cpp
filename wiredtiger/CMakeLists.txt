
cmake_minimum_required(VERSION 3.2)

project(test_wt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

include_directories("/Users/xingkd/opt/wiredtiger/include")
link_directories("/Users/xingkd/opt/wiredtiger/lib")

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC wiredtiger)