// WonResWrap.cpp
// Author: katahiromz
// License: GPL-3 or later
#include <windows.h>
#include "RisohSettings.hpp"
#include "WonRes.h"

HRSRC Wrap_FindResourceExW(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage)
{
	if (g_settings.bUseWonRes)
		return WonFindResourceExW(hModule, lpType, lpName, wLanguage);
	return FindResourceExW(hModule, lpType, lpName, wLanguage);
}

HGLOBAL Wrap_LoadResource(HMODULE hModule, HRSRC hResInfo)
{
	if (g_settings.bUseWonRes)
		return WonLoadResource(hModule, hResInfo);
	return LoadResource(hModule, hResInfo);
}

DWORD Wrap_SizeofResource(HMODULE hModule, HRSRC hResInfo)
{
	if (g_settings.bUseWonRes)
		return WonSizeofResource(hModule, hResInfo);
	return SizeofResource(hModule, hResInfo);;
}

PVOID Wrap_LockResource(HGLOBAL hResData)
{
	if (g_settings.bUseWonRes)
		return WonLockResource(hResData);
	return LockResource(hResData);
}

BOOL Wrap_EnumResourceTypesW(HMODULE hModule, ENUMRESTYPEPROCW lpEnumFunc, LONG_PTR lParam)
{
	if (g_settings.bUseWonRes)
		return WonEnumResourceTypesW(hModule, lpEnumFunc, lParam);
	return EnumResourceTypesW(hModule, lpEnumFunc, lParam);
}

BOOL Wrap_EnumResourceNamesW(HMODULE hModule, LPCWSTR lpType, ENUMRESNAMEPROCW lpEnumFunc, LONG_PTR lParam)
{
	if (g_settings.bUseWonRes)
		return WonEnumResourceNamesW(hModule, lpType, lpEnumFunc, lParam);
	return EnumResourceNamesW(hModule, lpType, lpEnumFunc, lParam);
}

BOOL Wrap_EnumResourceLanguagesW(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, ENUMRESLANGPROCW lpEnumFunc, LONG_PTR lParam)
{
	if (g_settings.bUseWonRes)
		return WonEnumResourceLanguagesW(hModule, lpType, lpName, lpEnumFunc, lParam);
	return EnumResourceLanguagesW(hModule, lpType, lpName, lpEnumFunc, lParam);
}

HANDLE Wrap_BeginUpdateResourceW(LPCWSTR pFileName, BOOL bDeleteExistingResources)
{
	if (g_settings.bUseWonRes)
		return WonBeginUpdateResourceW(pFileName, bDeleteExistingResources);
	return BeginUpdateResourceW(pFileName, bDeleteExistingResources);
}

BOOL Wrap_EndUpdateResourceW(HANDLE hUpdate, BOOL fDiscard)
{
	if (g_settings.bUseWonRes)
		return WonEndUpdateResourceW(hUpdate, fDiscard);
	return EndUpdateResourceW(hUpdate, fDiscard);
}

BOOL Wrap_UpdateResourceW(
	HANDLE hUpdate,
	LPCWSTR lpType,
	LPCWSTR lpName,
	WORD wLanguage,
	LPVOID lpData,
	DWORD cbData)
{
	if (g_settings.bUseWonRes)
		return WonUpdateResourceW(hUpdate, lpType, lpName, wLanguage, lpData, cbData);
	return UpdateResourceW(hUpdate, lpType, lpName, wLanguage, lpData, cbData);
}
