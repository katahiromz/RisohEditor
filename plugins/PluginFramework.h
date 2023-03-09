// PluginFramework.h --- PluginFramework
// Copyright (C) 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#pragma once

#include "Plugin.h"
#include <vector>

BOOL PF_LoadOne(PLUGIN *pi, const TCHAR *pathname);
INT PF_LoadAll(std::vector<PLUGIN>& pis, const TCHAR *dir);
BOOL PF_IsLoaded(const PLUGIN *pi);
LRESULT PF_ActOne(PLUGIN *pi, UINT uAction, WPARAM wParam, LPARAM lParam);
LRESULT PF_ActAll(std::vector<PLUGIN>& pis, UINT uAction, WPARAM wParam, LPARAM lParam);
BOOL PF_UnloadOne(PLUGIN *pi);
BOOL PF_UnloadAll(std::vector<PLUGIN>& pis);
