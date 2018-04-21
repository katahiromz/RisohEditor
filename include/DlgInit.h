// DlgInit.h --- dialog initialization by RT_DLGINIT            -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_DLGINIT_H_
#define MZC4_DLGINIT_H_     4   /* Version 4 */

// RT_DLGINIT
// BOOL ExecuteDlgInitDx(HWND hwnd, HMODULE module, const TCHAR *res_name);

//////////////////////////////////////////////////////////////////////////////

#ifdef RC_INVOKED
    #ifndef DLGINIT
        #define DLGINIT     240
    #endif
#else   // ndef RC_INVOKED
    #include "MWindowBase.hpp"
    #include "MString.hpp"

    #ifndef RT_DLGINIT
        #define RT_DLGINIT  MAKEINTRESOURCE(240)
    #endif

    //////////////////////////////////////////////////////////////////////////////

    inline BOOL
    ExecuteDlgInitDataDx(HWND hwnd, const void *pData)
    {
        if (!pData)
            return FALSE;   // failure

        BOOL bSuccess = TRUE;
        const UNALIGNED WORD *pw =
            reinterpret_cast<const UNALIGNED WORD *>(pData);
        while (bSuccess && *pw != 0)
        {
            // get data
            WORD nIDC_ = *pw++;
            WORD msg = *pw++;
            DWORD dwLen = *reinterpret_cast<const UNALIGNED DWORD *&>(pw)++;

#define WIN16_LB_ADDSTRING  0x0401
#define WIN16_CB_ADDSTRING  0x0403
#define AFX_CB_ADDSTRING    0x1234

            // convert 16-bit messages
            switch (msg)
            {
            case AFX_CB_ADDSTRING: msg = CBEM_INSERTITEM; break;
            case WIN16_LB_ADDSTRING: msg = LB_ADDSTRING; break;
            case WIN16_CB_ADDSTRING: msg = CB_ADDSTRING; break;
            }

            // NOTE: There is no OCC support.
            assert(msg == LB_ADDSTRING || msg == CB_ADDSTRING ||
                   msg == CBEM_INSERTITEM);

#ifndef NDEBUG
            if (msg == LB_ADDSTRING || msg == CB_ADDSTRING ||
                msg == CBEM_INSERTITEM)
            {
                assert(*(reinterpret_cast<const BYTE *>(pw) + (dwLen - 1)) == 0);
            }
#endif

            // send the message
            if (msg == CBEM_INSERTITEM)
            {
                MString text = MAnsiToText(CP_ACP, LPSTR(pw)).c_str();

                COMBOBOXEXITEM item;
                item.mask = CBEIF_TEXT;
                item.iItem = -1;
                item.pszText = &text[0];

                if (::SendDlgItemMessageA(hwnd, nIDC_, msg, 0, LPARAM(&item)) == -1)
                    bSuccess = FALSE;
            }
            else
            {
                if (::SendDlgItemMessageA(hwnd, nIDC_, msg, 0, LPARAM(pw)) == -1)
                    bSuccess = FALSE;
            }

            // go to next entry
            pw = reinterpret_cast<const WORD *>(
                reinterpret_cast<const BYTE *>(pw) + dwLen);
        }

        // NOTE: We don't send WM_INITIALUPDATE messages.

        return bSuccess;
    }

    //////////////////////////////////////////////////////////////////////////////

    inline BOOL
    ExecuteDlgInitDx(HWND hwnd, HMODULE module, const TCHAR *res_name)
    {
        HGLOBAL hGlobal = NULL;
        void *pData = NULL;
        BOOL bSuccess = FALSE;

        // load from resource
        if (HRSRC hRsrc = FindResource(module, RT_DLGINIT, res_name))
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
#endif  // ndef RC_INVOKED

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_DLGINIT_H_
