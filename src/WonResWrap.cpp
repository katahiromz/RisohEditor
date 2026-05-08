// WonResWrap.cpp
// Author: katahiromz
// License: GPL-3 or later
#include <windows.h>
#include "../WonRes/WonRes.h"

HRSRC Wrap_FindResourceExW(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage)
{
    return WonFindResourceExW(hModule, lpType, lpName, wLanguage);
}

HGLOBAL Wrap_LoadResource(HMODULE hModule, HRSRC hResInfo)
{
    return WonLoadResource(hModule, hResInfo);
}

DWORD Wrap_SizeofResource(HMODULE hModule, HRSRC hResInfo)
{
    return WonSizeofResource(hModule, hResInfo);
}

PVOID Wrap_LockResource(HGLOBAL hResData)
{
    return WonLockResource(hResData);
}

BOOL Wrap_EnumResourceTypesW(HMODULE hModule, ENUMRESTYPEPROCW lpEnumFunc, LONG_PTR lParam)
{
    return WonEnumResourceTypesW(hModule, lpEnumFunc, lParam);
}

BOOL Wrap_EnumResourceNamesW(HMODULE hModule, LPCWSTR lpType, ENUMRESNAMEPROCW lpEnumFunc, LONG_PTR lParam)
{
    return WonEnumResourceNamesW(hModule, lpType, lpEnumFunc, lParam);
}

BOOL Wrap_EnumResourceLanguagesW(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, ENUMRESLANGPROCW lpEnumFunc, LONG_PTR lParam)
{
    return WonEnumResourceLanguagesW(hModule, lpType, lpName, lpEnumFunc, lParam);
}

HANDLE Wrap_BeginUpdateResourceW(LPCWSTR pFileName, BOOL bDeleteExistingResources)
{
    return WonBeginUpdateResourceW(pFileName, bDeleteExistingResources);
}

BOOL Wrap_EndUpdateResourceW(HANDLE hUpdate, BOOL fDiscard)
{
    return WonEndUpdateResourceW(hUpdate, fDiscard);
}

BOOL Wrap_UpdateResourceW(
    HANDLE hUpdate,
    LPCWSTR lpType,
    LPCWSTR lpName,
    WORD wLanguage,
    LPVOID lpData,
    DWORD cbData)
{
    return WonUpdateResourceW(hUpdate, lpType, lpName, wLanguage, lpData, cbData);
}
