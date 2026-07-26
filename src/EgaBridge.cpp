// EgaBridge.cpp --- Bridge for EGA Programming Language integration
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "EgaBridge.hpp"
#include <windows.h>
#include <queue>
#include <utility>
#include "MMainWnd.hpp"
#include "../EGA/ega.hpp"

using namespace EGA;

extern HWND s_hwndEga;
extern MMainWnd *s_pMainWnd;

#ifndef NDEBUG
	#define DBGOUT(str) OutputDebugStringA(str)
#else
	#define DBGOUT(str) do { } while (0)
#endif

namespace
{
	static CRITICAL_SECTION s_cs;
	static bool     s_bCsReady    = false; // s_csの準備ＯＫ？
	static HANDLE   s_hThread     = NULL;  // スレッド
	static HANDLE   s_hStopEvent  = NULL;  // manual-reset
	static volatile bool s_bRunning = false; // 実行中？
	static volatile bool s_bInitialized = false;// 初期化済み？
	static CRITICAL_SECTION s_fileCs; // ファイル処理のクリティカルセクション。
	static bool s_fileCsReady = false; // s_fileCsの準備ＯＫ？
	static std::queue<std::string> s_fileQueue; // ファイルのキュー。

	// UIスレッドで実行する処理のキュー。
	static std::queue<std::pair<std::function<void(void*)>, void*>> s_uiQueue;
	static CRITICAL_SECTION s_uiCs; // UIスレッドのクリティカルセクション。
	static HANDLE s_hUIDone = NULL; // UI処理終了か？

	// Enter/input handshake state (owned by EgaBridge so that it is
	// re-created every session -- see Initialize()/Uninitialize()).
	static volatile bool s_bEnterPressed;
	static CRITICAL_SECTION s_inputCs;
	static bool     s_inputCsReady = false;
	static HANDLE   s_hInputDone   = NULL;   // auto-reset event
	static std::wstring s_inputBuffer;       // protected by s_inputCs

	// Print-output buffer. Protected by s_printCs. The UI thread now
	// *pulls* this buffer on a WM_TIMER tick (see MEgaDlg::OnTimer)
	// instead of the worker thread pushing a WM_EGA_DO_PRINT message per
	// burst. This decouples the UI refresh rate from the EGA output
	// rate, so a fast producer can never flood the message queue.
	static CRITICAL_SECTION s_printCs; // 出力用のクリティカルセクション。
	static bool     s_printCsReady = false; // s_printCs準備ＯＫ？
	static std::wstring s_printBuffer; // 出力バッファ。protected by s_printCs
}

// EGAを実行するためのスレッド関数。
// すべてのRisohEditorのEGA実行単位はこの中で実行されなければならない。
// RisohEditorのEGA実行単位はこの関数の外では実行してはならない。
static DWORD WINAPI EgaBridgeThreadProc(LPVOID args)
{
	DBGOUT("EgaBridgeThreadProc: enter\n");

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	try
	{
		EGA_interactive(NULL, true);
	}
	catch (...)
	{
		DBGOUT("EgaBridgeThreadProc: exception caught\n");
	}

	DBGOUT("EgaBridgeThreadProc: forcing cleanup before leave\n");

	EnterCriticalSection(&s_cs);
	s_bRunning = false; // EGA実行終了。
	LeaveCriticalSection(&s_cs);

	DBGOUT("EgaBridgeThreadProc: leave\n");

	// Do not close s_hThread here. Owner thread handles it.
	return 0;
}

namespace EgaBridge
{
	// 初期化。
	bool Initialize()
	{
		if (s_bInitialized)
			return true;

		InitializeCriticalSection(&s_cs);
		InitializeCriticalSection(&s_fileCs);
		InitializeCriticalSection(&s_uiCs);
		InitializeCriticalSection(&s_inputCs);
		InitializeCriticalSection(&s_printCs);
		s_fileCsReady = true;
		s_inputCsReady = true;
		s_printCsReady = true;

		s_hStopEvent = ::CreateEventW(NULL, TRUE, FALSE, NULL); // Manual-reset
		s_hUIDone    = ::CreateEventW(NULL, FALSE, FALSE, NULL); // auto-reset
		s_hInputDone = ::CreateEventW(NULL, FALSE, FALSE, NULL); // auto-reset
		if (!s_hStopEvent || !s_hUIDone || !s_hInputDone)
		{
			if (s_hInputDone) { ::CloseHandle(s_hInputDone); s_hInputDone = NULL; }
			if (s_hUIDone) { ::CloseHandle(s_hUIDone); s_hUIDone = NULL; }
			if (s_hStopEvent) { ::CloseHandle(s_hStopEvent); s_hStopEvent = NULL; }
			DeleteCriticalSection(&s_printCs);
			DeleteCriticalSection(&s_inputCs);
			DeleteCriticalSection(&s_uiCs);
			DeleteCriticalSection(&s_fileCs);
			DeleteCriticalSection(&s_cs);
			s_printCsReady = false;
			s_inputCsReady = false;
			s_fileCsReady = false;
			return false;
		}

		s_bCsReady = true;

		// Fresh session: make sure no state leaks in from a previous
		// EGA dialog session.
		s_bEnterPressed = false;
		s_inputBuffer.clear();
		s_printBuffer.clear();

		if (!EGA_init())
		{
			s_bCsReady = false;
			::CloseHandle(s_hInputDone); s_hInputDone = NULL;
			::CloseHandle(s_hUIDone); s_hUIDone = NULL;
			::CloseHandle(s_hStopEvent); s_hStopEvent = NULL;
			DeleteCriticalSection(&s_printCs);
			DeleteCriticalSection(&s_inputCs);
			DeleteCriticalSection(&s_uiCs);
			DeleteCriticalSection(&s_fileCs);
			s_printCsReady = false;
			s_inputCsReady = false;
			s_fileCsReady = false;
			DeleteCriticalSection(&s_cs);
			return false;
		}

		s_bInitialized = true;
		return true;
	}

	// 終了。
	void Uninitialize()
	{
		if (!s_bInitialized)
			return;

		s_bInitialized = false;
		StopInteractive(true);
		EGA_uninit();

		if (s_hStopEvent) { ::CloseHandle(s_hStopEvent); s_hStopEvent = NULL; }
		if (s_bCsReady)   { DeleteCriticalSection(&s_cs); s_bCsReady = false; }
		if (s_fileCsReady) { DeleteCriticalSection(&s_fileCs); s_fileCsReady = false; }
		if (s_hUIDone) { ::CloseHandle(s_hUIDone); s_hUIDone = nullptr; }
		::DeleteCriticalSection(&s_uiCs);

		if (s_hInputDone) { ::CloseHandle(s_hInputDone); s_hInputDone = NULL; }
		if (s_inputCsReady) { DeleteCriticalSection(&s_inputCs); s_inputCsReady = false; }
		s_bEnterPressed = false;
		s_inputBuffer.clear();

		if (s_printCsReady) { DeleteCriticalSection(&s_printCs); s_printCsReady = false; }
		s_printBuffer.clear();

		while (!s_fileQueue.empty())
			s_fileQueue.pop();
		while (!s_uiQueue.empty())
			s_uiQueue.pop();
	}

	// EGA入力関数をセット。
	void SetInputFn(EgaInputFn fn)
	{
		EGA_set_input_fn(fn);
	}

	// EGA出力関数をセット。
	void SetPrintFn(EgaPrintFn fn)
	{
		EGA_set_print_fn(fn);
	}

	// EGAとの対話を開始。
	bool StartInteractive()
	{
		if (s_bRunning)
		{
			DBGOUT("already running\n");
			StopInteractive(true);
			DBGOUT("waited\n");
		}

		EnterCriticalSection(&s_cs);

		DBGOUT("StartInteractive\n");

		::ResetEvent(s_hStopEvent);
		s_bRunning = true;

		HANDLE hThread = ::CreateThread(NULL, 0, EgaBridgeThreadProc, NULL, 0, NULL);
		if (!hThread)
		{
			DBGOUT("CreateThread failed\n");
			s_bRunning = false;
			LeaveCriticalSection(&s_cs);
			return false;
		}

		if (s_hThread)
			::CloseHandle(s_hThread);
		s_hThread = hThread;

		LeaveCriticalSection(&s_cs);
		return true;
	}

	// EGAとの対話を停止。
	void StopInteractive(bool wait)
	{
		HANDLE hThread = NULL;

		EnterCriticalSection(&s_cs);

		if (!s_bRunning) // 実行されていない？
		{
			// スレッドを閉じる。
			if (s_hThread)
			{
				::CloseHandle(s_hThread);
				s_hThread = NULL;
			}

			LeaveCriticalSection(&s_cs);
			return;
		}

		EGA_stop();
		::SetEvent(s_hStopEvent);

		if (s_hwndEga)
			PostMessageW(s_hwndEga, WM_COMMAND, IDCANCEL, 0);

		hThread = s_hThread;

		LeaveCriticalSection(&s_cs);

		if (hThread && wait)
		{
			DWORD dwWait = WaitForSingleObject(hThread, 3000);  // 3秒待機
			if (dwWait == WAIT_TIMEOUT)
				DBGOUT("StopInteractive: Thread did not exit in time. Forcing...\n");

			CloseHandle(hThread);

			EnterCriticalSection(&s_cs);
			if (s_hThread == hThread)
				s_hThread = NULL;
			LeaveCriticalSection(&s_cs);
		}

		// ファイル入力をクリア。
		EnterCriticalSection(&s_fileCs);
		while (!s_fileQueue.empty())
			s_fileQueue.pop();
		LeaveCriticalSection(&s_fileCs);
	}

	// 停止要求されたか？
	bool IsStopRequested()
	{
		return EGA_is_stopping() || 
		       (s_hStopEvent && ::WaitForSingleObject(s_hStopEvent, 0) == WAIT_OBJECT_0);
	}

	// 停止ハンドルを取得。クライアントは閉じてはいけない。
	void* GetStopEventHandle()
	{
		return s_hStopEvent;
	}

	// 入力ファイルをプッシュ。
	void RequestFileInput(const std::string& filename)
	{
		if (!s_fileCsReady)
			return;
		EnterCriticalSection(&s_fileCs);
		s_fileQueue.push(filename);
		LeaveCriticalSection(&s_fileCs);
	}

	// 入力ファイルをポップ。
	bool TryTakeFileInputRequest(std::string& filename)
	{
		if (!s_fileCsReady)
			return false;
		bool ret = false;
		EnterCriticalSection(&s_fileCs);
		if (!s_fileQueue.empty())
		{
			filename = s_fileQueue.front();
			s_fileQueue.pop();
			ret = true;
		}
		LeaveCriticalSection(&s_fileCs);
		return ret;
	}

	// UIスレッドを実行。
	bool RunOnUIThread(std::function<void(void*)> fn, void* param)
	{
		DBGOUT("RunOnUIThread\n");
		if (IsStopRequested())
			return false;

		HWND hwnd;
		EnterCriticalSection(&s_uiCs);
		s_uiQueue.push(std::make_pair(fn, param));
		hwnd = s_hwndEga;
		LeaveCriticalSection(&s_uiCs);

		if (!::IsWindow(hwnd))
			return false;

		if (!::PostMessageW(hwnd, WM_EGA_DO_RUN_ON_UI, 0, 0))
			return false;

		HANDLE waitHandles[2] = { s_hUIDone, s_hStopEvent };
		DWORD wait = WaitForMultipleObjects(2, waitHandles, FALSE, 500);  // 500ms

		if (wait == WAIT_OBJECT_0 + 1)  // StopEvent
			return false;

		return wait == WAIT_OBJECT_0;
	}

	// UIタスクを実行。
	void ExecuteUITask(void*)
	{
		std::function<void(void*)> task;
		void* param = nullptr;

		EnterCriticalSection(&s_uiCs);

		if (!s_uiQueue.empty())
		{
			task  = std::move(s_uiQueue.front().first);
			param = s_uiQueue.front().second;
			s_uiQueue.pop();
		}

		LeaveCriticalSection(&s_uiCs);

		if (task)
		{
			try
			{
				task(param);
			}
			catch (...)
			{
				DBGOUT("EGA UI task exception\n");
			}
		}

		::SetEvent(s_hUIDone);
	}

	// Enterキーが押されたのを通知。
	void NotifyEnterPressed()
	{
		s_bEnterPressed = true;
	}

	// Enterキーが押された？
	bool IsEnterPressed()
	{
		return s_bEnterPressed;
	}

	// Enterキーが押された状態を解除。
	void ClearEnterPressed()
	{
		s_bEnterPressed = false;
	}

	// 入力を受け取る準備。
	void PrepareForInput()
	{
		if (s_hInputDone)
			::ResetEvent(s_hInputDone);
	}

	// 入力内容を投稿。
	void SubmitInputText(const std::wstring& text)
	{
		if (!s_inputCsReady)
			return;

		EnterCriticalSection(&s_inputCs);
		s_inputBuffer = text;
		LeaveCriticalSection(&s_inputCs);

		if (s_hInputDone)
			::SetEvent(s_hInputDone);
	}

	// 入力待ちして、入力文字列を取得。入力か停止要求が来るまでいつまでも待つ。
	bool WaitAndTakeInputText(std::wstring& outText, DWORD dwTimeout)
	{
		if (!s_hInputDone || !s_hStopEvent)
			return false;

		HANDLE waitHandles[2] = { s_hInputDone, s_hStopEvent };
		DWORD wait = ::WaitForMultipleObjects(2, waitHandles, FALSE, dwTimeout);
		if (wait != WAIT_OBJECT_0)
			return false; // stop request (or failure)

		if (s_inputCsReady)
		{
			EnterCriticalSection(&s_inputCs);
			outText = s_inputBuffer;
			LeaveCriticalSection(&s_inputCs);
		}

		return true;
	}

	// 出力文字列をバッファに追加するだけ。UIスレッドへの通知は行わない
	// (UI側がWM_TIMERで定期的に取りに来る。EGA_dialog_inputが実行単位の
	// 終わりに明示的にWM_EGA_DO_PRINTを投函するので、そこでも即時反映される)。
	void QueuePrintText(const std::wstring& text)
	{
		if (!s_printCsReady || text.empty())
			return;

		EnterCriticalSection(&s_printCs);
		s_printBuffer += text;

		// UI側が何らかの理由で長時間pullしなかった場合の
		// メモリ保護用トリム(通常は起きない)。
		const size_t kMaxBufferedChars = 2'000'000;
		if (s_printBuffer.size() > kMaxBufferedChars)
			s_printBuffer.erase(0, s_printBuffer.size() - kMaxBufferedChars);
		LeaveCriticalSection(&s_printCs);
	}

	// 未処理の出力文字列を取得する。出力文字列が空か、失敗したならfalseを返す。
	bool TakePendingPrintText(std::wstring& outText)
	{
		outText.clear();

		if (!s_printCsReady)
			return false;

		EnterCriticalSection(&s_printCs);
		outText.swap(s_printBuffer);
		LeaveCriticalSection(&s_printCs);

		return !outText.empty();
	}

	bool FileSecurity(std::string& filename)
	{
		return EGA_file_security(filename);
	}

	void HitSecurity(void)
	{
		EGA_hit_security();
	}
}
