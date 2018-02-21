cmake_minimum_required(VERSION 3.8)

# include directories
include_directories(../include)

# executable
add_executable(mcdx mcdx.cpp getoptwin.c)

# do statically link
set_target_properties(mcdx PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(mcdx PROPERTIES LINK_SEARCH_END_STATIC 1)

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
