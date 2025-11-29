// MSplitterWnd.hpp --- MZC4 splitter window                    -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MSPLITTERWND_HPP_
#define MZC4_MSPLITTERWND_HPP_      100 /* Version 100 */

class MSplitterWnd;

//////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include <vector>

// The styles of MSplitterWnd
#define SWS_HORZ            0
#define SWS_VERT            1
#define SWS_LEFTALIGN       0
#define SWS_TOPALIGN        0
#define SWS_RIGHTALIGN      2
#define SWS_BOTTOMALIGN     2

class MSplitterWnd : public MWindowBase
{
public:
	enum { m_cxyBorder = 4, m_cxyMin = 8 };
	enum { NOTIFY_CHANGED = 0x2934 };

	MSplitterWnd();

	BOOL CreateDx(HWND hwndParent, INT nPaneCount = 2,
				  DWORD dwStyle = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_LEFTALIGN,
				  DWORD dwExStyle = 0);
	BOOL IsHorizontal() const;
	BOOL IsVertical() const;
	BOOL IsRightBottomAlign() const;

	INT GetPaneCount() const;
	VOID SetPaneCount(INT nCount);

	HWND GetPane(INT nIndex) const;
	VOID SetPane(INT nIndex, HWND hwndPane);

	INT GetPanePos(INT nIndex) const;
	VOID SetPanePos(INT nIndex, INT nPos, BOOL bBounded = TRUE);

	INT GetPaneExtent(INT nIndex) const;
	VOID SetPaneExtent(INT nIndex, INT cxy, BOOL bUpdate = TRUE);

	VOID SetPaneMinExtent(INT nIndex, INT cxyMin = MSplitterWnd::m_cxyMin);

	INT GetTotalMinExtent() const;

	VOID GetPaneRect(INT nIndex, RECT *prc) const;

	INT HitTestBorder(POINT ptClient) const;

	void SplitEqually();

	void UpdatePanes();

	virtual LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnSysColorChange(HWND hwnd);

	virtual LPCTSTR GetWndClassNameDx() const;
	virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx);

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);

	static HCURSOR& CursorNS();
	static HCURSOR& CursorWE();

protected:
	struct PANEINFO
	{
		HWND    hwndPane;
		INT     xyPos;
		INT     cxyMin;

		PANEINFO()
		{
			hwndPane = NULL;
			xyPos = 0;
			cxyMin = m_cxyMin;
		}
	};
	INT                     m_iDraggingBorder;
	INT                     m_nPaneCount;
	std::vector<PANEINFO>   m_vecPanes;

	void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
	void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
	void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
	void DrawSplitter();
	void OnSize(HWND hwnd, UINT state, int cx, int cy);
	void Resize(INT cxy);
	BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg);
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MSPLITTERWND_HPP_
