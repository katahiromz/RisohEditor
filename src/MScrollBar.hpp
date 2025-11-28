// MScrollBar.hpp -- Win32API scroll bar wrapper -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MSCROLLBAR_HPP_
#define MZC4_MSCROLLBAR_HPP_        2   /* Version 2 */

class MScrollBar;
//class MSizeGrip;

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"

#define MSCROLLBAR_HSCROLL_STYLE \
    (WS_CHILD | WS_VISIBLE | SBS_HORZ)
#define MSCROLLBAR_VSCROLL_STYLE \
    (WS_CHILD | WS_VISIBLE | SBS_VERT)

class MScrollBar : public MWindowBase
{
public:
    MScrollBar();

    virtual LPCTSTR GetWndClassNameDx() const;

    INT GetScrollPos() const;
    INT SetScrollPos(INT nPos, BOOL bRedraw = TRUE);

    VOID GetScrollRange(LPINT lpMinPos, LPINT lpMaxPos) const;
    VOID SetScrollRange(INT nMinPos, INT nMaxPos, BOOL bRedraw = TRUE);

    VOID ShowScrollBar(BOOL bShow = TRUE);

    BOOL EnableScrollBar(UINT nArrowFlags = ESB_ENABLE_BOTH);

    BOOL GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nSIF_mask = SIF_ALL);
    BOOL SetScrollInfo(LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE);

    INT GetScrollLimit();

    static INT GetHScrollWidth();
    static INT GetHScrollHeight();
    static INT GetVScrollWidth();
    static INT GetVScrollHeight();
    static INT GetHThumbWidth();
    static INT GetVThumbHeight();
};
typedef MScrollBar MSizeGrip;

UINT GetMouseScrollLinesDx(VOID);

////////////////////////////////////////////////////////////////////////////

inline MScrollBar::MScrollBar()
{
}

inline /*virtual*/ LPCTSTR MScrollBar::GetWndClassNameDx() const
{
    return TEXT("SCROLLBAR");
}

inline INT MScrollBar::GetScrollPos() const
{
    return ::GetScrollPos(m_hwnd, SB_CTL);
}

inline INT MScrollBar::SetScrollPos(INT nPos, BOOL bRedraw/* = TRUE*/)
{
    return ::SetScrollPos(m_hwnd, SB_CTL, nPos, bRedraw);
}

inline VOID MScrollBar::GetScrollRange(
    LPINT lpMinPos, LPINT lpMaxPos) const
{
    ::GetScrollRange(m_hwnd, SB_CTL, lpMinPos, lpMaxPos);
}

inline VOID MScrollBar::SetScrollRange(
    INT nMinPos, INT nMaxPos, BOOL bRedraw/* = TRUE*/)
{
    ::SetScrollRange(m_hwnd, SB_CTL, nMinPos, nMaxPos, bRedraw);
}

inline VOID MScrollBar::ShowScrollBar(BOOL bShow/* = TRUE*/)
{
    ::ShowScrollBar(m_hwnd, SB_CTL, bShow);
}

inline BOOL MScrollBar::EnableScrollBar(
    UINT nArrowFlags/* = ESB_ENABLE_BOTH*/)
{
    return ::EnableScrollBar(m_hwnd, SB_CTL, nArrowFlags);
}

inline BOOL MScrollBar::SetScrollInfo(
    LPSCROLLINFO lpScrollInfo, BOOL bRedraw/* = TRUE*/)
{
    return ::SetScrollInfo(m_hwnd, SB_CTL, lpScrollInfo, bRedraw);
}

inline BOOL MScrollBar::GetScrollInfo(
    LPSCROLLINFO lpScrollInfo, UINT nSIF_mask/* = SIF_ALL*/)
{
    lpScrollInfo->fMask = nSIF_mask;
    return ::GetScrollInfo(m_hwnd, SB_CTL, lpScrollInfo);
}

inline INT MScrollBar::GetScrollLimit()
{
    INT nMin, nMax;
    ::GetScrollRange(m_hwnd, SB_CTL, &nMin, &nMax);

    SCROLLINFO info;
    info.cbSize = sizeof(SCROLLINFO);
    info.fMask = SIF_PAGE;
    if(::GetScrollInfo(m_hwnd, SB_CTL, &info))
        nMax -= ((info.nPage - 1) > 0) ? (info.nPage - 1) : 0;
    return nMax;
}

inline /*static*/ INT MScrollBar::GetHThumbWidth()
{
    return ::GetSystemMetrics(SM_CXHTHUMB);
}

inline /*static*/ INT MScrollBar::GetVThumbHeight()
{
    return ::GetSystemMetrics(SM_CYVTHUMB);
}

inline /*static*/ INT MScrollBar::GetHScrollWidth()
{
    return ::GetSystemMetrics(SM_CXHSCROLL);
}

inline /*static*/ INT MScrollBar::GetHScrollHeight()
{
    return ::GetSystemMetrics(SM_CYHSCROLL);
}

inline /*static*/ INT MScrollBar::GetVScrollWidth()
{
    return ::GetSystemMetrics(SM_CXVSCROLL);
}

inline /*static*/ INT MScrollBar::GetVScrollHeight()
{
    return ::GetSystemMetrics(SM_CYVSCROLL);
}

inline UINT GetMouseScrollLinesDx(VOID)
{
    static BOOL s_bGot = FALSE;
    static UINT s_uCachedScrollLines;

    if (s_bGot)
        return s_uCachedScrollLines;

#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
    s_bGot = ::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
        &s_uCachedScrollLines, 0);
    if (s_bGot)
        return s_uCachedScrollLines;
#endif  // (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)

    s_bGot = TRUE;
    HWND hwMouseWheel = ::FindWindow(TEXT("MouseZ"), TEXT("Magellan MSWHEEL"));
    UINT uMsg = ::RegisterWindowMessage(TEXT("MSH_SCROLL_LINES_MSG"));
    if (hwMouseWheel != NULL && uMsg != 0)
    {
        s_uCachedScrollLines = (UINT)::SendMessage(hwMouseWheel, uMsg, 0, 0);
        return s_uCachedScrollLines;
    }

    s_uCachedScrollLines = 3; // reasonable default
    HKEY hKey;
    if (::RegOpenKeyEx(HKEY_CURRENT_USER,  TEXT("Control Panel\\Desktop"),
                       0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        TCHAR szData[128];
        DWORD dwKeyDataType;
        DWORD dwDataBufSize = sizeof(szData);

        if (::RegQueryValueEx(hKey, TEXT("WheelScrollLines"), NULL,
            &dwKeyDataType, (LPBYTE)&szData, &dwDataBufSize) == ERROR_SUCCESS)
        {
            s_uCachedScrollLines = (UINT) _ttoi(szData);
        }
        RegCloseKey(hKey);
    }

    return s_uCachedScrollLines;
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MSCROLLBAR_HPP_
