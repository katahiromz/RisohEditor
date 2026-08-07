# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# project name and languages
project(MainWnd CXX RC)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # using Clang
    set(CMAKE_C_FLAGS "-static")
    set(CMAKE_CXX_FLAGS "-static")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
    set(CMAKE_C_FLAGS "-static")
    set(CMAKE_CXX_FLAGS "-static")
elseif (MSVC)
    # replace "/MD" with "/MT" (building without runtime DLLs)
    set(CompilerFlags
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    foreach(CompilerFlags ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlags} "${${CompilerFlags}}")
    endforeach()
endif()

# PROTECTION option
option(PROTECTION "Software Protection on Release" OFF)
message(STATUS "PROTECTION: ${PROTECTION}")
if(PROTECTION)
    add_definitions(-DPROTECTION)
endif()

# RESOURCE_PROTECTION option
option(RESOURCE_PROTECTION "Resource Protection on Release" OFF)
message(STATUS "RESOURCE_PROTECTION: ${RESOURCE_PROTECTION}")
if(RESOURCE_PROTECTION)
    add_definitions(-DRESOURCE_PROTECTION)
endif()

# MainWnd.exe
add_executable(MainWnd WIN32 MainWnd.cpp MainWnd_res.rc)
target_link_libraries(MainWnd comctl32)

##############################################################################
