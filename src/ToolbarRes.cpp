// ToolbarRes.cpp --- TOOLBAR Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "ToolbarRes.hpp"
#ifndef NO_CONSTANTS_DB
	#include "ConstantsDB.hpp"
#endif

bool ToolbarRes::LoadFromStream(const MByteStreamEx& stream)
{
	m_items.clear();

	WORD wVer;
	if (!stream.PeekWord(wVer))
		return false;

	if (wVer == 1)
	{
		WORD wWidth, wHeight, wCount;
		if (!stream.ReadWord(wVer) ||
			!stream.ReadWord(wWidth) ||
			!stream.ReadWord(wHeight) ||
			!stream.ReadWord(wCount))
		{
			return false;
		}

		if (wVer != 1 || wWidth < 3 || wHeight < 3)
			return false;

		if (wWidth > 0x7FFF || wHeight > 0x7FFF || wCount > 0x7FFF)
			return false;

		for (DWORD i = 0; i < wCount; ++i)
		{
			WORD item;
			if (!stream.ReadWord(item))
			{
				m_items.clear();
				return false;
			}
			m_items.push_back(item);
		}

		m_width = wWidth;
		m_height = wHeight;
		return true;
	}
	else if (wVer >= 3)
	{
		// wVer and wWidth are overlapping
		uint32_t wWidth, wHeight, dwCount;
		if (!stream.ReadDword(wWidth) ||
			!stream.ReadDword(wHeight) ||
			!stream.ReadDword(dwCount))
		{
			return false;
		}

		if (wWidth < 3 || wHeight < 3)
			return false;

		if (wWidth > 0x7FFF || wHeight > 0x7FFF || dwCount > 0x7FFFFFFF)
			return false;

		for (DWORD i = 0; i < dwCount; ++i)
		{
			uint32_t item;
			if (!stream.ReadDword(item))
			{
				m_items.clear();
				return false;
			}
			m_items.push_back(item);
		}

		m_width = wWidth;
		m_height = wHeight;
		return true;
	}
	else
	{
		return false;
	}
}

bool ToolbarRes::SaveToStream(MByteStreamEx& stream) const
{
	if (m_items.size() > 0xFFFF)
		return false;

	if (!stream.WriteWord(WORD(1)) ||
		!stream.WriteWord(WORD(m_width)) ||
		!stream.WriteWord(WORD(m_height)) ||
		!stream.WriteWord(WORD(m_items.size())))
	{
		return false;
	}

	for (auto item : m_items)
	{
		if (!stream.WriteWord(WORD(item)))
			return false;
	}

	return true;
}

MStringW ToolbarRes::Dump(const MIdOrString& id_or_str) const
{
	MStringW ret;

	if (id_or_str.is_str())
	{
		ret += id_or_str.str();
	}
	else
	{
#ifndef NO_CONSTANTS_DB
		ret += g_db.GetNameOfResID(IDTYPE_BITMAP, id_or_str.m_id);
#else
		ret += mstr_dec_short(id_or_str.m_id);
#endif
	}

	ret += L" TOOLBAR ";
	ret += std::to_wstring(m_width);
	ret += L", ";
	ret += std::to_wstring(m_height);
	ret += L"\r\n";

	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	for (auto item : m_items)
	{
		if (item == 0)
		{
			ret += L"    SEPARATOR\r\n";
		}
		else
		{
			ret += L"    BUTTON ";
#ifndef NO_CONSTANTS_DB
			ret += g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, item, true);
#else
			ret += mstr_dec(item);
#endif
			ret += L"\r\n";
		}
	}

	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";

	return ret;
}

std::vector<BYTE> ToolbarRes::data() const
{
	MByteStreamEx stream;
	SaveToStream(stream);
	return stream.data();
}
