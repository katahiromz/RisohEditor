// EgaBridge.cpp --- Bridge for EGA Programming Language integration
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "EgaBridge.hpp"
#include <windows.h>
#include <queue>
#include <memory>
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
	static bool     s_bPrimitivesReady = false; // CS/イベントがプロセス寿命で確保済みか？
	static HANDLE   s_hThread     = NULL;  // スレッド
	// A worker thread handle we gave up waiting on (StopInteractive()'s
	// 3-second wait timed out). We deliberately do NOT close this handle
	// and do NOT assume the thread is gone: EGA_init()/EGA_uninit() (in
	// ega.cpp) mutate global, unsynchronized state (s_fn_map, s_var_map)
	// that a still-running worker thread reads on every function call
	// and every variable access. Calling EGA_uninit()'s clear()s, or
	// EGA_init()'s re-population, while that could still be happening is
	// a real data race on those containers (crash / corruption), not
	// just a theoretical one. So: while a zombie is outstanding, we
	// refuse to touch s_fn_map/s_var_map at all (skip EGA_uninit() and
	// refuse to start a new session), and opportunistically check on
	// every Initialize()/StopInteractive() call whether it has actually
	// finished by now.
	static HANDLE   s_hZombieThread = NULL;
	static HANDLE   s_hStopEvent  = NULL;  // manual-reset
	static volatile bool s_bRunning = false; // 実行中？
	static volatile bool s_bInitialized = false;// 初期化済み？
	static CRITICAL_SECTION s_fileCs; // ファイル処理のクリティカルセクション。
	static bool s_fileCsReady = false; // s_fileCsの準備ＯＫ？
	static std::queue<std::string> s_fileQueue; // ファイルのキュー。

	// UIスレッドで実行する処理のキュー。
	//
	// NOTE: each queued task owns its OWN completion event (UiTaskDone),
	// instead of everyone sharing a single s_hUIDone. Previously, a
	// RunOnUIThread() call that gave up waiting (because s_hStopEvent
	// fired, the window was gone, or PostMessageW failed) left its task
	// sitting at the front of a single shared FIFO. The next, unrelated
	// RunOnUIThread() call could then have ITS OWN WM_EGA_DO_RUN_ON_UI
	// message end up popping and running that stale task instead of its
	// own -- and the shared s_hUIDone would still get signalled, so the
	// new caller would wrongly believe *its* task had completed, while
	// reading whatever default value its own (never actually touched)
	// output variable still held. Giving every task its own completion
	// object removes any possibility of cross-task mixups: whoever
	// signals a given UiTaskDone can only be that task's own execution.
	// The event is owned via shared_ptr so it stays alive exactly as
	// long as either side (the possibly-already-given-up caller, or the
	// not-yet-run queued task) might still touch it; nobody ever closes
	// a handle the other side could still be signalling/waiting on.
	struct UiTaskDone
	{
		HANDLE hEvent = nullptr;
		UiTaskDone() { hEvent = ::CreateEventW(NULL, TRUE, FALSE, NULL); } // manual-reset
		~UiTaskDone() { if (hEvent) ::CloseHandle(hEvent); }
	};
	struct UiTask
	{
		std::function<void(void*)> fn;
		void* param = nullptr;
		std::shared_ptr<UiTaskDone> done;

		UiTask() {}
		UiTask(std::function<void(void*)> fn_, void* param_, std::shared_ptr<UiTaskDone> done_)
			: fn(std::move(fn_)), param(param_), done(std::move(done_)) {}
	};
	static std::queue<UiTask> s_uiQueue;
	static CRITICAL_SECTION s_uiCs; // UIスレッドのクリティカルセクション。

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

	// Non-blocking check of s_hZombieThread (see its declaration above).
	// Returns true if there is no outstanding zombie (either there never
	// was one, or it has since actually exited and was just reaped).
	// Returns false if a zombie is still outstanding / unconfirmed.
	static bool ReapZombieThreadIfDone()
	{
		if (!s_hZombieThread)
			return true;

		if (::WaitForSingleObject(s_hZombieThread, 0) == WAIT_OBJECT_0)
		{
			DBGOUT("ReapZombieThreadIfDone: zombie thread finally exited\n");
			::CloseHandle(s_hZombieThread);
			s_hZombieThread = NULL;
			return true;
		}

		return false;
	}
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
	//
	// NOTE: the CRITICAL_SECTIONs and the two bridge-owned events
	// (s_hStopEvent, s_hInputDone) are now created ONCE, the first time
	// Initialize() succeeds, and are never destroyed by Uninitialize().
	// See the long comment in Uninitialize() for why: StopInteractive()'s
	// wait for the worker thread to exit can time out, and if we deleted
	// these out from under a thread that is still (rarely, but possibly)
	// running, any further EgaBridge call it makes -- EnterCriticalSection
	// on a deleted CRITICAL_SECTION, WaitForMultipleObjects on a closed
	// HANDLE -- is undefined behavior and can crash or corrupt memory.
	// Keeping them alive for the rest of the process sidesteps that
	// entirely; it is a deliberate trade-off of a small, one-time,
	// process-lifetime allocation against a use-after-free.
	bool Initialize()
	{
		if (s_bInitialized)
			return true;

		// Refuse to start a new session -- and, critically, refuse to
		// touch ega.cpp's s_fn_map/s_var_map via EGA_init() below --
		// while a previous session's worker thread might still be
		// running and reading/writing those same, unsynchronized
		// containers. See the comment on s_hZombieThread.
		if (!ReapZombieThreadIfDone())
		{
			DBGOUT("Initialize: refusing to start, previous worker thread not confirmed exited\n");
			return false;
		}

		if (!s_bPrimitivesReady)
		{
			InitializeCriticalSection(&s_cs);
			InitializeCriticalSection(&s_fileCs);
			InitializeCriticalSection(&s_uiCs);
			InitializeCriticalSection(&s_inputCs);
			InitializeCriticalSection(&s_printCs);

			s_hStopEvent = ::CreateEventW(NULL, TRUE, FALSE, NULL); // Manual-reset
			s_hInputDone = ::CreateEventW(NULL, FALSE, FALSE, NULL); // auto-reset
			if (!s_hStopEvent || !s_hInputDone)
			{
				if (s_hInputDone) { ::CloseHandle(s_hInputDone); s_hInputDone = NULL; }
				if (s_hStopEvent) { ::CloseHandle(s_hStopEvent); s_hStopEvent = NULL; }
				DeleteCriticalSection(&s_printCs);
				DeleteCriticalSection(&s_inputCs);
				DeleteCriticalSection(&s_uiCs);
				DeleteCriticalSection(&s_fileCs);
				DeleteCriticalSection(&s_cs);
				return false;
			}

			s_bPrimitivesReady = true;
		}
		else
		{
			// Reused from a previous session: a stale "stop requested"
			// or leftover input-done signal must not leak into this
			// fresh session.
			::ResetEvent(s_hStopEvent);
			::ResetEvent(s_hInputDone);
		}

		s_bCsReady = true;
		s_fileCsReady = true;
		s_inputCsReady = true;
		s_printCsReady = true;

		// Fresh session: make sure no state leaks in from a previous
		// EGA dialog session.
		s_bEnterPressed = false;
		s_inputBuffer.clear();
		s_printBuffer.clear();

		if (!EGA_init())
		{
			s_bCsReady = false;
			s_fileCsReady = false;
			s_inputCsReady = false;
			s_printCsReady = false;
			return false;
		}

		s_bInitialized = true;
		return true;
	}

	// 終了。
	//
	// NOTE: this intentionally does NOT delete the critical sections or
	// close s_hStopEvent/s_hInputDone anymore -- see the comment on
	// Initialize(). Only per-session *data* (queues, buffers, flags,
	// and the "ready" gates that other EgaBridge calls check) is reset
	// here; the OS synchronization objects themselves persist for the
	// life of the process.
	void Uninitialize()
	{
		if (!s_bInitialized)
			return;

		s_bInitialized = false;
		StopInteractive(true);

		// Only touch ega.cpp's s_fn_map/s_var_map if we're sure nothing
		// is still reading them. See the comment on s_hZombieThread: if
		// StopInteractive() just failed to confirm the worker thread's
		// exit, EGA_uninit()'s clear()s would race that thread's still-
		// possible EGA_get_fn()/EGA_eval_var() lookups on the very same,
		// unsynchronized containers. Skip it this time; Initialize()
		// will keep refusing to start a new session (and therefore won't
		// call EGA_init(), which repopulates the same containers) until
		// ReapZombieThreadIfDone() confirms the old thread is finally
		// gone.
		if (!s_hZombieThread)
			EGA_uninit();
		else
			DBGOUT("Uninitialize: skipping EGA_uninit(), zombie worker thread not confirmed exited\n");

		s_bCsReady = false;
		s_fileCsReady = false;
		s_inputCsReady = false;
		s_printCsReady = false;

		s_bEnterPressed = false;
		s_inputBuffer.clear();
		s_printBuffer.clear();

		EnterCriticalSection(&s_fileCs);
		while (!s_fileQueue.empty())
			s_fileQueue.pop();
		LeaveCriticalSection(&s_fileCs);

		// Drain any UI tasks nobody ran yet (see StopInteractive(), which
		// already does this on every stop -- this is just a final,
		// belt-and-braces sweep in case something was queued afterward).
		// Signal each one's completion event so nothing is left waiting
		// on it forever.
		EnterCriticalSection(&s_uiCs);
		while (!s_uiQueue.empty())
		{
			auto& front = s_uiQueue.front();
			if (front.done && front.done->hEvent)
				::SetEvent(front.done->hEvent);
			s_uiQueue.pop();
		}
		LeaveCriticalSection(&s_uiCs);
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
		// Must not proceed without a successful Initialize(): besides
		// the general precondition, this is also what makes the
		// s_hZombieThread guard in Initialize() actually effective --
		// if Initialize() refused to run (a previous session's worker
		// thread is not confirmed exited yet), a caller that pressed
		// ahead and called StartInteractive() anyway would spin up a
		// second thread that races the zombie on ega.cpp's s_fn_map/
		// s_var_map, exactly what Initialize() was refusing to allow.
		if (!s_bInitialized)
		{
			DBGOUT("StartInteractive: not initialized, refusing\n");
			return false;
		}

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

		// Drop any UI tasks that have been queued but not yet executed:
		// the script is being aborted, so their side effects (resource
		// edits, etc.) must not run. This also prevents a later, wholly
		// unrelated RunOnUIThread() call's WM_EGA_DO_RUN_ON_UI message
		// from ever finding one of these stale tasks still sitting at
		// the front of the queue. Each task's own completion event is
		// signalled so a caller that is (or was) waiting on it never
		// blocks forever.
		EnterCriticalSection(&s_uiCs);
		while (!s_uiQueue.empty())
		{
			auto& front = s_uiQueue.front();
			if (front.done && front.done->hEvent)
				::SetEvent(front.done->hEvent);
			s_uiQueue.pop();
		}
		LeaveCriticalSection(&s_uiCs);

		if (s_hwndEga)
			PostMessageW(s_hwndEga, WM_COMMAND, IDCANCEL, 0);

		hThread = s_hThread;

		LeaveCriticalSection(&s_cs);

		if (hThread && wait)
		{
			DWORD dwWait = WaitForSingleObject(hThread, 3000);  // 3秒待機
			if (dwWait == WAIT_TIMEOUT)
			{
				// NOTE: we do NOT know that the thread has exited, so we
				// must not pretend it has. Closing the handle here (as
				// the old code did) threw away the only way we had left
				// to ever find out it finished; instead, keep ownership
				// of the handle as a tracked "zombie" so
				// ReapZombieThreadIfDone() can notice, later, once it
				// really is done. Until then, Initialize()/Uninitialize()
				// will refuse to touch ega.cpp's shared, unsynchronized
				// s_fn_map/s_var_map (see EGA_init()/EGA_uninit()),
				// since this thread could still be reading them.
				DBGOUT("StopInteractive: Thread did not exit in time. Tracking as zombie, not forcing.\n");

				EnterCriticalSection(&s_cs);
				if (s_hZombieThread)
					::CloseHandle(s_hZombieThread); // an even older zombie that we're now replacing; best effort
				s_hZombieThread = hThread;
				if (s_hThread == hThread)
					s_hThread = NULL;
				LeaveCriticalSection(&s_cs);
			}
			else
			{
				CloseHandle(hThread);

				EnterCriticalSection(&s_cs);
				if (s_hThread == hThread)
					s_hThread = NULL;
				LeaveCriticalSection(&s_cs);
			}
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

		// See the comment on the UiTask/UiTaskDone definitions above:
		// this call's completion is tracked by an event that belongs
		// only to this one task, not a queue-wide shared one.
		auto done = std::make_shared<UiTaskDone>();
		if (!done->hEvent)
			return false;

		HWND hwnd;
		EnterCriticalSection(&s_uiCs);
		s_uiQueue.push(UiTask(std::move(fn), param, done));
		hwnd = s_hwndEga;
		LeaveCriticalSection(&s_uiCs);

		if (!::IsWindow(hwnd))
			return false;

		if (!::PostMessageW(hwnd, WM_EGA_DO_RUN_ON_UI, 0, 0))
			return false;

		// NOTE: this must NOT use a short arbitrary timeout. A caller
		// (e.g. RES_load/RES_save) treats a `false` return here as
		// "control break", which EGA_interactive/EGA_eval_text_ex
		// propagates as a request to abort the *entire* interactive
		// session, not just this one statement. A slow-but-legitimate
		// UI operation (loading a large file, a dialog the user is
		// looking at, a busy message pump, ...) must not be confused
		// with an actual stop request. So we wait indefinitely and
		// only give up when the real stop event (s_hStopEvent) fires.
		HANDLE waitHandles[2] = { done->hEvent, s_hStopEvent };
		DWORD wait = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

		if (wait == WAIT_OBJECT_0 + 1)  // StopEvent
			return false;

		return wait == WAIT_OBJECT_0;
	}

	// UIタスクを実行。
	void ExecuteUITask(void*)
	{
		UiTask entry;
		bool bHas = false;

		EnterCriticalSection(&s_uiCs);

		if (!s_uiQueue.empty())
		{
			entry = std::move(s_uiQueue.front());
			s_uiQueue.pop();
			bHas = true;
		}

		LeaveCriticalSection(&s_uiCs);

		if (bHas && entry.fn)
		{
			try
			{
				entry.fn(entry.param);
			}
			catch (...)
			{
				DBGOUT("EGA UI task exception\n");
			}
		}

		// Signal only this task's own completion event -- never a
		// shared one -- so we can only ever wake the caller that is
		// actually waiting on this specific task.
		if (bHas && entry.done && entry.done->hEvent)
			::SetEvent(entry.done->hEvent);
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

	bool FileSecurity0(std::string& filename, const char* tag)
	{
		return EGA_file_security_0(filename, tag);
	}

	bool FileSecurity1(std::string& filename, const char* tag)
	{
	    return EGA_file_security_1(filename, tag);
	}

	void HitSecurity(void)
	{
		EGA_hit_security();
	}
}
