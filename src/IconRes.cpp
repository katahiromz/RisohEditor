// IconRes.cpp --- Icon Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "IconRes.hpp"

bool IconFile::LoadFromStream(const MByteStreamEx& stream)
{
	DWORD size = sizeof(ICONDIR);
	if (stream.size() < size)
		return false;

	memcpy(&m_dir, stream.ptr(), size);
	if (!IsIconDirOK())
		return false;

	stream.pos(size);
	size = m_dir.idCount * sizeof(EntryType);
	if (stream.remainder() < size)
		return false;

	m_entries.resize(m_dir.idCount);
	memcpy(&m_entries[0], stream.ptr(stream.pos()), size);
	m_images.resize(m_dir.idCount);

	for (int i = 0; i < m_dir.idCount; i++)
	{
		stream.pos(m_entries[i].dwImageOffset);
		if (stream.remainder() < m_entries[i].dwBytesInRes)
		{
			return false;
		}

		LPBYTE pb = (LPBYTE)stream.ptr(stream.pos());
		m_images[i].assign(&pb[0], &pb[m_entries[i].dwBytesInRes]);
	}
	return true;
}

bool IconFile::SaveToStream(MByteStreamEx& stream)
{
	stream.clear();
	if (!IsIconDirOK())
		return false;

	DWORD offset = sizeof(ICONDIR);
	const DWORD SizeOfEntries = GetImageCount() * sizeof(EntryType);
	offset += SizeOfEntries;

	const DWORD nCount = GetImageCount();
	for (DWORD i = 0; i < nCount; ++i)
	{
		m_entries[i].dwImageOffset = offset;
		offset += m_entries[i].dwBytesInRes;
	}

	if (!stream.WriteRaw(m_dir) ||
		!stream.WriteData(&m_entries[0], SizeOfEntries))
	{
		return false;
	}

	for (DWORD i = 0; i < nCount; ++i)
	{
		if (!stream.WriteData(GetImagePtr(i), GetImageSize(i)))
			return false;
	}

	return true;
}

#include <intrin.h> // for _byteswap_ulong

IconFile::DataType
IconFile::GetIconGroup(int nBaseID) const
{
	DataType group(SizeOfIconGroup());

	memcpy(&group[0], &m_dir, sizeof(ICONDIR));

	int offset = sizeof(ICONDIR);
	for (int i = 0; i < GetImageCount(); i++)
	{
		if (GetImageSize(i) > 25 &&
			memcmp(GetImagePtr(i), "\x89PNG", 4) == 0)
		{
			ResourceEntryType grpEntry;
			grpEntry.bWidth = (BYTE)_byteswap_ulong(*(uint32_t*)&GetImagePtr(i)[16]);
			grpEntry.bHeight = (BYTE)_byteswap_ulong(*(uint32_t*)&GetImagePtr(i)[20]);
			grpEntry.bColorCount = 0;
			grpEntry.bReserved = 0;
			grpEntry.wPlanes = 1;
			uint8_t bit_depth = *(uint8_t*)&GetImagePtr(i)[24];
			uint8_t color_type = *(uint8_t*)&GetImagePtr(i)[25];
			if (color_type & 4)
				grpEntry.wBitCount = 32;
			else if (color_type & 2)
				grpEntry.wBitCount = 24;
			else
				grpEntry.wBitCount = bit_depth;
			grpEntry.dwBytesInRes = m_entries[i].dwBytesInRes;
			grpEntry.nID = WORD(nBaseID + i);
			memcpy(&group[offset], &grpEntry, sizeof(grpEntry));
			offset += sizeof(grpEntry);
			continue;
		}

		BITMAPCOREHEADER    bmch;
		BITMAPINFOHEADER    bmih;

		memcpy(&bmch, GetImagePtr(i), sizeof(bmch));
		bool bCoreOnly = (bmch.bcSize == sizeof(bmch));
		if (!bCoreOnly)
		{
			memcpy(&bmih, GetImagePtr(i), sizeof(bmih));
		}

		ResourceEntryType  grpEntry;
		grpEntry.bWidth             = m_entries[i].bWidth;
		grpEntry.bHeight            = m_entries[i].bHeight;
		grpEntry.bColorCount        = m_entries[i].bColorCount;
		grpEntry.bReserved          = m_entries[i].bReserved;

		if (bCoreOnly)
		{
			if (grpEntry.bWidth == 0)
				grpEntry.bWidth     = (BYTE)bmch.bcWidth;
			if (grpEntry.bHeight == 0)
				grpEntry.bHeight    = (BYTE)bmch.bcHeight;

			grpEntry.wPlanes        = bmch.bcPlanes;
			grpEntry.wBitCount      = bmch.bcBitCount;
		}
		else
		{
			if (grpEntry.bWidth == 0)
				grpEntry.bWidth     = (BYTE)bmih.biWidth;
			if (grpEntry.bHeight == 0)
				grpEntry.bHeight    = (BYTE)bmih.biHeight;

			grpEntry.wPlanes        = bmih.biPlanes;
			grpEntry.wBitCount      = bmih.biBitCount;
		}

		grpEntry.dwBytesInRes       = m_entries[i].dwBytesInRes;
		grpEntry.nID                = WORD(nBaseID + i);

		memcpy(&group[offset], &grpEntry, sizeof(grpEntry));
		offset += sizeof(grpEntry);
	}

	return group;
}

//////////////////////////////////////////////////////////////////////////////

bool CursorFile::LoadFromStream(const MByteStreamEx& stream)
{
	DWORD size = sizeof(ICONDIR);
	if (stream.size() < size)
		return false;

	memcpy(&m_dir, stream.ptr(), size);
	if (!IsIconDirOK())
		return false;

	stream.pos(size);
	size = m_dir.idCount * sizeof(EntryType);
	if (stream.remainder() < size)
		return false;

	m_entries.resize(m_dir.idCount);
	memcpy(&m_entries[0], stream.ptr(stream.pos()), size);
	m_images.resize(m_dir.idCount);

	for (int i = 0; i < m_dir.idCount; i++)
	{
		stream.pos(m_entries[i].dwImageOffset);
		if (stream.remainder() < m_entries[i].dwBytesInRes)
		{
			return false;
		}

		LOCALHEADER local;
		local.xHotSpot = m_entries[i].xHotSpot;
		local.yHotSpot = m_entries[i].yHotSpot;
		LPBYTE pb = (LPBYTE)stream.ptr(stream.pos());

		MByteStreamEx bs;
		if (!bs.WriteRaw(local))
			return false;
		if (!bs.WriteData(pb, m_entries[i].dwBytesInRes))
			return false;

		m_entries[i].dwBytesInRes += sizeof(local);
		m_images[i] = bs.data();
	}
	return true;
}

bool CursorFile::SaveToStream(MByteStreamEx& stream)
{
	stream.clear();
	if (!IsIconDirOK())
		return false;

	DWORD offset = sizeof(ICONDIR);
	const DWORD SizeOfEntries = GetImageCount() * sizeof(EntryType);
	offset += SizeOfEntries;

	const DWORD nCount = GetImageCount();
	for (DWORD i = 0; i < nCount; ++i)
	{
		m_entries[i].dwImageOffset = offset;
		offset += m_entries[i].dwBytesInRes;
	}

	if (!stream.WriteRaw(m_dir) ||
		!stream.WriteData(&m_entries[0], SizeOfEntries))
	{
		return false;
	}

	for (DWORD i = 0; i < nCount; ++i)
	{
		LPBYTE pb = GetImagePtr(i) + sizeof(LOCALHEADER);
		DWORD size = GetImageSize(i) - sizeof(LOCALHEADER);
		if (!stream.WriteData(pb, size))
			return false;
	}

	return true;
}

CursorFile::DataType
CursorFile::GetCursorGroup(int nBaseID) const
{
	DataType group(SizeOfCursorGroup());

	memcpy(&group[0], &m_dir, sizeof(ICONDIR));

	int offset = sizeof(ICONDIR);
	for (int i = 0; i < GetImageCount(); i++)
	{
		BITMAPCOREHEADER    bmch;
		BITMAPINFOHEADER    bmih;
		LOCALHEADER         local;

		const BYTE *pb = GetImagePtr(i);
		memcpy(&local, pb, sizeof(local));
		memcpy(&bmch, pb + sizeof(local), sizeof(bmch));

		bool bCoreOnly = (bmch.bcSize == sizeof(bmch));
		if (!bCoreOnly)
		{
			memcpy(&bmih, pb + sizeof(local), sizeof(bmih));
		}

		ResourceEntryType  grpEntry;
		grpEntry.wWidth             = m_entries[i].bWidth;
		grpEntry.wHeight            = m_entries[i].bHeight;

		if (bCoreOnly)
		{
			if (grpEntry.wWidth == 0)
				grpEntry.wWidth     = (WORD)bmch.bcWidth;
			if (grpEntry.wHeight == 0)
				grpEntry.wHeight    = (WORD)bmch.bcHeight;

			grpEntry.wPlanes        = bmch.bcPlanes;
			grpEntry.wBitCount      = bmch.bcBitCount;
		}
		else
		{
			if (grpEntry.wWidth == 0)
				grpEntry.wWidth     = (WORD)bmih.biWidth;
			if (grpEntry.wHeight == 0)
				grpEntry.wHeight    = (WORD)bmih.biHeight;

			grpEntry.wPlanes        = bmih.biPlanes;
			grpEntry.wBitCount      = bmih.biBitCount;
		}

		grpEntry.wHeight           *= 2;
		grpEntry.dwBytesInRes       = m_entries[i].dwBytesInRes;
		grpEntry.nID                = WORD(nBaseID + i);

		memcpy(&group[offset], &grpEntry, sizeof(grpEntry));
		offset += sizeof(grpEntry);
	}

	return group;
}
