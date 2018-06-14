// MToolBarCtrl.hpp -- Win32API tool bar control wrapper        -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MTOOLBARCTRL_HPP_
#define MZC4_MTOOLBARCTRL_HPP_      5   /* Version 5 */

class MToolBarCtrl;

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"

class MToolBarCtrl : public MWindowBase
{
public:
    MToolBarCtrl();
    virtual LPCTSTR GetWndClassNameDx() const;

    virtual HWND SetParent(HWND hWndNewParent);

    BOOL IsButtonEnabled(INT nButtonID) const;
    BOOL IsButtonChecked(INT nButtonID) const;
    BOOL IsButtonPressed(INT nButtonID) const;
    BOOL IsButtonHidden(INT nButtonID) const;
    BOOL IsButtonIndeterminate(INT nButtonID) const;
    #if (_WIN32_IE >= 0x0400)
        BOOL IsButtonHighlighted(INT nButtonID) const;
    #endif  // (_WIN32_IE >= 0x0400)

    INT GetState(INT nButtonID) const;
    BOOL SetState(INT nButtonID, UINT nTBSTATE_flags);

    BOOL GetButton(INT nIndex, LPTBBUTTON lpButton) const;
    BOOL SetButton(INT nIndex, CONST TBBUTTON *lpButton);
    INT GetButtonCount() const;
    BOOL GetItemRect(INT nIndex, LPRECT prc) const;
    VOID SetButtonStructSize(INT nSize);

    #if (_WIN32_IE >= 0x0300)
        BOOL GetRect(INT nButtonID, LPRECT prc) const;

        SIZE GetButtonSize() const;
    #endif  // (_WIN32_IE >= 0x0300)

    BOOL SetButtonSize(SIZE size);

    BOOL SetBitmapSize(SIZE size);

    HWND GetToolTips() const;
    VOID SetToolTips(HWND hwndTT);

    INT GetRows() const;
    VOID SetRows(INT nRows, BOOL bLarger, LPRECT prc);
    BOOL SetCmdID(INT nIndex, UINT nCommandID);
    UINT GetBitmapFlags() const;

    #if (_WIN32_IE >= 0x0300)
        INT GetTextRows() const;

        DWORD GetStyle() const;
        VOID SetStyle(DWORD dwStyle);

        BOOL SetButtonWidth(INT cxMin, INT cxMax);

        HIMAGELIST GetImageList(INT nIndex = 0) const;
        HIMAGELIST SetImageList(HIMAGELIST hImageList, INT nIndex = 0);

        HIMAGELIST GetDisabledImageList(INT nIndex = 0) const;
        HIMAGELIST SetDisabledImageList(HIMAGELIST hImageList, INT nIndex = 0);

        HIMAGELIST GetHotImageList(INT nIndex = 0) const;
        HIMAGELIST SetHotImageList(HIMAGELIST hImageList, INT nIndex = 0);

        BOOL SetIndent(INT iIndent);
        BOOL SetMaxTextRows(INT iMaxRows);
    #endif  // (_WIN32_IE >= 0x0300)

    #if (_WIN32_IE >= 0x400)
        BOOL GetButtonInfo(INT nButtonID, TBBUTTONINFO* ptbbi) const;
        BOOL SetButtonInfo(INT nButtonID, TBBUTTONINFO* ptbbi);

        DWORD SetDrawTextFlags(DWORD dwMask, DWORD dwDTFlags);

        BOOL GetAnchorHighlight() const;
        BOOL SetAnchorHighlight(BOOL bEnable = TRUE);

        INT GetHotItem() const;
        INT SetHotItem(INT nIndex);

        VOID GetInsertMark(TBINSERTMARK* ptbim) const;
        VOID SetInsertMark(TBINSERTMARK* ptbim);
        BOOL InsertMarkHitTest(LPPOINT ppt, LPTBINSERTMARK ptbim) const;
        COLORREF GetInsertMarkColor() const;
        COLORREF SetInsertMarkColor(COLORREF clrNew);

        BOOL GetMaxSize(LPSIZE pSize) const;

        DWORD GetExtendedStyle() const;
        DWORD SetExtendedStyle(DWORD dwExStyle);
    #endif  // (_WIN32_IE >= 0x400)

    BOOL EnableButton(INT nButtonID, BOOL bEnable = TRUE);
    BOOL CheckButton(INT nButtonID, BOOL bCheck = TRUE);
    BOOL PressButton(INT nButtonID, BOOL bPress = TRUE);
    BOOL HideButton(INT nButtonID, BOOL bHide = TRUE);
    BOOL Indeterminate(INT nButtonID, BOOL bIndeterminate = TRUE);
    INT AddBitmap(INT nNumButtons, UINT nBitmapID);
    INT AddBitmap(INT nNumButtons, HBITMAP hBitmap);
    BOOL AddButtons(INT nNumButtons, LPTBBUTTON lpButtons);
    BOOL InsertButton(INT nIndex, CONST TBBUTTON *lpButton);
    BOOL DeleteButton(INT nIndex);
    UINT CommandToIndex(UINT nCommandID) const;
    VOID SaveState(HKEY hKeyRoot, LPCTSTR lpszSubKey, 
        LPCTSTR lpszValueName);
    VOID RestoreState(HKEY hKeyRoot, LPCTSTR lpszSubKey, 
        LPCTSTR lpszValueName);

    #if (_WIN32_IE >= 0x0300)
        VOID LoadImages(INT iBitmapID, HINSTANCE hinst = ::GetModuleHandle(NULL));
    #endif  // (_WIN32_IE >= 0x0300)

    #if (_WIN32_IE >= 0x0400)
        BOOL MapAccelerator(TCHAR chAccel, UINT* pIDBtn);
        BOOL MarkButton(INT nButtonID, BOOL fHighlight = TRUE);
        BOOL MoveButton(UINT nOldPos, UINT nNewPos);
        INT HitTest(LPPOINT ppt) const;
    #endif  // (_WIN32_IE >= 0x0400)

    VOID Customize();
    INT AddString(UINT nStringID);
    INT AddStrings(LPCTSTR pszzStrings);
    VOID AutoSize();
};

////////////////////////////////////////////////////////////////////////////

inline MToolBarCtrl::MToolBarCtrl()
{
}

inline /*virtual*/ LPCTSTR MToolBarCtrl::GetWndClassNameDx() const
{
    return TOOLBARCLASSNAME;
}

inline /*virtual*/ HWND MToolBarCtrl::SetParent(HWND hWndNewParent)
{
    HWND hwndOldParent = ::GetParent(m_hwnd);
    SendMessageDx(TB_SETPARENT, (WPARAM)hWndNewParent);
    return hwndOldParent;
}

inline BOOL MToolBarCtrl::IsButtonEnabled(INT nButtonID) const
{
    return (BOOL)SendMessageDx(TB_ISBUTTONENABLED, (WPARAM)nButtonID);
}

inline BOOL MToolBarCtrl::IsButtonChecked(INT nButtonID) const
{
    return (BOOL)SendMessageDx(TB_ISBUTTONCHECKED, (WPARAM)nButtonID);
}

inline BOOL MToolBarCtrl::IsButtonPressed(INT nButtonID) const
{
    return (BOOL)SendMessageDx(TB_ISBUTTONPRESSED, (WPARAM)nButtonID);
}

inline BOOL MToolBarCtrl::IsButtonHidden(INT nButtonID) const
{
    return (BOOL)SendMessageDx(TB_ISBUTTONHIDDEN, (WPARAM)nButtonID);
}

inline BOOL MToolBarCtrl::IsButtonIndeterminate(INT nButtonID) const
{
    return (BOOL)SendMessageDx(TB_ISBUTTONINDETERMINATE, (WPARAM)nButtonID);
}
#if (_WIN32_IE >= 0x0400)

    inline BOOL MToolBarCtrl::IsButtonHighlighted(INT nButtonID) const
    {
        return (BOOL)SendMessageDx(TB_ISBUTTONHIGHLIGHTED, (WPARAM)nButtonID);
    }
#endif  // (_WIN32_IE >= 0x0400)

inline INT MToolBarCtrl::GetState(INT nButtonID) const
{
    return (INT)SendMessageDx(TB_GETSTATE, (WPARAM)nButtonID);
}

inline BOOL MToolBarCtrl::SetState(INT nButtonID, UINT nTBSTATE_flags)
{
    return (BOOL)SendMessageDx(TB_SETSTATE, (WPARAM)nButtonID, MAKELPARAM(nTBSTATE_flags, 0));
}

inline BOOL MToolBarCtrl::GetButton(INT nIndex, LPTBBUTTON lpButton) const
{
    return (BOOL)SendMessageDx(TB_GETBUTTON, (WPARAM)nIndex, (LPARAM)lpButton);
}

inline BOOL MToolBarCtrl::SetButton(INT nIndex, CONST TBBUTTON *lpButton)
{
    return DeleteButton(nIndex) && InsertButton(nIndex, lpButton);
}

inline INT MToolBarCtrl::GetButtonCount() const
{
    return (INT)SendMessageDx(TB_BUTTONCOUNT);
}

inline BOOL MToolBarCtrl::GetItemRect(INT nIndex, LPRECT prc) const
{
    return (BOOL)SendMessageDx(TB_GETITEMRECT, (WPARAM)nIndex, (LPARAM)prc);
}

inline VOID MToolBarCtrl::SetButtonStructSize(INT nSize)
{
    SendMessageDx(TB_BUTTONSTRUCTSIZE, (WPARAM)nSize);
}

#if (_WIN32_IE >= 0x0300)
    inline BOOL MToolBarCtrl::GetRect(INT nButtonID, LPRECT prc) const
    {
        return (BOOL)SendMessageDx(TB_GETRECT, (WPARAM)nButtonID, (LPARAM)prc);
    }

    inline SIZE MToolBarCtrl::GetButtonSize() const
    {
        SIZE siz;
        DWORD dw = (DWORD)SendMessageDx(TB_GETBUTTONSIZE);
        siz.cx = (SHORT)LOWORD(dw);
        siz.cy = (SHORT)HIWORD(dw);
        return siz;
    }
#endif  // (_WIN32_IE >= 0x0300)

inline BOOL MToolBarCtrl::SetButtonSize(SIZE size)
{
    return (BOOL)SendMessageDx(TB_SETBUTTONSIZE, 0, MAKELPARAM(size.cx, size.cy));
}

inline BOOL MToolBarCtrl::SetBitmapSize(SIZE size)
{
    return (BOOL)SendMessageDx(TB_SETBITMAPSIZE, 0, MAKELPARAM(size.cx, size.cy));
}

inline HWND MToolBarCtrl::GetToolTips() const
{
    return (HWND)SendMessageDx(TB_GETTOOLTIPS);
}

inline VOID MToolBarCtrl::SetToolTips(HWND hwndTT)
{
    SendMessageDx(TB_SETTOOLTIPS, (WPARAM)hwndTT);
}

inline INT MToolBarCtrl::GetRows() const
{
    return (INT)SendMessageDx(TB_GETROWS);
}

inline VOID MToolBarCtrl::SetRows(INT nRows, BOOL bLarger, LPRECT prc)
{
    SendMessageDx(TB_SETROWS, MAKEWPARAM(nRows, bLarger), (LPARAM)prc);
}

inline BOOL MToolBarCtrl::SetCmdID(INT nIndex, UINT nCommandID)
{
    return (BOOL)SendMessageDx(TB_SETCMDID, (WPARAM)nIndex, (LPARAM)nCommandID);
}

inline UINT MToolBarCtrl::GetBitmapFlags() const
{
    return (DWORD)SendMessageDx(TB_GETBITMAPFLAGS);
}

#if (_WIN32_IE >= 0x0300)
    inline INT MToolBarCtrl::GetTextRows() const
    {
        return (INT)SendMessageDx(TB_GETTEXTROWS);
    }

    inline DWORD MToolBarCtrl::GetStyle() const
    {
        return (DWORD)SendMessageDx(TB_GETSTYLE);
    }

    inline VOID MToolBarCtrl::SetStyle(DWORD dwStyle)
    {
        SendMessageDx(TB_SETSTYLE, 0, (LPARAM)dwStyle);
    }

    inline BOOL MToolBarCtrl::SetButtonWidth(INT cxMin, INT cxMax)
    {
        return (BOOL)SendMessageDx(TB_SETBUTTONWIDTH, 0, MAKELPARAM(cxMin, cxMax));
    }

    inline HIMAGELIST
    MToolBarCtrl::GetImageList(INT nIndex/* = 0*/) const
    {
        return (HIMAGELIST)SendMessageDx(TB_GETIMAGELIST, (WPARAM)nIndex);
    }

    inline HIMAGELIST
    MToolBarCtrl::SetImageList(HIMAGELIST hImageList, INT nIndex/* = 0*/)
    {
        return (HIMAGELIST)SendMessageDx(TB_SETIMAGELIST, (WPARAM)nIndex, (LPARAM)hImageList);
    }

    inline HIMAGELIST MToolBarCtrl::GetDisabledImageList(
        INT nIndex/* = 0*/) const
    {
        return (HIMAGELIST)SendMessageDx(TB_GETDISABLEDIMAGELIST, (WPARAM)nIndex);
    }

    inline HIMAGELIST MToolBarCtrl::SetDisabledImageList(
        HIMAGELIST hImageList, INT nIndex/* = 0*/)
    {
        return (HIMAGELIST)SendMessageDx(TB_SETDISABLEDIMAGELIST, (WPARAM)nIndex, (LPARAM)hImageList);
    }

    inline HIMAGELIST
    MToolBarCtrl::GetHotImageList(INT nIndex/* = 0*/) const
    {
        return (HIMAGELIST)SendMessageDx(TB_GETHOTIMAGELIST, (WPARAM)nIndex);
    }

    inline HIMAGELIST
    MToolBarCtrl::SetHotImageList(HIMAGELIST hImageList, INT nIndex/* = 0*/)
    {
        return (HIMAGELIST)SendMessageDx(TB_SETHOTIMAGELIST, (WPARAM)nIndex, (LPARAM)hImageList);
    }

    inline BOOL MToolBarCtrl::SetIndent(INT iIndent)
    {
        return (BOOL)SendMessageDx(TB_SETINDENT, (WPARAM)iIndent);
    }

    inline BOOL MToolBarCtrl::SetMaxTextRows(INT iMaxRows)
    {
        return (BOOL)SendMessageDx(TB_SETMAXTEXTROWS, (WPARAM)iMaxRows);
    }
#endif  // (_WIN32_IE >= 0x0300)

#if (_WIN32_IE >= 0x400)
    inline BOOL MToolBarCtrl::GetButtonInfo(
        INT nButtonID, TBBUTTONINFO* ptbbi) const
    {
        return (INT)SendMessageDx(TB_GETBUTTONINFO, (WPARAM)nButtonID, (LPARAM)ptbbi);
    }

    inline BOOL MToolBarCtrl::SetButtonInfo(
        INT nButtonID, TBBUTTONINFO* ptbbi)
    {
        return (BOOL)SendMessageDx(TB_SETBUTTONINFO, (WPARAM)nButtonID, (LPARAM)ptbbi);
    }

    inline DWORD MToolBarCtrl::SetDrawTextFlags(
        DWORD dwMask, DWORD dwDTFlags)
    {
        return (DWORD)SendMessageDx(TB_SETDRAWTEXTFLAGS, (WPARAM)dwMask, (LPARAM)dwDTFlags);
    }

    inline BOOL MToolBarCtrl::GetAnchorHighlight() const
    {
        return (BOOL)SendMessageDx(TB_GETANCHORHIGHLIGHT);
    }

    inline BOOL MToolBarCtrl::SetAnchorHighlight(
        BOOL bEnable/* = TRUE*/)
    {
        return (BOOL)SendMessageDx(TB_SETANCHORHIGHLIGHT, (WPARAM)bEnable);
    }

    inline INT MToolBarCtrl::GetHotItem() const
    {
        return (INT)SendMessageDx(TB_GETHOTITEM);
    }

    inline INT MToolBarCtrl::SetHotItem(INT nIndex)
    {
        return (INT)SendMessageDx(TB_SETHOTITEM, (WPARAM)nIndex);
    }

    inline VOID MToolBarCtrl::GetInsertMark(TBINSERTMARK* ptbim) const
    {
        SendMessageDx(TB_GETINSERTMARK, 0, (LPARAM)ptbim);
    }

    inline VOID MToolBarCtrl::SetInsertMark(TBINSERTMARK* ptbim)
    {
        SendMessageDx(TB_SETINSERTMARK, 0, (LPARAM)ptbim);
    }

    inline BOOL MToolBarCtrl::InsertMarkHitTest(
        LPPOINT ppt, LPTBINSERTMARK ptbim) const
    {
        return (BOOL)SendMessageDx(TB_INSERTMARKHITTEST, (WPARAM)ppt, (LPARAM)ptbim);
    }

    inline BOOL MToolBarCtrl::GetMaxSize(LPSIZE pSize) const
    {
        return (BOOL)SendMessageDx(TB_GETMAXSIZE, 0, (LPARAM)pSize);
    }

    inline DWORD MToolBarCtrl::GetExtendedStyle() const
    {
        return (DWORD)SendMessageDx(TB_GETEXTENDEDSTYLE);
    }

    inline DWORD MToolBarCtrl::SetExtendedStyle(DWORD dwExStyle)
    {
        return (DWORD)SendMessageDx(TB_SETEXTENDEDSTYLE, 0, (LPARAM)dwExStyle);
    }

    inline COLORREF MToolBarCtrl::GetInsertMarkColor() const
    {
        return (COLORREF)SendMessageDx(TB_GETINSERTMARKCOLOR);
    }

    inline COLORREF MToolBarCtrl::SetInsertMarkColor(COLORREF clrNew)
    {
        return (COLORREF)SendMessageDx(TB_SETINSERTMARKCOLOR, 0, (LPARAM)clrNew);
    }
#endif  // (_WIN32_IE >= 0x400)

inline BOOL MToolBarCtrl::EnableButton(
    INT nButtonID, BOOL bEnable/* = TRUE*/)
{
    return (BOOL)SendMessageDx(TB_ENABLEBUTTON, (WPARAM)nButtonID, MAKELPARAM(bEnable, 0));
}

inline BOOL MToolBarCtrl::CheckButton(
    INT nButtonID, BOOL bCheck/* = TRUE*/)
{
    return (BOOL)SendMessageDx(TB_CHECKBUTTON, (WPARAM)nButtonID, MAKELPARAM(bCheck, 0));
}

inline BOOL MToolBarCtrl::PressButton(
    INT nButtonID, BOOL bPress/* = TRUE*/)
{
    return (BOOL)SendMessageDx(TB_PRESSBUTTON, (WPARAM)nButtonID, MAKELPARAM(bPress, 0));
}

inline BOOL MToolBarCtrl::HideButton(INT nButtonID, BOOL bHide/* = TRUE*/)
{
    return (BOOL)SendMessageDx(TB_HIDEBUTTON, (WPARAM)nButtonID, MAKELPARAM(bHide, 0));
}

inline BOOL MToolBarCtrl::Indeterminate(
    INT nButtonID, BOOL bIndeterminate/* = TRUE*/)
{
    return (BOOL)SendMessageDx(TB_INDETERMINATE, (WPARAM)nButtonID, MAKELPARAM(bIndeterminate, 0));
}

inline INT MToolBarCtrl::AddBitmap(INT nNumButtons, UINT nBitmapID)
{
    TBADDBITMAP tbab;
    ZeroMemory(&tbab, sizeof(tbab));
    tbab.hInst = ::GetModuleHandle(NULL);
    tbab.nID = nBitmapID;
    return (INT)SendMessageDx(TB_ADDBITMAP, (WPARAM)nNumButtons, (LPARAM)&tbab);
}

inline INT MToolBarCtrl::AddBitmap(INT nNumButtons, HBITMAP hBitmap)
{
    TBADDBITMAP tbab;
    ZeroMemory(&tbab, sizeof(tbab));
    tbab.hInst = NULL;
    tbab.nID = (UINT_PTR) hBitmap;
    return (INT)SendMessageDx(TB_ADDBITMAP, (WPARAM)nNumButtons, (LPARAM)&tbab);
}

inline BOOL MToolBarCtrl::AddButtons(
    INT nNumButtons, LPTBBUTTON lpButtons)
{
    return (BOOL)SendMessageDx(TB_ADDBUTTONS, (WPARAM)nNumButtons, (LPARAM)lpButtons);
}

inline BOOL MToolBarCtrl::InsertButton(
    INT nIndex, CONST TBBUTTON *lpButton)
{
    return (BOOL)SendMessageDx(TB_INSERTBUTTON, (WPARAM)nIndex, (LPARAM)lpButton);
}

inline BOOL MToolBarCtrl::DeleteButton(INT nIndex)
{
    return (BOOL)SendMessageDx(TB_DELETEBUTTON, (WPARAM)nIndex);
}

inline UINT MToolBarCtrl::CommandToIndex(UINT nCommandID) const
{
    return (UINT)SendMessageDx(TB_COMMANDTOINDEX, nCommandID);
}

inline VOID MToolBarCtrl::SaveState(
    HKEY hKeyRoot, LPCTSTR lpszSubKey, LPCTSTR lpszValueName)
{
    TBSAVEPARAMS tbs;
    ZeroMemory(&tbs, sizeof(tbs));
    tbs.hkr = hKeyRoot;
    tbs.pszSubKey = lpszSubKey;
    tbs.pszValueName = lpszValueName;
    SendMessageDx(TB_SAVERESTORE, (WPARAM)TRUE, (LPARAM)&tbs);
}

inline VOID MToolBarCtrl::RestoreState(
    HKEY hKeyRoot, LPCTSTR lpszSubKey, LPCTSTR lpszValueName)
{
    TBSAVEPARAMS tbs;
    ZeroMemory(&tbs, sizeof(tbs));
    tbs.hkr = hKeyRoot;
    tbs.pszSubKey = lpszSubKey;
    tbs.pszValueName = lpszValueName;
    SendMessageDx(TB_SAVERESTORE, (WPARAM)FALSE, (LPARAM)&tbs);
}

#if (_WIN32_IE >= 0x0300)
    inline VOID MToolBarCtrl::LoadImages(
        INT iBitmapID, HINSTANCE hinst/* = ::GetModuleHandle(NULL)*/)
    {
        SendMessageDx(TB_LOADIMAGES, (WPARAM)iBitmapID, (LPARAM)hinst);
    }
#endif  // (_WIN32_IE >= 0x0300)

#if (_WIN32_IE >= 0x0400)
    inline BOOL MToolBarCtrl::MapAccelerator(TCHAR chAccel, UINT* pIDBtn)
    {
        return (BOOL)SendMessageDx(TB_MAPACCELERATOR, (WPARAM)chAccel, (LPARAM)&pIDBtn);
    }

    inline BOOL MToolBarCtrl::MarkButton(
        INT nButtonID, BOOL fHighlight/* = TRUE*/)
    {
        return (BOOL)SendMessageDx(TB_MARKBUTTON, (WPARAM)nButtonID, MAKELPARAM(fHighlight, 0));
    }

    inline BOOL MToolBarCtrl::MoveButton(UINT nOldPos, UINT nNewPos)
    {
        return (BOOL)SendMessageDx(TB_MOVEBUTTON, (WPARAM)nOldPos, (LPARAM)nNewPos);
    }

    inline INT MToolBarCtrl::HitTest(LPPOINT ppt) const
    {
        return (INT)SendMessageDx(TB_HITTEST, 0, (LPARAM)ppt);
    }
#endif  // (_WIN32_IE >= 0x0400)

inline VOID MToolBarCtrl::Customize()
{
    SendMessageDx(TB_CUSTOMIZE);
}

inline INT MToolBarCtrl::AddString(UINT nStringID)
{
    return (INT)SendMessageDx(TB_ADDSTRING, (WPARAM)::GetModuleHandle(NULL), (LPARAM)nStringID);
}

inline INT MToolBarCtrl::AddStrings(LPCTSTR pszzStrings)
{
    return (INT)SendMessageDx(TB_ADDSTRING, 0, (LPARAM)pszzStrings);
}

inline VOID MToolBarCtrl::AutoSize()
{
    SendMessageDx(TB_AUTOSIZE);
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTOOLBARCTRL_HPP_
