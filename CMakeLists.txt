cmake_minimum_required (VERSION 3.5)

project(cpp_project)

# Add sub directories
add_subdirectory(cpp_base)

if (WIN32)
    add_subdirectory(cpp_qt)
endif()
