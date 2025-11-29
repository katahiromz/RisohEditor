// ResHeader.hpp --- Win32 Resource Header
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MIdOrString.hpp"
#include "MByteStreamEx.hpp"

class ResHeader;

//////////////////////////////////////////////////////////////////////////////

#define    MEMORYFLAG_MOVEABLE     0x0010
#define    MEMORYFLAG_PURE         0x0020
#define    MEMORYFLAG_PRELOAD      0x0040
#define    MEMORYFLAG_DISCARDABLE  0x1000

class ResHeader
{
public:
	DWORD           DataSize;
	DWORD           HeaderSize;
	MIdOrString     type;
	MIdOrString     name;
	DWORD           DataVersion;
	WORD            MemoryFlags;
	WORD            LanguageId;
	DWORD           Version;
	DWORD           Characteristics;

	ResHeader()
	{
		DataSize = 0;
		HeaderSize = 0x20;
		type = (WORD)0;
		name = (WORD)0;
		DataVersion = 0;
		MemoryFlags = 0;
		LanguageId = 0;
		Version = 0;
		Characteristics = 0;
	}

	bool ReadFrom(const MByteStreamEx& bs)
	{
		if (!bs.ReadRaw(DataSize) || !bs.ReadRaw(HeaderSize) ||
			!bs.ReadID(type) || !bs.ReadID(name))
		{
			return false;
		}
		bs.ReadDwordAlignment();

		if (!bs.ReadRaw(DataVersion) || !bs.ReadRaw(MemoryFlags) ||
			!bs.ReadRaw(LanguageId) || !bs.ReadRaw(Version) ||
			!bs.ReadRaw(Characteristics))
		{
			return false;
		}
		bs.ReadDwordAlignment();

		return true;
	}

	bool WriteTo(MByteStreamEx& bs) const
	{
		if (!bs.WriteRaw(DataSize) || !bs.WriteRaw(HeaderSize) ||
			!bs.WriteID(type) || !bs.WriteID(name))
		{
			return false;
		}
		bs.WriteDwordAlignment();

		if (!bs.WriteRaw(DataVersion) || !bs.WriteRaw(MemoryFlags) ||
			!bs.WriteRaw(LanguageId) || !bs.WriteRaw(Version) ||
			!bs.WriteRaw(Characteristics))
		{
			return false;
		}
		bs.WriteDwordAlignment();

		return true;
	}

	DWORD GetHeaderSize(const MIdOrString& type, MIdOrString name) const
	{
		size_t size = 0;
		if (type.is_str())
			size += (type.m_str.size() + 1) * sizeof(WCHAR);
		else
			size += sizeof(WORD) * 2;

		if (name.is_str())
			size += (name.m_str.size() + 1) * sizeof(WCHAR);
		else
			size += sizeof(WORD) * 2;

		if (size & 3)
			size += 4 - (size & 3);

		return DWORD(sizeof(DWORD) * 6 + size);
	}
};
