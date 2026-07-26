// EgaBridge.hpp --- Bridge for EGA Programming Language integration
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include <cstdarg>
#include <cstddef>
#include <string>
#include <functional>
#include <windows.h>

#define WM_EGA_DO_PRINT     (WM_APP + 101)
#define WM_EGA_DO_RUN_ON_UI (WM_APP + 102)

// Function pointer types matching EGA API
typedef bool (*EgaInputFn)(char* buf, size_t buflen);
typedef void (*EgaPrintFn)(const char* fmt, va_list va);

namespace EgaBridge
{
	bool Initialize();
	void Uninitialize();
	void SetInputFn(EgaInputFn fn);
	void SetPrintFn(EgaPrintFn fn);
	bool StartInteractive();
	void StopInteractive(bool wait = true);
	bool IsStopRequested();
	void* GetStopEventHandle();
	void RequestFileInput(const std::string& filename);
	bool TryTakeFileInputRequest(std::string& filename);
	bool RunOnUIThread(std::function<void(void*)> fn, void* param = nullptr);
	void ExecuteUITask(void* param);
	bool WaitAndTakeInputText(std::wstring& outText, DWORD dwTimeout);

	void NotifyEnterPressed();                          // UI thread: user pressed Enter / clicked OK
	bool IsEnterPressed();                              // EGA thread: non-destructive poll
	void ClearEnterPressed();                           // EGA thread: call once the wait is over

	void PrepareForInput();                             // EGA thread: call before requesting input
	void SubmitInputText(const std::wstring& text);     // UI thread: call after reading the edit control
	void QueuePrintText(const std::wstring& text);      // any thread (typically EGA worker thread)
	bool TakePendingPrintText(std::wstring& outText);   // UI thread: called on WM_EGA_DO_PRINT
	bool FileSecurity(std::string& filename);
	void HitSecurity(void);
}
