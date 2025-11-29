// MTabCtrl.hpp -- Win32API tab control wrapper                 -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MTABCTRL_HPP_
#define MZC4_MTABCTRL_HPP_      4   /* Version 4 */

class MTabCtrl;

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"

class MTabCtrl : public MWindowBase
{
public:
	MTabCtrl();
	virtual LPCTSTR GetWndClassNameDx() const;

	HIMAGELIST GetImageList() const;
	HIMAGELIST SetImageList(HIMAGELIST hImageList);
	INT GetItemCount() const;

	BOOL GetItem(INT iItem, TCITEM* pTabCtrlItem) const;
	BOOL SetItem(INT iItem, TCITEM* pTabCtrlItem);

	BOOL SetItemExtra(INT nBytes);
	BOOL GetItemRect(INT iItem, LPRECT prc) const;

	INT GetCurSel() const;
	INT SetCurSel(INT iItem);

	SIZE SetItemSize(SIZE size);
	VOID SetPadding(SIZE size);
	INT GetRowCount() const;

	HWND GetToolTips() const;
	VOID SetToolTips(HWND hwndToolTips);

	INT GetCurFocus() const;
	VOID SetCurFocus(INT iItem);

	#if (_WIN32_IE >= 0x0300)
		INT SetMinTabWidth(INT cx);
	#endif  // (_WIN32_IE >= 0x0300)

	#if (_WIN32_IE >= 0x0400)
		DWORD GetExtendedStyle();
		DWORD SetExtendedStyle(DWORD dwNewStyle, DWORD dwExMask = 0);
		BOOL HighlightItem(INT nIndex, BOOL fHighlight = TRUE);
	#endif  // (_WIN32_IE >= 0x0400)

	INT InsertItem(INT iItem, TCITEM* pTabCtrlItem);
	INT InsertItem(INT iItem, LPCTSTR lpszItem);
	INT InsertItem(INT iItem, LPCTSTR lpszItem, INT iImage);
	INT InsertItem(UINT nMask, INT iItem, LPCTSTR lpszItem,
				   INT iImage, LPARAM lParam);
	BOOL DeleteItem(INT iItem);
	BOOL DeleteAllItems();
	VOID AdjustRect(BOOL bLarger, LPRECT prc);
	VOID RemoveImage(INT iImage);
	INT HitTest(TCHITTESTINFO* pHitTestInfo) const;

	#if (_WIN32_IE >= 0x0300)
		VOID DeselectAll(BOOL fExcludeFocus);
	#endif  // (_WIN32_IE >= 0x0300)
};

////////////////////////////////////////////////////////////////////////////

inline MTabCtrl::MTabCtrl()
{
}

inline /*virtual*/ LPCTSTR MTabCtrl::GetWndClassNameDx() const
{
	return WC_TABCONTROL;
}

inline HIMAGELIST MTabCtrl::GetImageList() const
{
	return (HIMAGELIST)SendMessageDx(TCM_GETIMAGELIST);
}

inline HIMAGELIST MTabCtrl::SetImageList(HIMAGELIST hImageList)
{
	return (HIMAGELIST)SendMessageDx(TCM_SETIMAGELIST, 0, (LPARAM)hImageList);
}

inline INT MTabCtrl::GetItemCount() const
{
	return (INT)SendMessageDx(TCM_GETITEMCOUNT);
}

inline BOOL MTabCtrl::GetItem(INT iItem, TCITEM* pTabCtrlItem) const
{
	return (BOOL)SendMessageDx(TCM_GETITEM, (WPARAM)iItem, (LPARAM)pTabCtrlItem);
}

inline BOOL MTabCtrl::SetItem(INT iItem, TCITEM* pTabCtrlItem)
{
	return (BOOL)SendMessageDx(TCM_SETITEM, (WPARAM)iItem, (LPARAM)pTabCtrlItem);
}

inline BOOL MTabCtrl::SetItemExtra(INT nBytes)
{
	assert(GetItemCount() == 0);
	return (BOOL)SendMessageDx(TCM_SETITEMEXTRA, (WPARAM)nBytes);
}

inline BOOL MTabCtrl::GetItemRect(INT iItem, LPRECT prc) const
{
	return (BOOL)SendMessageDx(TCM_GETITEMRECT, (WPARAM)iItem, (LPARAM)prc);
}

inline INT MTabCtrl::GetCurSel() const
{
	return (INT)SendMessageDx(TCM_GETCURSEL);
}

inline INT MTabCtrl::SetCurSel(INT iItem)
{
	return (INT)SendMessageDx(TCM_SETCURSEL, (WPARAM)iItem);
}

inline SIZE MTabCtrl::SetItemSize(SIZE size)
{
	DWORD dwSize = (DWORD)SendMessageDx(TCM_SETITEMSIZE, 0, MAKELPARAM(size.cx, size.cy));
	SIZE ret = { GET_X_LPARAM(dwSize), GET_Y_LPARAM(dwSize) };
	return ret;
}

inline VOID MTabCtrl::SetPadding(SIZE size)
{
	SendMessageDx(TCM_SETPADDING, 0, MAKELPARAM(size.cx, size.cy));
}

inline INT MTabCtrl::GetRowCount() const
{
	return (INT)SendMessageDx(TCM_GETROWCOUNT);
}

inline HWND MTabCtrl::GetToolTips() const
{
	return (HWND)SendMessageDx(TCM_GETTOOLTIPS);
}

inline VOID MTabCtrl::SetToolTips(HWND hwndToolTips)
{
	SendMessageDx(TCM_SETTOOLTIPS, (WPARAM)hwndToolTips);
}

inline INT MTabCtrl::GetCurFocus() const
{
	return (INT)SendMessageDx(TCM_GETCURFOCUS);
}

inline VOID MTabCtrl::SetCurFocus(INT iItem)
{
	SendMessageDx(TCM_SETCURFOCUS, (WPARAM)iItem);
}

#if (_WIN32_IE >= 0x0300)
	inline INT MTabCtrl::SetMinTabWidth(INT cx)
	{
		return (INT)SendMessageDx(TCM_SETMINTABWIDTH, 0, cx);
	}
#endif  // (_WIN32_IE >= 0x0300)

#if (_WIN32_IE >= 0x0400)
	inline DWORD MTabCtrl::GetExtendedStyle()
	{
		return (DWORD)SendMessageDx(TCM_GETEXTENDEDSTYLE);
	}

	inline DWORD MTabCtrl::SetExtendedStyle(
		DWORD dwNewStyle, DWORD dwExMask/* = 0*/)
	{
		return (DWORD)SendMessageDx(TCM_SETEXTENDEDSTYLE, (WPARAM)dwExMask, (LPARAM)dwNewStyle);
	}

	inline BOOL MTabCtrl::HighlightItem(
		INT nIndex, BOOL fHighlight/* = TRUE*/)
	{
		return (BOOL)SendMessageDx(TCM_HIGHLIGHTITEM, (WPARAM)nIndex, MAKELPARAM(fHighlight, 0));
	}
#endif  // (_WIN32_IE >= 0x0400)

inline INT MTabCtrl::InsertItem(INT iItem, TCITEM* pTabCtrlItem)
{
	return (INT)SendMessageDx(TCM_INSERTITEM, (WPARAM)iItem, (LPARAM)pTabCtrlItem);
}

inline INT MTabCtrl::InsertItem(INT iItem, LPCTSTR lpszItem)
{
	return InsertItem(TCIF_TEXT, iItem, lpszItem, 0, 0);
}

inline INT MTabCtrl::InsertItem(INT iItem, LPCTSTR lpszItem, INT iImage)
{
	return InsertItem(TCIF_TEXT | TCIF_IMAGE, iItem, lpszItem, iImage, 0);
}

inline INT MTabCtrl::InsertItem(UINT nMask, INT iItem,
	LPCTSTR lpszItem, INT iImage, LPARAM lParam)
{
	TCITEM tci;
	ZeroMemory(&tci, sizeof(tci));
	tci.mask = nMask;
	tci.pszText = (LPTSTR) lpszItem;
	tci.iImage = iImage;
	tci.lParam = lParam;
	return (INT)SendMessageDx(TCM_INSERTITEM, (WPARAM)iItem, (LPARAM)&tci);
}

inline BOOL MTabCtrl::DeleteItem(INT iItem)
{
	return (BOOL)SendMessageDx(TCM_DELETEITEM, (WPARAM)iItem);
}

inline BOOL MTabCtrl::DeleteAllItems()
{
	return (BOOL)SendMessageDx(TCM_DELETEALLITEMS);
}

inline VOID MTabCtrl::AdjustRect(BOOL bLarger, LPRECT prc)
{
	SendMessageDx(TCM_ADJUSTRECT, (WPARAM)bLarger, (LPARAM)prc);
}

inline VOID MTabCtrl::RemoveImage(INT iImage)
{
	SendMessageDx(TCM_REMOVEIMAGE, (WPARAM)iImage);
}

inline INT MTabCtrl::HitTest(TCHITTESTINFO* pHitTestInfo) const
{
	return (INT)SendMessageDx(TCM_HITTEST, 0, (LPARAM)pHitTestInfo);
}

#if (_WIN32_IE >= 0x0300)
	inline VOID MTabCtrl::DeselectAll(BOOL fExcludeFocus)
	{
		SendMessageDx(TCM_DESELECTALL, (WPARAM)fExcludeFocus);
	}
#endif  // (_WIN32_IE >= 0x0300)

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTABCTRL_HPP_
