// MWindowBase.hpp --- MZC4 window base and dialog base         -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MWINDOWBASE_HPP_
#define MZC4_MWINDOWBASE_HPP_    171     /* Version 171 */

class MWindowBase;
class MDialogBase;

//////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
    #pragma once        // For speed!
#endif

#ifndef STRICT
    #define STRICT  1   // We must do strictly! Good bye to old style!
#endif

#if defined(_MSC_VER) && !defined(NDEBUG) && !defined(_CRTDBG_MAP_ALLOC)
    // for detecting memory leak (MSVC only)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#if __STDC_VERSION__ >= 199901L && !defined(C99)
    #define C99             1
#endif
#if __cplusplus >= 201103L && !defined(CXX11)
    #define CXX11           1
#endif

//#define MZC4_FAT_AND_RICH   1
//#define MZC4_HANDLE_MAP     1

//////////////////////////////////////////////////////////////////////////////
// headers

// Win32API headers
#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif
#ifndef _INC_WINDOWSX
    #include <windowsx.h>   // Win32API Macro APIs
#endif
#ifndef _INC_COMMCTRL
    #include <commctrl.h>   // common controls
#endif
#ifndef _INC_COMMDLG
    #include <commdlg.h>    // common dialogs
#endif
#ifndef _INC_SHLWAPI
    #include <shlwapi.h>    // Shell Light-Weight API
#endif
#ifndef _INC_TCHAR
    #include <tchar.h>      // generic text mappings
#endif

#include <dlgs.h>           // dialog control IDs

#ifndef NO_STRSAFE
    #include <strsafe.h>    // StringCch* or StringCb*
#endif

// standard C/C++ library
#include <cassert>          // assert
#include <cstring>          // C string library

#include <map>              // std::map

//////////////////////////////////////////////////////////////////////////////

#ifndef _countof
    #define _countof(array)     (sizeof(array) / sizeof(array[0]))
#endif

// MString
#ifndef MString
    #include <string>       // std::string and std::wstring
    typedef std::string     MStringA;
    typedef std::wstring    MStringW;
    #ifdef UNICODE
        #define MString     MStringW
    #else
        #define MString     MStringA
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

//////////////////////////////////////////////////////////////////////////////
// public functions

#ifndef MZCAPI
    #define MZCAPI      WINAPI
#endif

#ifndef MZCAPIV
    #define MZCAPIV     WINAPIV
#endif

VOID MZCAPIV DebugPrintDx(const char *format, ...);
VOID MZCAPIV DebugPrintDx(const WCHAR *format, ...);
RECT MZCAPI GetVirtualScreenRectDx();
VOID MZCAPI RepositionPointDx(LPPOINT ppt, SIZE siz, LPCRECT prc);
RECT MZCAPI WorkAreaFromWindowDx(HWND hwnd);
SIZE MZCAPI SizeFromRectDx(LPCRECT prc);
LPTSTR MZCAPI LoadStringDx(INT nID);
LPTSTR MZCAPIV LoadStringPrintfDx(INT nID, ...);
LPTSTR MZCAPI LoadStringVPrintfDx(INT nID, va_list va);
LPCTSTR MZCAPI GetStringDx(INT nStringID);
LPCTSTR MZCAPI GetStringDx(LPCTSTR psz);
BOOL MZCAPI GetWindowPosDx(HWND hwnd, POINT *ppt = NULL, SIZE *psiz = NULL);
BOOL MZCAPI GetWindowPosDx(HWND hwnd, RECT *prc);
BOOL MZCAPI SetWindowPosDx(HWND hwnd, LPPOINT ppt = NULL, LPSIZE psiz = NULL, 
    HWND hwndInsertAfter = NULL, 
    UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER);
BOOL MZCAPI SetWindowPosDx(HWND hwnd, const RECT *prc);
LPSTR MZCAPI MakeFilterDx(LPSTR psz);
LPWSTR MZCAPI MakeFilterDx(LPWSTR psz);
HBITMAP MZCAPI Create24BppBitmapDx(INT width, INT height);
void MZCAPI FillBitmapDx(HBITMAP hbm, HBRUSH hbr);
void MZCAPI PremultiplyDx(HBITMAP hbm32bpp);
BOOL MZCAPI CopyTextDx(HWND hwnd, const MString& text);
void MZCAPI
PopupMenuDx(HWND hwnd, HWND hContext, UINT nMenuID, INT iSubMenu, INT x, INT y);

//////////////////////////////////////////////////////////////////////////////
// Messaging

// LRESULT OnMessage(HWND hwnd, WPARAM wParam, LPARAM lParam);
#define HANDLE_MESSAGE(hwnd, uMsg, OnMessage) \
    case (uMsg): return (OnMessage)((hwnd), (wParam), (lParam))

// LRESULT OnMessageEx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#define HANDLE_MESSAGE_EX(hwnd, uMsg, OnMessageEx) \
    case (uMsg): return (OnMessageEx)((hwnd), (uMsg), (wParam), (lParam))

#define DO_MSG(WM_, fn) \
    case (WM_): return HANDLE_##WM_(hwnd, (wParam), (lParam), (fn))
#define DO_MESSAGE(uMsg, OnMessage) \
    HANDLE_MESSAGE(hwnd, uMsg, OnMessage)
#define DO_MESSAGE_EX(uMsg, OnMessageEx) \
    HANDLE_MESSAGE_EX(hwnd, uMsg, OnMessage)

//////////////////////////////////////////////////////////////////////////////

class MWindowBase
{
protected:
    DWORD           m_dwWindowBaseMagic;
    MSG             m_msg;
public:
    HWND            m_hwnd;
    WNDPROC         m_fnOldProc;
    MWindowBase *   m_pwndSub;
    bool            m_bDynamicCreated;

    MWindowBase() :
        m_dwWindowBaseMagic(0xFEEDFEED), m_hwnd(NULL), m_fnOldProc(NULL), 
        m_pwndSub(NULL), m_bDynamicCreated(false)
    {
    }

    MWindowBase(HWND hwnd) :
        m_dwWindowBaseMagic(0xFEEDFEED), m_hwnd(hwnd), m_fnOldProc(NULL), 
        m_pwndSub(NULL), m_bDynamicCreated(false)
    {
    }

    virtual ~MWindowBase()
    {
    }

    operator HWND() const
    {
        return Handle();
    }
    bool operator!() const
    {
        return Handle() == NULL;
    }
    HWND Handle() const
    {
        return (this ? m_hwnd : NULL);
    }

#ifdef MZC4_HANDLE_MAP
    typedef std::map<HWND, void *> handle_map_type;
    static handle_map_type& GetHandleMap()
    {
        static handle_map_type s_map;
        return s_map;
    }
    static MWindowBase *GetUserData(HWND hwnd)
    {
        auto it = GetHandleMap().find(hwnd);
        if (it == GetHandleMap().end())
            return NULL;
        return reinterpret_cast<MWindowBase *>(it->second);
    }
    static void SetUserData(HWND hwnd, void *ptr)
    {
        if (ptr)
        {
            GetHandleMap()[hwnd] = ptr;
        }
        else
        {
            GetHandleMap().erase(hwnd);
        }
    }
#else
    static MWindowBase *GetUserData(HWND hwnd)
    {
        return reinterpret_cast<MWindowBase *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    static void SetUserData(HWND hwnd, void *ptr)
    {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptr);
    }
#endif
    virtual void PostNcDestroy()
    {
        SetUserData(m_hwnd, NULL);
        m_hwnd = NULL;
        if (m_bDynamicCreated)
        {
            delete this;
        }
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
        if (m_pwndSub)
        {
            assert(this != m_pwndSub);
            return m_pwndSub->WindowProcDx(hwnd, uMsg, wParam, lParam);
        }
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

    VOID SaveMessageDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("katahiromz's MWindowBase Class");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
    }

    BOOL RegisterClassDx();

    BOOL CreateWindowDx(HWND hwndParent, LPCTSTR pszText, 
                        DWORD Style = WS_OVERLAPPEDWINDOW, DWORD ExStyle = 0, 
                        INT x = CW_USEDEFAULT, INT y = CW_USEDEFAULT, 
                        INT cx = CW_USEDEFAULT, INT cy = CW_USEDEFAULT, 
                        HMENU hMenu = NULL);
    BOOL CreateAsChildDx(HWND hwndParent, LPCTSTR pszText, 
                         DWORD Style = WS_CHILD | WS_VISIBLE, 
                         DWORD ExStyle = 0, INT nCtrlID = -1, 
                         INT x = CW_USEDEFAULT, INT y = CW_USEDEFAULT, 
                         INT cx = CW_USEDEFAULT, INT cy = CW_USEDEFAULT);

    BOOL Attach(HWND hwnd)
    {
        MWindowBase *pwndSub = GetUserData(hwnd);
        m_hwnd = hwnd;
        SetUserData(m_hwnd, this);
        m_pwndSub = pwndSub;
        return m_hwnd != NULL;
    }

    HWND Detach()
    {
        HWND hwnd = m_hwnd;
        SetUserData(hwnd, m_pwndSub);
        m_hwnd = NULL;
        m_pwndSub = NULL;
        return hwnd;
    }

    virtual void PostSubclassDx(HWND hwnd)
    {
    }

    BOOL SubclassDx(HWND hwnd)
    {
        Attach(hwnd);
        if (!m_pwndSub)
        {
            m_fnOldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)MWindowBase::WindowProc);
        }
        PostSubclassDx(hwnd);
        return m_pwndSub || m_fnOldProc;
    }

    VOID UnsubclassDx()
    {
        SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, (LONG_PTR)m_fnOldProc);
        SetUserData(m_hwnd, m_pwndSub);
        m_fnOldProc = NULL;
    }

    BOOL SubclassChildDx(MWindowBase& child, UINT nItemID)
    {
        assert(::IsWindow(m_hwnd));
        return child.SubclassDx(::GetDlgItem(m_hwnd, nItemID));
    }

    INT MsgBoxDx(LPCTSTR pszString, LPCTSTR pszTitle, 
                 UINT uType = MB_ICONINFORMATION);

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

    static INT GetWindowTextLength(HWND hwnd)
    {
        assert(::IsWindow(hwnd));
        return ::GetWindowTextLength(hwnd);
    }
    INT GetWindowTextLength() const
    {
        assert(::IsWindow(Handle()));
        return ::GetWindowTextLength(Handle());
    }

    static MString GetWindowText(HWND hwnd);
    static INT GetWindowText(HWND hwnd, LPTSTR pszText, INT cchMax)
    {
        assert(::IsWindow(hwnd));
        return ::GetWindowText(hwnd, pszText, cchMax);
    }
    MString GetWindowText() const
    {
        return GetWindowText(m_hwnd);
    }

    static UINT GetDlgItemText(HWND hwnd, INT nCtrlID, LPTSTR psz, INT cchMax)
    {
        assert(::IsWindow(hwnd));
        return ::GetDlgItemText(hwnd, nCtrlID, psz, cchMax);
    }
    static MString GetDlgItemText(HWND hwnd, INT nCtrlID)
    {
        assert(::IsWindow(hwnd));
        return GetWindowText(::GetDlgItem(hwnd, nCtrlID));
    }
    MString GetDlgItemText(INT nCtrlID) const
    {
        return GetWindowText(::GetDlgItem(m_hwnd, nCtrlID));
    }

    static BOOL SetWindowText(HWND hwnd, LPCTSTR pszText = NULL)
    {
        assert(::IsWindow(hwnd));
        return ::SetWindowText(hwnd, pszText);
    }
    BOOL SetWindowText(LPCTSTR pszText = NULL)
    {
        return SetWindowText(Handle(), pszText);
    }

    static VOID CenterWindowDx(HWND hwnd);

    static BOOL MZCAPI GetWindowPosDx(HWND hwnd, POINT *ppt = NULL, SIZE *psiz = NULL);
    static BOOL MZCAPI GetWindowPosDx(HWND hwnd, RECT *prc);

    BOOL MZCAPI GetWindowPosDx(POINT *ppt = NULL, SIZE *psiz = NULL);
    BOOL MZCAPI GetWindowPosDx(RECT *prc);

    static BOOL SetWindowPosDx(HWND hwnd, 
                               LPPOINT ppt = NULL, LPSIZE psiz = NULL, 
                               HWND hwndInsertAfter = NULL, 
                               UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    static BOOL SetWindowPosDx(HWND hwnd, const RECT *prc);

    BOOL SetWindowPosDx(LPPOINT ppt = NULL, LPSIZE psiz = NULL, 
                        HWND hwndInsertAfter = NULL, 
                        UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    BOOL SetWindowPosDx(const RECT *prc);

    static HWND GetAncestorDx(HWND hTarget);
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
        INT cxSmall = GetSystemMetrics(SM_CXSMICON);
        INT cySmall = GetSystemMetrics(SM_CYSMICON);
        return HICON(LoadImage(GetModuleHandle(NULL), 
                               MAKEINTRESOURCE(id), 
                               IMAGE_ICON, cxSmall, cySmall, 0));
    }

    HFONT GetFontDx() const
    {
        assert(::IsWindow(m_hwnd));
        return GetWindowFont(m_hwnd);
    }
    void SetFontDx(HFONT hFont, BOOL bRedraw = TRUE)
    {
        assert(::IsWindow(m_hwnd));
        SetWindowFont(m_hwnd, hFont, bRedraw);
    }
    DWORD GetStyleDx() const
    {
        assert(::IsWindow(m_hwnd));
        return GetWindowStyle(m_hwnd);
    }
    DWORD SetStyleDx(DWORD dwNewStyle)
    {
        assert(::IsWindow(m_hwnd));
        return SetWindowLong(m_hwnd, GWL_STYLE, dwNewStyle);
    }
    void ModifyStyleDx(DWORD dwRemove, DWORD dwAdd)
    {
        assert(::IsWindow(m_hwnd));
        DWORD style = GetStyleDx();
        style &= ~dwRemove;
        style |= dwAdd;
        SetStyleDx(style);
    }
    DWORD GetExStyleDx() const
    {
        assert(::IsWindow(m_hwnd));
        return GetWindowExStyle(m_hwnd);
    }
    DWORD SetExStyleDx(DWORD dwNewExStyle)
    {
        assert(::IsWindow(m_hwnd));
        return SetWindowLong(m_hwnd, GWL_EXSTYLE, dwNewExStyle);
    }
    void ModifyExStyleDx(DWORD dwRemove, DWORD dwAdd)
    {
        assert(::IsWindow(m_hwnd));
        DWORD exstyle = GetExStyleDx();
        exstyle &= ~dwRemove;
        exstyle |= dwAdd;
        SetExStyleDx(exstyle);
    }

    HWND GetChildDx(HWND hwndChild) const
    {
        return hwndChild;
    }
    HWND GetChildDx(UINT id) const
    {
        return ::GetDlgItem(m_hwnd, id);
    }
    UINT GetCtrlIDDx(HWND hwndChild) const
    {
        return ::GetDlgCtrlID(hwndChild);
    }
    UINT GetCtrlIDDx(UINT nChildID) const
    {
        return nChildID;
    }

    LRESULT SendMessageDx(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) const
    {
        assert(::IsWindow(m_hwnd));
        return ::SendMessage(m_hwnd, uMsg, wParam, lParam);
    }
    BOOL PostMessageDx(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) const
    {
        assert(::IsWindow(m_hwnd));
        return ::PostMessage(m_hwnd, uMsg, wParam, lParam);
    }

    static HHOOK HookCenterMsgBoxDx(BOOL bHook);

private:
    static inline LRESULT CALLBACK
    _msgBoxCbtProcDx(INT nCode, WPARAM wParam, LPARAM lParam);

#ifdef MZC4_FAT_AND_RICH
public:
    #include "MWindowBaseRichMethods.hpp"
#endif

public:
    // for DECLARE_DYNAMIC/IMPLEMENT_DYNAMIC
    typedef MWindowBase *(*FNCREATEINSTANCEDX)();
    typedef std::map<MString, FNCREATEINSTANCEDX> class_to_create_map_t;

    static class_to_create_map_t& ClassToCreateMap()
    {
        static class_to_create_map_t s_class_to_create_map;
        return s_class_to_create_map;
    }
};

//////////////////////////////////////////////////////////////////////////////
// DECLARE_DYNAMIC/IMPLEMENT_DYNAMIC

#define DECLARE_DYNAMIC(class_name) \
    static MWindowBase *CreateInstanceDx();

#define IMPLEMENT_DYNAMIC(class_name) \
    /*static*/ MWindowBase *class_name::CreateInstanceDx() \
    { \
        return new class_name(); \
    } \
    struct class_name##AutoDynamicRegister \
    { \
        class_name##AutoDynamicRegister() \
        { \
            MWindowBase::ClassToCreateMap()[TEXT(#class_name)] = &class_name::CreateInstanceDx; \
        } \
    } class_name##AutoDynamicRegister##__LINE__;

#define IMPLEMENT_DYNAMIC2(class_name, wndclass_name) \
    /*static*/ MWindowBase *class_name::CreateInstanceDx() \
    { \
        return new class_name(); \
    } \
    struct class_name##AutoDynamicRegister \
    { \
        class_name##AutoDynamicRegister() \
        { \
            MWindowBase::ClassToCreateMap()[TEXT(#wndclass_name)] = &class_name::CreateInstanceDx; \
        } \
    } class_name##AutoDynamicRegister##__LINE__;

//////////////////////////////////////////////////////////////////////////////

class MDialogBase : public MWindowBase
{
public:
    BOOL    m_bModal;
    HWND    m_hwndOwner;
    LPCTSTR m_pDialogName;

    MDialogBase() : m_bModal(FALSE), m_hwndOwner(NULL), m_pDialogName(NULL)
    {
    }

    MDialogBase(HWND hwndOwner)
        : m_bModal(FALSE), m_hwndOwner(hwndOwner), m_pDialogName(NULL)
    {
    }

    MDialogBase(HWND hwndOwner, INT nDialogID)
        : m_bModal(FALSE), m_hwndOwner(hwndOwner), 
          m_pDialogName(MAKEINTRESOURCE(nDialogID))
    {
    }

    MDialogBase(HWND hwndOwner, LPCTSTR pDialogName)
        : m_bModal(FALSE), m_hwndOwner(hwndOwner), 
          m_pDialogName(pDialogName)
    {
    }

    MDialogBase(INT nDialogID)
        : m_bModal(FALSE), m_hwndOwner(NULL), 
          m_pDialogName(MAKEINTRESOURCE(nDialogID))
    {
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("#32770");
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
    DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static MDialogBase *GetUserData(HWND hwnd)
    {
        return (MDialogBase *)GetWindowLongPtr(hwnd, DWLP_USER);
    }
    static VOID SetUserData(HWND hwnd, VOID *ptr)
    {
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)ptr);
    }

    BOOL CreateDialogDx(HWND hwndOwner, INT nDialogID);
    BOOL CreateDialogDx(HWND hwndOwner, LPCTSTR pDialogName);
    INT_PTR DialogBoxDx(HWND hwndOwner, INT nDialogID);
    INT_PTR DialogBoxDx(HWND hwndOwner, LPCTSTR pDialogName);
    BOOL CreateDialogIndirectDx(HWND hwndOwner, const VOID *ptr);
    INT_PTR DialogBoxIndirectDx(HWND hwndOwner, const VOID *ptr);

    BOOL CreateDialogDx(HWND hwndOwner)
    {
        return CreateDialogDx(hwndOwner, m_pDialogName);
    }

    INT_PTR DialogBoxDx(HWND hwndOwner)
    {
        return DialogBoxDx(hwndOwner, m_pDialogName);
    }

    DWORD GetDefID() const
    {
        return (DWORD)SendMessageDx(DM_GETDEFID);
    }
    BOOL SetDefID(UINT nID)
    {
        return (BOOL)SendMessageDx(DM_SETDEFID, (WPARAM)nID);
    }

    BOOL EndDialog(INT_PTR nResult)
    {
        assert(::IsWindow(Handle()));
        return ::EndDialog(Handle(), nResult);
    }
    static BOOL EndDialog(HWND hwnd, INT_PTR nResult)
    {
        assert(::IsWindow(hwnd));
        return ::EndDialog(hwnd, nResult);
    }

    VOID NextDlgCtrl() const
    {
        PostMessageDx(WM_NEXTDLGCTL, FALSE);
    }
    VOID PrevDlgCtrl() const
    {
        PostMessageDx(WM_NEXTDLGCTL, TRUE);
    }
    VOID GotoDlgCtrl(HWND hwndCtrl)
    {
        PostMessageDx(WM_NEXTDLGCTL, (WPARAM)hwndCtrl, 1);
    }

    BOOL MapDialogRect(LPRECT prc) const
    {
        assert(IsWindow(Handle()));
        return ::MapDialogRect(Handle(), prc);
    }
    static BOOL MapDialogRect(HWND hwnd, LPRECT prc)
    {
        assert(IsWindow(hwnd));
        return ::MapDialogRect(hwnd, prc);
    }

    BOOL IsDialogMessage(LPMSG pMsg)
    {
        return ::IsDialogMessage(Handle(), pMsg);
    }

    BOOL Attach(HWND hwnd)
    {
        m_hwnd = hwnd;
        SetUserData(m_hwnd, this);
        return m_hwnd != NULL;
    }

    HWND Detach()
    {
        HWND hwnd = m_hwnd;
        SetUserData(hwnd, NULL);
        m_hwnd = NULL;
        return hwnd;
    }
};

//////////////////////////////////////////////////////////////////////////////
// public inline functions

inline VOID MZCAPIV DebugPrintDx(const char *format, ...)
{
    #ifndef NDEBUG
        char szBuff[512];
        va_list va;
        va_start(va, format);
#ifdef NO_STRSAFE
        wsprintfA(szBuff, format, va);
#else
        StringCchVPrintfA(szBuff, _countof(szBuff), format, va);
#endif
        va_end(va);
        OutputDebugStringA(szBuff);
    #endif
}

inline VOID MZCAPIV DebugPrintDx(const WCHAR *format, ...)
{
    #ifndef NDEBUG
        WCHAR szBuff[512];
        va_list va;
        va_start(va, format);
#ifdef NO_STRSAFE
        wsprintfW(szBuff, format, va);
#else
        StringCchVPrintfW(szBuff, _countof(szBuff), format, va);
#endif
        va_end(va);
        OutputDebugStringW(szBuff);
    #endif
}

inline RECT MZCAPI GetVirtualScreenRectDx()
{
    RECT rc;
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
    SetRect(&rc, x, y, x + cx, y + cy);
    return rc;
}

inline VOID MZCAPI RepositionPointDx(LPPOINT ppt, SIZE siz, LPCRECT prc)
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

inline RECT MZCAPI WorkAreaFromWindowDx(HWND hwnd)
{
#if (WINVER >= 0x0500)
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (GetMonitorInfo(hMonitor, &mi))
    {
        return mi.rcWork;
    }
#endif
    RECT rc;
    ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
    return rc;
}

inline SIZE MZCAPI SizeFromRectDx(LPCRECT prc)
{
    SIZE siz;
    siz.cx = prc->right - prc->left;
    siz.cy = prc->bottom - prc->top;
    return siz;
}

inline LPTSTR MZCAPI LoadStringDx(INT nID)
{
    static UINT s_index = 0;
    const UINT cchBuffMax = 1024;
    static TCHAR s_sz[4][cchBuffMax];

    TCHAR *pszBuff = s_sz[s_index];
    s_index = (s_index + 1) % _countof(s_sz);
    pszBuff[0] = 0;
    if (!::LoadString(NULL, nID, pszBuff, cchBuffMax))
        assert(0);
    return pszBuff;
}

inline LPTSTR MZCAPI LoadStringVPrintfDx(INT nID, va_list va)
{
    static UINT s_index = 0;
    const UINT cchBuffMax = 1024;
    static TCHAR s_sz[4][cchBuffMax];

    TCHAR *pszBuff = s_sz[s_index];
    s_index = (s_index + 1) % _countof(s_sz);
    pszBuff[0] = 0;
#ifdef NO_STRSAFE
    wvsprintf(pszBuff, LoadStringDx(nID), va);
#else
    StringCchVPrintf(pszBuff, cchBuffMax, LoadStringDx(nID), va);
#endif
    return pszBuff;
}

inline LPTSTR MZCAPIV LoadStringPrintfDx(INT nID, ...)
{
    va_list va;
    va_start(va, nID);
    LPTSTR psz = LoadStringVPrintfDx(nID, va);
    va_end(va);
    return psz;
}

inline LPCTSTR MZCAPI GetStringDx(LPCTSTR psz)
{
    if (psz == NULL)
        return NULL;
    if (IS_INTRESOURCE(psz))
        return LoadStringDx(LOWORD(psz));
    return psz;
}

inline LPCTSTR MZCAPI GetStringDx(INT nStringID)
{
    return LoadStringDx(nStringID);
}

inline BOOL MZCAPI
GetWindowPosDx(HWND hwnd, POINT *ppt/* = NULL*/, SIZE *psiz/* = NULL*/)
{
    RECT rc;
    if (!GetWindowRect(hwnd, &rc))
        return FALSE;

    if (ppt)
    {
        ppt->x = rc.left;
        ppt->y = rc.top;
        if (GetWindowStyle(hwnd) & WS_CHILD)
        {
            MapWindowPoints(NULL, GetParent(hwnd), ppt, 1);
        }
    }
    if (psiz)
    {
        psiz->cx = rc.right - rc.left;
        psiz->cy = rc.bottom - rc.top;
    }
    return TRUE;
}

inline BOOL MZCAPI GetWindowPosDx(HWND hwnd, RECT *prc)
{
    POINT pt;
    SIZE siz;
    if (!GetWindowPosDx(hwnd, &pt, &siz))
        return FALSE;
    prc->left = pt.x;
    prc->top = pt.y;
    prc->right = pt.x + siz.cx;
    prc->bottom = pt.x + siz.cy;
    return TRUE;
}

inline BOOL MZCAPI
SetWindowPosDx(HWND hwnd, LPPOINT ppt/* = NULL*/, LPSIZE psiz/* = NULL*/, 
    HWND hwndInsertAfter/* = NULL*/, 
    UINT uFlags/* = SWP_NOACTIVATE | SWP_NOOWNERZORDER*/)
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

    return ::SetWindowPos(hwnd, hwndInsertAfter, x, y, cx, cy, uFlags);
}

inline BOOL MZCAPI
SetWindowPosDx(HWND hwnd, const RECT *prc)
{
    assert(prc);
    POINT pt;
    SIZE siz;
    pt.x = prc->left;
    pt.y = prc->top;
    siz.cx = prc->right - prc->left;
    siz.cy = prc->bottom - prc->top;
    return ::SetWindowPosDx(hwnd, &pt, &siz);
}

inline LPSTR MZCAPI MakeFilterDx(LPSTR psz)
{
    for (LPSTR pch = psz; *pch; ++pch)
    {
        if (*pch == '|')
            *pch = 0;
    }
    return psz;
}

inline LPWSTR MZCAPI MakeFilterDx(LPWSTR psz)
{
    for (LPWSTR pch = psz; *pch; ++pch)
    {
        if (*pch == L'|')
            *pch = 0;
    }
    return psz;
}

inline HBITMAP MZCAPI Create24BppBitmapDx(INT width, INT height)
{
    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    HDC hDC = CreateCompatibleDC(NULL);
    LPVOID pvBits;
    HBITMAP hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, 
                                   &pvBits, NULL, 0);
    DeleteDC(hDC);
    return hbm;
}

inline void MZCAPI FillBitmapDx(HBITMAP hbm, HBRUSH hbr)
{
    BITMAP bm;
    if (!GetObject(hbm, sizeof(bm), &bm))
        return;

    HDC hDC = CreateCompatibleDC(NULL);
    {
        HGDIOBJ hbmOld = SelectObject(hDC, hbm);
        {
            RECT rc;
            SetRect(&rc, 0, 0, bm.bmWidth, bm.bmHeight);
            FillRect(hDC, &rc, hbr);
            DeleteObject(hbr);
        }
        SelectObject(hDC, hbmOld);
    }
    DeleteDC(hDC);
}

inline void MZCAPI
PremultiplyDx(HBITMAP hbm32bpp)
{
    BITMAP bm;
    DWORD cdw;
    LPBYTE pb;
    BYTE alpha;
    GetObject(hbm32bpp, sizeof(bm), &bm);
    if (bm.bmBitsPixel == 32)
    {
        cdw = bm.bmWidth * bm.bmHeight;
        pb = (LPBYTE) bm.bmBits;
        while (cdw--)
        {
            alpha = pb[3];
            pb[0] = (BYTE) ((DWORD) pb[0] * alpha / 255);
            pb[1] = (BYTE) ((DWORD) pb[1] * alpha / 255);
            pb[2] = (BYTE) ((DWORD) pb[2] * alpha / 255);
            pb += 4;
        }
    }
}

inline BOOL MZCAPI CopyTextDx(HWND hwnd, const MString& text)
{
#ifdef UNICODE
    UINT CF_ = CF_UNICODETEXT;
#else
    UINT CF_ = CF_TEXT;
#endif
    DWORD size = DWORD((text.size() + 1) * sizeof(TCHAR));
    LPTSTR psz = (LPTSTR)GlobalAllocPtr(GMEM_SHARE | GMEM_MOVEABLE, size);
    if (psz)
    {
        HGLOBAL hGlobal = GlobalPtrHandle(psz);
        CopyMemory(psz, text.c_str(), size);
        GlobalUnlockPtr(psz);

        if (OpenClipboard(hwnd))
        {
            EmptyClipboard();
            SetClipboardData(CF_, hGlobal);
            return CloseClipboard();
        }
    }
    return FALSE;
}

inline void MZCAPI
PopupMenuDx(HWND hwnd, HWND hContext, UINT nMenuID, INT iSubMenu, INT x, INT y)
{
    HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(nMenuID));
    HMENU hSubMenu = GetSubMenu(hMenu, iSubMenu);

    if (x == 0xFFFF && y == 0xFFFF)
    {
        RECT rc;
        GetWindowRect(hContext, &rc);
        x = rc.left;
        y = rc.top;
    }

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
        x, y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

//////////////////////////////////////////////////////////////////////////////

inline VOID
MWindowBase::SaveMessageDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

inline /*static*/ LRESULT CALLBACK
MWindowBase::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
 {
    MWindowBase *base;
    if (uMsg == WM_CREATE)
    {
        TCHAR szClass[128];
        GetClassName(hwnd, szClass, _countof(szClass));

        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        if (pcs->lpCreateParams &&
            !IsBadReadPtr(pcs->lpCreateParams, sizeof(MWindowBase)) &&
            ((MWindowBase*)(pcs->lpCreateParams))->m_dwWindowBaseMagic == 0xFEEDFEED)
        {
            base = reinterpret_cast<MWindowBase *>(pcs->lpCreateParams);
            base->m_hwnd = hwnd;
        }
        else
        {
            // for DECLARE_DYNAMIC/IMPLEMENT_DYNAMIC
            auto it = MWindowBase::ClassToCreateMap().find(szClass);
            if (it == MWindowBase::ClassToCreateMap().end())
            {
                assert(0);
                return 0;
            }

            base = (*it->second)();
            base->m_bDynamicCreated = true;
            base->Attach(hwnd);
        }
    }
    else
    {
        base = GetUserData(hwnd);
    }

    LRESULT ret = 0;
    if (base)
    {
#ifndef NDEBUG
        TCHAR szClass[128];
        GetClassName(hwnd, szClass, _countof(szClass));
#endif
        base->SaveMessageDx(hwnd, uMsg, wParam, lParam);
        ret = base->WindowProcDx(hwnd, uMsg, wParam, lParam);

        if (uMsg == WM_NCDESTROY)
        {
            base->PostNcDestroy();
        }
    }
    else
    {
        ret = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return ret;
}

inline BOOL MWindowBase::RegisterClassDx()
{
    HMODULE hMod = ::GetModuleHandle(NULL);

    WNDCLASSEX wcx;
    LPCTSTR pszClass = GetWndClassNameDx();
    if (::GetClassInfoEx(NULL, pszClass, &wcx) ||
        ::GetClassInfoEx(hMod, pszClass, &wcx))
    {
        return TRUE;
    }

    ZeroMemory(&wcx, sizeof(wcx));
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

inline BOOL MWindowBase::CreateWindowDx(
    HWND hwndParent, LPCTSTR pszText, 
    DWORD Style/* = WS_OVERLAPPEDWINDOW*/, DWORD ExStyle/* = 0*/, 
    INT x/* = CW_USEDEFAULT*/, INT y/* = CW_USEDEFAULT*/, 
    INT cx/* = CW_USEDEFAULT*/, INT cy/* = CW_USEDEFAULT*/, 
    HMENU hMenu/* = NULL*/)
{
    if (!RegisterClassDx())
        return FALSE;

    HWND hwnd = ::CreateWindowEx(ExStyle, GetWndClassNameDx(), 
        GetStringDx(pszText), Style, x, y, cx, cy, hwndParent, 
        hMenu, GetModuleHandle(NULL), this);
    return Attach(hwnd);
}

inline BOOL
MWindowBase::CreateAsChildDx(
    HWND hwndParent, LPCTSTR pszText, 
    DWORD Style/* = WS_CHILD | WS_VISIBLE*/, 
    DWORD ExStyle/* = 0*/, INT nCtrlID/* = -1*/, 
    INT x/* = CW_USEDEFAULT*/, INT y/* = CW_USEDEFAULT*/, 
    INT cx/* = CW_USEDEFAULT*/, INT cy/* = CW_USEDEFAULT*/)
{
    Style |= WS_CHILD;
    return CreateWindowDx(hwndParent, pszText, Style, ExStyle, 
        x, y, cx, cy, (HMENU)(INT_PTR)nCtrlID);
}

inline INT
MWindowBase::MsgBoxDx(LPCTSTR pszString, LPCTSTR pszTitle, 
                      UINT uType/* = MB_ICONINFORMATION*/)
{
    MString Title;
    if (pszTitle == NULL)
    {
#ifdef IDS_APPNAME
        Title = LoadStringDx(IDS_APPNAME);
#else
        if (m_hwnd)
        {
            Title = GetWindowText(GetAncestorDx());
            size_t i = Title.find(TEXT(" - "));
            if (i != MString::npos)
            {
                Title = Title.substr(i + 3);
            }
        }
        else
        {
            Title = TEXT("ERROR");
        }
#endif
    }
    else
    {
        Title = GetStringDx(pszTitle);
    }

    extern BOOL g_bNoGuiMode;
    extern LPWSTR g_pszLogFile;
    UINT nID;
    if (g_bNoGuiMode)
    {
        if (g_pszLogFile)
        {
            if (FILE *fp = _wfopen(g_pszLogFile, L"a"))
            {
                fprintf(fp, "%ls\n", GetStringDx(pszString));
                fclose(fp);
            }
        }
        nID = IDYES;
    }
    else
    {
        MWindowBase::HookCenterMsgBoxDx(TRUE);
        nID = ::MessageBox(m_hwnd, GetStringDx(pszString), Title.c_str(), uType);
        MWindowBase::HookCenterMsgBoxDx(FALSE);
    }

    return nID;
}

inline /*static*/ MString MWindowBase::GetWindowText(HWND hwnd)
{
    INT cch = ::GetWindowTextLength(hwnd);
    MString ret;
    ret.resize(cch);
    if (!::GetWindowText(hwnd, &ret[0], cch + 1))
        ret.clear();
    return ret;
}

inline /*static*/ VOID MWindowBase::CenterWindowDx(HWND hwnd)
{
    assert(IsWindow(hwnd));

    BOOL bChild = !!(GetWindowStyle(hwnd) & WS_CHILD);

    HWND hwndParent;
    if (bChild)
        hwndParent = ::GetParent(hwnd);
    else
        hwndParent = ::GetWindow(hwnd, GW_OWNER);

    RECT rcWorkArea = WorkAreaFromWindowDx(hwnd);

    RECT rcParent;
    if (hwndParent)
        ::GetWindowRect(hwndParent, &rcParent);
    else
        rcParent = rcWorkArea;

    SIZE sizParent = SizeFromRectDx(&rcParent);

    RECT rc;
    ::GetWindowRect(hwnd, &rc);
    SIZE siz = SizeFromRectDx(&rc);

    POINT pt;
    pt.x = rcParent.left + (sizParent.cx - siz.cx) / 2;
    pt.y = rcParent.top + (sizParent.cy - siz.cy) / 2;

    if (bChild && hwndParent)
    {
        ::GetClientRect(hwndParent, &rcParent);
        ::MapWindowPoints(hwndParent, NULL, (LPPOINT)&rcParent, 2);
        RepositionPointDx(&pt, siz, &rcParent);

        ::ScreenToClient(hwndParent, &pt);
    }
    else
    {
        RepositionPointDx(&pt, siz, &rcWorkArea);
    }

    ::SetWindowPos(hwnd, NULL, pt.x, pt.y, 0, 0, 
                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

inline /*static*/ BOOL MZCAPI
MWindowBase::GetWindowPosDx(HWND hwnd, POINT *ppt/* = NULL*/, SIZE *psiz/* = NULL*/)
{
    assert(IsWindow(hwnd));
    return ::GetWindowPosDx(hwnd, ppt, psiz);
}

inline BOOL MZCAPI
MWindowBase::GetWindowPosDx(POINT *ppt/* = NULL*/, SIZE *psiz/* = NULL*/)
{
    assert(IsWindow(m_hwnd));
    return ::GetWindowPosDx(m_hwnd, ppt, psiz);
}

inline /*static*/ BOOL MZCAPI MWindowBase::GetWindowPosDx(HWND hwnd, RECT *prc)
{
    assert(IsWindow(hwnd));
    return ::GetWindowPosDx(hwnd, prc);
}

inline BOOL MZCAPI MWindowBase::GetWindowPosDx(RECT *prc)
{
    assert(IsWindow(m_hwnd));
    return ::GetWindowPosDx(m_hwnd, prc);
}

inline /*static*/ BOOL
MWindowBase::SetWindowPosDx(
    HWND hwnd, LPPOINT ppt/* = NULL*/, LPSIZE psiz/* = NULL*/, 
    HWND hwndInsertAfter/* = NULL*/, 
    UINT uFlags/* = SWP_NOACTIVATE | SWP_NOOWNERZORDER*/)
{
    assert(IsWindow(hwnd));
    return ::SetWindowPosDx(hwnd, ppt, psiz, hwndInsertAfter, uFlags);
}

inline /*static*/ BOOL MWindowBase::SetWindowPosDx(HWND hwnd, const RECT *prc)
{
    assert(IsWindow(hwnd));
    return ::SetWindowPosDx(hwnd, prc);
}

inline BOOL
MWindowBase::SetWindowPosDx(
    LPPOINT ppt/* = NULL*/, LPSIZE psiz/* = NULL*/, 
    HWND hwndInsertAfter/* = NULL*/, 
    UINT uFlags/* = SWP_NOACTIVATE | SWP_NOOWNERZORDER*/)
{
    assert(IsWindow(m_hwnd));
    return ::SetWindowPosDx(m_hwnd, ppt, psiz, hwndInsertAfter, uFlags);
}

inline BOOL MWindowBase::SetWindowPosDx(const RECT *prc)
{
    assert(IsWindow(m_hwnd));
    return ::SetWindowPosDx(m_hwnd, prc);
}

inline /*static*/ HWND MWindowBase::GetAncestorDx(HWND hTarget)
{
    HWND hwndParent;
    for (;;)
    {
        hwndParent = ::GetParent(hTarget);
        if (hwndParent == NULL)
        {
            hwndParent = ::GetWindow(hTarget, GW_OWNER);
            if (hwndParent == NULL)
            {
                break;
            }
        }
        hTarget = hwndParent;
    }
    return hTarget;
}

/*static*/ inline LRESULT CALLBACK
MWindowBase::_msgBoxCbtProcDx(INT nCode, WPARAM wParam, LPARAM lParam)
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

inline /*static*/ HHOOK MWindowBase::HookCenterMsgBoxDx(BOOL bHook)
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

//////////////////////////////////////////////////////////////////////////////

inline /*static*/ INT_PTR CALLBACK
MDialogBase::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MDialogBase *base;
    if (uMsg == WM_INITDIALOG)
    {
        assert(lParam);
        base = (MDialogBase *)lParam;
        base->Attach(hwnd);
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
            base->PostNcDestroy();
        }
    }

    return ret;
}

inline BOOL MDialogBase::CreateDialogDx(HWND hwndOwner, INT nDialogID)
{
    return CreateDialogDx(hwndOwner, MAKEINTRESOURCE(nDialogID));
}

inline INT_PTR MDialogBase::DialogBoxDx(HWND hwndOwner, INT nDialogID)
{
    return DialogBoxDx(hwndOwner, MAKEINTRESOURCE(nDialogID));
}

inline BOOL
MDialogBase::CreateDialogIndirectDx(HWND hwndOwner, const VOID *ptr)
{
    if (hwndOwner)
    {
        m_hwndOwner = hwndOwner;
    }
    m_bModal = FALSE;
    HWND hwnd = ::CreateDialogIndirectParam(::GetModuleHandle(NULL), 
        reinterpret_cast<const DLGTEMPLATE *>(ptr), 
        m_hwndOwner, MDialogBase::DialogProc, 
        reinterpret_cast<LPARAM>(this));
    if (hwnd == NULL)
    {
        Detach();
    }
    return hwnd != NULL;
}

inline INT_PTR
MDialogBase::DialogBoxIndirectDx(HWND hwndOwner, const VOID *ptr)
{
    if (hwndOwner)
    {
        m_hwndOwner = hwndOwner;
    }
    m_bModal = TRUE;
    INT_PTR nID = ::DialogBoxIndirectParam(::GetModuleHandle(NULL), 
                                           (const DLGTEMPLATE *)ptr, 
                                           m_hwndOwner, 
                                           MDialogBase::DialogProc, 
                                           reinterpret_cast<LPARAM>(this));
    return nID;
}

inline BOOL
MDialogBase::CreateDialogDx(HWND hwndOwner, LPCTSTR pDialogName)
{
    if (hwndOwner)
    {
        m_hwndOwner = hwndOwner;
    }
    m_bModal = FALSE;
    HWND hwnd = ::CreateDialogParam(::GetModuleHandle(NULL), pDialogName, 
                                    m_hwndOwner, MDialogBase::DialogProc, 
                                    reinterpret_cast<LPARAM>(this));
    if (hwnd == NULL)
    {
        Detach();
    }
    return hwnd != NULL;
}

inline INT_PTR
MDialogBase::DialogBoxDx(HWND hwndOwner, LPCTSTR pDialogName)
{
    if (hwndOwner)
    {
        m_hwndOwner = hwndOwner;
    }
    m_bModal = TRUE;
    INT_PTR nID = ::DialogBoxParam(::GetModuleHandle(NULL), 
        pDialogName, m_hwndOwner, MDialogBase::DialogProc, (LPARAM)this);
    return nID;
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MWINDOWBASE_HPP_
