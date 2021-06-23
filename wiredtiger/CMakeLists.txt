
cmake_minimum_required(VERSION 3.2)

#set(CMAKE_C_COMPILER "/usr/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/bin/g++")

project(test_wt)

# gdb
# warning: Error disabling address space randomization: Operation not permitted
# docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined

#mac os 下的gcc和g++都是x-code的那套。要用brew install gcc 安装。 


# compile wiredtiger
# env CFLAGS="-O0 -g" CXXFLAGS="-O0 -g" CC=/usr/local/brion_AVX/gcc/7.3.1_3/bin/gcc CXX=/usr/local/brion_AVX/gcc/7.3.1_3/bin/g++ ./configure --prefix=/home/kxing/opt/wiredtiger

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

include_directories("$ENV{HOME}/opt/wiredtiger/include")
link_directories("$ENV{HOME}/opt/wiredtiger/lib")

include_directories("$ENV{HOME}/opt/bson/include/libbson-1.0")
link_directories("$ENV{HOME}/opt/bson/lib")


add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC wiredtiger bson-1.0)