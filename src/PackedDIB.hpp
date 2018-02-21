// PackedDIB.hpp --- Packed DIB
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#ifndef PACKED_DIB_HPP_
#define PACKED_DIB_HPP_

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include <windows.h>
#include <vector>

#include <gdiplus.h>
#pragma comment (lib, "gdiplus.lib")

//////////////////////////////////////////////////////////////////////////////

INT GetEncoderClsid(const WCHAR *format, CLSID *pClsid);
DWORD PackedDIB_GetBitsOffset(const void *pPackedDIB, DWORD dwSize);
BOOL PackedDIB_GetInfo(const void *pPackedDIB, DWORD dwSize, BITMAP& bm);
HBITMAP PackedDIB_CreateBitmap(const void *pPackedDIB, DWORD dwSize);
HICON PackedDIB_CreateIcon(const void *pPackedDIB, DWORD dwSize, BITMAP& bm, BOOL bIcon);
BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm);
BOOL PackedDIB_Extract(LPCWSTR FileName, const void *ptr, size_t siz, BOOL WritePNG);
HBITMAP PackedDIB_CreateBitmapFromMemory(const void *ptr, size_t siz);

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef PACKED_DIB_HPP_
