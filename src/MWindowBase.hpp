// MWindowBase.hpp --- MZC4 window base
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MWINDOWBASE_HPP_
#define MZC4_MWINDOWBASE_HPP_    23   /* Version 23 */

#pragma once

//////////////////////////////////////////////////////////////////////////////
// headers

// Win32 API headers
#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32 API
#endif
#ifndef _INC_WINDOWSX
    #include <windowsx.h>   // Win32 Macro APIs
#endif
#ifndef _INC_COMMCTRL
    #include <commctrl.h>   // common controls
#endif
#ifndef _INC_COMMDLG
    #include <commdlg.h>    // common dialogs
#endif
#ifndef _INC_TCHAR
    #include <tchar.h>      // generic text mappings
#endif

#include <dlgs.h>           // dialog control IDs

// standard C/C++ library
#include <string>           // std::string and std::wstring
#include <cassert>          // assert
#include <cstring>          // C string library

#include "resource.h"       // resource IDs

//////////////////////////////////////////////////////////////////////////////

#ifndef _countof
    #define _countof(array)     (sizeof(array) / sizeof(array[0]))
#endif

// tstring
#ifndef tstring
    #ifdef UNICODE
        typedef std::wstring    tstring;
    #else
        typedef std::string     tstring;
    #endif
#endif

// NOTE: Old Digital Mars C/C++ Compiler doesn't define INT_PTR type likely.
#ifdef __DMC__
    #ifndef INT_PTR
        #ifdef _WIN64
            #define INT_PTR     LPARAM
        #else
            #define INT_PTR     BOOL
        #endif
    #endif
#endif

struct MWindowBase;
struct MDialogBase;

//////////////////////////////////////////////////////////////////////////////
// public functions

#ifndef MZCAPI
    #define MZCAPI      WINAPI
#endif

#ifndef MZCAPIV
    #define MZCAPIV     WINAPIV
#endif

void MZCAPIV DebugPrintDx(const char *format, ...);
void MZCAPIV DebugPrintDx(const WCHAR *format, ...);
void MZCAPI GetVirtualScreenRectDx(LPRECT prc);
void MZCAPI RepositionPointDx(LPPOINT ppt, SIZE siz, LPCRECT prc);
void MZCAPI WorkAreaFromWindowDx(LPRECT prcWorkArea, HWND hwnd);
SIZE MZCAPI SizeFromRectDx(LPCRECT prc);
LPTSTR MZCAPI LoadStringDx(INT nID);
LPTSTR MZCAPI LoadStringDx2(INT nID);
LPCTSTR MZCAPI GetStringDx(INT nStringID);
LPCTSTR MZCAPI GetStringDx2(INT nStringID);
LPCTSTR MZCAPI GetStringDx(LPCTSTR psz);
LPCTSTR MZCAPI GetStringDx2(LPCTSTR psz);

//////////////////////////////////////////////////////////////////////////////

class MWindowBase
{
protected:
    MSG m_msg;
    WNDPROC m_fnOldProc;
public:
    HWND m_hwnd;

    MWindowBase() : m_hwnd(NULL), m_fnOldProc(NULL)
    {
    }

    virtual ~MWindowBase()
    {
    }

    operator HWND() const
    {
        return m_hwnd;
    }
    operator bool() const
    {
        return m_hwnd != NULL;
    }
    bool operator!() const
    {
        return m_hwnd == NULL;
    }

    static MWindowBase *GetUserData(HWND hwnd)
    {
        return (MWindowBase *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    static void SetUserData(HWND hwnd, void *ptr)
    {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptr);
    }

    MWindowBase *GetUserData() const
    {
        return GetUserData(m_hwnd);
    }

    LRESULT CALLBACK
    CallWindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return ::CallWindowProc(m_fnOldProc, hwnd, uMsg, wParam, lParam);
    }

    virtual LRESULT MZCAPI
    DefaultProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (m_fnOldProc)
        {
            return ::CallWindowProc(m_fnOldProc, hwnd, uMsg, wParam, lParam);
        }
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT MZCAPI DefaultProcDx()
    {
        return DefaultProcDx(m_msg.hwnd, m_msg.message, m_msg.wParam, m_msg.lParam);
    }

    void SaveMessageDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        m_msg.hwnd = hwnd;
        m_msg.message = uMsg;
        m_msg.wParam = wParam;
        m_msg.lParam = lParam;
        m_msg.time = GetMessageTime();
        LONG nPos = GetMessagePos();
        m_msg.pt.x = GET_X_LPARAM(nPos);
        m_msg.pt.y = GET_Y_LPARAM(nPos);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        MWindowBase *base;
        if (uMsg == WM_CREATE)
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
            assert(pcs->lpCreateParams);
            base = (MWindowBase *)pcs->lpCreateParams;
            base->m_hwnd = hwnd;
        }
        else
        {
            base = GetUserData(hwnd);
        }

        LRESULT ret = 0;
        if (base)
        {
            base->SaveMessageDx(hwnd, uMsg, wParam, lParam);
            ret = base->WindowProcDx(hwnd, uMsg, wParam, lParam);

            if (uMsg == WM_NCDESTROY)
            {
                base->m_hwnd = NULL;
            }
        }
        else
        {
            ret = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        return ret;
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("katahiromz's MWindowBase Class");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
    }

    BOOL RegisterClassDx()
    {
        HMODULE hMod = ::GetModuleHandle(NULL);

        WNDCLASSEX wcx;
        ZeroMemory(&wcx, sizeof(wcx));
        LPCTSTR pszClass = GetWndClassNameDx();
        if (GetClassInfoEx(hMod, pszClass, &wcx))
            return TRUE;

        wcx.cbSize = sizeof(wcx);
        wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcx.lpfnWndProc = MWindowBase::WindowProc;
        wcx.hInstance = hMod;
        wcx.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
        wcx.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wcx.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
        wcx.lpszMenuName = NULL;
        wcx.lpszClassName = pszClass;
        wcx.hIconSm = ::LoadIcon(NULL, IDI_APPLICATION);

        ModifyWndClassDx(wcx);

        return ::RegisterClassEx(&wcx);
    }

    BOOL CreateWindowDx(HWND hwndParent, LPCTSTR pszText,
                        DWORD Style = WS_OVERLAPPEDWINDOW, DWORD ExStyle = 0,
                        INT x = CW_USEDEFAULT, INT y = CW_USEDEFAULT,
                        INT cx = CW_USEDEFAULT, INT cy = CW_USEDEFAULT,
                        HMENU hMenu = NULL)
    {
        if (!RegisterClassDx())
            return FALSE;

        m_hwnd = ::CreateWindowEx(ExStyle, GetWndClassNameDx(),
            GetStringDx(pszText), Style, x, y, cx, cy, hwndParent,
            hMenu, GetModuleHandle(NULL), this);
        if (m_hwnd)
        {
            SetUserData(m_hwnd, this);
        }
        return (m_hwnd != NULL);
    }

    void SubclassDx(HWND hwnd)
    {
        m_hwnd = hwnd;
        SetUserData(hwnd, this);
        m_fnOldProc = SubclassWindow(hwnd, MWindowBase::WindowProc);
    }

    void UnsubclassDx(HWND hwnd)
    {
        m_hwnd = NULL;
        SetUserData(hwnd, NULL);
        SubclassWindow(hwnd, m_fnOldProc);
        m_fnOldProc = NULL;
    }

    INT MsgBoxDx(LPCTSTR pszString, LPCTSTR pszTitle,
                 UINT uType = MB_ICONINFORMATION)
    {
        tstring Title;
        if (pszTitle == NULL)
        {
#ifdef IDS_APPNAME
            Title = LoadStringDx(IDS_APPNAME);
#else
            if (m_hwnd)
                Title = GetWindowTextDx();
            else
                Title = TEXT("ERROR");
#endif
        }
        else
        {
            Title = GetStringDx(pszTitle);
        }

        MWindowBase::_doHookCenterMsgBoxDx(TRUE);
        INT nID = ::MessageBox(m_hwnd, GetStringDx(pszString),
                               Title.c_str(), uType);
        DWORD err = GetLastError();
        MWindowBase::_doHookCenterMsgBoxDx(FALSE);

        return nID;
    }

    INT MsgBoxDx(UINT nStringID, UINT nTitleID, UINT uType)
    {
        return MsgBoxDx(MAKEINTRESOURCE(nStringID), MAKEINTRESOURCE(nTitleID), uType);
    }

    INT MsgBoxDx(UINT nStringID, LPCTSTR pszTitle, UINT uType)
    {
        return MsgBoxDx(MAKEINTRESOURCE(nStringID), pszTitle, uType);
    }

    INT MsgBoxDx(UINT nStringID, UINT uType)
    {
        return MsgBoxDx(MAKEINTRESOURCE(nStringID), NULL, uType);
    }

    INT MsgBoxDx(LPCTSTR pszString, UINT uType)
    {
        return MsgBoxDx(pszString, NULL, uType);
    }

    INT ErrorBoxDx(UINT nStringID, UINT uType = MB_ICONERROR)
    {
        return MsgBoxDx(MAKEINTRESOURCE(nStringID), TEXT("ERROR"), uType);
    }
    INT ErrorBoxDx(LPCTSTR pszString, UINT uType = MB_ICONERROR)
    {
        return MsgBoxDx(pszString, TEXT("ERROR"), uType);
    }

    VOID CenterWindowDx() const
    {
        CenterWindowDx(m_hwnd);
    }

    static tstring GetWindowTextDx(HWND hwnd)
    {
        INT cch = ::GetWindowTextLength(hwnd);
        tstring ret;
        ret.resize(cch);
        ::GetWindowText(hwnd, &ret[0], cch + 1);
        return ret;
    }
    tstring GetWindowTextDx() const
    {
        return GetWindowTextDx(m_hwnd);
    }

    static tstring GetDlgItemTextDx(HWND hwnd, INT nCtrlID)
    {
        return GetWindowTextDx(::GetDlgItem(hwnd, nCtrlID));
    }
    tstring GetDlgItemTextDx(INT nCtrlID) const
    {
        return GetWindowTextDx(::GetDlgItem(m_hwnd, nCtrlID));
    }

    static void CenterWindowDx(HWND hwnd)
    {
        assert(IsWindow(hwnd));

        BOOL bChild = !!(GetWindowStyle(hwnd) & WS_CHILD);

        HWND hwndParent;
        if (bChild)
            hwndParent = GetParent(hwnd);
        else
            hwndParent = GetWindow(hwnd, GW_OWNER);

        RECT rcWorkArea;
        WorkAreaFromWindowDx(&rcWorkArea, hwnd);

        RECT rcParent;
        if (hwndParent)
            GetWindowRect(hwndParent, &rcParent);
        else
            rcParent = rcWorkArea;

        SIZE sizParent = SizeFromRectDx(&rcParent);

        RECT rc;
        GetWindowRect(hwnd, &rc);
        SIZE siz = SizeFromRectDx(&rc);

        POINT pt;
        pt.x = rcParent.left + (sizParent.cx - siz.cx) / 2;
        pt.y = rcParent.top + (sizParent.cy - siz.cy) / 2;

        if (bChild && hwndParent)
        {
            GetClientRect(hwndParent, &rcParent);
            MapWindowRect(hwndParent, NULL, &rcParent);
            RepositionPointDx(&pt, siz, &rcParent);

            ScreenToClient(hwndParent, &pt);
        }
        else
        {
            RepositionPointDx(&pt, siz, &rcWorkArea);
        }

        SetWindowPos(hwnd, NULL, pt.x, pt.y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void SetWindowPosDx(LPPOINT ppt = NULL, LPSIZE psiz = NULL,
                        HWND hwndInsertAfter = NULL,
                        UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER)
    {
        if (hwndInsertAfter == NULL)
        {
            uFlags |= SWP_NOZORDER;
        }

        INT x = 0, y = 0;
        if (ppt == NULL)
        {
            uFlags |= SWP_NOMOVE;
        }
        else
        {
            x = ppt->x;
            y = ppt->y;
        }

        INT cx = 0, cy = 0;
        if (psiz == NULL)
        {
            uFlags |= SWP_NOSIZE;
        }
        else
        {
            cx = psiz->cx;
            cy = psiz->cy;
        }

        ::SetWindowPos(m_hwnd, hwndInsertAfter, x, y, cx, cy, uFlags);
    }

    static HWND GetAncestorDx(HWND hTarget)
    {
        for (;;)
        {
            if (GetParent(hTarget) == NULL)
                break;
            hTarget = GetParent(hTarget);
        }
        return hTarget;
    }
    HWND GetAncestorDx() const
    {
        return GetAncestorDx(m_hwnd);
    }

    HICON LoadIconDx(INT id)
    {
        return LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(id));
    }

    HICON LoadSmallIconDx(UINT id)
    {
        return HICON(LoadImage(GetModuleHandle(NULL), 
                               MAKEINTRESOURCE(id),
                               IMAGE_ICON, 16, 16, 0));
    }

private:
    static inline LRESULT CALLBACK
    _msgBoxCbtProcDx(INT nCode, WPARAM wParam, LPARAM lParam)
    {
#ifndef MZC_NO_CENTER_MSGBOX
        if (nCode == HCBT_ACTIVATE)
        {
            HWND hwnd = (HWND)wParam;
            TCHAR szClassName[16];
            ::GetClassName(hwnd, szClassName, _countof(szClassName));
            if (lstrcmpi(szClassName, TEXT("#32770")) == 0)
            {
                CenterWindowDx(hwnd);
            }
        }
#endif  // ndef MZC_NO_CENTER_MSGBOX

        return 0;   // allow the operation
    }

    static HHOOK _doHookCenterMsgBoxDx(BOOL bHook)
    {
#ifdef MZC_NO_CENTER_MSGBOX
        return NULL;
#else   // ndef MZC_NO_CENTER_MSGBOX
        static HHOOK s_hHook = NULL;
        if (bHook)
        {
            if (s_hHook == NULL)
            {
                DWORD dwThreadID = GetCurrentThreadId();
                s_hHook = ::SetWindowsHookEx(WH_CBT, _msgBoxCbtProcDx, NULL, dwThreadID);
            }
        }
        else
        {
            if (s_hHook)
            {
                if (::UnhookWindowsHookEx(s_hHook))
                {
                    s_hHook = NULL;
                }
            }
        }
        return s_hHook;
#endif  // ndef MZC_NO_CENTER_MSGBOX
    }

#ifdef MZC4_FAT_AND_RICH
public:
    #include "MWindowBaseRichMethods.hpp"
#endif
};

//////////////////////////////////////////////////////////////////////////////

class MDialogBase : public MWindowBase
{
protected:
    BOOL    m_bModal;
public:
    INT     m_nDialogID;

    MDialogBase(INT nDialogID = 0) : m_bModal(FALSE), m_nDialogID(nDialogID)
    {
    }

    virtual LRESULT MZCAPI
    DefaultProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return 0;
    }

    LRESULT MZCAPI DefaultProcDx()
    {
        return DefaultProcDx(m_msg.hwnd, m_msg.message, m_msg.wParam, m_msg.lParam);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return 0;
    }

    static INT_PTR CALLBACK
    DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        MDialogBase *base;
        if (uMsg == WM_INITDIALOG)
        {
            assert(lParam);
            base = (MDialogBase *)lParam;
            base->m_hwnd = hwnd;
            if (base->m_bModal)
            {
                SetUserData(hwnd, base);
            }
        }
        else
        {
            base = GetUserData(hwnd);
        }

        INT_PTR ret = 0;
        if (base)
        {
            base->SaveMessageDx(hwnd, uMsg, wParam, lParam);
            ret = base->DialogProcDx(hwnd, uMsg, wParam, lParam);
            if (uMsg == WM_NCDESTROY)
            {
                base->m_hwnd = NULL;
            }
        }

        return ret;
    }

    static MDialogBase *GetUserData(HWND hwnd)
    {
        return (MDialogBase *)GetWindowLongPtr(hwnd, DWLP_USER);
    }
    static void SetUserData(HWND hwnd, void *ptr)
    {
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)ptr);
    }

    BOOL CreateDialogDx(HWND hwndParent, INT nDialogID)
    {
        m_bModal = FALSE;
        m_hwnd = ::CreateDialogParam(::GetModuleHandle(NULL),
            MAKEINTRESOURCE(nDialogID), hwndParent, MDialogBase::DialogProc,
            (LPARAM)this);
        if (m_hwnd)
        {
            SetUserData(m_hwnd, this);
        }
        return (m_hwnd != NULL);
    }
    BOOL CreateDialogDx(HWND hwndParent)
    {
        return CreateDialogDx(hwndParent, m_nDialogID);
    }

    INT_PTR DialogBoxDx(HWND hwndParent, INT nDialogID)
    {
        m_bModal = TRUE;
        INT_PTR nID = ::DialogBoxParam(::GetModuleHandle(NULL),
            MAKEINTRESOURCE(nDialogID), hwndParent,
            MDialogBase::DialogProc, (LPARAM)this);
        return nID;
    }
    INT_PTR DialogBoxDx(HWND hwndParent)
    {
        return DialogBoxDx(hwndParent, m_nDialogID);
    }

    BOOL CreateDialogIndirectDx(HWND hwndParent, const void *ptr)
    {
        m_bModal = FALSE;
        m_hwnd = ::CreateDialogIndirectParam(
            ::GetModuleHandle(NULL),
            (const DLGTEMPLATE *)ptr,
            hwndParent, MDialogBase::DialogProc, (LPARAM)this);
        if (m_hwnd)
        {
            SetUserData(m_hwnd, this);
        }
        return (m_hwnd != NULL);
    }

    INT_PTR DialogBoxIndirectDx(HWND hwndParent, const void *ptr)
    {
        m_bModal = TRUE;
        INT_PTR nID = ::DialogBoxIndirectParam(::GetModuleHandle(NULL),
            (const DLGTEMPLATE *)ptr, hwndParent,
            MDialogBase::DialogProc, (LPARAM)this);
        return nID;
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("#32770");
    }

#ifdef MZC4_FAT_AND_RICH
public:
    #include "MDialogBaseRichMethods.hpp"
#endif
};

//////////////////////////////////////////////////////////////////////////////
// public inline functions

inline void MZCAPIV DebugPrintDx(const char *format, ...)
{
    #ifdef _DEBUG
        char buffer[512];
        va_list va;
        va_start(va, format);
        ::wvsprintfA(buffer, format, va);
        va_end(va);
        OutputDebugStringA(buffer);
    #endif
}

inline void MZCAPIV DebugPrintDx(const WCHAR *format, ...)
{
    #ifdef _DEBUG
        WCHAR buffer[512];
        va_list va;
        va_start(va, format);
        ::wvsprintfW(buffer, format, va);
        va_end(va);
        OutputDebugStringW(buffer);
    #endif
}

inline void MZCAPI GetVirtualScreenRectDx(LPRECT prc)
{
#ifndef SM_XVIRTUALSCREEN
    #define SM_XVIRTUALSCREEN   76
    #define SM_YVIRTUALSCREEN   77
    #define SM_CXVIRTUALSCREEN  78
    #define SM_CYVIRTUALSCREEN  79
#endif
    INT x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
    INT y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    INT cx = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    INT cy = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (cx == 0)
        cx = ::GetSystemMetrics(SM_CXSCREEN);
    if (cy == 0)
        cy = ::GetSystemMetrics(SM_CYSCREEN);
    SetRect(prc, x, y, x + cx, y + cy);
}

inline void MZCAPI RepositionPointDx(LPPOINT ppt, SIZE siz, LPCRECT prc)
{
    if (ppt->x + siz.cx > prc->right)
        ppt->x = prc->right - siz.cx;
    if (ppt->y + siz.cy > prc->bottom)
        ppt->y = prc->bottom - siz.cy;
    if (ppt->x < prc->left)
        ppt->x = prc->left;
    if (ppt->y < prc->top)
        ppt->y = prc->top;
}

inline void MZCAPI WorkAreaFromWindowDx(LPRECT prcWorkArea, HWND hwnd)
{
#if (WINVER >= 0x0500)
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (GetMonitorInfo(hMonitor, &mi))
    {
        *prcWorkArea = mi.rcWork;
        return;
    }
#endif
    ::SystemParametersInfo(SPI_GETWORKAREA, 0, prcWorkArea, 0);
}

inline SIZE MZCAPI SizeFromRectDx(LPCRECT prc)
{
    SIZE siz = { prc->right - prc->left, prc->bottom - prc->top };
    return siz;
}

inline LPTSTR MZCAPI LoadStringDx(INT nID)
{
    static TCHAR s_sz[1024];
    s_sz[0] = 0;
    ::LoadString(::GetModuleHandle(NULL), nID, s_sz, _countof(s_sz));
    return s_sz;
}

inline LPTSTR MZCAPI LoadStringDx2(INT nID)
{
    static TCHAR s_sz[1024];
    s_sz[0] = 0;
    ::LoadString(::GetModuleHandle(NULL), nID, s_sz, _countof(s_sz));
    return s_sz;
}

inline LPCTSTR MZCAPI GetStringDx(LPCTSTR psz)
{
    if (psz == NULL)
        return NULL;
    if (IS_INTRESOURCE(psz))
        return LoadStringDx(LOWORD(psz));
    return psz;
}

inline LPCTSTR MZCAPI GetStringDx2(LPCTSTR psz)
{
    if (psz == NULL)
        return NULL;
    if (IS_INTRESOURCE(psz))
        return LoadStringDx2(LOWORD(psz));
    return psz;
}

inline LPCTSTR MZCAPI GetStringDx(INT nStringID)
{
    return LoadStringDx(nStringID);
}

inline LPCTSTR MZCAPI GetStringDx2(INT nStringID)
{
    return LoadStringDx2(nStringID);
}

//////////////////////////////////////////////////////////////////////////////

#ifdef MZC4_FAT_AND_RICH
    #include "MPointSizeRect.hpp"
    #include "MButton.hpp"
    #include "MComboBox.hpp"
    #include "MEdit.hpp"
    #include "MListBox.hpp"
    #include "MScrollBar.hpp"
    #include "MStatic.hpp"
    #include "MCommCtrl.hpp"
    #include "MCommDlg.hpp"
#endif

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MWINDOWBASE_HPP_

//////////////////////////////////////////////////////////////////////////////
