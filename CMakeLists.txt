cmake_minimum_required (VERSION 3.5)

project(cpp_project)

# Add sub directories
add_subdirectory(cpp_base)
<<<<<<< HEAD
add_subdirectory(wiredtiger)

MESSAGE(STATUS "----   "  ${UNIX})
MESSAGE(STATUS "CMAKE_VERSION     " ${CMAKE_SYSTEM} " " ${CMAKE_SYSTEM_NAME} " " ${CMAKE_SYSTEM_VERSION})

if (NOT UNIX)
    add_subdirectory(cpp_qt)
endif()
=======

if (WIN32)
    add_subdirectory(cpp_qt)
endif()
>>>>>>> 6e1963902e8f7a9ef6e00b493ff581f6184bce4a
