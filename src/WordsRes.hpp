// WordsRes.hpp  --- Resource that uses WORDs
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#ifndef _INC_WINDOWS
	#include <windows.h>
#endif
#include <vector>
#include "MByteStreamEx.hpp"
#include "MIdOrString.hpp"

//////////////////////////////////////////////////////////////////////////////

struct WordsRes
{
	std::vector<BYTE> m_data;

	bool LoadFromStream(const MByteStreamEx& stream)
	{
		m_data.clear();

		WORD w;
		while (stream.ReadWord(w))
		{
			m_data.push_back(LOBYTE(w));
			m_data.push_back(HIBYTE(w));
		}
		return !m_data.empty();
	}

	bool SaveToStream(MByteStreamEx& stream) const
	{
		if (m_data.empty())
			return false;
		if (m_data.size() & 1)
			return false;

		return stream.WriteData(m_data.data(), m_data.size());
	}

	MStringW Dump(MIdOrString type, MIdOrString name) const
	{
		MStringW ret;

		if (name.is_str())
		{
			ret += name.str();
		}
		else
		{
#ifndef NO_CONSTANTS_DB
			ret += g_db.GetNameOfResID(IDTYPE_MENU, name.m_id, true);
#else
			ret += mstr_dec_short(name.m_id);
#endif
		}

		ret += L' ';

		if (type.is_str())
			ret += type.str();
		else
			ret += mstr_dec_short(type.m_id);

		ret += L"\r\n";
		ret += g_settings.bUseBeginEnd ? L"BEGIN\r\n" : L"{\r\n";

		const WORD* pw = reinterpret_cast<const WORD*>(m_data.data());
		const size_t cw = m_data.size() / sizeof(WORD);
		const size_t columns = 6;
		static const WCHAR hex[] = L"0123456789ABCDEF";

		for (size_t iw = 0; iw < cw; ++iw)
		{
			if (iw % columns == 0)
				ret += L"    ";
			else
				ret += L", ";

			WORD w = pw[iw];
			ret += L"0x";
			ret += hex[(w >> 12) & 0xF];
			ret += hex[(w >>  8) & 0xF];
			ret += hex[(w >>  4) & 0xF];
			ret += hex[ w        & 0xF];

			if (iw + 1 == cw)
				ret += L"\r\n";
			else if ((iw + 1) % columns == 0)
				ret += L",\r\n";
		}

		ret += g_settings.bUseBeginEnd ? L"END\r\n" : L"}\r\n";
		return ret;
	}
};
