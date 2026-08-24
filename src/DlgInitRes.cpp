// DlgInitRes.cpp --- DLGINIT Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "DlgInitRes.hpp"
#ifndef NO_CONSTANTS_DB
	#include "ConstantsDB.hpp"
#endif

bool DlgInitRes::LoadFromStream(const MByteStreamEx& stream)
{
	m_entries.clear();

	WORD wCtrl;
	while (stream.ReadWord(wCtrl) && wCtrl)
	{
		DlgInitEntry entry;
		entry.wCtrl = wCtrl;

		int32_t cchLen;
		if (!stream.ReadWord(entry.wMsg) || !stream.ReadDword(cchLen))
			return false;

		if (cchLen < 0 || cchLen > int32_t(LONG_MAX / sizeof(WCHAR)))
			return false;

		if (cchLen)
		{
			// NOTE: cchLen bytes are read from the stream, and per the
			// RT_DLGINIT format the last of those bytes is expected to be
			// a NUL terminator. Size the buffer to the full cchLen so the
			// read never writes past the string's logical end (writing
			// into std::string's implicit terminator slot with anything
			// other than '\0' is undefined behavior, and untrusted/corrupt
			// resource data cannot be trusted to end with a real 0 byte).
			// Then drop the trailing byte to get the logical string.
			entry.strText.resize(cchLen);
			if (!stream.ReadData(&entry.strText[0], cchLen))
				return false;
			entry.strText.resize(cchLen - 1);
		}

		m_entries.push_back(entry);
	}

	return true;
}

bool DlgInitRes::SaveToStream(MByteStreamEx& stream) const
{
	for (auto& entry : m_entries)
	{
		DWORD dwLen = DWORD(entry.strText.size() + 1);
		if (!stream.WriteWord(entry.wCtrl) ||
			!stream.WriteWord(entry.wMsg) ||
			!stream.WriteDword(dwLen) ||
			!stream.WriteData(entry.strText.c_str(), dwLen))
		{
			return false;
		}
	}

	return stream.WriteWord(0);
}

MStringW DlgInitRes::Dump(const MIdOrString& id_or_str) const
{
	MStringW ret;

	if (id_or_str.is_str())
	{
		ret += id_or_str.str();
	}
	else
	{
#ifndef NO_CONSTANTS_DB
		ret += g_db.GetNameOfResID(IDTYPE_DIALOG, id_or_str.m_id);
#else
		ret += mstr_dec_short(id_or_str.m_id);
#endif
	}

	ret += L" 240\r\n";
	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	if (m_entries.size() == 0)
	{
		ret += L"    0\r\n";
		if (g_settings.bUseBeginEnd)
			ret += L"END\r\n";
		else
			ret += L"}\r\n";
		return ret;
	}

	for (auto& entry : m_entries)
	{
		ret += L"    ";
#ifndef NO_CONSTANTS_DB
		ret += g_db.GetCtrlOrCmdName(entry.wCtrl);
#else
		ret += mstr_dec_short(entry.wCtrl);
#endif
		ret += L", ";

// Win16 messages
#define WIN16_LB_ADDSTRING  0x0401
#define WIN16_CB_ADDSTRING  0x0403
#define AFX_CB_ADDSTRING    0x1234

		switch (entry.wMsg)
		{
		case WIN16_LB_ADDSTRING:
		case LB_ADDSTRING:
			ret += mstr_hex_word(LB_ADDSTRING);
			break;
		case WIN16_CB_ADDSTRING:
		case CB_ADDSTRING:
			ret += mstr_hex_word(CB_ADDSTRING);
			break;
		case AFX_CB_ADDSTRING:
		case CBEM_INSERTITEM:
			ret += mstr_hex_word(CBEM_INSERTITEM);
			break;
		default:
			ret += mstr_hex_word(entry.wMsg);
		}

		ret += L", ";
		ret += mstr_hex_word(WORD(entry.strText.size() + 1));
		ret += L", 0";

		auto pw = reinterpret_cast<const UNALIGNED WORD *>(entry.strText.c_str());
		size_t len = (entry.strText.size() + 1) / 2;
		for (size_t k = 0; k < len; ++k)
		{
			ret += L", ";
			ret += mstr_hex_word(pw[k]);
		}
		if (entry.strText.size() % 2 == 0)
		{
			ret += L", \"\\000\"";
		}
		ret += L", ";

		ret += L"\r\n";
	}

	ret += L"    0\r\n";
	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";

	return ret;
}

std::vector<BYTE> DlgInitRes::data() const
{
	MByteStreamEx stream;
	SaveToStream(stream);
	return stream.data();
}
