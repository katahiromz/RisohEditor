# enable Win32 resource
ENABLE_LANGUAGE(RC)

# include directories
include_directories(../include)

# RisohEditor.exe
add_executable(RisohEditor WIN32 
    PackedDIB.cpp
    RisohEditor.cpp
    Samples.cpp
    RisohEditor_res.rc)
target_link_libraries(RisohEditor comctl32 msimg32 winmm gdiplus vfw32)

# messagetables.res
add_custom_target(messagetables ALL
    mcdx RisohEditor_res.rc ../build/messagetables.res
    DEPENDS mcdx 
    BYPRODUCTS ../build/messagetables.res
    COMMENT [[ mcdx RisohEditor_res.rc ../build/messagetables.res ]]
    VERBATIM)
add_dependencies(RisohEditor messagetables)
add_dependencies(messagetables mcdx)
set_target_properties(RisohEditor PROPERTIES LINK_FLAGS ../build/messagetables.res)

target_compile_definitions(RisohEditor PRIVATE -DUNICODE -D_UNICODE)

# do statically link
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_END_STATIC 1)
