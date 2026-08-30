// MEgaDlg.hpp --- Programming Language EGA dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MResizable.hpp"
#include "settings.h"
#include "../EGA/ega.hpp"
#include "EgaBridge.hpp"
#include <deque>

using namespace EGA;

#define WM_EGA_DO_GETINPUT (WM_APP + 100)  // UI thread reads edt2 and clear
// WM_EGA_DO_PRINT is defined in EgaBridge.hpp (included above), since
// EgaBridge itself now posts it from QueuePrintText().
#define WM_EGA_FINISH (WM_APP + 101)

// EGA出力バッファを定期的にpullするためのタイマーID。
// WM_EGA_DO_PRINT (worker thread からの明示フラッシュ) と併用し、
// タイマー側を主な取得経路とする。
#define TIMER_ID_EGA_PRINT 1
#define EGA_PRINT_POLL_MS  100

// lst1に保持させる出力の上限。無制限にテキストを溜め込まないための
// リングバッファのキャパシティ。どちらかに達したら古い行から
// (必ず行単位で)捨てる。
#define EGA_OUTPUT_MAX_LINES 5000
#define EGA_OUTPUT_MAX_CHARS 500000

class MEgaDlg;
extern HWND s_hwndEga;
extern HWND g_hMainWnd;
extern MIdOrString g_RES_select_type;
extern MIdOrString g_RES_select_name;
extern LANGID g_RES_select_lang;

bool EGA_dialog_input(char *buf, size_t buflen);
void EGA_dialog_print(const char *fmt, va_list va);
void EGA_extension(void);

//////////////////////////////////////////////////////////////////////////////

class MEgaDlg : public MDialogBase
{
public:
	std::wstring m_filename;

	DECLARE_DYNAMIC(MEgaDlg)

	MEgaDlg();
	virtual ~MEgaDlg();

	void ExecuteEgaFile(LPCWSTR filename = nullptr);

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

protected:
	HFONT m_hFont;
	HICON m_hIcon;
	HICON m_hIconSm;
	// True only if EgaBridge::Initialize() actually succeeded for this
	// dialog instance. It can legitimately fail if a previous session's
	// worker thread has not yet been confirmed to have exited (see
	// EgaBridge.cpp's s_hZombieThread) -- in that case we must not call
	// EGA_extension()/StartInteractive(), since those touch the same
	// unsynchronized ega.cpp globals (s_fn_map/s_var_map) that thread
	// might still be reading.
	bool m_bEgaReady = false;
	MResizable m_resizable;
	WNDPROC m_fnOldEditWndProc = nullptr;
	WNDPROC m_fnOldLst1WndProc = nullptr;

	// lst1の内容と1対1対応する行ベースのリングバッファ。
	// リストボックスは行単位で管理されるため、文字数上限も行単位で制御。
	std::deque<std::wstring> m_lines;  // 確定済みの行(各行末尾に"\r\n"を含む)。古い順
	std::wstring m_openLine;           // まだ改行されていない末尾の行
	size_t m_cchLines = 0;             // m_lines + m_openLine の合計文字数
	INT m_nLst1MaxExtent = 0;          // lst1の水平スクロール範囲(LB_SETHORIZONTALEXTENT用の最大行幅px)

	// History for edt2 input (command history with Up/Down arrows)
	std::vector<std::wstring> m_history;
	size_t m_nHistoryPos = 0;

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
	void OnOK(HWND hwnd);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void OnDestroy(HWND hwnd);
	HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type);
	void OnMove(HWND hwnd, int x, int y);
	void OnSize(HWND hwnd, UINT state, int cx, int cy);
	void OnEgaGetInput(HWND hwnd);
	void OnEgaPrint(HWND hwnd);
	void OnTimer(HWND hwnd, UINT id);
	void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);

	void AddToHistory(const std::wstring& str);
	void NavigateHistory(HWND hEdt2, bool bUp);
	void AppendEgaOutput(HWND hwnd, const std::wstring& text);
	void DoCopyList(HWND hwndList);
	void DoSelectAll(HWND hwndList);
	void UpdateLst1HorizontalExtent(HWND hwndLst1, const std::wstring& text, bool bReset = false);

	static LRESULT CALLBACK Edt2WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK Lst1WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
