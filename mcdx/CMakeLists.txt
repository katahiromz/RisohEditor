# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# project name, version, and languages
project(mcdx VERSION 0.9.3 LANGUAGES CXX RC)

# include directories
include_directories(../src)

# mcdx.exe
add_executable(mcdx mcdx.cpp mcdx_res.rc)

if (WIN32)
    target_compile_definitions(mcdx PRIVATE UNICODE _UNICODE)
    target_link_libraries(mcdx shell32 shlwapi)
endif()

option(USE_WONAPI "Use Wonders API" OFF)
option(USE_ICONV "Use Iconv" OFF)

if(USE_WONAPI)
    set(USE_ICONV ON)
    target_compile_definitions(mcdx PRIVATE WONVER)
elseif(NOT WIN32)
    set(USE_ICONV ON)
endif()

if(USE_ICONV)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    find_package(Iconv REQUIRED)
    target_include_directories(mcdx PRIVATE ${ICONV_INCLUDE_DIRS})
    target_link_libraries(mcdx ${ICONV_LIBRARIES})
endif()
