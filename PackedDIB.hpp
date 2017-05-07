#ifndef PACKED_DIB_HPP_
#define PACKED_DIB_HPP_

#include <windows.h>
#include <vector>

DWORD PackedDIB_GetBitsOffset(const void *pPackedDIB, DWORD Size);
BOOL PackedDIB_GetInfo(const void *pPackedDIB, DWORD Size, BITMAP& bm);
HBITMAP PackedDIB_CreateBitmap(const void *pPackedDIB, DWORD Size);
HICON PackedDIB_CreateIcon(const void *pPackedDIB, DWORD Size, BITMAP& bm, BOOL bIcon);
BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& Data, HBITMAP hbm);

#endif  // ndef PACKED_DIB_HPP_
