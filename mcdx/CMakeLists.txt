# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 2.9)

# Enable C/C++
ENABLE_LANGUAGE(C)
ENABLE_LANGUAGE(CXX)

# support C++11
macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
  else()
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(use_cxx11)

if (NOT WIN32)
    use_cxx11()
endif()

# include directories
include_directories(../src)

# executable
add_executable(mcdx mcdx.cpp getoptwin.c)

# do statically link
if (WIN32)
    set_target_properties(mcdx PROPERTIES LINK_SEARCH_START_STATIC 1)
    set_target_properties(mcdx PROPERTIES LINK_SEARCH_END_STATIC 1)
endif()

option(USE_WONAPI "Use Wonders API" OFF)
option(USE_ICONV "Use Iconv" OFF)

if(USE_WONAPI)
    set(USE_ICONV ON)
    target_compile_definitions(mcdx PRIVATE -DWONVER)
elseif(NOT WIN32)
    set(USE_ICONV ON)
endif()

if(USE_ICONV)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    find_package(Iconv REQUIRED)
    target_include_directories(mcdx PRIVATE ${ICONV_INCLUDE_DIRS})
    target_link_libraries(mcdx ${ICONV_LIBRARIES})
endif()
