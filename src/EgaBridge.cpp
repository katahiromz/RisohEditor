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
#include <atomic>
#include <thread>
#include <utility>

#include "../EGA/ega.hpp"

using namespace EGA;

namespace
{
    static EgaInputFn  s_inputFn;
    static EgaPrintFn  s_printFn;
    static std::atomic<bool> s_threadRunning(false);
    static std::thread s_thread;
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

    static void ThreadProc()
    {
        EGA_interactive(nullptr, true);
        s_threadRunning.store(false);
    }

    bool StartInteractive()
    {
        bool expected = false;
        if (!s_threadRunning.compare_exchange_strong(expected, true))
        {
            return true;
        }

        s_thread = std::thread(ThreadProc);
        return true;
    }

    void StopInteractive()
    {
        // If EGA exposes a finish API, call it here. Otherwise rely on EGA_interactive returning when input 'exit' is sent.
        if (s_thread.joinable())
        {
            // Attempt to join; EGA_interactive should exit when signaled.
            s_thread.join();
        }
    }
}
