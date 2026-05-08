// WonResWrap.cpp
// Author: katahiromz
// License: GPL-3 or later

#pragma once

HRSRC Wrap_FindResourceExW(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage);
HGLOBAL Wrap_LoadResource(HMODULE hModule, HRSRC hResInfo);
DWORD Wrap_SizeofResource(HMODULE hModule, HRSRC hResInfo);
PVOID Wrap_LockResource(HGLOBAL hResData);
BOOL Wrap_EnumResourceTypesW(HMODULE hModule, ENUMRESTYPEPROCW lpEnumFunc, LONG_PTR lParam);
BOOL Wrap_EnumResourceNamesW(HMODULE hModule, LPCWSTR lpType, ENUMRESNAMEPROCW lpEnumFunc, LONG_PTR lParam);
BOOL Wrap_EnumResourceLanguagesW(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, ENUMRESLANGPROCW lpEnumFunc, LONG_PTR lParam);
HANDLE Wrap_BeginUpdateResourceW(LPCWSTR pFileName, BOOL bDeleteExistingResources);
BOOL Wrap_EndUpdateResourceW(HANDLE hUpdate, BOOL fDiscard);
BOOL Wrap_UpdateResourceW(
    HANDLE hUpdate,
    LPCWSTR lpType,
    LPCWSTR lpName,
    WORD wLanguage,
    LPVOID lpData,
    DWORD cbData);
