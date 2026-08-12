# libRisohEditor.a
add_library(libRisohEditor STATIC
    AccelRes.cpp
    DialogRes.cpp
    DlgInitRes.cpp
    EgaBridge.cpp
    IconRes.cpp
    MBitmapDx.cpp
    MEditMenuDlg.cpp
    MEgaDlg.cpp
    MenuRes.cpp
    MIDListDlg.cpp
    MMainWnd_Command.cpp
    MMainWnd_Preview.cpp
    MMainWnd_Toolbar.cpp
    MMainWnd_EGA.cpp
    MMainWnd_Tests.cpp
    MOleHost.cpp
    MRadWindow.cpp
    MSplitterWnd.cpp
    PackedDIB.cpp
    Res.cpp
    ResToText.cpp
    StringRes.cpp
    ToolbarRes.cpp
    Utils.cpp
    VersionRes.cpp
    WonResWrap.cpp)
target_link_libraries(libRisohEditor PUBLIC WonRes)
set_target_properties(libRisohEditor PROPERTIES PREFIX "")

# Create object libraries for shared source files.
add_library(RisohEditorMain_obj OBJECT RisohEditor.cpp)
add_library(settings_obj OBJECT settings.cpp)
add_library(settingsPortable_obj OBJECT settings.cpp)
target_compile_definitions(settingsPortable_obj PRIVATE PORTABLE)
target_link_libraries(RisohEditorMain_obj PRIVATE WonRes)
target_link_libraries(settings_obj PRIVATE WonRes)
target_link_libraries(settingsPortable_obj PRIVATE WonRes)

# RisohEditor.exe
add_executable(RisohEditor WIN32
	$<TARGET_OBJECTS:RisohEditorMain_obj>
	$<TARGET_OBJECTS:settings_obj>
	RisohEditor_res.rc
	res/Manifest_1.manifest)
target_link_libraries(RisohEditor
    LineNumEdit_static libRisohEditor
    libega gdi32 comctl32 msimg32 winmm gdiplus vfw32 wininet crypt32 wintrust
    oledlg msimg32 advapi32 shlwapi version urlmon WonRes)
target_compile_definitions(RisohEditor PRIVATE -DUNICODE -D_UNICODE)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(RisohEditor PROPERTIES OUTPUT_NAME "RisohEditor64")
endif()
if(MSVC)
    target_link_options(RisohEditor PRIVATE "LINKER:/MANIFEST:NO")
endif()

# RisohEditorPortable.exe
add_executable(RisohEditorPortable WIN32
	$<TARGET_OBJECTS:RisohEditorMain_obj>
	$<TARGET_OBJECTS:settingsPortable_obj>
	RisohEditor_res.rc
	res/Manifest_1.manifest)
target_link_libraries(RisohEditorPortable
    LineNumEdit_static libRisohEditor
    libega gdi32 comctl32 msimg32 winmm gdiplus vfw32 wininet crypt32 wintrust
    oledlg msimg32 advapi32 shlwapi version urlmon WonRes)
target_compile_definitions(RisohEditorPortable PRIVATE -DPORTABLE -DUNICODE -D_UNICODE)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(RisohEditorPortable PROPERTIES OUTPUT_NAME "RisohEditorPortable64")
endif()
if(MSVC)
    target_link_options(RisohEditorPortable PRIVATE "LINKER:/MANIFEST:NO")
endif()

option(ATL_SUPPORT "Add ATL support" OFF)

if (MSVC)
	set(DOTRES ".res") # resource object extension
	# Enable parallel C++ compilation for build targets.
	foreach(target_name
		libRisohEditor
		RisohEditorMain_obj
		settings_obj
		settingsPortable_obj)
		target_compile_options(${target_name} PRIVATE /MP)
	endforeach()
	# ATL Support
	if (ATL_SUPPORT)
		add_definitions(-DATL_SUPPORT)
	endif()
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
