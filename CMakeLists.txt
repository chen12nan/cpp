cmake_minimum_required (VERSION 3.5)

project(cpp_project)

# for windows os
# cmake -G "MinGW Makefiles" ../
# mingw32-make.exe

# Add sub directories
add_subdirectory(cpp_base)

MESSAGE(STATUS "----   "  ${UNIX})
MESSAGE(STATUS "CMAKE_VERSION     " ${CMAKE_SYSTEM} " " ${CMAKE_SYSTEM_NAME} " " ${CMAKE_SYSTEM_VERSION})

# NOT WIN32
if (UNIX)
    add_subdirectory(wiredtiger)
endif()

# if (WIN32)
#     add_subdirectory(cpp_qt)
# endif()
