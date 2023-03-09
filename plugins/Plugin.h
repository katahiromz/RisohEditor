// Plugin.h --- PluginFramework Plugin interface
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#ifndef PLUGIN_H_
#define PLUGIN_H_
// TODO: Rename this file

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

#ifndef FRAMEWORK_NAME
    #define FRAMEWORK_NAME TEXT("RisohPluginFramework")
#endif

#ifndef FRAMEWORK_SPEC
    #define FRAMEWORK_SPEC TEXT("*.risoh")
#endif

#ifndef FRAMEWORK_VERSION
    #define FRAMEWORK_VERSION 1
#endif

struct PLUGIN;
struct PLUGIN_FRAMEWORK_IMPL;
struct PLUGIN_IMPL;

typedef BOOL (APIENTRY *PLUGIN_LOAD)(struct PLUGIN *pi, LPARAM lParam);
typedef BOOL (APIENTRY *PLUGIN_UNLOAD)(struct PLUGIN *pi, LPARAM lParam);
typedef LRESULT (APIENTRY *PLUGIN_ACT)(struct PLUGIN *pi, UINT uAction, WPARAM wParam, LPARAM lParam);
typedef LRESULT (APIENTRY *PLUGIN_DRIVER)(struct PLUGIN *pi, UINT uFunc, WPARAM wParam, LPARAM lParam);

// Flags for PLUGIN.dwStdFlags
#define PLUGIN_STDFLAG_STANDARD 0x40000000
#define PLUGIN_STDFLAG_RESOURCE 0x20000000
#define PLUGIN_STDFLAG_LANGUAGE 0x10000000

// NOTE: This structure must be a POD (Plain Old Data).
typedef struct PLUGIN
{
    // Don't change:
    DWORD framework_version;
    TCHAR framework_name[32];
    HINSTANCE framework_instance;
    HWND framework_window;
    TCHAR plugin_pathname[MAX_PATH];
    struct PLUGIN_FRAMEWORK_IMPL *framework_impl;
    PLUGIN_DRIVER driver;

    // Please fill them in Plugin_Load:
    DWORD plugin_version;
    TCHAR plugin_product_name[64];
    TCHAR plugin_filename[32];
    TCHAR plugin_company[64];
    TCHAR plugin_copyright[128];
    HINSTANCE plugin_instance;
    DWORD dwStdFlags;
    struct PLUGIN_IMPL *plugin_impl;

    // Use freely:
    HWND plugin_window;
    void *p_user_data;
    LPARAM l_user_data;

    // TODO: Add more members and version up...
} PLUGIN;


#ifdef __cplusplus
extern "C" {
#endif

// API Name: Plugin_Load
// Purpose: The framework want to load the plugin component.
// TODO: Load the plugin component.
BOOL APIENTRY Plugin_Load(PLUGIN *pi, LPARAM lParam);

// API Name: Plugin_Unload
// Purpose: The framework want to unload the plugin component.
// TODO: Unload the plugin component.
BOOL APIENTRY Plugin_Unload(PLUGIN *pi, LPARAM lParam);

// API Name: Plugin_Act
// Purpose: Act something on the plugin.
// TODO: Act something on the plugin.
LRESULT APIENTRY Plugin_Act(PLUGIN *pi, UINT uAction, WPARAM wParam, LPARAM lParam);

//////////////////////////////////////////////////////////////////////////////
// Actions

// Action PLUGIN_ACTION_GETINFO.
//    wParam = DWORD dwType;
//    lParam = PVOID pInfo OPTIONAL.
//    return: TRUE if successful.
#define PLUGIN_ACTION_GETINFO       0x1000

// Action PLUGIN_ACTION_SETINFO.
//    wParam = DWORD dwType.
//    lParam = LPCVOID pInfo OPTIONAL.
//    return: TRUE if successful.
#define PLUGIN_ACTION_SETINFO       0x1001

// Action PLUGIN_ACTION_ACTIVATE.
//    wParam: HWND hwndMainWnd.
//    lParam: Reserved.
//    return: TRUE if successful.
#define PLUGIN_ACTION_ACTIVATE      0x1002

// Action PLUGIN_ACTION_DEACTIVATE.
//    wParam: HWND hwndMainWnd.
//    lParam: Reserved.
//    return: TRUE if successful.
#define PLUGIN_ACTION_DEACTIVATE    0x1003

// Action PLUGIN_ACTION_DRAWCANVAS.
//    wParam = HDC hDC.
//    lParam = LPCRECT prc.
//    return: TRUE if successful.
#define PLUGIN_ACTION_DRAWCANVAS    0x1004

//////////////////////////////////////////////////////////////////////////////
// Driver Functions

// Driver PLUGIN_DRIVER_GETINFO.
//    wParam = DWORD dwType;
//    lParam = PLUGIN_INFO pInfo OPTIONAL.
//    return: TRUE if successful.
#define PLUGIN_DRIVER_GETINFO       0x1000

// Driver PLUGIN_DRIVER_SETINFO.
//    wParam = DWORD dwType.
//    lParam = LPCVOID pInfo OPTIONAL.
//    return: TRUE if successful.
#define PLUGIN_DRIVER_SETINFO       0x1001

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // ndef PLUGIN_H_
