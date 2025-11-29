
#include <windows.h>
#include "MSplitterWnd.hpp"

#define LESS_FLICKER

MSplitterWnd::MSplitterWnd() : m_iDraggingBorder(-1), m_nPaneCount(0)
{
	m_vecPanes.resize(1);
}

BOOL MSplitterWnd::CreateDx(HWND hwndParent, INT nPaneCount, DWORD dwStyle, DWORD dwExStyle)
{
	RECT rc;
	GetClientRect(hwndParent, &rc);

	if (!CreateWindowDx(hwndParent, NULL, dwStyle, dwExStyle,
						rc.left, rc.top,
						rc.right - rc.left, rc.bottom - rc.top))
	{
		return FALSE;
	}

	SetPaneCount(nPaneCount);
	PostMessageDx(WM_SIZE);
	return TRUE;
}

BOOL MSplitterWnd::IsHorizontal() const
{
	return !IsVertical();
}

BOOL MSplitterWnd::IsVertical() const
{
	return (GetStyleDx() & SWS_VERT) == SWS_VERT;
}

BOOL MSplitterWnd::IsRightBottomAlign() const
{
	return (GetStyleDx() & SWS_RIGHTALIGN) == SWS_RIGHTALIGN;
}

INT MSplitterWnd::GetPaneCount() const
{
	return m_nPaneCount;
}

VOID MSplitterWnd::SetPaneCount(INT nCount)
{
	m_vecPanes.resize(nCount + 1);
	m_nPaneCount = nCount;
	SplitEqually();
}

HWND MSplitterWnd::GetPane(INT nIndex) const
{
	assert(0 <= nIndex && nIndex < m_nPaneCount);
	return m_vecPanes[nIndex].hwndPane;
}

VOID MSplitterWnd::SetPane(INT nIndex, HWND hwndPane)
{
	if (m_nPaneCount == 0)
		return;

	assert(0 <= nIndex && nIndex < m_nPaneCount);
	m_vecPanes[nIndex].hwndPane = hwndPane;
}

INT MSplitterWnd::GetPanePos(INT nIndex) const
{
	assert(0 <= nIndex && nIndex <= m_nPaneCount);
	return m_vecPanes[nIndex].xyPos;
}

VOID MSplitterWnd::SetPanePos(INT nIndex, INT nPos, BOOL bBounded)
{
	if (m_nPaneCount == 0)
		return;

	assert(0 <= nIndex && nIndex <= m_nPaneCount);
	if (nIndex == 0)
		return;

	if (bBounded)
	{
		if (nIndex < m_nPaneCount)
		{
			const PANEINFO& info = m_vecPanes[nIndex];
			const PANEINFO& next_info = m_vecPanes[nIndex + 1];
			if (next_info.xyPos < nPos + info.cxyMin)
				nPos = next_info.xyPos - info.cxyMin;
		}

		const PANEINFO& prev_info = m_vecPanes[nIndex - 1];
		if (nPos < prev_info.xyPos + prev_info.cxyMin)
			nPos = prev_info.xyPos + prev_info.cxyMin;
	}

	m_vecPanes[nIndex].xyPos = nPos;
}

INT MSplitterWnd::GetPaneExtent(INT nIndex) const
{
	assert(0 <= nIndex && nIndex < m_nPaneCount);
	return m_vecPanes[nIndex + 1].xyPos - m_vecPanes[nIndex].xyPos;
}

VOID MSplitterWnd::SetPaneExtent(INT nIndex, INT cxy, BOOL bUpdate)
{
	if (m_nPaneCount == 0)
		return;

	assert(0 <= nIndex && nIndex < m_nPaneCount);
	if (nIndex == m_nPaneCount - 1)
	{
		SetPanePos(nIndex, m_vecPanes[m_nPaneCount].xyPos - cxy);
	}
	else
	{
		SetPanePos(nIndex + 1, m_vecPanes[nIndex].xyPos + cxy);
	}
	UpdatePanes();
}

VOID MSplitterWnd::SetPaneMinExtent(INT nIndex, INT cxyMin)
{
	if (m_nPaneCount == 0)
		return;

	assert(0 <= nIndex && nIndex < m_nPaneCount);
	m_vecPanes[nIndex].cxyMin = cxyMin;
}

INT MSplitterWnd::GetTotalMinExtent() const
{
	INT cxy = 0;
	for (INT i = 0; i < m_nPaneCount; ++i)
	{
		cxy += m_vecPanes[i].cxyMin;
	}
	return cxy;
}

VOID MSplitterWnd::GetPaneRect(INT nIndex, RECT *prc) const
{
	assert(0 <= nIndex && nIndex < m_nPaneCount);
	GetClientRect(m_hwnd, prc);
	if (IsVertical())
	{
		prc->top = m_vecPanes[nIndex].xyPos;
		prc->bottom = m_vecPanes[nIndex + 1].xyPos;
		if (nIndex < m_nPaneCount - 1)
			prc->bottom -= m_cxyBorder;
	}
	else
	{
		prc->left = m_vecPanes[nIndex].xyPos;
		prc->right = m_vecPanes[nIndex + 1].xyPos;
		if (nIndex < m_nPaneCount - 1)
			prc->right -= m_cxyBorder;
	}
}

INT MSplitterWnd::HitTestBorder(POINT ptClient) const
{
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	if (!::PtInRect(&rcClient, ptClient))
		return -1;

	INT xy = (IsVertical() ? ptClient.y : ptClient.x);
	for (INT i = 1; i < m_nPaneCount; ++i)
	{
		INT xyPos = m_vecPanes[i].xyPos;
		if (xyPos - m_cxyBorder <= xy && xy <= xyPos)
		{
			return i;
		}
	}
	return -1;
}

void MSplitterWnd::SplitEqually()
{
	if (m_nPaneCount == 0)
		return;

	RECT rc;
	GetClientRect(m_hwnd, &rc);

	INT cxy = (IsVertical() ? rc.bottom : rc.right);
	INT xy = 0, cxyPane = cxy / m_nPaneCount;
	for (INT i = 0; i < m_nPaneCount; ++i)
	{
		m_vecPanes[i].xyPos = xy;
		xy += cxyPane;
	}
	m_vecPanes[m_nPaneCount].xyPos = cxy;
	PostMessageDx(WM_SIZE);
}

void MSplitterWnd::UpdatePanes()
{
	RECT rc;
	HDWP hDWP = BeginDeferWindowPos(m_nPaneCount);
	for (INT i = 0; i < m_nPaneCount; ++i)
	{
		const PANEINFO *pInfo = &m_vecPanes[i];
		HWND hwndPane = pInfo->hwndPane;
		if (hwndPane == NULL)
			continue;

		GetPaneRect(i, &rc);
		hDWP = DeferWindowPos(hDWP, hwndPane, NULL,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}
	EndDeferWindowPos(hDWP);

	UINT nID = GetDlgCtrlID(m_hwnd);
	NMHDR notify = { m_hwnd, nID, NOTIFY_CHANGED };
	FORWARD_WM_NOTIFY(GetParent(m_hwnd), nID, &notify, SendMessage);
}

LRESULT CALLBACK
MSplitterWnd::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwnd, WM_SIZE, OnSize);
	HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
	HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
	HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
	HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
	HANDLE_MSG(hwnd, WM_SETCURSOR, OnSetCursor);
	HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
	HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
	HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
	HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE, OnSysColorChange);
	case WM_CAPTURECHANGED:
		m_iDraggingBorder = -1;
		return 0;
	default:
		return DefaultProcDx();
	}
}

void MSplitterWnd::OnSysColorChange(HWND hwnd)
{
	for (size_t i = 0; i < m_vecPanes.size(); ++i)
	{
		SendMessage(m_vecPanes[i].hwndPane, WM_SYSCOLORCHANGE, 0, 0);
	}
}

LPCTSTR MSplitterWnd::GetWndClassNameDx() const
{
	return TEXT("MZC4 MSplitterWnd Class");
}

VOID MSplitterWnd::ModifyWndClassDx(WNDCLASSEX& wcx)
{
}

void MSplitterWnd::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	FORWARD_WM_COMMAND(GetParent(hwnd), id, hwndCtl, codeNotify, PostMessage);
}

LRESULT MSplitterWnd::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
	return FORWARD_WM_NOTIFY(GetParent(hwnd), idFrom, pnmhdr, SendMessage);
}

void MSplitterWnd::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
	FORWARD_WM_CONTEXTMENU(GetParent(hwnd), hwndContext, xPos, yPos, SendMessage);
}

HCURSOR& MSplitterWnd::CursorNS()
{
	static HCURSOR s_hcurNS = ::LoadCursor(NULL, IDC_SIZENS);
	return s_hcurNS;
}

HCURSOR& MSplitterWnd::CursorWE()
{
	static HCURSOR s_hcurWE = ::LoadCursor(NULL, IDC_SIZEWE);
	return s_hcurWE;
}

void MSplitterWnd::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	if (fDoubleClick)
		return;

	POINT pt = { x, y };
	INT iBorder = HitTestBorder(pt);
	if (iBorder < 0)
		return;

	SetCapture(hwnd);
	m_iDraggingBorder = iBorder;

	if (IsVertical())
		SetCursor(CursorNS());
	else
		SetCursor(CursorWE());

#ifdef LESS_FLICKER
	DrawSplitter();
#endif
}

void MSplitterWnd::OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	if (m_iDraggingBorder == -1)
		return;

#ifdef LESS_FLICKER
	DrawSplitter();
#endif

	SetPanePos(m_iDraggingBorder, (IsVertical() ? y : x) + m_cxyBorder / 2);
	UpdatePanes();

	m_iDraggingBorder = -1;
	ReleaseCapture();
}

void MSplitterWnd::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	if (m_iDraggingBorder == -1)
		return;

#ifdef LESS_FLICKER
	DrawSplitter();
#endif

	SetPanePos(m_iDraggingBorder, (IsVertical() ? y : x) + m_cxyBorder / 2);

#ifdef LESS_FLICKER
	DrawSplitter();
#else
	UpdatePanes();
#endif
}

typedef struct tagBITMAPINFO1BPP
{
   BITMAPINFOHEADER bmiHeader;
   RGBQUAD          bmiColors[2];
} BITMAPINFO1BPP, *PBITMAPINFO1BPP;

static HBRUSH CreateHalftoneBrush(HDC hDC)
{
	BYTE bits[] = { 0x55, 0, 0, 0, 0xAA, 0, 0, 0 };

	BITMAPINFO1BPP bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 8;
	bmi.bmiHeader.biHeight = -2; // top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiColors[0].rgbBlue = 0;
	bmi.bmiColors[0].rgbGreen = 0;
	bmi.bmiColors[0].rgbRed = 0;
	bmi.bmiColors[0].rgbReserved = 0;
	bmi.bmiColors[1].rgbBlue = 0xFF;
	bmi.bmiColors[1].rgbGreen = 0xFF;
	bmi.bmiColors[1].rgbRed = 0xFF;
	bmi.bmiColors[1].rgbReserved = 0;
	HBITMAP hBitmap = CreateDIBitmap(hDC, &bmi.bmiHeader, CBM_INIT, bits,
									 (BITMAPINFO *)&bmi, DIB_RGB_COLORS);
	if (!hBitmap)
		return NULL;

	HBRUSH hHalftoneBrush = CreatePatternBrush(hBitmap);
	DeleteObject(hBitmap);
	return hHalftoneBrush;
}

void MSplitterWnd::DrawSplitter()
{
	if (m_iDraggingBorder == -1)
		return;

	PANEINFO& info = m_vecPanes[m_iDraggingBorder];

	HDC hDC = ::GetDCEx(info.hwndPane, NULL, DCX_CACHE | DCX_LOCKWINDOWUPDATE | DCX_PARENTCLIP);

	RECT rcClient;
	::GetClientRect(info.hwndPane, &rcClient);

	HBRUSH hbr = CreateHalftoneBrush(hDC);
	HGDIOBJ hbrOld = ::SelectObject(hDC, hbr);
	if (IsVertical())
	{
		RECT rc = { 0, info.xyPos - m_cxyBorder, 0, info.xyPos };
		MapWindowRect(::GetParent(info.hwndPane), info.hwndPane, &rc);
		rc.left = rcClient.left;
		rc.right = rcClient.right;
		::PatBlt(hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATINVERT);
	}
	else
	{
		RECT rc = { info.xyPos - m_cxyBorder, 0, info.xyPos, 0 };
		MapWindowRect(::GetParent(info.hwndPane), info.hwndPane, &rc);
		rc.top = rcClient.top;
		rc.bottom = rcClient.bottom;
		::PatBlt(hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATINVERT);
	}
	::SelectObject(hDC, hbrOld);
	::DeleteObject(hbr);

	::ReleaseDC(info.hwndPane, hDC);
}

void MSplitterWnd::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	if (m_nPaneCount == 0)
		return;

	RECT rc;
	GetClientRect(hwnd, &rc);
	INT cxy = (IsVertical() ? rc.bottom : rc.right);
	Resize(cxy);
}

void MSplitterWnd::Resize(INT cxy)
{
	if (IsRightBottomAlign())
	{
		INT dxy = cxy - m_vecPanes[m_nPaneCount].xyPos;
		for (INT i = 1; i < m_nPaneCount; ++i)
		{
			SetPanePos(i, m_vecPanes[i].xyPos + dxy, FALSE);
		}
	}

	SetPanePos(m_nPaneCount, cxy, FALSE);
	UpdatePanes();
}

BOOL MSplitterWnd::OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hwnd, &pt);

	if (HitTestBorder(pt) == -1)
	{
		SetCursor(::LoadCursor(NULL, IDC_ARROW));
		return TRUE;
	}

	if (IsVertical())
		SetCursor(CursorNS());
	else
		SetCursor(CursorWE());
	return TRUE;
}
