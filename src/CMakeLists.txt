# libRisohEditor.a
add_library(libRisohEditor STATIC
    AccelRes.cpp
    DialogRes.cpp
    DlgInitRes.cpp
    IconRes.cpp
    MenuRes.cpp
    MOleHost.cpp
    PackedDIB.cpp
    Res.cpp
    ResToText.cpp
    StringRes.cpp
    ToolbarRes.cpp
    VersionRes.cpp)
set_target_properties(libRisohEditor PROPERTIES PREFIX "")

# RisohEditor.exe
add_executable(RisohEditor WIN32
    RisohEditor.cpp
    RisohEditor_res.rc
    res/Manifest_1.manifest)
target_link_libraries(RisohEditor
    LineNumEdit_static libRisohEditor
    libega gdi32 comctl32 msimg32 winmm gdiplus vfw32 wininet
    oledlg msimg32 advapi32 shlwapi version urlmon)
target_compile_definitions(RisohEditor PRIVATE -DUNICODE -D_UNICODE)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(RisohEditor PROPERTIES OUTPUT_NAME "RisohEditor64")
endif()

# RisohEditorPortable.exe
add_executable(RisohEditorPortable WIN32
    RisohEditor.cpp
    RisohEditor_res.rc
    res/Manifest_1.manifest)
target_link_libraries(RisohEditorPortable
    LineNumEdit_static libRisohEditor
    libega gdi32 comctl32 msimg32 winmm gdiplus vfw32 wininet
    oledlg msimg32 advapi32 shlwapi version urlmon)
target_compile_definitions(RisohEditorPortable PRIVATE -DPORTABLE -DUNICODE -D_UNICODE)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(RisohEditorPortable PROPERTIES OUTPUT_NAME "RisohEditorPortable64")
endif()

option(ATL_SUPPORT "Add ATL support" OFF)

if (MSVC)
    set(DOTRES ".res") # resource object extension
    # ATL Support
    if (ATL_SUPPORT)
        add_definitions(-DATL_SUPPORT)
    endif()
    # No auto manifest
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
else()
    set(DOTRES ".o") # resource object extension
    list(APPEND CMAKE_CXX_LINK_FLAGS -municode) # Unicode support
endif()

# use Unicode
add_definitions(-DUNICODE=1 -D_UNICODE=1)

## messagetables.res or messagetables.o
#add_custom_target(messagetables ALL
#    mcdx ${CMAKE_CURRENT_SOURCE_DIR}/RisohEditor_res.rc --preprocessor=${CMAKE_SOURCE_DIR}\\data\\bin\\cpp.exe ${LANG_DEFS} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/messagetables${DOTRES}
#    DEPENDS mcdx ${CMAKE_CURRENT_SOURCE_DIR}/RisohEditor_res.rc
#    BYPRODUCTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/messagetables${DOTRES}
#    COMMENT "mcdx ${CMAKE_CURRENT_SOURCE_DIR}/RisohEditor_res.rc --preprocessor=${CMAKE_SOURCE_DIR}\\data\\bin\\cpp.exe ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/messagetables${DOTRES}"
#    VERBATIM)
#add_dependencies(RisohEditor messagetables)
#add_dependencies(messagetables mcdx)
#set_target_properties(RisohEditor PROPERTIES LINK_FLAGS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/messagetables${DOTRES})

# do statically link
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(RisohEditor PROPERTIES LINK_SEARCH_END_STATIC 1)
