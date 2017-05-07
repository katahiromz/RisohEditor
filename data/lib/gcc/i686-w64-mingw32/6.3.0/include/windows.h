/*
 * windows.h
 *
 * Include supplementary headers for core Win32 API definitions.
 *
 * $Id: windows.h,v f02731a7c98a 2016/06/02 21:15:06 keithmarshall $
 *
 * Written by Anders Norlander <anorland@hem2.passagen.se>
 * Copyright (C) 1998-2003, 2006, 2007, 2016, MinGW.org Project
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef _WINDOWS_H
#pragma GCC system_header
#define _WINDOWS_H

#ifdef RC_INVOKED
/* winresrc.h includes the necessary headers */
#include <winresrc.h>
#else

#include <stdarg.h>
#include <windef.h>
#include <wincon.h>
#include <winbase.h>
#if !(defined NOGDI || defined  _WINGDI_H)
#include <wingdi.h>
#endif
#include <winuser.h>
#include <winnls.h>
#include <winver.h>
#include <winnetwk.h>
#include <winreg.h>
#include <winsvc.h>

#ifndef WIN32_LEAN_AND_MEAN
#include <cderr.h>
#include <dde.h>
#include <ddeml.h>
#include <dlgs.h>
#include <imm.h>
#include <lzexpand.h>
#include <mmsystem.h>
#include <nb30.h>
#include <rpc.h>
#include <shellapi.h>
#include <winperf.h>
#ifndef NOGDI
#include <commdlg.h>
#include <winspool.h>
#endif
#if defined(Win32_Winsock)
#warning "The  Win32_Winsock macro name is deprecated.\
    Please use __USE_W32_SOCKETS instead"
#ifndef __USE_W32_SOCKETS
#define __USE_W32_SOCKETS
#endif
#endif
#if defined(__USE_W32_SOCKETS) || !(defined(__CYGWIN__) || defined(__MSYS__) || defined(_UWIN))
#if (_WIN32_WINNT >= 0x0400)
#include <winsock2.h>
/*
 * MS likes to include mswsock.h here as well,
 * but that can cause undefined symbols if
 * winsock2.h is included before windows.h
 */
#else
#include <winsock.h>
#endif /*  (_WIN32_WINNT >= 0x0400) */
#endif
#ifndef NOGDI
/* In older versions we disallowed COM declarations in __OBJC__
   because of conflicts with @interface directive.  Define _OBJC_NO_COM
   to keep this behaviour.  */
#if !defined (_OBJC_NO_COM)
#if (__GNUC__ >= 3) || defined (__WATCOMC__)
#include <ole2.h>
#endif
#endif /* _OBJC_NO_COM */
#endif

#endif /* WIN32_LEAN_AND_MEAN */

#endif /* RC_INVOKED */

#ifdef __OBJC__
/* FIXME: Not undefining BOOL here causes all BOOLs to be WINBOOL (int),
   but undefining it causes trouble as well if a file is included after
   windows.h
*/
#undef BOOL
#endif

#endif	/* _WINDOWS_H: $RCSfile: windows.h,v $: end of file */
