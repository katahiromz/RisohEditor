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

# messagetables.res and messagetables.o
if (MSVC)
    add_custom_target(messagetables ALL
        mcdx RisohEditor_res.rc ../build/${CMAKE_CFG_INTDIR}/messagetables.res
        DEPENDS mcdx RisohEditor_res.rc
        BYPRODUCTS ../build/${CMAKE_CFG_INTDIR}/messagetables.res
        COMMENT "mcdx RisohEditor_res.rc ../build/${CMAKE_CFG_INTDIR}/messagetables.res"
        VERBATIM)
    add_dependencies(RisohEditor messagetables)
    add_dependencies(messagetables mcdx)
    set_target_properties(RisohEditor PROPERTIES LINK_FLAGS ../build/${CMAKE_CFG_INTDIR}/messagetables.res)
else()
    add_custom_target(messagetables ALL
        mcdx RisohEditor_res.rc ../build/messagetables.res
        DEPENDS mcdx RisohEditor_res.rc
        BYPRODUCTS ../build/messagetables.res
        COMMENT [[mcdx RisohEditor_res.rc ../build/messagetables.res]]
        VERBATIM)
    add_dependencies(RisohEditor messagetables)
    add_dependencies(messagetables mcdx)
    add_custom_target(res2obj ALL
        windres ../build/messagetables.res ../build/messagetables.o
        DEPENDS messagetables
        BYPRODUCTS ../build/messagetables.o
        COMMENT "windres ../build/messagetables.res ../build/messagetables.o"
        VERBATIM)
    add_dependencies(RisohEditor res2obj)
    set_target_properties(RisohEditor PROPERTIES LINK_FLAGS ../build/messagetables.o)
endif()

target_compile_definitions(RisohEditor PRIVATE -DUNICODE -D_UNICODE)

# do statically link
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_END_STATIC 1)
