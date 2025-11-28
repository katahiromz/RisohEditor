// EgaBridge.cpp --- Bridge for EGA Programming Language integration
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#include "EgaBridge.hpp"
#include <windows.h>
#include <utility>

#include "../EGA/ega.hpp"

using namespace EGA;

namespace
{
    static EgaInputFn  s_inputFn;
    static EgaPrintFn  s_printFn;
    static LONG s_nRunning = 0;
    static HANDLE s_hThread = NULL;
}

static bool Ega_CInput(char* buf, size_t buflen)
{
    if (s_inputFn)
        return s_inputFn(buf, buflen);
    return false;
}

static void Ega_CPrint(const char* fmt, va_list va)
{
    if (s_printFn)
        s_printFn(fmt, va);
}

static DWORD WINAPI EgaBridgeThreadProc(LPVOID args)
{
    if (InterlockedIncrement(&s_nRunning) == 1)
    {
        EGA_interactive(NULL, true);
        InterlockedDecrement(&s_nRunning);
    }
    return 0;
}

namespace EgaBridge
{
    bool Initialize()
    {
        if (!EGA_init())
            return false;

        EGA_set_input_fn(Ega_CInput);
        EGA_set_print_fn(Ega_CPrint);

        return true;
    }

    void Uninitialize()
    {
        StopInteractive();
        EGA_uninit();
    }

    void SetInputFn(EgaInputFn fn)
    {
        s_inputFn = std::move(fn);
    }

    void SetPrintFn(EgaPrintFn fn)
    {
        s_printFn = std::move(fn);
    }

    bool StartInteractive()
    {
        if (s_nRunning)
        {
            return true;
        }

        s_hThread = ::CreateThread(NULL, 0, EgaBridgeThreadProc, NULL, 0, NULL);
        if (s_hThread)
        {
            ::CloseHandle(s_hThread);
            s_hThread = NULL;
        }
        return true;
    }

    void StopInteractive()
    {
        // EGA_interactive will return when input 'exit' is sent.
        // The thread handle was already closed in StartInteractive.
    }
}
