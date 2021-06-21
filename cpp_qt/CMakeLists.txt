
cmake_minimum_required(VERSION 3.2)


# set(CMAKE_C_COMPILER "C://Program Files//mingw-w64//mingw64//bin/x86_64-w64-mingw32-gcc.exe")
# set(CMAKE_CXX_COMPILER "C://Program Files//mingw-w64//mingw64//bin/x86_64-w64-mingw32-g++.exe")

# set(CMAKE_C_COMPILER "C://Qt//Qt5.8.0//5.8//mingw53_32//bin//gcc.exe")
# set(CMAKE_CXX_COMPILER "C://Qt//Qt5.8.0//5.8//mingw53_32//bin//g++.exe")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# ----------------------------------------Windows-----------------------------
# -rpath is not necessary. For windows, please remember to set path environment variable.
# set path=%path%;dll_path_location. PS: must be the location of dll library.
<<<<<<< HEAD
if (NOT UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath=C:\\Qt\\Qt5.8.0\\5.8\\mingw53_32\\bin")
endif()

project(qt_test)

if (NOT UNIX)
    include_directories("C:\\Qt\\Qt5.8.0\\5.8\\mingw53_32\\include")
    link_directories("C:\\Qt\\Qt5.8.0\\5.8\\mingw53_32\\lib")
endif()

=======

if (NOT UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath=C:\\Qt\\Qt5.8.0\\5.8\\mingw53_32\\bin")
endif()


project(qt_test)

if (WIN32)
    include_directories("C:\\Qt\\Qt5.8.0\\5.8\\mingw53_32\\include")
    link_directories("C:\\Qt\\Qt5.8.0\\5.8\\mingw53_32\\lib")
endif()
>>>>>>> 6e1963902e8f7a9ef6e00b493ff581f6184bce4a

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt5Core Qt5Widgets)

