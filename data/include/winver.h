/*
 * winver.h
 *
 * Declarations supporting the API for retrieval of version informatation
 * from a PE-COFF file's associated version resources.
 *
 * $Id: winver.h,v 9f500a353390 2016/06/03 22:05:03 keithmarshall $
 *
 * Written by Anders Norlander <anorland@hem2.passagen.se>
 * Copyright (C) 1998, 1999, 2002, 2006, 2009, 2015, MinGW.org Project
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
#ifndef _WINVER_H
#define _WINVER_H
#pragma GCC system_header

#define VS_FILE_INFO		RT_VERSION
#define VS_VERSION_INFO 		 1
#define VS_USER_DEFINED 	       100
#define VS_FFI_SIGNATURE	0xFEEF04BD
#define VS_FFI_STRUCVERSION	   0x10000
#define VS_FFI_FILEFLAGSMASK	      0x3F
#define VS_FF_DEBUG			 1
#define VS_FF_PRERELEASE		 2
#define VS_FF_PATCHED			 4
#define VS_FF_PRIVATEBUILD		 8
#define VS_FF_INFOINFERRED		16
#define VS_FF_SPECIALBUILD		32
#define VOS_UNKNOWN			 0
#define VOS_DOS 		   0x10000
#define VOS_OS216		   0x20000
#define VOS_OS232		   0x30000
#define VOS_NT			   0x40000
#define VOS__BASE			 0
#define VOS__WINDOWS16			 1
#define VOS__PM16			 2
#define VOS__PM32			 3
#define VOS__WINDOWS32			 4
#define VOS_DOS_WINDOWS16	   0x10001
#define VOS_DOS_WINDOWS32	   0x10004
#define VOS_OS216_PM16		   0x20002
#define VOS_OS232_PM32		   0x30003
#define VOS_NT_WINDOWS32	   0x40004
#define VFT_UNKNOWN			 0
#define VFT_APP 			 1
#define VFT_DLL 			 2
#define VFT_DRV 			 3
#define VFT_FONT			 4
#define VFT_VXD 			 5
#define VFT_STATIC_LIB			 7
#define VFT2_UNKNOWN			 0
#define VFT2_DRV_PRINTER		 1
#define VFT2_DRV_KEYBOARD		 2
#define VFT2_DRV_LANGUAGE		 3
#define VFT2_DRV_DISPLAY		 4
#define VFT2_DRV_MOUSE			 5
#define VFT2_DRV_NETWORK		 6
#define VFT2_DRV_SYSTEM 		 7
#define VFT2_DRV_INSTALLABLE		 8
#define VFT2_DRV_SOUND			 9
#define VFT2_DRV_COMM			10
#define VFT2_DRV_INPUTMETHOD		11
#define VFT2_FONT_RASTER		 1
#define VFT2_FONT_VECTOR		 2
#define VFT2_FONT_TRUETYPE		 3
#define VFFF_ISSHAREDFILE		 1
#define VFF_CURNEDEST			 1
#define VFF_FILEINUSE			 2
#define VFF_BUFFTOOSMALL		 4
#define VIFF_FORCEINSTALL		 1
#define VIFF_DONTDELETEOLD		 2
#define VIF_TEMPFILE			 1
#define VIF_MISMATCH			 2
#define VIF_SRCOLD			 4
#define VIF_DIFFLANG			 8
#define VIF_DIFFCODEPG			16
#define VIF_DIFFTYPE			32
#define VIF_WRITEPROT			64
#define VIF_FILEINUSE		       128
#define VIF_OUTOFSPACE		       256
#define VIF_ACCESSVIOLATION	       512
#define VIF_SHARINGVIOLATION	      1024
#define VIF_CANNOTCREATE	      2048
#define VIF_CANNOTDELETE	      4096
#define VIF_CANNOTRENAME	      8192
#define VIF_CANNOTDELETECUR	     16384
#define VIF_OUTOFMEMORY 	     32768
#define VIF_CANNOTREADSRC	     65536
#define VIF_CANNOTREADDST	   0x20000
#define VIF_BUFFTOOSMALL	   0x40000

#ifndef RC_INVOKED

_BEGIN_C_DECLS

typedef
struct tagVS_FIXEDFILEINFO {
  DWORD dwSignature;
  DWORD dwStrucVersion;
  DWORD dwFileVersionMS;
  DWORD dwFileVersionLS;
  DWORD dwProductVersionMS;
  DWORD dwProductVersionLS;
  DWORD dwFileFlagsMask;
  DWORD dwFileFlags;
  DWORD dwFileOS;
  DWORD dwFileType;
  DWORD dwFileSubtype;
  DWORD dwFileDateMS;
  DWORD dwFileDateLS;
} VS_FIXEDFILEINFO;

/* The following functions are each provided with prototype variants
 * supporting strings in ANSI or UTF-16LE encodings, distinguised by
 * the "A" suffix in the ANSI case, or the "W" suffix for UTF-16LE.
 * Each pair is associated with a single generic function name, which
 * is mapped to the appropriate prototype by the __AW_SUFFIXED__ macro,
 * according to the defined state of the UNICODE macro; representation
 * of the generic name becomes the ANSI prototype when UNICODE is NOT
 * defined, or the UTF-16LE prototype when UNICODE is defined.
 */
#define VerFindFile __AW_SUFFIXED__( VerFindFile )
DWORD WINAPI VerFindFileA( DWORD, LPSTR, LPSTR, LPSTR, LPSTR, PUINT, LPSTR, PUINT );
DWORD WINAPI VerFindFileW( DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT, LPWSTR, PUINT );

#define VerInstallFile __AW_SUFFIXED__( VerInstallFile )
DWORD WINAPI VerInstallFileA( DWORD, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, PUINT );
DWORD WINAPI VerInstallFileW( DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT );

#define GetFileVersionInfoSize __AW_SUFFIXED__( GetFileVersionInfoSize )
DWORD WINAPI GetFileVersionInfoSizeA( LPCSTR, PDWORD );
DWORD WINAPI GetFileVersionInfoSizeW( LPCWSTR, PDWORD );

#define GetFileVersionInfo __AW_SUFFIXED__( GetFileVersionInfo )
BOOL WINAPI GetFileVersionInfoA( LPCSTR, DWORD, DWORD, PVOID );
BOOL WINAPI GetFileVersionInfoW( LPCWSTR, DWORD, DWORD, PVOID );

#if NTDDI_VERSION >= NTDDI_VISTA
/* Windows-Vista added extended variants of the preceding two functions,
 * each taking one additional DWORD flags argument, (in first position).
 *
 * WARNING: Although documented as supported by Windows-Vista and later,
 * the ANSI variants of these two functions appear to be unsupported in
 * VERSION.DLL, prior to Windows-8.  If using these functions, you are
 * advised to use a LoadLibrary/GetProcAddress (or dlopen/dlsym) probe,
 * to confirm availability of the API entry points within VERSION.DLL
 * on the run-time host.
 */
#define GetFileVersionInfoEx __AW_SUFFIXED__( GetFileVersionInfoEx )
#define GetFileVersionInfoSizeEx __AW_SUFFIXED__( GetFileVersionInfoSizeEx )
#if NTDDI_VERSION >= NTDDI_WIN8
DWORD WINAPI GetFileVersionInfoSizeExA( DWORD, LPCSTR, PDWORD );
BOOL WINAPI GetFileVersionInfoExA( DWORD, LPCSTR, DWORD, DWORD, PVOID );
#endif
DWORD WINAPI GetFileVersionInfoSizeExW( DWORD, LPCWSTR, PDWORD );
BOOL WINAPI GetFileVersionInfoExW( DWORD, LPCWSTR, DWORD, DWORD, PVOID );

/* Values permitted for the flags in each of these functions; these may be
 * combined by logically adding them together (using the OR operator).
 */
#define FILE_VER_GET_LOCALISED	      0x01
#define FILE_VER_GET_NEUTRAL	      0x02
/*
 * ...and an additional one, for the GetFileVersionInfoEx() function
 * only; (must be combined with the value derived from the preceding
 * pair, as used in a prior call to GetFileVersionInfoSizeEx()).
 */
#define FILE_VER_GET_PREFETCHED       0x04
#endif

#define VerLanguageName __AW_SUFFIXED__( VerLanguageName )
DWORD WINAPI VerLanguageNameA( DWORD, LPSTR, DWORD );
DWORD WINAPI VerLanguageNameW( DWORD, LPWSTR, DWORD );

#define VerQueryValue __AW_SUFFIXED__( VerQueryValue )
BOOL WINAPI VerQueryValueA( LPCVOID, LPCSTR, LPVOID *, PUINT );
BOOL WINAPI VerQueryValueW( LPCVOID, LPCWSTR, LPVOID *, PUINT );

_END_C_DECLS

#endif /* ! defined RC_INVOKED */
#endif /* ! defined _WINVER_H: $RCSfile: winver.h,v $: end of file */
