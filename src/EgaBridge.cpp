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

#include "../EGA/ega.hpp"

using namespace EGA;

namespace
{
    static LONG s_nRunning = 0;
    static bool s_bInitialized = false;
}

static DWORD WINAPI EgaBridgeThreadProc(LPVOID args)
{
    if (InterlockedIncrement(&s_nRunning) == 1)
    {
        try
        {
            EGA_interactive(NULL, true);
        }
        catch (...)
        {
            // Exception caught - continue to decrement counter below
        }
        InterlockedDecrement(&s_nRunning);
    }
    return 0;
}

namespace EgaBridge
{
    bool Initialize()
    {
        if (s_bInitialized)
            return true;

        if (!EGA_init())
            return false;

        s_bInitialized = true;
        return true;
    }

    void Uninitialize()
    {
        if (!s_bInitialized)
            return;

        StopInteractive();
        EGA_uninit();
        s_bInitialized = false;
    }

    void SetInputFn(EgaInputFn fn)
    {
        EGA_set_input_fn(fn);
    }

    void SetPrintFn(EgaPrintFn fn)
    {
        EGA_set_print_fn(fn);
    }

    bool StartInteractive()
    {
        if (s_nRunning)
        {
            return true;
        }

        HANDLE hThread = ::CreateThread(NULL, 0, EgaBridgeThreadProc, NULL, 0, NULL);
        if (hThread)
        {
            // Close handle immediately; the thread runs independently.
            // The s_nRunning counter tracks if the thread is active.
            ::CloseHandle(hThread);
            return true;
        }
        return false;
    }

    void StopInteractive()
    {
        // EGA_interactive will return when input 'exit' is sent.
        // The thread handle was already closed; s_nRunning tracks completion.
    }
}
