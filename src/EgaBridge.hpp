// EgaBridge.hpp --- Bridge for EGA Programming Language integration
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include <cstdarg>
#include <cstddef>

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
	void StopInteractive();
}
