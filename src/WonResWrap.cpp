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
extern std::vector<MString> g_encrypted_types;

INT ParseType(const MStringW& input, MIdOrString& type);

BOOL SetWonResPassword(PCWSTR password, PCWSTR salt)
{
	if (!password || !password[0])
	{
		WonClearEncryptionKey();
		return TRUE;
	}

	// WonSetEncryptionPasswordW (crypto.c) rejects any salt shorter than 16
	// bytes. Silently substituting an empty salt here (the previous
	// behavior for salt == nullptr, and the effective behavior whenever a
	// caller passes an empty g_salt) therefore guaranteed a failed call
	// every time -- and since every caller of SetWonResPassword (Res.cpp,
	// RisohEditor.cpp) discards its return value, "password set, salt left
	// blank" silently left encryption disabled instead of enabling it, with
	// no indication of why. Fail explicitly instead, and make sure no stale
	// key from a previous, successful call is left active.
	DWORD cbSalt = salt ? lstrlenW(salt) * sizeof(WCHAR) : 0;
	if (cbSalt < 16)
	{
		WonClearEncryptionKey();
		return FALSE;
	}

	return WonSetEncryptionPasswordW(password, (PBYTE)salt, cbSalt, WON_ENCRYPTION_MIN_ITERATIONS);
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
		for (auto& item : g_encrypted_types)
		{
			MIdOrString type;
			INT ids = ParseType(item, type);
			if (!ids && type == lpType)
			{
				return WonUpdateResourceEncryptedW(hUpdate, lpType, lpName, wLanguage, lpData, cbData);
			}
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
	WonFreeResourceMemory(pMemory);
#endif
}
