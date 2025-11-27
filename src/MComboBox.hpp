// MComboBox.hpp -- Win32API combo box wrapper                  -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MCOMBOBOX_HPP_
#define MZC4_MCOMBOBOX_HPP_     2   /* Version 2 */

class MComboBox;

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"

class MComboBox : public MWindowBase
{
public:
    MComboBox();

    virtual LPCTSTR GetWndClassNameDx() const;

    INT GetCount() const;
    INT GetCurSel() const;
    INT SetCurSel(INT nSelect);
    LCID GetLocale() const;
    LCID SetLocale(LCID nNewLocale);

    INT  GetTopIndex() const;
    INT  SetTopIndex(INT nIndex);
    INT  InitStorage(INT nItems, UINT nBytes);
    VOID SetHorizontalExtent(UINT nExtent);
    UINT GetHorizontalExtent() const;

    INT  SetDroppedWidth(UINT nWidth);
    INT  GetDroppedWidth() const;

    DWORD GetEditSel() const;
    BOOL  LimitText(INT nMaxChars);
    BOOL  SetEditSel(INT nStartChar, INT nEndChar);
    VOID SelectAll();
    VOID SelectNone();

    DWORD GetItemData(INT nIndex) const;
    INT   SetItemData(INT nIndex, DWORD dwItemData);
    LPVOID GetItemDataPtr(INT nIndex) const;
    INT    SetItemDataPtr(INT nIndex, LPVOID pData);

    INT  GetLBText(INT nIndex, LPTSTR lpszText) const;
    BOOL GetLBText(INT nIndex, MString& rString) const;
    MString GetLBText(INT nIndex) const;
    INT  GetLBTextLen(INT nIndex) const;

    INT GetItemHeight(INT nIndex) const;
    INT SetItemHeight(INT nIndex, UINT cyItemHeight);

    INT FindString(INT nStartAfter, LPCTSTR lpszString) const;
    INT FindStringExact(INT nIndexStart, LPCTSTR lpszFind) const;
    INT SelectString(INT nStartAfter, LPCTSTR lpszString);

    BOOL GetExtendedUI() const;
    INT  SetExtendedUI(BOOL bExtended = TRUE);

    VOID GetDroppedControlRect(LPRECT lprect) const;
    BOOL GetDroppedState() const;

    VOID ShowDropDown(BOOL bShowIt = TRUE);

    INT  AddString(LPCTSTR lpszString);
    INT  DeleteString(UINT nIndex);
    INT  InsertString(INT nIndex, LPCTSTR lpszString);
    VOID ResetContent();
    INT  Dir(UINT attr, LPCTSTR lpszWildCard);

    VOID Clear();
    VOID Copy();
    VOID Cut();
    VOID Paste();
};

////////////////////////////////////////////////////////////////////////////

inline MComboBox::MComboBox()
{
}

inline /*virtual*/ LPCTSTR MComboBox::GetWndClassNameDx() const
{
    return TEXT("COMBOBOX");
}

inline INT MComboBox::GetCount() const
{
    return (INT)SendMessageDx(CB_GETCOUNT);
}

inline INT MComboBox::GetCurSel() const
{
    return (INT)SendMessageDx(CB_GETCURSEL);
}

inline INT MComboBox::SetCurSel(INT nSelect)
{
    return (INT)SendMessageDx(CB_SETCURSEL, (WPARAM)nSelect);
}

inline LCID MComboBox::GetLocale() const
{
    return (LCID)SendMessageDx(CB_GETLOCALE);
}

inline LCID MComboBox::SetLocale(LCID nNewLocale)
{
    return (LCID)SendMessageDx(CB_SETLOCALE, nNewLocale);
}

inline INT MComboBox::GetTopIndex() const
{
    return (INT)SendMessageDx(CB_GETTOPINDEX);
}

inline INT MComboBox::SetTopIndex(INT nIndex)
{
    return (INT)SendMessageDx(CB_SETTOPINDEX, (WPARAM)nIndex);
}

inline INT MComboBox::InitStorage(INT nItems, UINT nBytes)
{
    return (INT)SendMessageDx(CB_INITSTORAGE, (WPARAM)nItems, (LPARAM)nBytes);
}

inline VOID MComboBox::SetHorizontalExtent(UINT nExtent)
{
    SendMessageDx(CB_SETHORIZONTALEXTENT, nExtent);
}

inline UINT MComboBox::GetHorizontalExtent() const
{
    return (UINT)SendMessageDx(CB_GETHORIZONTALEXTENT);
}

inline INT MComboBox::SetDroppedWidth(UINT nWidth)
{
    return (INT)SendMessageDx(CB_SETDROPPEDWIDTH, nWidth);
}

inline INT MComboBox::GetDroppedWidth() const
{
    return (INT)SendMessageDx(CB_GETDROPPEDWIDTH);
}

inline DWORD MComboBox::GetEditSel() const
{
    return (DWORD)SendMessageDx(CB_GETEDITSEL);
}

inline BOOL MComboBox::LimitText(INT nMaxChars)
{
    return (BOOL)SendMessageDx(CB_LIMITTEXT, (WPARAM)nMaxChars);
}

inline BOOL MComboBox::SetEditSel(INT nStartChar, INT nEndChar)
{
    return (BOOL)SendMessageDx(CB_SETEDITSEL, 0, 
                                MAKELPARAM(nStartChar, nEndChar));
}

inline VOID MComboBox::SelectAll()
{
    SetEditSel(0, -1);
}

inline VOID MComboBox::SelectNone()
{
    SetEditSel(-1, -1);
}

inline DWORD MComboBox::GetItemData(INT nIndex) const
{
    return (DWORD)SendMessageDx(CB_GETITEMDATA, (WPARAM)nIndex);
}

inline INT MComboBox::SetItemData(INT nIndex, DWORD dwItemData)
{
    return (INT)SendMessageDx(CB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)dwItemData);
}

inline LPVOID MComboBox::GetItemDataPtr(INT nIndex) const
{
    return (LPVOID)SendMessageDx(CB_GETITEMDATA, (WPARAM)nIndex);
}

inline INT MComboBox::SetItemDataPtr(INT nIndex, LPVOID pData)
{
    return (INT)SendMessageDx(CB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)pData);
}

inline INT MComboBox::GetLBText(INT nIndex, LPTSTR lpszText) const
{
    return (INT)SendMessageDx(CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)lpszText);
}

inline INT MComboBox::GetLBTextLen(INT nIndex) const
{
    return (INT)SendMessageDx(CB_GETLBTEXTLEN, (WPARAM)nIndex);
}

inline INT MComboBox::SetItemHeight(INT nIndex, UINT cyItemHeight)
{
    return (INT)SendMessageDx(CB_SETITEMHEIGHT, (WPARAM)nIndex, (LPARAM)cyItemHeight);
}

inline INT MComboBox::GetItemHeight(INT nIndex) const
{
    return (INT)SendMessageDx(CB_GETITEMHEIGHT, (WPARAM)nIndex);
}

inline INT MComboBox::FindStringExact(
    INT nIndexStart, LPCTSTR lpszFind) const
{
    return (INT)SendMessageDx(CB_FINDSTRINGEXACT, (WPARAM)nIndexStart, (LPARAM)lpszFind);
}

inline INT MComboBox::SetExtendedUI(BOOL bExtended/* = TRUE*/)
{
    return (INT)SendMessageDx(CB_SETEXTENDEDUI, (WPARAM)bExtended);
}

inline BOOL MComboBox::GetExtendedUI() const
{
    return (BOOL)SendMessageDx(CB_GETEXTENDEDUI);
}

inline VOID MComboBox::GetDroppedControlRect(LPRECT lprect) const
{
    SendMessageDx(CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)lprect);
}

inline BOOL MComboBox::GetDroppedState() const
{
    return (BOOL)SendMessageDx(CB_GETDROPPEDSTATE);
}

inline VOID MComboBox::ShowDropDown(BOOL bShowIt/* = TRUE*/)
{
    SendMessageDx(CB_SHOWDROPDOWN, (WPARAM)bShowIt);
}

inline INT MComboBox::AddString(LPCTSTR lpszString)
{
    return (INT)SendMessageDx(CB_ADDSTRING, 0, (LPARAM)lpszString);
}

inline INT MComboBox::DeleteString(UINT nIndex)
{
    return (INT)SendMessageDx(CB_DELETESTRING, (WPARAM)nIndex);
}

inline INT MComboBox::InsertString(INT nIndex, LPCTSTR lpszString)
{
    return (INT)SendMessageDx(CB_INSERTSTRING, (WPARAM)nIndex, (LPARAM)lpszString);
}

inline VOID MComboBox::ResetContent()
{
    SendMessageDx(CB_RESETCONTENT);
}

inline INT MComboBox::Dir(UINT attr, LPCTSTR lpszWildCard)
{
    return (INT)SendMessageDx(CB_DIR, (WPARAM)attr, (LPARAM)lpszWildCard);
}

inline INT MComboBox::FindString(INT nStartAfter, LPCTSTR lpszString) const
{
    return (INT)SendMessageDx(CB_FINDSTRING, (WPARAM)nStartAfter, (LPARAM)lpszString);
}

inline INT MComboBox::SelectString(INT nStartAfter, LPCTSTR lpszString)
{
    return (INT)SendMessageDx(CB_SELECTSTRING, (WPARAM)nStartAfter, (LPARAM)lpszString);
}

inline VOID MComboBox::Clear()
{
    SendMessageDx(WM_CLEAR);
}

inline VOID MComboBox::Copy()
{
    SendMessageDx(WM_COPY);
}

inline VOID MComboBox::Cut()
{
    SendMessageDx(WM_CUT);
}

inline VOID MComboBox::Paste()
{
    SendMessageDx(WM_PASTE);
}

inline BOOL MComboBox::GetLBText(INT nIndex, MString& rString) const
{
    rString.clear();

    INT cch = GetLBTextLen(nIndex);
    if (cch == CB_ERR)
        return FALSE;

    rString.resize(cch);
    cch = GetLBText(nIndex, &rString[0]);
    if (cch == CB_ERR)
        rString.clear();
    return cch != CB_ERR;
}

inline MString MComboBox::GetLBText(INT nIndex) const
{
    MString rString;
    GetLBText(nIndex, rString);
    return rString;
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MCOMBOBOX_HPP_
