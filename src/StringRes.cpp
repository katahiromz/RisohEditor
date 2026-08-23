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
	WORD first, last;
	IdRangeFromName(wName, first, last);

	// Drop any previously-loaded entries for this block in one shot,
	// instead of erasing each of the 16 IDs individually.
	m_map.erase(m_map.lower_bound(first), m_map.upper_bound(last));

	WORD wLen;
	for (UINT i = 0; i < STRINGRES_BLOCK_SIZE; ++i)
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

			m_map[static_cast<WORD>(first + i)] = std::move(str);
		}
	}

	return true;
}

bool
StringRes::SaveToStream(MByteStreamEx& stream, WORD wName) const
{
	WORD first, last;
	IdRangeFromName(wName, first, last);

	for (UINT i = first; i <= last; ++i)
	{
		WORD wLen = 0;
		const WCHAR *pch = NULL;
		auto it = m_map.find(static_cast<WORD>(i));
		if (it != m_map.end())
		{
			wLen = WORD(it->second.size());
			if (wLen)
				pch = it->second.c_str();
		}
		if (!stream.WriteWord(wLen) ||
			(wLen && !stream.WriteData(pch, wLen * sizeof(WCHAR))))
		{
			return false;
		}
	}

	return true;
}

// Shared rendering logic used by both Dump() overloads, avoiding the
// previous copy-pasted STRINGTABLE/BEGIN.../END formatting code.
StringRes::string_type
StringRes::DumpRange(map_type::const_iterator itBegin, map_type::const_iterator itEnd) const
{
	string_type ret;

	ret += L"STRINGTABLE\r\n";
	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	for (auto it = itBegin; it != itEnd; ++it)
	{
		if (it->second.empty())
			continue;

		ret += L"    ";
#ifndef NO_CONSTANTS_DB
		ret += g_db.GetNameOfResID(IDTYPE_STRING, IDTYPE_PROMPT, it->first);
#else
		ret += mstr_dec_short(it->first);
#endif

		ret += L" \"";
		ret += mstr_escape_with_wrap(it->second);
		ret += L"\"\r\n";
	}

	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";

	return ret;
}

StringRes::string_type
StringRes::Dump(WORD wName) const
{
	WORD first, last;
	IdRangeFromName(wName, first, last);

	return DumpRange(m_map.lower_bound(first), m_map.upper_bound(last));
}

StringRes::string_type
StringRes::Dump() const
{
	return DumpRange(m_map.cbegin(), m_map.cend());
}
