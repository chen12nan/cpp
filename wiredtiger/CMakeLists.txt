
cmake_minimum_required(VERSION 3.2)

project(test_wt)

# gdb
# warning: Error disabling address space randomization: Operation not permitted
# docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined

# compile wiredtiger
# env CFLAGS="-O0 -g" CXXFLAGS="-O0 -g" CC=/usr/local/brion_AVX/gcc/7.3.1_3/bin/gcc CXX=/usr/local/brion_AVX/gcc/7.3.1_3/bin/g++ ./configure --prefix=/home/kxing/opt/wiredtiger

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

include_directories("/Users/xingkd/opt/wiredtiger/include")
link_directories("/Users/xingkd/opt/wiredtiger/lib")

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC wiredtiger)