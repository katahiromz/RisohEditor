// AccelRes.cpp --- Accelerator-Table Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later
//////////////////////////////////////////////////////////////////////////////

#include "AccelRes.hpp"
#ifndef NO_CONSTANTS_DB
	#include "ConstantsDB.hpp"
#endif

static inline void AppendAccelFlags(MStringW& ret, WORD fFlags)
{
    if (fFlags & FNOINVERT) ret += L", NOINVERT";
    if (fFlags & FALT)      ret += L", ALT";
    if (fFlags & FCONTROL)  ret += L", CONTROL";
    if (fFlags & FSHIFT)    ret += L", SHIFT";
    ret += (fFlags & FVIRTKEY) ? L", VIRTKEY" : L", ASCII";
}

bool AccelRes::LoadFromStream(const MByteStreamEx& stream)
{
	m_entries.clear();
	if (stream.size() < sizeof(entry_type))
		return false;

	entry_type entry;
	size_t i, count = stream.size() / sizeof(entry_type);
	for (i = 0; i < count; ++i)
	{
		if (!stream.ReadRaw(entry))
			return false;

		m_entries.push_back(entry);

		if (entry.fFlags & ACCEL_LAST)
			break;
	}

	return true;
}

void AccelRes::Update()
{
	if (m_entries.empty())
		return;

	for (auto& entry : m_entries)
		entry.fFlags &= ~ACCEL_LAST;

	m_entries.back().fFlags |= ACCEL_LAST;
}

std::vector<BYTE> AccelRes::data() const
{
	if (m_entries.empty())
		return {};
	const auto* pb = reinterpret_cast<const BYTE*>(m_entries.data());
	return {pb, pb + m_entries.size() * sizeof(entry_type)};
}

MStringW AccelRes::Dump(const MIdOrString &id_or_str) const
{
	MStringW ret;

	if (id_or_str.is_str())
	{
		ret += id_or_str.str();
	}
	else
	{
#ifdef NO_CONSTANTS_DB
		ret += mstr_dec_short(id_or_str.m_id);
#else
		ret += g_db.GetNameOfResID(IDTYPE_ACCEL, id_or_str.m_id);
#endif
	}
	ret += L" ";
	ret += L"ACCELERATORS\r\n";
	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	for (auto& entry : m_entries)
	{
		ret += L"    ";
#ifndef NO_CONSTANTS_DB
		if (entry.fFlags & FVIRTKEY)
		{
			ret += g_db.GetName(L"VIRTUALKEYS", entry.wAscii);
		}
		else
#endif
		{
			std::string str;
			if (('A' <= entry.wAscii && entry.wAscii <= 'Z') ||
				('0' <= entry.wAscii && entry.wAscii <= '9'))
			{
				str += (char)entry.wAscii;
				ret += MAnsiToWide(CP_ACP, mstr_quote(str));
			}
			else
			{
				// NOTE: "^A" will be lost in windres compilation. We should avoid it.
				ret += L"\"\\x";
				ret += L"0123456789ABCDEF"[entry.wAscii & 0xF];
				ret += L"0123456789ABCDEF"[(entry.wAscii >> 4) & 0xF];
				ret += L"\"";
			}
		}
		ret += L", ";
#ifdef NO_CONSTANTS_DB
		ret += mstr_dec_word(entry.wId);
#else
		ret += g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND,
		                           entry.wId, true);
#endif

		AppendAccelFlags(ret, entry.fFlags);
		ret += L"\r\n";
	}

	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";
	return ret;
}
