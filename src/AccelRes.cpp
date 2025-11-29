// AccelRes.cpp --- Accelerator-Table Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later
//////////////////////////////////////////////////////////////////////////////

#include "AccelRes.hpp"
#include "ConstantsDB.hpp"

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

		if (entry.fFlags & 0x80)
			break;
	}

	return true;
}

void AccelRes::Update()
{
	size_t i, count = m_entries.size();
	for (i = 0; i < count; ++i)
	{
		entry_type& entry = m_entries[i];

		if (i + 1 == count)
			entry.fFlags |= 0x80;
		else
			entry.fFlags &= ~0x80;
	}
}

std::vector<BYTE> AccelRes::data() const
{
	size_t size = m_entries.size() * sizeof(entry_type);
	const BYTE *pb = (const BYTE *)&m_entries[0];
	return std::vector<BYTE>(pb, pb + size);
}

MStringW AccelRes::Dump(const MIdOrString &id_or_str) const
{
	MStringW ret;

	if (id_or_str.m_id == 0)
	{
		ret += id_or_str.str();
	}
	else
	{
		ret += g_db.GetNameOfResID(IDTYPE_ACCEL, id_or_str.m_id);
	}
	ret += L" ";
	ret += L"ACCELERATORS\r\n";
	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	for (auto& entry : m_entries)
	{
		bool VIRTKEY = !!(entry.fFlags & FVIRTKEY);
		bool NOINVERT = !!(entry.fFlags & FNOINVERT);
		bool SHIFT = !!(entry.fFlags & FSHIFT);
		bool CONTROL = !!(entry.fFlags & FCONTROL);
		bool ALT = !!(entry.fFlags & FALT);

		ret += L"    ";
		if (VIRTKEY)
		{
			ret += g_db.GetName(L"VIRTUALKEYS", entry.wAscii);
		}
		else
		{
			std::string str;
			str += (char)entry.wAscii;
			ret += MAnsiToWide(CP_ACP, mstr_quote(str));
		}
		ret += L", ";
		if (0)
		{
			ret += mstr_dec_word(entry.wId);
		}
		else
		{
			ret += g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND,
									   entry.wId, true);
		}

		if (NOINVERT)
			ret += L", NOINVERT";
		if (ALT)
			ret += L", ALT";
		if (CONTROL)
			ret += L", CONTROL";
		if (SHIFT)
			ret += L", SHIFT";

		if (VIRTKEY)
			ret += L", VIRTKEY";
		else
			ret += L", ASCII";

		ret += L"\r\n";
	}

	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";
	return ret;
}
