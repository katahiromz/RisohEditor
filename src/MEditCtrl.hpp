// MEditCtrl.hpp -- Win32API edit control wrapper               -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MEDITCTRL_HPP_
#define MZC4_MEDITCTRL_HPP_     3   /* Version 3 */

class MEditCtrl;

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"

class MEditCtrl : public MWindowBase
{
public:
    MEditCtrl();

    virtual LPCTSTR GetWndClassNameDx() const;

    BOOL CanUndo() const;
    INT GetLineCount() const;

    BOOL GetModify() const;
    VOID SetModify(BOOL bModified = TRUE);

    BOOL IsEmpty() const;

    HLOCAL GetHandle() const;
    VOID SetHandle(HLOCAL hBuffer);

    DWORD GetMargins() const;
    VOID SetMargins(UINT nLeft, UINT nRight);

    UINT GetLimitText() const;
    VOID SetLimitText(UINT nMax);

    POINT PosFromChar(UINT nChar) const;
    INT CharFromPos(POINT pt) const;

    VOID EmptyUndoBuffer();
    BOOL FmtLines(BOOL bAddEOL);

    VOID LimitText(INT nChars = 0);
    INT  LineFromChar(INT nIndex = -1) const;
    INT  LineIndex(INT nLine = -1) const;
    INT  LineLength(INT nLine = -1) const;
    INT  GetLine(INT nIndex, LPTSTR lpszBuffer, INT nMaxLength) const;
    VOID LineScroll(INT nLines, INT nChars = 0);

    TCHAR GetPasswordChar() const;
    VOID  SetPasswordChar(TCHAR ch);

    VOID GetRect(LPRECT prc) const;
    VOID SetRect(LPCRECT prc);
    VOID SetRectNP(LPCRECT prc);

    DWORD GetSel() const;
    VOID  GetSel(INT& nStartChar, INT& nEndChar) const;
    VOID  SetSel(DWORD dwSelection, BOOL bNoScroll = FALSE);
    VOID  SetSel(INT nStartChar, INT nEndChar, BOOL bNoScroll = FALSE);
    VOID  ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE);
    VOID  SelectAll(BOOL bNoScroll = FALSE);
    VOID  SelectNone(BOOL bNoScroll = FALSE);

    BOOL SetTabStops(INT nTabStops, LPINT rgTabStops);
    VOID SetTabStops();
    BOOL SetTabStops(const INT& cxEachStop);

    BOOL Undo();
    VOID Clear();
    VOID Copy();
    VOID Cut();
    VOID Paste();

    BOOL SetReadOnly(BOOL bReadOnly = TRUE);
    INT GetFirstVisibleLine() const;

    static void SetCtrlAHookDx(BOOL bHook);

protected:
    static HHOOK& OldHookProc();
    static LRESULT CALLBACK
    CtrlAMsgProcDx(INT nCode, WPARAM wParam, LPARAM lParam);
};

////////////////////////////////////////////////////////////////////////////

inline MEditCtrl::MEditCtrl()
{
}

inline /*virtual*/ LPCTSTR MEditCtrl::GetWndClassNameDx() const
{
    return TEXT("EDIT");
}

inline BOOL MEditCtrl::CanUndo() const
{
    return (BOOL)SendMessageDx(EM_CANUNDO);
}

inline INT MEditCtrl::GetLineCount() const
{
    return (INT)SendMessageDx(EM_GETLINECOUNT);
}

inline BOOL MEditCtrl::GetModify() const
{
    return (BOOL)SendMessageDx(EM_GETMODIFY);
}

inline VOID MEditCtrl::SetModify(BOOL bModified/* = TRUE*/)
{
    SendMessageDx(EM_SETMODIFY, (WPARAM)bModified);
}

inline BOOL MEditCtrl::IsEmpty() const
{
    return ::GetWindowTextLength(m_hwnd) == 0;
}

inline VOID MEditCtrl::GetRect(LPRECT prc) const
{
    SendMessageDx(EM_GETRECT, 0, (LPARAM)prc);
}

inline DWORD MEditCtrl::GetSel() const
{
    return (DWORD)SendMessageDx(EM_GETSEL);
}

inline VOID MEditCtrl::GetSel(INT& nStartChar, INT& nEndChar) const
{
    SendMessageDx(EM_GETSEL, (WPARAM)&nStartChar, (LPARAM)&nEndChar);
}

inline HLOCAL MEditCtrl::GetHandle() const
{
    return (HLOCAL)SendMessageDx(EM_GETHANDLE);
}

inline VOID MEditCtrl::SetHandle(HLOCAL hBuffer)
{
    SendMessageDx(EM_SETHANDLE, (WPARAM)hBuffer, 0);
}

inline VOID MEditCtrl::SetMargins(UINT nLeft, UINT nRight)
{
    SendMessageDx(EM_SETMARGINS,
        EC_LEFTMARGIN | EC_RIGHTMARGIN | EC_USEFONTINFO,
        MAKELPARAM(nLeft, nRight));
}

inline DWORD MEditCtrl::GetMargins() const
{
    return (DWORD)SendMessageDx(EM_GETMARGINS);
}

inline VOID MEditCtrl::SetLimitText(UINT nMax)
{
    SendMessageDx(EM_SETLIMITTEXT, nMax);
}

inline UINT MEditCtrl::GetLimitText() const
{
    return (UINT)SendMessageDx(EM_GETLIMITTEXT);
}

inline POINT MEditCtrl::PosFromChar(UINT nChar) const
{
    POINT pt;
    SendMessageDx(EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)nChar);
    return pt;
}

inline INT MEditCtrl::CharFromPos(POINT pt) const
{
    return (INT)SendMessageDx(EM_CHARFROMPOS, 0, MAKELPARAM(pt.x, pt.y));
}

inline INT MEditCtrl::GetLine(
    INT nIndex, LPTSTR lpszBuffer, INT nMaxLength) const
{
    assert(2 <= nMaxLength && nMaxLength <= 0xFFFF);
    *reinterpret_cast<WORD *>(lpszBuffer) = (WORD) nMaxLength;
    return (INT)SendMessageDx(EM_GETLINE, (WPARAM)nIndex, (LPARAM)lpszBuffer);
}

inline VOID MEditCtrl::EmptyUndoBuffer()
{
    SendMessageDx(EM_EMPTYUNDOBUFFER, 0, 0);
}

inline BOOL MEditCtrl::FmtLines(BOOL bAddEOL)
{
    return (BOOL)SendMessageDx(EM_FMTLINES, (WPARAM)bAddEOL);
}

inline VOID MEditCtrl::LimitText(INT nChars/* = 0*/)
{
    SendMessageDx(EM_LIMITTEXT, (WPARAM)nChars);
}

inline INT MEditCtrl::LineFromChar(INT nIndex/* = -1*/) const
{
    return (INT)SendMessageDx(EM_LINEFROMCHAR, (WPARAM)nIndex);
}

inline INT MEditCtrl::LineIndex(INT nLine/* = -1*/) const
{
    return (INT)SendMessageDx(EM_LINEINDEX, (WPARAM)nLine);
}

inline INT MEditCtrl::LineLength(INT nLine/* = -1*/) const
{
    return (INT)SendMessageDx(EM_LINELENGTH, (WPARAM)nLine);
}

inline VOID MEditCtrl::LineScroll(INT nLines, INT nChars/* = 0*/)
{
    SendMessageDx(EM_LINESCROLL, (WPARAM)nChars, (LPARAM)nLines);
}

inline VOID MEditCtrl::ReplaceSel(
    LPCTSTR lpszNewText, BOOL bCanUndo/* = FALSE*/)
{
    SendMessageDx(EM_REPLACESEL, (WPARAM)bCanUndo, (LPARAM)lpszNewText);
}

inline VOID MEditCtrl::SelectAll(BOOL bNoScroll/* = FALSE*/)
{
    SetSel(0, -1, bNoScroll);
}

inline VOID MEditCtrl::SelectNone(BOOL bNoScroll/* = FALSE*/)
{
    SetSel(-1, -1, bNoScroll);
}

inline VOID MEditCtrl::SetPasswordChar(TCHAR ch)
{
    SendMessageDx(EM_SETPASSWORDCHAR, (WPARAM)ch);
}

inline VOID MEditCtrl::SetRect(LPCRECT prc)
{
    SendMessageDx(EM_SETRECT, 0, (LPARAM)prc);
}

inline VOID MEditCtrl::SetRectNP(LPCRECT prc)
{
    SendMessageDx(EM_SETRECTNP, 0, (LPARAM)prc);
}

inline VOID MEditCtrl::SetSel(
    DWORD dwSelection, BOOL bNoScroll/* = FALSE*/)
{
    SetSel((INT) LOWORD(dwSelection), (INT) HIWORD(dwSelection), bNoScroll);
}

inline VOID MEditCtrl::SetSel(
    INT nStartChar, INT nEndChar, BOOL bNoScroll/* = FALSE*/)
{
    SendMessageDx(EM_SETSEL, (WPARAM)nStartChar, (LPARAM)nEndChar);
    if (!bNoScroll)
        SendMessageDx(EM_SCROLLCARET);
}

inline BOOL MEditCtrl::SetTabStops(INT nTabStops, LPINT rgTabStops)
{
    return (BOOL)SendMessageDx(EM_SETTABSTOPS, (WPARAM)nTabStops, (LPARAM)rgTabStops);
}

inline VOID MEditCtrl::SetTabStops()
{
    SendMessageDx(EM_SETTABSTOPS);
}

inline BOOL MEditCtrl::SetTabStops(const INT& cxEachStop)
{
    return (BOOL)SendMessageDx(EM_SETTABSTOPS, 1, (LPARAM)&cxEachStop);
}

inline BOOL MEditCtrl::Undo()
{
    return (BOOL)SendMessageDx(EM_UNDO);
}

inline VOID MEditCtrl::Clear()
{
    SendMessageDx(WM_CLEAR, 0, 0);
}

inline VOID MEditCtrl::Copy()
{
    SendMessageDx(WM_COPY, 0, 0);
}

inline VOID MEditCtrl::Cut()
{
    SendMessageDx(WM_CUT, 0, 0);
}

inline VOID MEditCtrl::Paste()
{
    SendMessageDx(WM_PASTE, 0, 0);
}

inline BOOL MEditCtrl::SetReadOnly(BOOL bReadOnly/* = TRUE*/)
{
    return (BOOL)SendMessageDx(EM_SETREADONLY, (WPARAM)bReadOnly);
}

inline INT MEditCtrl::GetFirstVisibleLine() const
{
    return (INT)SendMessageDx(EM_GETFIRSTVISIBLELINE);
}

inline TCHAR MEditCtrl::GetPasswordChar() const
{
    return (TCHAR)SendMessageDx(EM_GETPASSWORDCHAR);
}

inline /*static*/ HHOOK& MEditCtrl::OldHookProc()
{
    static HHOOK s_hHook = NULL;
    return s_hHook;
}

inline /*static*/ LRESULT CALLBACK
MEditCtrl::CtrlAMsgProcDx(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
        return ::CallNextHookEx(OldHookProc(), nCode, wParam, lParam);

    MSG *pMsg = (MSG *)lParam;
    TCHAR szClassName[8];
    HWND hWnd;
    if (pMsg->message == WM_KEYDOWN)
    {
        if ((INT) pMsg->wParam == 'A' &&
            ::GetAsyncKeyState(VK_CONTROL) < 0 &&
            ::GetAsyncKeyState(VK_SHIFT) >= 0 &&
            ::GetAsyncKeyState(VK_MENU) >= 0)
        {
            // Ctrl+A is pressed
            hWnd = ::GetFocus();
            if (hWnd != NULL)
            {
                ::GetClassName(hWnd, szClassName, _countof(szClassName));
                if (lstrcmpi(szClassName, TEXT("EDIT")) == 0)
                {
                    ::SendMessage(hWnd, EM_SETSEL, 0, -1);
                    return 1;
                }
            }
        }
    }
    return ::CallNextHookEx(OldHookProc(), nCode, wParam, lParam);
}

inline /*static*/ void MEditCtrl::SetCtrlAHookDx(BOOL bHook)
{
    if (bHook)
    {
        assert(OldHookProc() == NULL);
        OldHookProc() = ::SetWindowsHookEx(
            WH_MSGFILTER, MEditCtrl::CtrlAMsgProcDx, NULL,
            ::GetCurrentThreadId());
    }
    else
    {
        assert(OldHookProc() != NULL);
        ::UnhookWindowsHookEx(OldHookProc());
        OldHookProc() = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MEDITCTRL_HPP_
