// protection.h
// Author: katahiromz
// License: CC0

#pragma once

////////////////////////////////////////////////////////////////////////////
// Digital signature

#if defined(NDEBUG) && defined(PROTECTION)
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include <mscat.h>
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

static inline BOOL VerifyCertThumbprint(PCCERT_CONTEXT pCertContext)
{
	// TODO: Set your thumbprint from your signed EXE properties
	static const BYTE expectedThumbprint[20] =
	{
		0x32, 0xf0, 0xd5, 0xec, 0x8c, 0x4a, 0x28, 0x82, 0x27, 0x04,
		0x59, 0xa9, 0x67, 0xf6, 0x46, 0xf3, 0xfc, 0x2b, 0x58, 0x8e
	};

	if (!pCertContext)
		return FALSE;

	BYTE hash[20] = {0};
	DWORD hashSize = sizeof(hash);

	if (!CertGetCertificateContextProperty(
			pCertContext, CERT_SHA1_HASH_PROP_ID, hash, &hashSize))
		return FALSE;

	if (hashSize != sizeof(expectedThumbprint))
		return FALSE;

	return (memcmp(hash, expectedThumbprint, hashSize) == 0);
}

static inline BOOL IsExeSigned(VOID)
{
	WCHAR szPath[MAX_PATH] = {0};
	if (GetModuleFileNameW(NULL, szPath, MAX_PATH) == 0)
		return FALSE;

	WINTRUST_FILE_INFO fileInfo = {0};
	fileInfo.cbStruct	   = sizeof(WINTRUST_FILE_INFO);
	fileInfo.pcwszFilePath  = szPath;

	GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_DATA trustData = {0};
	trustData.cbStruct			  = sizeof(WINTRUST_DATA);
	trustData.dwUIChoice			= WTD_UI_NONE;
	trustData.fdwRevocationChecks   = WTD_REVOKE_NONE;
	trustData.dwUnionChoice		 = WTD_CHOICE_FILE;
	trustData.pFile				 = &fileInfo;
	trustData.dwStateAction		 = WTD_STATEACTION_VERIFY;
	trustData.dwProvFlags		   = WTD_SAFER_FLAG;

	LONG status = WinVerifyTrust(NULL, &action, &trustData);

	BOOL result = FALSE;

	if (status == ERROR_SUCCESS)
	{
		CRYPT_PROVIDER_DATA const* pProvData =
			WTHelperProvDataFromStateData(trustData.hWVTStateData);

		if (pProvData)
		{
			CRYPT_PROVIDER_SGNR* pSigner =
				WTHelperGetProvSignerFromChain(
					(PCRYPT_PROVIDER_DATA)pProvData, 0, FALSE, 0);

			if (pSigner)
			{
				CRYPT_PROVIDER_CERT* pCert =
					WTHelperGetProvCertFromChain(pSigner, 0);

				if (pCert && pCert->pCert)
				{
					result = VerifyCertThumbprint(pCert->pCert);
				}
			}
		}
	}

	trustData.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust(NULL, &action, &trustData);

	return result;
}
#endif // defined(NDEBUG) && defined(PROTECTION)
