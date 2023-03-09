// Plugin1.c --- PluginFramework Plugin #1
// Copyright (C) 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#include "../Plugin.h"
#include <assert.h>
#include <strsafe.h>

static HINSTANCE s_hinstDLL;

struct PLUGIN_IMPL
{
    HWND m_hMainWnd;

    PLUGIN_IMPL() : m_hMainWnd(NULL)
    {
    }

private:
    PLUGIN_IMPL(const PLUGIN_IMPL&);
    PLUGIN_IMPL& operator=(const PLUGIN_IMPL&);
};

// API Name: Plugin_Load
// Purpose: The framework want to load the plugin component.
// TODO: Load the plugin component.
BOOL APIENTRY
Plugin_Load(PLUGIN *pi, LPARAM lParam)
{
    if (!pi)
    {
        assert(0);
        return FALSE;
    }
    if (pi->framework_version < FRAMEWORK_VERSION)
    {
        assert(0);
        return FALSE;
    }
    if (lstrcmpi(pi->framework_name, FRAMEWORK_NAME) != 0)
    {
        assert(0);
        return FALSE;
    }
    if (pi->framework_instance == NULL)
    {
        assert(0);
        return FALSE;
    }

    pi->plugin_version = 1;
    StringCbCopy(pi->plugin_product_name, sizeof(pi->plugin_product_name), TEXT("Plugin #1"));
    StringCbCopy(pi->plugin_filename, sizeof(pi->plugin_filename), TEXT("Plugin1.risoh"));
    StringCbCopy(pi->plugin_company, sizeof(pi->plugin_company), TEXT("Katayama Hirofumi MZ"));
    StringCbCopy(pi->plugin_copyright, sizeof(pi->plugin_copyright), TEXT("Copyright (C) 2023 Katayama Hirofumi MZ"));
    pi->plugin_instance = s_hinstDLL;
    pi->plugin_window = NULL;
    pi->dwStdFlags = PLUGIN_STDFLAG_STANDARD | PLUGIN_STDFLAG_RESOURCE;
    pi->plugin_impl = new PLUGIN_IMPL;

    return TRUE;
}

// API Name: Plugin_Unload
// Purpose: The framework want to unload the plugin component.
// TODO: Unload the plugin component.
BOOL APIENTRY
Plugin_Unload(PLUGIN *pi, LPARAM lParam)
{
    delete pi->plugin_impl;
    return TRUE;
}

// API Name: Plugin_Act
// Purpose: Act something on the plugin.
// TODO: Act something on the plugin.
LRESULT APIENTRY
Plugin_Act(PLUGIN *pi, UINT uAction, WPARAM wParam, LPARAM lParam)
{
    HWND hMainWnd;
    switch (uAction)
    {
    case PLUGIN_ACTION_GETINFO:
        break;
    case PLUGIN_ACTION_SETINFO:
        break;
    case PLUGIN_ACTION_ACTIVATE:
        hMainWnd = (HWND)wParam;
        pi->plugin_impl->m_hMainWnd = hMainWnd;
        MessageBoxA(hMainWnd, "activate", NULL, 0);
        break;
    case PLUGIN_ACTION_DEACTIVATE:
        hMainWnd = (HWND)wParam;
        assert(pi->plugin_impl->m_hMainWnd == hMainWnd);
        MessageBoxA(hMainWnd, "de-activate", NULL, 0);
        break;
    case PLUGIN_ACTION_DRAWCANVAS:
        break;
    }
    return 0;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        s_hinstDLL = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
