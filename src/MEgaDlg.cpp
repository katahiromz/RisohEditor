// MEgaDlg.cpp --- Programming Language EGA dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MEgaDlg.hpp"
#include "Res.hpp"
#include <vector>
#include <string>

// EGA入力関数。EGA_set_input_fnに渡される。
// この関数がfalseを返せば、EGAの実行単位が終了する。
bool EGA_dialog_input(char *buf, size_t buflen)
{
	if (buf == NULL && buflen == 0) // １つ実行単位の実行が終わったとき？
	{
		// 特殊なメッセージを投函する。
		::PostMessageW(s_hwndEga, WM_EGA_DO_PRINT, 0, 0);
		::PostMessageW(g_hMainWnd, WM_COMMAND, ID_EGAFINISH, 0);
		return true;
	}

	// 入力と終了を待つ。
	while (true)
	{
		if (EgaBridge::IsStopRequested())
		{
#ifndef NDEBUG
			OutputDebugStringA("EGA_dialog_input: stop requested -> return false\n");
#endif
			return false;
		}

		if (EgaBridge::IsEnterPressed() && ::IsWindowVisible(s_hwndEga))
			break;   // 入力可能

		// ファイル入力
		std::string pendingFile;
		if (EgaBridge::TryTakeFileInputRequest(pendingFile))
		{
			EGA_file_input(pendingFile.c_str());
			continue;
		}

		Sleep(10);
	}

	// 入力を受け入れる準備をする。
	EgaBridge::ClearEnterPressed();
	EgaBridge::PrepareForInput();

	// 入力を要求する。
	::PostMessageW(s_hwndEga, WM_EGA_DO_GETINPUT, 0, 0);

	// このスレッドで実際に入力と終了要求を待つ。
	std::wstring textW;
	if (!EgaBridge::WaitAndTakeInputText(textW, INFINITE))
		return false; // Stop request

	// UTF-8化してbufに格納する。
	char szTextA[512];
	WideCharToMultiByte(CP_UTF8, 0, textW.c_str(), -1, szTextA, ARRAYSIZE(szTextA), NULL, NULL);
	StringCchCopyA(buf, buflen, szTextA);

	// 「exit」か「exit;」の場合は終了を要求するためのコマンドを投函する。
	if (lstrcmpA(szTextA, "exit") == 0 || lstrcmpA(szTextA, "exit;") == 0)
		PostMessageW(s_hwndEga, WM_COMMAND, IDCANCEL, 0);

	return true;
}

// EGA出力関数。EGA_set_print_fnに渡される。
void EGA_dialog_print(const char *fmt, va_list va)
{
	// s_hwndEgaが無効なら戻る。
	if (!IsWindow(s_hwndEga))
		return;

	// 出力バッファを確保して出力文字列を取得する。
	std::string str;
	str.resize(512);
	for (;;)
	{
		va_list va2;
		va_copy(va2, va);
		HRESULT hr = StringCbVPrintfA(&str[0], str.size(), fmt, va2);
		va_end(va2);

		if (hr != STRSAFE_E_INSUFFICIENT_BUFFER)
			break;
		str.resize(str.size() * 2);
	}

	// 文字列を整える。
	str.resize(lstrlenA(str.c_str()));
	mstr_replace_all(str, "\n", "\r\n");

	// ワイド文字列にしてブリッジに渡す。
	MAnsiToWide wide(CP_UTF8, str.c_str());
	EgaBridge::QueuePrintText(wide.c_str());
}

MEgaDlg::MEgaDlg() : MDialogBase(IDD_EGA)
{
	MTRACEA("%s\n", __FUNCTION__);

	// アイコン読み込み。
	m_hIcon = LoadIconDx(IDI_SMILY);
	m_hIconSm = LoadSmallIconDx(IDI_SMILY);

	EgaBridge::Initialize();
	EgaBridge::SetInputFn(EGA_dialog_input);
	EgaBridge::SetPrintFn(EGA_dialog_print);

	EGA_extension();

	m_bDynamicCreated = true;

	// Initialize history
	m_history.clear();
	m_nHistoryPos = 0;
}

MEgaDlg::~MEgaDlg()
{
	MTRACEA("%s\n", __FUNCTION__);
	EgaBridge::Uninitialize();

	DeleteObject(m_hFont);
	DestroyIcon(m_hIcon);
	DestroyIcon(m_hIconSm);
}

void MEgaDlg::ExecuteEgaFile(LPCWSTR filename)
{
	MTRACEA("%s\n", __FUNCTION__);
	char szFileName[MAX_PATH];
	if (filename && filename[0])
	{
		WideCharToMultiByte(CP_ACP, 0, filename, -1, szFileName, _countof(szFileName), NULL, NULL);
		g_RES_select_type = BAD_TYPE;
		g_RES_select_name = BAD_NAME;
		g_RES_select_lang = BAD_LANG;
		EgaBridge::RequestFileInput(szFileName);
	}
}

LRESULT CALLBACK
MEgaDlg::Edt2WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto pThis = (MEgaDlg*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (GetFocus() == hwnd)
		{
			if (wParam == VK_UP)
			{
				pThis->NavigateHistory(hwnd, true);
				return 0;
			}
			else if (wParam == VK_DOWN)
			{
				pThis->NavigateHistory(hwnd, false);
				return 0;
			}
		}
		break;
	}
	return CallWindowProcW(pThis->m_fnOldEditWndProc, hwnd, uMsg, wParam, lParam);
}

BOOL MEgaDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	MTRACEA("%s\n", __FUNCTION__);
	s_hwndEga = hwnd; // Remember

	// Subclassing edt2
	HWND hEdt2 = GetDlgItem(hwnd, edt2);
	m_fnOldEditWndProc = (WNDPROC)SetWindowLongPtrW(hEdt2, GWLP_WNDPROC, (LONG_PTR)Edt2WndProc);
	SetWindowLongPtrW(hEdt2, GWLP_USERDATA, (LONG_PTR)this);

	// Make it a resizable dialog
	m_resizable.OnParentCreate(hwnd);
	m_resizable.SetLayoutAnchor(edt1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
	m_resizable.SetLayoutAnchor(stc1, mzcLA_BOTTOM_LEFT);
	m_resizable.SetLayoutAnchor(edt2, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);
	m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);

	// Set dialog icon
	SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
	SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

	// No limit
	SendDlgItemMessageW(hwnd, edt1, EM_SETLIMITTEXT, 0, 0);

	// Create font
	LOGFONTW lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = -12;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	m_hFont = CreateFontIndirectW(&lf);
	SendDlgItemMessageW(hwnd, edt1, WM_SETFONT, (WPARAM)m_hFont, TRUE);

	// edt1 starts out empty; track its length ourselves from here on
	// (see the m_cchEdt1 comment in MEgaDlg.hpp).
	m_cchEdt1 = GetWindowTextLengthW(GetDlgItem(hwnd, edt1));

	// Move and resize
	if (g_settings.nEgaX != CW_USEDEFAULT && g_settings.nEgaWidth != CW_USEDEFAULT)
	{
		SetWindowPos(hwnd, NULL,
			g_settings.nEgaX, g_settings.nEgaY,
			g_settings.nEgaWidth, g_settings.nEgaHeight,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}
	else if (g_settings.nEgaX != CW_USEDEFAULT)
	{
		SetWindowPos(hwnd, NULL,
			g_settings.nEgaX, g_settings.nEgaY,
			g_settings.nEgaWidth, g_settings.nEgaHeight,
			SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}
	else if (g_settings.nEgaWidth != CW_USEDEFAULT)
	{
		SetWindowPos(hwnd, NULL,
			g_settings.nEgaX, g_settings.nEgaY,
			g_settings.nEgaWidth, g_settings.nEgaHeight,
			SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}
	else
	{
		CenterWindowDx();
	}

	// Start the EGA thread
	EgaBridge::StartInteractive();
	::SetFocus(::GetDlgItem(hwnd, edt2));

	return TRUE;
}

void MEgaDlg::OnOK(HWND hwnd) // Enterキーが押された？
{
	MTRACEA("%s\n", __FUNCTION__);

	// Get current text from edt2
	WCHAR szTextW[512];
	GetDlgItemTextW(hwnd, edt2, szTextW, ARRAYSIZE(szTextW));
	std::wstring str(szTextW);
	mstr_trim(str);

	// Add to history if not empty
	if (!str.empty())
	{
		AddToHistory(str);
	}

	// リソース項目の選択情報をクリアする。
	g_RES_select_type = BAD_TYPE;
	g_RES_select_name = BAD_NAME;
	g_RES_select_lang = BAD_LANG;

	// Enterを押したことを伝える。
	EgaBridge::NotifyEnterPressed();

	// フォーカスを edt2 に移動。
	::SetFocus(::GetDlgItem(hwnd, edt2));

	// Reset history position
	m_nHistoryPos = m_history.size();
}

void MEgaDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK: // Enter
		OnOK(hwnd);
		break;
	case IDCANCEL: // Close
		EgaBridge::StopInteractive(false);
		::DestroyWindow(hwnd);
		break;
	}
}

void MEgaDlg::OnDestroy(HWND hwnd)
{
	MTRACEA("%s\n", __FUNCTION__);

	// 終了前に特殊なメッセージを投函する。
	PostMessageW(g_hMainWnd, WM_COMMAND, ID_EGAFINISH, 0);

	// EGAスレッドを終了する。
	EgaBridge::StopInteractive(true);

	// EGAを破棄する。
	EgaBridge::Uninitialize();

	s_hwndEga = NULL;
}

HBRUSH MEgaDlg::OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
	UINT id;
	switch (type)
	{
	case CTLCOLOR_EDIT:
	case CTLCOLOR_STATIC:
	case CTLCOLOR_BTN:
		id = GetDlgCtrlID(hwndChild);
		switch (id)
		{
		case edt1:
		case edt2:
			SetTextColor(hdc, RGB(0, 255, 0));
			SetBkColor(hdc, RGB(0, 0, 0));
			return GetStockBrush(BLACK_BRUSH);
		}
	}
	return (HBRUSH)(COLOR_3DFACE + 1);
}

void MEgaDlg::OnMove(HWND hwnd, int x, int y)
{
	if (IsWindowVisible(hwnd) && !IsMinimized(hwnd) && !IsMaximized(hwnd))
	{
		RECT rc;
		GetWindowRect(hwnd, &rc);
		g_settings.nEgaX = rc.left;
		g_settings.nEgaY = rc.top;
	}
}

void MEgaDlg::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	m_resizable.OnSize();

	if (IsWindowVisible(hwnd) && !IsMinimized(hwnd) && !IsMaximized(hwnd))
	{
		RECT rc;
		GetWindowRect(hwnd, &rc);
		g_settings.nEgaWidth = rc.right - rc.left;
		g_settings.nEgaHeight = rc.bottom - rc.top;
	}
}

// WM_EGA_DO_GETINPUT
void MEgaDlg::OnEgaGetInput(HWND hwnd)
{
	MTRACEA("%s\n", __FUNCTION__);

	// edt2から文字列を取得して、edt2をクリア。
	WCHAR szTextW[512];
	GetDlgItemTextW(hwnd, edt2, szTextW, ARRAYSIZE(szTextW));
	std::wstring str(szTextW);
	mstr_trim(str);
	SetDlgItemTextW(hwnd, edt2, L"");

	// 入力文字列を投函。
	EgaBridge::SubmitInputText(str.c_str());

	// Reset history position after input
	m_nHistoryPos = m_history.size();
}

// WM_EGA_DO_PRINT
void MEgaDlg::OnEgaPrint(HWND hwnd)
{
	std::wstring text;
	if (!EgaBridge::TakePendingPrintText(text) || text.empty())
		return;

	if (EgaBridge::IsStopRequested())
		return;

	static DWORD s_lastHeavyPrint = 0;
	static int   s_skipCounter = 0;
	DWORD now = GetTickCount();

	// 出力が巨大になったら強くスキップ
	if (m_cchEdt1 > 1'000'000)
	{
		if (++s_skipCounter < 50)  // 50回に1回だけ処理
			return;
		s_skipCounter = 0;
	}
	else if (m_cchEdt1 > 500'000 && (now - s_lastHeavyPrint < 200))
	{
		return; // 200ms以内に連続printはスキップ
	}

	s_lastHeavyPrint = now;

	// 極端に巨大ならさらに削る
	if (m_cchEdt1 > 3'000'000)
	{
		SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, 1'500'000);
		SendDlgItemMessageW(hwnd, edt1, EM_REPLACESEL, FALSE, (LPARAM)L"[... truncated ...]\r\n");
		m_cchEdt1 = GetWindowTextLengthW(GetDlgItem(hwnd, edt1));
	}

	SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, m_cchEdt1, m_cchEdt1);
	SendDlgItemMessageW(hwnd, edt1, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
	SendDlgItemMessageW(hwnd, edt1, EM_SCROLLCARET, 0, 0);
	m_cchEdt1 += (INT)text.size();

	::SetCursor(::LoadCursorW(NULL, IDC_ARROW));
}

// Add non-empty trimmed string to history (avoid duplicates at end)
void MEgaDlg::AddToHistory(const std::wstring& str)
{
	if (str.empty())
		return;

	// Remove trailing duplicates if same as last
	if (!m_history.empty() && m_history.back() == str)
		return;

	m_history.push_back(str);
	if (m_history.size() > 100) // limit history size
		m_history.erase(m_history.begin());
}

// Navigate history with Up/Down arrows
void MEgaDlg::NavigateHistory(HWND hEdt2, bool bUp)
{
	if (m_history.empty())
		return;

	if (bUp)
	{
		if (m_nHistoryPos > 0)
			--m_nHistoryPos;
	}
	else
	{
		if (m_nHistoryPos < m_history.size())
			++m_nHistoryPos;
	}

	if (m_nHistoryPos < m_history.size())
	{
		// Show history item
		SetWindowTextW(hEdt2, m_history[m_nHistoryPos].c_str());
		SendMessageW(hEdt2, EM_SETSEL, 0, -1); // select all
	}
	else
	{
		// Back to empty/new input
		SetWindowTextW(hEdt2, L"");
	}
}

// WM_GETMINMAXINFO
void MEgaDlg::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	lpMinMaxInfo->ptMinTrackSize.x = 380;
	lpMinMaxInfo->ptMinTrackSize.y = 250;
}

INT_PTR CALLBACK
MEgaDlg::DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
	HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
	HANDLE_MSG(hwnd, WM_CTLCOLOREDIT, OnCtlColor);
	HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC, OnCtlColor);
	HANDLE_MSG(hwnd, WM_MOVE, OnMove);
	HANDLE_MSG(hwnd, WM_SIZE, OnSize);
	HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
	HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
	case WM_EGA_DO_GETINPUT: // 入力を取得する。
		OnEgaGetInput(hwnd);
		break;
	case WM_EGA_DO_PRINT: // EGA出力を行う。
		OnEgaPrint(hwnd);
		break;
	case WM_EGA_DO_RUN_ON_UI: // UIタスクを実行。
		EgaBridge::ExecuteUITask((void*)lParam);
		break;
	default:
		return DefaultProcDx();
	}
	return 0;
}
