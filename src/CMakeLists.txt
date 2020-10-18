# CMake minimum version
cmake_minimum_required(VERSION 2.4)

# use new policy
cmake_policy(SET CMP0046 NEW)
cmake_policy(SET CMP0003 NEW)

# EGA
subdirs(../EGA)

# RisohEditor.exe
add_executable(RisohEditor WIN32
    AccelRes.cpp
    DialogRes.cpp
    DlgInitRes.cpp
    IconRes.cpp
    MenuRes.cpp
    PackedDIB.cpp
    Res.cpp
    ResToText.cpp
    StringRes.cpp
    VersionRes.cpp
    RisohEditor.cpp
    RisohEditor_res.rc
    res/Manifest_1.manifest)
target_link_libraries(RisohEditor libega comctl32 msimg32 winmm gdiplus vfw32 wininet
                                  oledlg msimg32 advapi32 shlwapi version urlmon)
target_compile_definitions(RisohEditor PRIVATE -DUNICODE -D_UNICODE)

# RisohEditorPortable.exe
add_executable(RisohEditorPortable WIN32
    AccelRes.cpp
    DialogRes.cpp
    DlgInitRes.cpp
    IconRes.cpp
    MenuRes.cpp
    PackedDIB.cpp
    Res.cpp
    ResToText.cpp
    StringRes.cpp
    VersionRes.cpp
    RisohEditor.cpp
    RisohEditor_res.rc
    res/Manifest_1.manifest)
target_link_libraries(RisohEditorPortable libega comctl32 msimg32 winmm gdiplus vfw32 wininet
                                          oledlg msimg32 advapi32 shlwapi version urlmon)
target_compile_definitions(RisohEditorPortable PRIVATE -DPORTABLE -DUNICODE -D_UNICODE)

option(ATL_SUPPORT "Add ATL support" ON)

if (MSVC)
    set(DOTRES ".res") # resource object extension
    add_definitions(-DMSVC=1)
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
