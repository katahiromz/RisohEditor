cmake_minimum_required(VERSION 3.8)

# enable Win32 resource
ENABLE_LANGUAGE(RC)

# include directories
include_directories(../include)

# executable
add_executable(mcdx mcdx.cpp)
target_compile_definitions(mcdx PRIVATE -DUNICODE -D_UNICODE)

# do statically link
set_target_properties(mcdx PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(mcdx PROPERTIES LINK_SEARCH_END_STATIC 1)
