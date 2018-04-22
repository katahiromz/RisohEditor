// DlgInit.h --- dialog initialization by RT_DLGINIT            -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_DLGINIT_H_
#define MZC4_DLGINIT_H_     8   /* Version 8 */

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

inline const UNALIGNED WORD *
ExecuteDlgInitEntryDx(HWND hwnd, const UNALIGNED WORD *pw)
{
    // get data
    WORD ctrl = *pw++;
    WORD msg = *pw++;
    WORD w0 = *pw++;
    WORD w1 = *pw++;
    DWORD dwLen = MAKELONG(w0, w1);

    // convert Win16 messages
    switch (msg)
    {
    case AFX_CB_ADDSTRING: msg = CBEM_INSERTITEM; break;
    case WIN16_LB_ADDSTRING: msg = LB_ADDSTRING; break;
    case WIN16_CB_ADDSTRING: msg = CB_ADDSTRING; break;
    }

    // NOTE: We don't support OCC.
    assert(msg == LB_ADDSTRING || msg == CB_ADDSTRING ||
           msg == CBEM_INSERTITEM);

#ifndef NDEBUG
    const BYTE *pb = reinterpret_cast<const BYTE *>(pw);
    assert(pb[dwLen - 1] == 0);
#endif

    // send the message
    if (msg == CBEM_INSERTITEM)
    {
        MString text = MAnsiToText(CP_ACP, LPSTR(pw)).c_str();

        COMBOBOXEXITEM item;
        item.mask = CBEIF_TEXT;
        item.iItem = -1;
        item.pszText = &text[0];

        if (::SendDlgItemMessageA(hwnd, ctrl, msg, 0, LPARAM(&item)) == -1)
            return NULL;
    }
    else if (msg == LB_ADDSTRING || msg == CB_ADDSTRING)
    {
        if (::SendDlgItemMessageA(hwnd, ctrl, msg, 0, LPARAM(pw)) == -1)
            return NULL;
    }

    // go to next entry
    return reinterpret_cast<const UNALIGNED WORD *>(
        reinterpret_cast<const BYTE *>(pw) + dwLen);
}

inline BOOL
ExecuteDlgInitDataDx(HWND hwnd, const void *pData)
{
#ifndef NDEBUG
    DWORD i = 0;
#endif
    const UNALIGNED WORD *pw;
    pw = reinterpret_cast<const UNALIGNED WORD *>(pData);
    while (pw && *pw)
    {
        pw = ExecuteDlgInitEntryDx(hwnd, pw);
#ifndef NDEBUG
        ++i;
#endif
    }

    // NOTE: We don't send WM_INITIALUPDATE messages.
    return pw != NULL;
}

//////////////////////////////////////////////////////////////////////////////

inline BOOL
ExecuteDlgInitDx(HWND hwnd, HMODULE module, const TCHAR *res_name)
{
    HGLOBAL hGlobal = NULL;
    void *pData = NULL;
    BOOL bSuccess = FALSE;

    // load from resource
    if (HRSRC hRsrc = FindResource(module, res_name, RT_DLGINIT))
    {
        hGlobal = LoadResource(module, hRsrc);
        pData = LockResource(hGlobal);
    }

    // execute
    if (pData)
    {
        bSuccess = ExecuteDlgInitDataDx(hwnd, pData);
    }

    // clean up
    UnlockResource(hGlobal);
    FreeResource(hGlobal);

    return bSuccess;
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_DLGINIT_H_
