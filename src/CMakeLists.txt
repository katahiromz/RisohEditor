# enable Win32 resource
ENABLE_LANGUAGE(RC)

# include directories
include_directories(../include)

add_executable(RisohEditor WIN32 
    PackedDIB.cpp
    RisohEditor.cpp
    Samples.cpp
    RisohEditor_res.rc)
target_link_libraries(RisohEditor comctl32 msimg32 winmm gdiplus vfw32)
target_compile_definitions(RisohEditor PRIVATE -DUNICODE -D_UNICODE)

# do statically link
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_END_STATIC 1)
