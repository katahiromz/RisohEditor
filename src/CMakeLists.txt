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
target_compile_definitions(RisohEditor PRIVATE -DUNICODE -D_UNICODE)

# resource object extension
if (MSVC)
    set(DOTRES ".res")
else()
    set(DOTRES ".o")
endif()

# messagetables.res or messagetables.o
add_custom_target(messagetables ALL
    mcdx RisohEditor_res.rc --preprocessor=..\\data\\bin\\cpp.exe ../build/${CMAKE_CFG_INTDIR}/messagetables${DOTRES}
    DEPENDS mcdx RisohEditor_res.rc
    BYPRODUCTS ../build/${CMAKE_CFG_INTDIR}/messagetables${DOTRES}
    COMMENT "mcdx RisohEditor_res.rc --preprocessor=..\\data\\bin\\cpp.exe ../build/${CMAKE_CFG_INTDIR}/messagetables${DOTRES}"
    VERBATIM)
add_dependencies(RisohEditor messagetables)
add_dependencies(messagetables mcdx)
set_target_properties(RisohEditor PROPERTIES LINK_FLAGS ../build/${CMAKE_CFG_INTDIR}/messagetables${DOTRES})

# do statically link
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_END_STATIC 1)
