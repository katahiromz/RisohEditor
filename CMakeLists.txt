# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 2.4)

# use new policy
cmake_policy(SET CMP0054 NEW)

# project name and language
project(RisohEditor CXX)

# check build type
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# make uppercase string of build type
string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

# check build type again
if (NOT uppercase_CMAKE_BUILD_TYPE MATCHES "^(DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL)$")
    message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

# set output directory (build/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# Win32 or not?
if (NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "You must use a Win32 compiler")
endif()

# do statically link
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBRARIES OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
    set(CMAKE_EXE_LINKER_FLAGS " -static -static-libgcc -static-libstdc++")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using MSVC
    add_definitions(-DTHIS_IS_MSVC)
endif()

# enable Win32 resource
ENABLE_LANGUAGE(RC)

##############################################################################

add_definitions(-DMZC4_HANDLE_MAP=1)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(mcdx)

##############################################################################
