// DlgInit.h --- dialog initialization by RT_DLGINIT            -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_DLGINIT_H_
#define MZC4_DLGINIT_H_     9   /* Version 9 */

// RT_DLGINIT
// BOOL ExecuteDlgInitDx(HWND hwnd, HMODULE module, const TCHAR *res_name);

//////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include "MString.hpp"

#ifndef RT_DLGINIT
	#define RT_DLGINIT  MAKEINTRESOURCE(240)
#endif

// Win16 messages
#define WIN16_LB_ADDSTRING  0x0401
#define WIN16_CB_ADDSTRING  0x0403
#define AFX_CB_ADDSTRING    0x1234

//////////////////////////////////////////////////////////////////////////////

inline const WORD *
ExecuteDlgInitEntryDx(HWND hwnd, const WORD *pw, SIZE_T& cbData)
{
    const SIZE_T cbHeader = 4 * sizeof(WORD);
    if (cbData < cbHeader)
        return nullptr;

    WORD ctrl = *pw++;
    WORD msg  = *pw++;
    WORD w0   = *pw++;
    WORD w1   = *pw++;
	DWORD dwLen = MAKELONG(w0, w1);
    cbData -= cbHeader;

    if (cbData < dwLen)
        return nullptr;

    switch (msg)
    {
    case AFX_CB_ADDSTRING:   msg = CBEM_INSERTITEM; break;
    case WIN16_LB_ADDSTRING: msg = LB_ADDSTRING; break;
    case WIN16_CB_ADDSTRING: msg = CB_ADDSTRING; break;
    }

    assert(msg == LB_ADDSTRING || msg == CB_ADDSTRING || msg == CBEM_INSERTITEM);

#ifndef NDEBUG
    if (dwLen != 0)
    {
        const BYTE *pb = reinterpret_cast<const BYTE *>(pw);
        assert(pb[dwLen - 1] == 0);
    }
#endif

    if (msg == CBEM_INSERTITEM)
    {
        LPCSTR pszText = reinterpret_cast<LPCSTR>(pw);
        COMBOBOXEXITEMA item = {};
        item.mask = CBEIF_TEXT;
        item.iItem = -1;
        item.pszText = const_cast<LPSTR>(pszText);

        if (::SendDlgItemMessageA(hwnd, ctrl, msg, 0, reinterpret_cast<LPARAM>(&item)) == -1)
            return nullptr;
    }
    else if (msg == LB_ADDSTRING || msg == CB_ADDSTRING)
    {
        if (::SendDlgItemMessageA(hwnd, ctrl, msg, 0, reinterpret_cast<LPARAM>(pw)) == -1)
            return nullptr;
    }

    cbData -= dwLen;
    return reinterpret_cast<const WORD *>(reinterpret_cast<const BYTE *>(pw) + dwLen);
}

inline BOOL
ExecuteDlgInitDataDx(HWND hwnd, const void *pData, SIZE_T& cbData)
{
	const UNALIGNED WORD *pw;
	pw = reinterpret_cast<const UNALIGNED WORD *>(pData);
	while (pw && cbData >= sizeof(WORD) && *pw)
	{
		pw = ExecuteDlgInitEntryDx(hwnd, pw, cbData);
	}

	// NOTE: We don't send WM_INITIALUPDATE messages.
	return pw != nullptr;
}

//////////////////////////////////////////////////////////////////////////////

inline BOOL
ExecuteDlgInitDx(HWND hwnd, HMODULE module, const TCHAR *res_name)
{
	HRSRC hRsrc = FindResource(module, res_name, RT_DLGINIT);
	if (!hRsrc)
        return FALSE;

	HGLOBAL hGlobal = LoadResource(module, hRsrc);
	if (!hGlobal)
		return FALSE;

	BOOL bOK = FALSE;
	SIZE_T cbData = SizeofResource(module, hRsrc);
	PVOID pData = LockResource(hGlobal);
	if (pData && cbData)
		bOK = ExecuteDlgInitDataDx(hwnd, pData, cbData);

	UnlockResource(hGlobal);
	FreeResource(hGlobal);
	return bOK;
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_DLGINIT_H_
