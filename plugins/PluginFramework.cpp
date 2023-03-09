// PluginFramework.cpp --- PluginFramework
// Copyright (C) 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.

#include "PluginFramework.h"
#include <cstdio>
#include <cassert>
#include <shlwapi.h>
#include <strsafe.h>

struct PLUGIN_FRAMEWORK_IMPL
{
    PLUGIN_LOAD load;
    PLUGIN_UNLOAD unload;
    PLUGIN_ACT act;

    PLUGIN_FRAMEWORK_IMPL()
    {
    }
private:
    PLUGIN_FRAMEWORK_IMPL(const PLUGIN_FRAMEWORK_IMPL&);
    PLUGIN_FRAMEWORK_IMPL& operator=(const PLUGIN_FRAMEWORK_IMPL&);
};

static PLUGIN_DRIVER s_driver = NULL;

static LRESULT APIENTRY driver(struct PLUGIN *pi, UINT uFunc, WPARAM wParam, LPARAM lParam)
{
    if (!s_driver)
        return 0;

    return s_driver(pi, uFunc, wParam, lParam);
}

BOOL PF_SetDriver(PLUGIN_DRIVER driver)
{
    s_driver = driver;
    return TRUE;
}

static void PF_Init(PLUGIN *pi)
{
    assert(pi);
    ZeroMemory(pi, sizeof(*pi));
    pi->framework_version = FRAMEWORK_VERSION;
    StringCbCopy(pi->framework_name, sizeof(pi->framework_name), FRAMEWORK_NAME);
    pi->framework_instance = GetModuleHandle(NULL);
    pi->framework_window = NULL;
    pi->driver = driver;
    //pi->driver = NULL;
}

static BOOL PF_Validate(PLUGIN *pi)
{
    if (!pi)
    {
        assert(0);
        return FALSE;
    }

    pi->plugin_product_name[ARRAYSIZE(pi->plugin_product_name) - 1] = 0;
    pi->plugin_filename[ARRAYSIZE(pi->plugin_filename) - 1] = 0;
    pi->plugin_company[ARRAYSIZE(pi->plugin_company) - 1] = 0;
    pi->plugin_copyright[ARRAYSIZE(pi->plugin_copyright) - 1] = 0;

    if (pi->framework_version != FRAMEWORK_VERSION)
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

    static const TCHAR s_szSpace[] = TEXT(" \t\n\r\f\v");
    StrTrim(pi->plugin_product_name, s_szSpace);
    StrTrim(pi->plugin_company, s_szSpace);
    StrTrim(pi->plugin_copyright, s_szSpace);

    if (pi->plugin_product_name[0] == 0)
    {
        assert(0);
        return FALSE;
    }
    if (pi->plugin_filename[0] == 0)
    {
        assert(0);
        return FALSE;
    }
    if (pi->plugin_company[0] == 0)
    {
        assert(0);
        return FALSE;
    }
    if (pi->plugin_copyright[0] == 0)
    {
        assert(0);
        return FALSE;
    }

    LPCTSTR filename = PathFindFileName(pi->plugin_pathname);
    if (!filename || lstrcmpi(filename, pi->plugin_filename) != 0)
    {
        assert(0);
        return FALSE;
    }

    if (!pi->plugin_instance)
    {
        assert(0);
        return FALSE;
    }

    if (!(pi->dwStdFlags & PLUGIN_STDFLAG_RESOURCE))
        return FALSE; // Currently not supported

    return TRUE;
}

BOOL PF_IsLoaded(const PLUGIN *pi)
{
    return pi && !!pi->plugin_instance;
}

LRESULT PF_ActOne(PLUGIN *pi, UINT uAction, WPARAM wParam, LPARAM lParam)
{
    if (!pi || !pi->framework_impl || !pi->framework_impl->act)
    {
        assert(0);
        return FALSE;
    }

    LRESULT ret = pi->framework_impl->act(pi, uAction, wParam, lParam);
    return ret;
}

LRESULT PF_ActAll(std::vector<PLUGIN>& pis, UINT uAction, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = 0;
    for (size_t i = 0; i < pis.size(); ++i)
    {
        ret = PF_ActOne(&pis[i], uAction, wParam, lParam);
    }
    return ret;
}

BOOL PF_UnloadOne(PLUGIN *pi)
{
    if (!pi || !pi->framework_impl || !pi->framework_impl->unload)
    {
        assert(0);
        return FALSE;
    }

    BOOL ret = pi->framework_impl->unload(pi, 0);
    pi->plugin_instance = NULL;
    delete pi->framework_impl;
    return ret;
}

BOOL PF_UnloadAll(std::vector<PLUGIN>& pis)
{
    for (size_t i = 0; i < pis.size(); ++i)
    {
        PF_UnloadOne(&pis[i]);
    }
    pis.clear();
    return TRUE;
}

BOOL PF_LoadOne(PLUGIN *pi, const TCHAR *pathname)
{
    PF_Init(pi);

    assert(pathname != NULL);
    GetFullPathName(pathname, ARRAYSIZE(pi->plugin_pathname),
                    pi->plugin_pathname, NULL);

    DWORD attrs = GetFileAttributes(pi->plugin_pathname);
    if (attrs == 0xFFFFFFFF || (attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
#ifndef NDEBUG
        //printf("'%s'\n", pi->plugin_pathname);
        assert(0);
#endif
        return FALSE;
    }

    HINSTANCE hInstDLL = LoadLibrary(pi->plugin_pathname);
    if (!hInstDLL)
    {
        assert(0);
        return FALSE;
    }

    pi->framework_impl = new PLUGIN_FRAMEWORK_IMPL;
    pi->plugin_impl = NULL;

    // Load API functions
    pi->framework_impl->load =
        (PLUGIN_LOAD)GetProcAddress(hInstDLL, "Plugin_Load");
    pi->framework_impl->unload =
        (PLUGIN_UNLOAD)GetProcAddress(hInstDLL, "Plugin_Unload");
    pi->framework_impl->act =
        (PLUGIN_ACT)GetProcAddress(hInstDLL, "Plugin_Act");

    if (!pi->framework_impl->load ||
        !pi->framework_impl->unload ||
        !pi->framework_impl->act)
    {
        assert(0);
        FreeLibrary(hInstDLL);
        return FALSE;
    }

    pi->plugin_instance = hInstDLL;

    if (pi->framework_impl->load(pi, 0) && PF_Validate(pi))
        return TRUE;

    assert(0);

    pi->framework_impl->unload(pi, 0);
    delete pi->framework_impl;

    FreeLibrary(hInstDLL);
    return FALSE;
}

INT PF_LoadAll(std::vector<PLUGIN>& pis, const TCHAR *dir)
{
    pis.clear();

    if (!dir || dir[0] == 0)
    {
        dir = TEXT(".");
    }

    TCHAR szSpec[MAX_PATH];
    StringCbCopy(szSpec, sizeof(szSpec), dir);
    PathAppend(szSpec, FRAMEWORK_SPEC);
#ifndef NDEBUG
    //printf(szSpec: "'%s'\n", szSpec);
#endif

    TCHAR szPath[MAX_PATH];
    WIN32_FIND_DATA find;
    HANDLE hFind = FindFirstFile(szSpec, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            PLUGIN plugin;

            StringCbCopy(szPath, sizeof(szPath), dir);
            PathAppend(szPath, find.cFileName);
#ifndef NDEBUG
            //printf("szPath: %s\n", szPath);
#endif
            if (PF_LoadOne(&plugin, szPath))
            {
                pis.push_back(plugin);
            }
        } while (FindNextFile(hFind, &find));

        FindClose(hFind);
    }

    return INT(pis.size());
}
