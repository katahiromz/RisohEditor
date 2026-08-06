// WonResWrap.cpp
// Author: katahiromz
// License: GPL-3 or later
#include <windows.h>
#include "RisohSettings.hpp"
#define WONRES_ENABLE_CRYPTO
#include "WonRes.h"

#ifdef ENABLE_CRYPTO
BOOL g_bEnableCrypto = FALSE;
MStringW g_password;
MStringW g_salt;

BOOL SetWonResPassword(PCWSTR password, PCWSTR salt)
{
	if (!password || !password[0])
	{
		WonClearEncryptionKey();
		return TRUE;
	}
	if (salt == nullptr)
		salt = L"";
	return WonSetEncryptionPasswordW(password, (PBYTE)salt, lstrlenW(salt) * sizeof(WCHAR), WON_ENCRYPTION_MIN_ITERATIONS);
}
#endif

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
#ifdef ENABLE_CRYPTO
	if (g_bEnableCrypto && g_password.size())
	{
#ifndef RT_DLGINIT
	#define RT_DLGINIT  MAKEINTRESOURCE(240)
#endif
		if (lpType != RT_ACCELERATOR &&
			lpType != RT_ANICURSOR &&
			lpType != RT_ANIICON &&
			lpType != RT_CURSOR &&
			lpType != RT_DIALOG &&
			lpType != RT_DLGINIT &&
			lpType != RT_GROUP_CURSOR &&
			lpType != RT_GROUP_ICON &&
			lpType != RT_ICON &&
			lpType != RT_MANIFEST &&
			lpType != RT_MENU &&
			lpType != RT_MESSAGETABLE &&
			lpType != RT_VERSION)
		{
			return WonUpdateResourceEncryptedW(hUpdate, lpType, lpName, wLanguage, lpData, cbData);
		}
	}
#endif
	if (g_settings.bUseWonRes)
		return WonUpdateResourceW(hUpdate, lpType, lpName, wLanguage, lpData, cbData);
	return UpdateResourceW(hUpdate, lpType, lpName, wLanguage, lpData, cbData);
}

VOID Wrap_FreeResourceMemory(LPVOID pMemory)
{
#ifdef ENABLE_CRYPTO
	if (g_bEnableCrypto)
		WonFreeResourceMemory(pMemory);
#endif
}
