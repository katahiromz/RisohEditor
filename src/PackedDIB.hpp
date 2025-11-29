// PackedDIB.hpp --- Packed DIB
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#ifndef _INC_WINDOWS
	#include <windows.h>
#endif
#include <vector>

//////////////////////////////////////////////////////////////////////////////

INT GetEncoderClsid(const WCHAR *format, CLSID *pClsid);
DWORD PackedDIB_GetBitsOffset(const void *pPackedDIB, DWORD dwSize);
BOOL PackedDIB_GetInfo(const void *pPackedDIB, DWORD dwSize, BITMAP& bm);
HBITMAP PackedDIB_CreateBitmap(const void *pPackedDIB, DWORD dwSize);
HICON PackedDIB_CreateIcon(const void *pPackedDIB, DWORD dwSize, BITMAP& bm, BOOL bIcon);
BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm);
BOOL PackedDIB_Extract(LPCWSTR FileName, const void *ptr, size_t siz, BOOL WritePNG);
HBITMAP PackedDIB_CreateBitmapFromMemory(const void *ptr, size_t siz);
