// MPropSheet.hpp -- Win32API property sheet wrapper             -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MPROPSHEET_HPP_
#define MZC4_MPROPSHEET_HPP_     1   /* Version 1 */

class MPropSheetPage;
class MPropSheet;

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"

#ifndef _INC_COMMCTRL
	#include <commctrl.h>   // common controls (PROPSHEETPAGE, PROPSHEETHEADER, ...)
#endif

#include <vector>           // std::vector

////////////////////////////////////////////////////////////////////////////
// MPropSheetPage --- a single page of a property sheet.
//
// Derive from MPropSheetPage the same way you would derive from
// MDialogBase, and override the On... virtuals you need.  A page is
// bound to a dialog template (IDD_...) just like an ordinary dialog.

class MPropSheetPage : public MDialogBase
{
public:
	PROPSHEETPAGE   m_psp;
	HPROPSHEETPAGE  m_hPage;
	MString m_title;

	MPropSheetPage(INT nDialogID, LPCTSTR pszTitle = NULL, UINT nIconID = 0)
		: MDialogBase(nDialogID), m_hPage(NULL)
	{
		ZeroMemory(&m_psp, sizeof(m_psp));
		m_psp.dwSize = sizeof(m_psp);
		m_psp.dwFlags = 0;
		m_psp.hInstance = ::GetModuleHandle(NULL);
		m_psp.pszTemplate = MAKEINTRESOURCE(nDialogID);
		m_psp.pfnDlgProc = MPropSheetPage::DialogProc;
		m_psp.lParam = (LPARAM)this;

		if (pszTitle)
		{
			m_title = pszTitle;
			m_psp.pszTitle = m_title.c_str();
			m_psp.dwFlags |= PSP_USETITLE;
		}
		if (nIconID)
		{
			m_psp.pszIcon = MAKEINTRESOURCE(nIconID);
			m_psp.dwFlags |= PSP_USEICONID;
		}
	}

	virtual ~MPropSheetPage()
	{
	}

	virtual LPCTSTR GetWndClassNameDx() const override
	{
		return TEXT("#32770");
	}

	LPPROPSHEETPAGE GetPropSheetPageDx()
	{
		return &m_psp;
	}

	// Turns m_psp into an HPROPSHEETPAGE. Call this before adding the
	// page to a property sheet (MPropSheet::AddPage does this for you).
	HPROPSHEETPAGE CreatePropSheetPageDx()
	{
		if (!m_hPage)
			m_hPage = ::CreatePropertySheetPage(&m_psp);
		return m_hPage;
	}

	static MPropSheetPage *GetUserData(HWND hwnd)
	{
		return reinterpret_cast<MPropSheetPage *>(::GetWindowLongPtr(hwnd, DWLP_USER));
	}

	// The HWND of the property sheet frame this page currently lives in.
	HWND GetSheetDx() const
	{
		return ::GetParent(m_hwnd);
	}

	////////////////////////////////////////////////////////////////////
	// PropSheet_* wrappers that act on the owning sheet

	VOID SetModifiedDx(BOOL bChanged = TRUE)
	{
		if (bChanged)
			PropSheet_Changed(GetSheetDx(), m_hwnd);
		else
			PropSheet_UnChanged(GetSheetDx(), m_hwnd);
	}
	VOID CancelToCloseDx()
	{
		PropSheet_CancelToClose(GetSheetDx());
	}
	VOID PressButtonDx(INT nButton)
	{
		PropSheet_PressButton(GetSheetDx(), nButton);
	}
	LRESULT QuerySiblingsDx(WPARAM wParam, LPARAM lParam)
	{
		return PropSheet_QuerySiblings(GetSheetDx(), wParam, lParam);
	}
	HWND GetTabControlDx() const
	{
		return PropSheet_GetTabControl(GetSheetDx());
	}
	VOID SetWizButtonsDx(DWORD dwFlags)
	{
		PropSheet_SetWizButtons(GetSheetDx(), dwFlags);
	}
	BOOL SetHeaderTitleDx(LPCTSTR pszText)
	{
		return (BOOL)PropSheet_SetHeaderTitle(GetSheetDx(), GetIndexDx(), pszText);
	}
	BOOL SetHeaderSubTitleDx(LPCTSTR pszText)
	{
		return (BOOL)PropSheet_SetHeaderSubTitle(GetSheetDx(), GetIndexDx(), pszText);
	}
	INT GetIndexDx() const
	{
		return (INT)::SendMessage(GetSheetDx(), PSM_INDEXTOPAGE, 0, (LPARAM)m_hPage);
	}

	LONG_PTR SetResultDx(LONG_PTR nResult)
	{
		::SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, nResult);
		return nResult;
	}

	////////////////////////////////////////////////////////////////////
	// message / notification handlers meant to be overridden

	virtual BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		return TRUE;
	}
	virtual VOID OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
	}
	virtual VOID OnDestroy(HWND hwnd)
	{
	}

	// Return TRUE to allow activation (PSN_SETACTIVE)
	virtual BOOL OnSetActive(HWND hwnd)
	{
		return TRUE;
	}
	// Return TRUE to prevent losing activation (PSN_KILLACTIVE)
	virtual BOOL OnKillActive(HWND hwnd)
	{
		return FALSE;
	}
	// Return TRUE if the changes were applied successfully (PSN_APPLY)
	virtual BOOL OnApply(HWND hwnd, BOOL bAllPages)
	{
		return TRUE;
	}
	virtual VOID OnReset(HWND hwnd, BOOL bAllPages)
	{
	}
	virtual VOID OnHelpDx(HWND hwnd)
	{
	}
	// PSN_WIZBACK/PSN_WIZNEXT: return 0 to go on, -1 to prevent the
	// change, or the resource/dialog ID of the page to jump to.
	virtual INT_PTR OnWizBack(HWND hwnd)
	{
		return 0;
	}
	virtual INT_PTR OnWizNext(HWND hwnd)
	{
		return 0;
	}
	// Return TRUE to allow the wizard to finish (PSN_WIZFINISH)
	virtual BOOL OnWizFinish(HWND hwnd)
	{
		return TRUE;
	}

	virtual LRESULT OnNotify(HWND hwnd, INT idFrom, LPNMHDR pnmhdr)
	{
		switch (pnmhdr->code)
		{
		case PSN_SETACTIVE:
			return SetResultDx(OnSetActive(hwnd) ? 0 : -1);

		case PSN_KILLACTIVE:
			return SetResultDx(OnKillActive(hwnd));

		case PSN_APPLY:
			{
				LPPSHNOTIFY pshn = (LPPSHNOTIFY)pnmhdr;
				BOOL bAllPages = (pshn->lParam != 0);
				BOOL bOK = OnApply(hwnd, bAllPages);
				return SetResultDx(bOK ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE);
			}

		case PSN_RESET:
			{
				LPPSHNOTIFY pshn = (LPPSHNOTIFY)pnmhdr;
				OnReset(hwnd, pshn->lParam != 0);
			}
			return 0;

		case PSN_HELP:
			OnHelpDx(hwnd);
			return 0;

		case PSN_WIZBACK:
			return SetResultDx(OnWizBack(hwnd));

		case PSN_WIZNEXT:
			return SetResultDx(OnWizNext(hwnd));

		case PSN_WIZFINISH:
			return SetResultDx(OnWizFinish(hwnd) ? 0 : -1);

		default:
			return 0;
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			DO_MSG(WM_INITDIALOG, OnInitDialog);
			DO_MSG(WM_COMMAND, OnCommand);
			DO_MSG(WM_DESTROY, OnDestroy);

		case WM_NOTIFY:
			return OnNotify(hwnd, (INT)wParam, (LPNMHDR)lParam);
		}
		return 0;
	}

	static INT_PTR CALLBACK
	DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

////////////////////////////////////////////////////////////////////////////
// MPropSheet --- the property sheet (or wizard) frame itself.
//
// Usage:
//     MPropSheet sheet(hwndParent, TEXT("Options"));
//     MyPage1 page1;
//     MyPage2 page2;
//     sheet.AddPage(page1);
//     sheet.AddPage(page2);
//     sheet.DoSheetDx();          // modal
//   or
//     sheet.SetModelessDx();
//     sheet.DoSheetDx();          // modeless; returns the HWND cast to INT_PTR

class MPropSheet : public MWindowBase
{
public:
	PROPSHEETHEADER               m_psh;
	std::vector<HPROPSHEETPAGE>   m_pages;
	MStringW m_title;

	MPropSheet(HWND hwndParent = NULL, LPCTSTR pszCaption = NULL, UINT nIconID = 0)
	{
		if (pszCaption)
			m_title = pszCaption;

		ZeroMemory(&m_psh, sizeof(m_psh));
		m_psh.dwSize = sizeof(m_psh);
		m_psh.dwFlags = PSH_USECALLBACK;
		m_psh.hwndParent = hwndParent;
		m_psh.hInstance = ::GetModuleHandle(NULL);
		m_psh.pszCaption = m_title.c_str();
		m_psh.nStartPage = 0;
		m_psh.pfnCallback = MPropSheet::PropSheetProcDx;

		if (nIconID)
		{
			m_psh.pszIcon = MAKEINTRESOURCE(nIconID);
			m_psh.dwFlags |= PSH_USEICONID;
		}
	}

	virtual ~MPropSheet()
	{
	}

	virtual LPCTSTR GetWndClassNameDx() const override
	{
		return TEXT("#32770");
	}

	////////////////////////////////////////////////////////////////////
	// page management

	// Creates the HPROPSHEETPAGE from the page (if not created yet) and
	// appends it to the sheet. Must be called before DoSheetDx().
	BOOL AddPage(MPropSheetPage& page)
	{
		HPROPSHEETPAGE hPage = page.CreatePropSheetPageDx();
		if (!hPage)
			return FALSE;
		m_pages.push_back(hPage);
		return TRUE;
	}

	BOOL AddPage(HPROPSHEETPAGE hPage)
	{
		if (!hPage)
			return FALSE;
		m_pages.push_back(hPage);
		return TRUE;
	}

	// Only valid before the sheet has been created.
	VOID RemoveAllPagesDx()
	{
		for (size_t i = 0; i < m_pages.size(); ++i)
		{
			::DestroyPropertySheetPage(m_pages[i]);
		}
		m_pages.clear();
	}

	// Adds/removes a page while the sheet is already running.
	BOOL InsertPageDx(MPropSheetPage& page, INT nIndex = -1)
	{
		HPROPSHEETPAGE hPage = page.CreatePropSheetPageDx();
		if (!hPage)
			return FALSE;
		return (BOOL)PropSheet_AddPage(m_hwnd, hPage);
	}
	VOID RemovePageDx(INT nIndex)
	{
		PropSheet_RemovePage(m_hwnd, nIndex, NULL);
	}

	////////////////////////////////////////////////////////////////////
	// mode flags (set these before calling DoSheetDx)

	VOID SetWizardModeDx(BOOL bWizard = TRUE, BOOL bWizard97 = FALSE)
	{
		m_psh.dwFlags &= ~(PSH_WIZARD | PSH_WIZARD97);
		if (bWizard)
			m_psh.dwFlags |= (bWizard97 ? PSH_WIZARD97 : PSH_WIZARD);
	}
	VOID SetModelessDx(BOOL bModeless = TRUE)
	{
		if (bModeless)
			m_psh.dwFlags |= PSH_MODELESS;
		else
			m_psh.dwFlags &= ~PSH_MODELESS;
	}
	VOID SetNoApplyNowDx(BOOL bNoApplyNow = TRUE)
	{
		if (bNoApplyNow)
			m_psh.dwFlags |= PSH_NOAPPLYNOW;
		else
			m_psh.dwFlags &= ~PSH_NOAPPLYNOW;
	}
	VOID SetCaptionDx(LPCTSTR pszCaption)
	{
		m_psh.pszCaption = pszCaption;
	}
	VOID SetStartPageDx(UINT nStartPage)
	{
		m_psh.nStartPage = nStartPage;
	}

	////////////////////////////////////////////////////////////////////
	// creation / running

	// Runs the sheet. For a modal sheet, this blocks and returns IDOK,
	// IDCANCEL, or -1 on failure. For a modeless sheet (SetModelessDx),
	// this creates the window and returns non-zero on success; the
	// caller owns the (modeless) message loop.
	INT_PTR DoSheetDx()
	{
		m_psh.nPages = (UINT)m_pages.size();
		m_psh.phpage = m_pages.empty() ? NULL : &m_pages[0];

		s_pCreatingDx() = this;
		INT_PTR ret = ::PropertySheet(&m_psh);
		s_pCreatingDx() = NULL;
		return ret;
	}

	////////////////////////////////////////////////////////////////////
	// PropSheet_* wrappers acting on this sheet

	HWND GetTabControlDx() const
	{
		return PropSheet_GetTabControl(m_hwnd);
	}
	BOOL SetCurSelDx(INT nIndex, HPROPSHEETPAGE hPage = NULL)
	{
		if (m_hwnd)
			return (BOOL)PropSheet_SetCurSel(m_hwnd, hPage, nIndex);
		m_psh.nStartPage = nIndex;
		return TRUE;
	}
	BOOL SetCurSelIDDx(INT nDialogID)
	{
		return (BOOL)PropSheet_SetCurSelByID(m_hwnd, nDialogID);
	}
	VOID ApplyDx()
	{
		PropSheet_Apply(m_hwnd);
	}
	VOID CancelToCloseDx()
	{
		PropSheet_CancelToClose(m_hwnd);
	}
	VOID PressButtonDx(INT nButton)
	{
		PropSheet_PressButton(m_hwnd, nButton);
	}
	VOID SetWizButtonsDx(DWORD dwFlags)
	{
		PropSheet_SetWizButtons(m_hwnd, dwFlags);
	}
	VOID SetTitleDx(LPCTSTR pszText, DWORD dwStyle = 0)
	{
		PropSheet_SetTitle(m_hwnd, dwStyle, pszText);
	}
	INT GetPageCountDx() const
	{
		return (INT)m_pages.size();
	}
	HWND GetCurrentPageHwndDx() const
	{
		return PropSheet_GetCurrentPageHwnd(m_hwnd);
	}
	BOOL IsDialogMessageDx(LPMSG pMsg)
	{
		return ::IsDialogMessage(m_hwnd, pMsg);
	}

	////////////////////////////////////////////////////////////////////
	// overridables for the frame window itself

	virtual VOID OnDestroy(HWND hwnd)
	{
	}

	virtual LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_DESTROY:
			OnDestroy(hwnd);
			break;
		}
		return DefaultProcDx(hwnd, uMsg, wParam, lParam);
	}

	static INT CALLBACK
	PropSheetProcDx(HWND hwnd, UINT uMsg, LPARAM lParam);

protected:
	// A pointer stashed just before ::PropertySheet(...) is invoked, so
	// that PropSheetProcDx (a plain C callback with no user-data slot of
	// its own at PSCB_PRECREATE time) can find its way back to "this"
	// once the frame HWND is available at PSCB_INITIALIZED.
	static MPropSheet*& s_pCreatingDx()
	{
		static MPropSheet *s_p = NULL;
		return s_p;
	}

	// IMPORTANT: comctl32 keeps its own internal PROPSHEETINFO pointer in
	// GWLP_USERDATA of the property sheet *frame* window. MWindowBase's
	// usual Attach()/SubclassDx() store "this" there too via SetUserData,
	// which clobbers comctl32's own pointer and reliably crashes inside
	// ::PropertySheet() (often as an access violation) the next time it
	// touches that internal data. So the frame gets its own, separate
	// subclassing path that keeps "this" in a window property instead.
	static LPCTSTR PropNameDx()
	{
		return TEXT("MZC4_MPropSheet_this");
	}

	static MPropSheet *GetFrameDx(HWND hwnd)
	{
		return reinterpret_cast<MPropSheet *>(::GetProp(hwnd, PropNameDx()));
	}

	BOOL SubclassFrameDx(HWND hwnd)
	{
		m_hwnd = hwnd;
		::SetProp(hwnd, PropNameDx(), reinterpret_cast<HANDLE>(this));
		m_fnOldProc = (WNDPROC)::SetWindowLongPtr(
			hwnd, GWLP_WNDPROC, (LONG_PTR)MPropSheet::FrameWindowProc);
		return m_fnOldProc != NULL;
	}

	VOID UnsubclassFrameDx(HWND hwnd)
	{
		if (m_fnOldProc)
		{
			::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)m_fnOldProc);
			m_fnOldProc = NULL;
		}
		::RemoveProp(hwnd, PropNameDx());
	}

	static LRESULT CALLBACK
	FrameWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

////////////////////////////////////////////////////////////////////////////
// inline implementations

inline /*static*/ INT_PTR CALLBACK
MPropSheetPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MPropSheetPage *base;
	if (uMsg == WM_INITDIALOG)
	{
		LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)lParam;
		base = reinterpret_cast<MPropSheetPage *>(psp->lParam);
		base->Attach(hwnd);
		::SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)base);
	}
	else
	{
		base = MPropSheetPage::GetUserData(hwnd);
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

inline /*static*/ INT CALLBACK
MPropSheet::PropSheetProcDx(HWND hwnd, UINT uMsg, LPARAM lParam)
{
	switch (uMsg)
	{
	case PSCB_INITIALIZED:
		// The frame window now exists; hook it up via SetProp, NOT via
		// MWindowBase::SubclassDx (which would stomp on comctl32's own
		// GWLP_USERDATA on this window -- see SubclassFrameDx's comment).
		if (MPropSheet *pThis = MPropSheet::s_pCreatingDx())
		{
			pThis->SubclassFrameDx(hwnd);
		}
		break;

	case PSCB_PRECREATE:
		// return TRUE to allow creation; DS_CONTEXTHELP etc. could be
		// toggled here on the DLGTEMPLATE/DLGTEMPLATEEX pointed to by
		// lParam if desired.
		break;

#ifdef PSCB_BUTTONPRESSED
	case PSCB_BUTTONPRESSED:
		break;
#endif
	}
	return 0;
}

inline /*static*/ LRESULT CALLBACK
MPropSheet::FrameWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MPropSheet *pThis = MPropSheet::GetFrameDx(hwnd);

	LRESULT ret = 0;
	if (pThis)
	{
		pThis->SaveMessageDx(hwnd, uMsg, wParam, lParam);
		ret = pThis->WindowProcDx(hwnd, uMsg, wParam, lParam);

		if (uMsg == WM_NCDESTROY)
		{
			// Unhook before the window goes away. Restoring the old
			// wndproc is mostly a formality here since the HWND is
			// being destroyed, but it keeps things tidy if anything
			// still holds a reference during WM_NCDESTROY handling.
			pThis->UnsubclassFrameDx(hwnd);
			pThis->m_hwnd = NULL;
		}
	}
	else
	{
		ret = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return ret;
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MPROPSHEET_HPP_
