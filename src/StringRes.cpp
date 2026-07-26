// StringRes.cpp --- String Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "StringRes.hpp"
#ifndef NO_CONSTANTS_DB
	#include "ConstantsDB.hpp"
#endif

bool
StringRes::LoadFromStream(const MByteStreamEx& stream, WORD wName)
{
	for (UINT i = 0; i < 16; ++i)
	{
		m_map.erase((wName - 1) * 16 + i);
	}

	WORD wLen;
	for (UINT i = 0; i < 16; ++i)
	{
		if (!stream.ReadWord(wLen))
			return false;

		if (wLen > 0x7FFF)
			return false;

		if (wLen > 0)
		{
			string_type str(wLen, 0);
			if (!stream.ReadData(&str[0], wLen * sizeof(WCHAR)))
				return false;

			m_map[(wName - 1) * 16 + i] = std::move(str);
		}
	}

	return true;
}

bool
StringRes::SaveToStream(MByteStreamEx& stream, WORD wName)
{
	WORD first, last;
	IdRangeFromName(wName, first, last);

	for (UINT i = first; i <= last; ++i)
	{
		const string_type& str = m_map[i];
		WORD wLen = WORD(str.size());
		if (!stream.WriteWord(wLen) ||
			!stream.WriteData(&str[0], wLen * sizeof(WCHAR)))
			return false;
	}

	return true;
}

StringRes::string_type
StringRes::Dump(WORD wName)
{
	string_type ret;

	ret += L"STRINGTABLE\r\n";
	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	WORD first, last;
	IdRangeFromName(wName, first, last);
	for (UINT i = first; i <= last; ++i)
	{
		if (m_map[i].empty())
			continue;

		ret += L"    ";
		if (0)
		{
			ret += mstr_dec_word(i);
		}
		else
		{
#ifndef NO_CONSTANTS_DB
			ret += g_db.GetNameOfResID(IDTYPE_STRING, IDTYPE_PROMPT, i);
#else
			ret += mstr_dec_short(i);
#endif
		}

		ret += L", \"";
		ret += mstr_escape_with_wrap(m_map[i]);
		ret += L"\"\r\n";
	}

	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";

	return ret;
}

StringRes::string_type
StringRes::Dump()
{
	string_type ret;

	ret += L"STRINGTABLE\r\n";
	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	for (auto& pair : m_map)
	{
		if (pair.second.empty())
			continue;

		ret += L"    ";
		if (0)
		{
			ret += mstr_dec_word(pair.first);
		}
		else
		{
#ifndef NO_CONSTANTS_DB
			ret += g_db.GetNameOfResID(IDTYPE_STRING, IDTYPE_PROMPT, pair.first);
#else
			ret += mstr_dec_short(pair.first);
#endif
		}

		ret += L" \"";
		ret += mstr_escape_with_wrap(pair.second);
		ret += L"\"\r\n";
	}

	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";

	return ret;
}
