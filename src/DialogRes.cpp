// DialogRes.cpp --- Dialog Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "DialogRes.hpp"
#include "ConstantsDB.hpp"
#include "RisohSettings.hpp"
#include <shlwapi.h>
#include <unordered_set>

bool PredefClassToID(MStringW name, WORD& w)
{
	w = 0;
	CharUpperW(&name[0]);
	if (name == L"BUTTON")
		w = 0x0080;
	else if (name == L"EDIT")
		w = 0x0081;
	else if (name == L"STATIC")
		w = 0x0082;
	else if (name == L"LISTBOX")
		w = 0x0083;
	else if (name == L"SCROLLBAR")
		w = 0x0084;
	else if (name == L"COMBOBOX")
		w = 0x0085;
	return w != 0;
}

bool IDToPredefClass(WORD w, MStringW& name)
{
	switch (w)
	{
	case 0x0080:
		name = L"BUTTON";
		return true;
	case 0x0081:
		name = L"EDIT";
		return true;
	case 0x0082:
		name = L"STATIC";
		return true;
	case 0x0083:
		name = L"LISTBOX";
		return true;
	case 0x0084:
		name = L"SCROLLBAR";
		return true;
	case 0x0085:
		name = L"COMBOBOX";
		return true;
	}
	return false;
}

void FixClassName(MStringW& cls)
{
	ConstantsDB::TableType table = g_db.GetTable(L"CONTROL.CLASSES");
	for (auto& table_entry : table)
	{
		if (lstrcmpiW(table_entry.name.c_str(), cls.c_str()) == 0)
		{
			cls = table_entry.name;
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

DialogItem::DialogItem()
{
	m_help_id = 0;
	m_style = 0;
	m_ex_style = 0;
	m_pt.x = 0;
	m_pt.y = 0;
	m_siz.cx = 0;
	m_siz.cy = 0;
	m_id = 0;
}

bool DialogItem::LoadFromStream(const MByteStreamEx& stream, bool extended)
{
	if (extended)
		return LoadFromStreamEx(stream);

	stream.ReadDwordAlignment();

	DLGITEMTEMPLATE item;
	if (!stream.ReadRaw(item))
		return false;

	m_help_id = 0;
	m_style = item.style;
	m_ex_style = item.dwExtendedStyle;
	m_pt.x = item.x;
	m_pt.y = item.y;
	m_siz.cx = item.cx;
	m_siz.cy = item.cy;
	m_id = item.id;

	if (!stream.ReadString(m_class) ||
		!stream.ReadString(m_title))
	{
		return false;
	}

	BYTE b;
	if (!stream.ReadByte(b))
		return false;

	if (b)
	{
		m_extra.resize(b);
		if (!stream.ReadData(&m_extra[0], b))
			return false;
	}

	return true;
}

bool DialogItem::LoadFromStreamEx(const MByteStreamEx& stream)
{
	stream.ReadDwordAlignment();

	DLGITEMTEMPLATEEXHEAD item;
	if (!stream.ReadRaw(item))
	{
		return false;
	}

	m_help_id = item.helpID;
	m_style = item.style;
	m_ex_style = item.exStyle;
	m_pt.x = item.x;
	m_pt.y = item.y;
	m_siz.cx = item.cx;
	m_siz.cy = item.cy;
	m_id = item.id;

	stream.ReadDwordAlignment();

	if (!stream.ReadString(m_class) || !stream.ReadString(m_title))
	{
		return false;
	}

	WORD extraCount;
	if (!stream.ReadWord(extraCount))
		return false;

	if (extraCount)
	{
		m_extra.resize(extraCount);
		if (!stream.ReadData(&m_extra[0], extraCount))
			return false;
	}

	return true;
}

bool DialogItem::SaveToStream(MByteStreamEx& stream, bool extended) const
{
	if (extended)
	{
		return SaveToStreamEx(stream);
	}

	stream.WriteDwordAlignment();

	DLGITEMTEMPLATE item;
	item.style = m_style;
	item.dwExtendedStyle = m_ex_style;
	item.x = (SHORT)m_pt.x;
	item.y = (SHORT)m_pt.y;
	item.cx = (SHORT)m_siz.cx;
	item.cy = (SHORT)m_siz.cy;
	item.id = m_id;
	if (!stream.WriteData(&item, sizeof(item)))
		return false;

	WORD w;
	if (!IS_INTRESOURCE(m_class.ptr()) &&
		PredefClassToID(m_class.ptr(), w))
	{
		if (!stream.WriteString(MAKEINTRESOURCEW(w)))
			return false;
	}
	else
	{
		if (!stream.WriteString(m_class.ptr()))
			return false;
	}

	if (!stream.WriteString(m_title.ptr()))
		return false;

	BYTE b = BYTE(m_extra.size());
	if (!stream.WriteRaw(b))
		return false;

	if (b)
	{
		stream.WriteDwordAlignment();
		if (!stream.WriteData(&m_extra[0], b))
			return false;
	}

	return true;
}

bool DialogItem::SaveToStreamEx(MByteStreamEx& stream) const
{
	stream.WriteDwordAlignment();

	DLGITEMTEMPLATEEXHEAD ItemEx;
	ItemEx.helpID = m_help_id;
	ItemEx.exStyle = m_ex_style;
	ItemEx.style = m_style;
	ItemEx.x = (short)m_pt.x;
	ItemEx.y = (short)m_pt.y;
	ItemEx.cx = (short)m_siz.cx;
	ItemEx.cy = (short)m_siz.cy;
	ItemEx.id = m_id;
	if (!stream.WriteRaw(ItemEx))
		return false;

	stream.WriteDwordAlignment();

	WORD w;
	if (!IS_INTRESOURCE(m_class.ptr()) &&
		PredefClassToID(m_class.ptr(), w))
	{
		if (!stream.WriteString(MAKEINTRESOURCEW(w)))
			return false;
	}
	else
	{
		if (!stream.WriteString(m_class.ptr()))
			return false;
	}

	if (!stream.WriteString(m_title.ptr()) ||
		!stream.WriteWord(WORD(m_extra.size())))
	{
		return false;
	}

	if (m_extra.size() > 0)
	{
		WORD ExtraSize = WORD(m_extra.size());
		if (!stream.WriteData(&m_extra[0], ExtraSize))
			return false;
	}

	return true;
}

MStringW DialogItem::Dump(bool bAlwaysControl) const
{
	MStringW cls;

	if (m_class.is_int())
	{
		if (!IDToPredefClass(m_class.m_id, cls))
			cls = mstr_dec_short(m_class.m_id);
	}
	else
	{
		cls = m_class.str();
	}

	if (!bAlwaysControl)
	{
		if (lstrcmpiW(cls.c_str(), L"BUTTON") == 0)
		{
#ifndef BS_TYPEMASK
#define BS_TYPEMASK     0x0000000F
#endif
			if ((m_style & BS_TYPEMASK) == BS_AUTO3STATE)
				return _do_AUTO3STATE();
			if ((m_style & BS_TYPEMASK) == BS_AUTOCHECKBOX)
				return _do_AUTOCHECKBOX();
			if ((m_style & BS_TYPEMASK) == BS_AUTORADIOBUTTON)
				return _do_AUTORADIOBUTTON();
			if ((m_style & BS_TYPEMASK) == BS_CHECKBOX)
				return _do_CHECKBOX();
			if ((m_style & BS_TYPEMASK) == BS_DEFPUSHBUTTON)
				return _do_DEFPUSHBUTTON();
			if ((m_style & BS_TYPEMASK) == BS_GROUPBOX)
				return _do_GROUPBOX();
			if ((m_style & BS_TYPEMASK) == BS_PUSHBUTTON || (m_style & BS_TYPEMASK) == BS_OWNERDRAW)
				return _do_PUSHBUTTON();
			if ((m_style & BS_TYPEMASK) == BS_PUSHBOX ||
				(m_style & BS_TYPEMASK) == 0xC)
			{
				return _do_PUSHBOX();
			}
			if ((m_style & BS_TYPEMASK) == BS_RADIOBUTTON)
				return _do_RADIOBUTTON();
			if ((m_style & BS_TYPEMASK) == BS_3STATE)
				return _do_STATE3();
		}
		if (lstrcmpiW(cls.c_str(), L"STATIC") == 0)
		{
			if ((m_style & SS_TYPEMASK) == SS_LEFT)
				return _do_LTEXT();
			if ((m_style & SS_TYPEMASK) == SS_CENTER)
				return _do_CTEXT();
			if ((m_style & SS_TYPEMASK) == SS_RIGHT)
				return _do_RTEXT();
			if ((m_style & SS_TYPEMASK) == SS_ICON)
				return _do_ICON();
		}
		if (m_title.empty())
		{
			if (lstrcmpiW(cls.c_str(), L"EDIT") == 0)
				return _do_EDITTEXT();
			if (lstrcmpiW(cls.c_str(), L"COMBOBOX") == 0)
				return _do_COMBOBOX();
			if (lstrcmpiW(cls.c_str(), L"LISTBOX") == 0)
				return _do_LISTBOX();
			if (lstrcmpiW(cls.c_str(), L"SCROLLBAR") == 0)
				return _do_SCROLLBAR();
		}
	}
	return DumpControl(cls);
}

MStringW DialogItem::DumpControl(MStringW& cls) const
{
	MStringW ret;

	ret += L"CONTROL ";

	if (IsStaticIcon())
	{
		ret += g_db.GetNameOfResID(IDTYPE_ICON, m_title.m_id);
	}
	else
	{
		ret += m_title.quoted_wstr_with_wrap();
	}

	ret += L", ";
	ret += g_db.GetNameOfResID(IDTYPE_CONTROL, m_id);
	ret += L", ";
	if (m_class.is_int())
	{
		if (IDToPredefClass(m_class.m_id, cls))
			ret += mstr_quote_with_wrap(cls);
		else
			ret += mstr_dec_short(m_class.m_id);
	}
	else
	{
		cls = m_class.str();
		FixClassName(cls);
		ret += mstr_quote_with_wrap(cls);
	}

	ret += L", ";
	{
		DWORD value = m_style;
		DWORD def_value = WS_CHILD | WS_VISIBLE;
		ret += g_db.DumpBitFieldOrZero(cls.c_str(), L"STYLE", value, def_value);
	}

	ret += L", ";
	ret += mstr_dec_short((SHORT)m_pt.x);
	ret += L", ";
	ret += mstr_dec_short((SHORT)m_pt.y);
	ret += L", ";
	ret += mstr_dec_short((SHORT)m_siz.cx);
	ret += L", ";
	ret += mstr_dec_short((SHORT)m_siz.cy);
	if (m_ex_style || m_help_id)
	{
		ret += L", ";
		DWORD value = m_ex_style;
		ret += g_db.DumpBitFieldOrZero(L"EXSTYLE", L"", value);
	}
	if (m_help_id)
	{
		ret += L", ";
		ret += g_db.GetNameOfResID(IDTYPE_HELP, m_help_id);
	}
	if (m_extra.size() && m_extra.size() % 2 == 0)
	{
		size_t count = m_extra.size() / sizeof(WORD);
		const WORD *pw = (const WORD *)&m_extra[0];
		if (g_settings.bUseBeginEnd)
			ret += L"\r\n    BEGIN\r\n";
		else
			ret += L"\r\n    {\r\n        ";
		ret += mstr_hex_word(pw[0]);
		for (size_t i = 1; i < count; ++i)
		{
			ret += L", ";
			ret += mstr_hex_word(pw[i]);
		}
		ret += L"\r\n    }";
	}

	return ret;
}

MStringW
DialogItem::_do_CONTROL(bool bNeedsText,
					   const MStringW& ctrl,
					   const MStringW& cls,
					   DWORD DefStyle) const
{
	MStringW ret;
	ret += ctrl;
	ret += L" ";

	if (!m_title.empty() || bNeedsText)
	{
		if (IsStaticIcon())
		{
			ret += g_db.GetNameOfResID(IDTYPE_ICON, m_title.m_id);
		}
		else
		{
			ret += m_title.quoted_wstr_with_wrap();
		}
		ret += L", ";
	}

	ret += g_db.GetNameOfResID(IDTYPE_CONTROL, m_id);
	ret += L", ";
	ret += mstr_dec_short((SHORT)m_pt.x);
	ret += L", ";
	ret += mstr_dec_short((SHORT)m_pt.y);
	// NOTE: Don't omit cx and cy! Visual Studio 2017 won't accept omission.
	//if (!IsStaticIcon() || m_siz.cx || m_siz.cy || m_style != DefStyle || m_ex_style || m_help_id)
	{
		ret += L", ";
		ret += mstr_dec_short((SHORT)m_siz.cx);
		ret += L", ";
		ret += mstr_dec_short((SHORT)m_siz.cy);
	}
	if (m_style != DefStyle || m_ex_style || m_help_id)
	{
		ret += L", ";
		DWORD value = m_style;
		if (ctrl == L"PUSHBOX" && (value & BS_TYPEMASK) == 0xC)
		{
			value &= ~BS_TYPEMASK;
			value |= BS_PUSHBOX;
		}
		// NOTE: RC won't add WS_TABSTOP. Microsoft document said a lie.
		std::wstring str = g_db.DumpBitFieldOrZero(cls.c_str(), L"STYLE", value, DefStyle);
		ret += str;
		if (ctrl == L"AUTORADIOBUTTON" && str.find(L"WS_TABSTOP") == std::wstring::npos)
		{
			if (m_style & WS_TABSTOP)
				ret += L" | WS_TABSTOP";
			else
				ret += L" | NOT WS_TABSTOP";
		}
	}
	else
	{
		// NOTE: RC won't add WS_TABSTOP. Microsoft document said a lie.
		if (ctrl == L"AUTORADIOBUTTON")
		{
			if (m_style & WS_TABSTOP)
				ret += L", WS_TABSTOP";
			else
				ret += L", NOT WS_TABSTOP";
		}
	}
	if (m_ex_style || m_help_id)
	{
		ret += L", ";
		DWORD value = m_ex_style;
		ret += g_db.DumpBitFieldOrZero(L"EXSTYLE", L"", value);
	}
	if (m_help_id)
	{
		ret += L", ";
		ret += g_db.GetNameOfResID(IDTYPE_HELP, m_help_id);
	}
	if (m_extra.size() && m_extra.size() % 2 == 0)
	{
		size_t count = m_extra.size() / sizeof(WORD);
		const WORD *pw = (const WORD *)&m_extra[0];
		if (g_settings.bUseBeginEnd)
			ret += L"\r\n    BEGIN\r\n";
		else
			ret += L"\r\n    {\r\n        ";
		ret += mstr_hex_word(pw[0]);
		for (size_t i = 1; i < count; ++i)
		{
			ret += L", ";
			ret += mstr_hex_word(pw[i]);
		}
		ret += L"\r\n    }";
	}
	return ret;
}

BOOL DialogItem::IsClassRegd(const WCHAR *name) const
{
	HMODULE hMod = ::GetModuleHandle(NULL);

	WNDCLASSEXW wcx;
	if (::GetClassInfoExW(NULL, name, &wcx) ||
		::GetClassInfoExW(hMod, name, &wcx))
	{
		return TRUE;
	}
	return FALSE;
}

void DialogItem::FixupForRad(bool bRevert)
{
	if (bRevert)
	{
		m_style = m_old_style;
		m_ex_style = m_old_ex_style;
		m_siz = m_sizOld;
		m_class = m_old_class;
		m_title = m_old_title;
	}
	else
	{
		m_old_style = m_style;
		m_old_ex_style = m_ex_style;

		m_style &= ~WS_DISABLED;
		m_style |= WS_CHILD | WS_VISIBLE;

		m_ex_style &= ~(WS_EX_ACCEPTFILES | WS_EX_LAYERED | WS_EX_TRANSPARENT |
						WS_EX_TOPMOST);
		m_ex_style |= WS_EX_NOACTIVATE;
		m_sizOld = m_siz;
		m_old_class = m_class;
		m_old_title = m_title;

		if (m_siz.cx == 0 && m_siz.cy == 0)
		{
			m_siz.cx = 20;
			m_siz.cy = 20;
		}
		if (StrCmpNIW(m_class.c_str(), TEXT("AtlAxWin"), 8) == 0)
		{
			m_class = L"STATIC";
			m_style |= WS_BORDER;
		}
		else if (m_class.c_str()[0] == L'{')
		{
#if 1
			m_title = m_class.c_str();
			m_class = L"MOleSite";
			m_style |= WS_BORDER;
#else
			m_title = m_class.c_str();
			m_class = L"STATIC";
			m_style |= WS_BORDER;
#endif
		}
		else if (m_class.is_str() && !IsClassRegd(m_class.c_str()))
		{
			m_class = L"STATIC";
			m_style &= ~SS_TYPEMASK;
			m_style |= SS_GRAYRECT;
		}
		if (m_class.m_id == 0x0080 ||
			lstrcmpiW(m_class.str().c_str(), L"BUTTON") == 0)
		{
			if ((m_style & BS_TYPEMASK) == BS_OWNERDRAW)
			{
				m_style &= ~BS_TYPEMASK;
				m_style |= BS_PUSHBUTTON;
			}
		}
		if (m_class.m_id == 0x0082 ||
			lstrcmpiW(m_class.str().c_str(), L"STATIC") == 0)
		{
			if ((m_style & SS_TYPEMASK) == SS_OWNERDRAW)
			{
				m_style &= ~SS_TYPEMASK;
				m_style |= SS_LEFT;
			}
		}
		if (m_class.m_id == 0x0083 ||
			lstrcmpiW(m_class.str().c_str(), L"LISTBOX") == 0)
		{
			if (m_style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))
			{
				m_style &= ~(LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE);
			}
		}
		if (m_class.m_id == 0x0085 ||
			lstrcmpiW(m_class.str().c_str(), L"COMBOBOX") == 0 ||
			lstrcmpiW(m_class.str().c_str(), WC_COMBOBOXEX) == 0)
		{
			if (m_style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
			{
				m_style &= ~(CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE);
			}
		}
	}
}

void DialogItem::FixupForTest(bool bRevert)
{
	if (bRevert)
	{
		m_class = m_old_class;
		m_style = m_old_style;
		m_title = m_old_title;
	}
	else
	{
		m_old_style = m_style;
		m_old_class = m_class;
		m_old_title = m_title;
#ifndef ATL_SUPPORT
		if (StrCmpNIW(m_class.c_str(), TEXT("AtlAxWin"), 8) == 0)
		{
			m_class = L"STATIC";
			m_style |= WS_BORDER;
		}
#endif
		if (m_class.c_str()[0] == L'{')
		{
#if 1
			m_title = m_class.c_str();
			m_class = L"MOleSite";
			m_style |= WS_BORDER;
#else
			m_title = m_class.c_str();
			m_class = L"STATIC";
			m_style |= WS_BORDER;
#endif
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

void DialogRes::ReplaceFont()
{
	m_replaced_type_face = m_type_face;
	auto& face = m_replaced_type_face;

	if (face.str() == g_settings.strFontReplaceFrom1)
		face = g_settings.strFontReplaceTo1.c_str();
	else if (face.str() == g_settings.strFontReplaceFrom2)
		face = g_settings.strFontReplaceTo2.c_str();
	else if (face.str() == g_settings.strFontReplaceFrom3)
		face = g_settings.strFontReplaceTo3.c_str();
}

DialogRes::DialogRes()
{
	m_version = 0;
	m_signature = 0;
	m_help_id = 0;
	m_style = 0;
	m_ex_style = 0;
	m_cItems = 0;
	m_pt.x = 0;
	m_pt.y = 0;
	m_siz.cx = 0;
	m_siz.cy = 0;
	m_point_size = 0;
	m_weight = FW_NORMAL;
	m_italic = false;
	m_charset = DEFAULT_CHARSET;
	m_lang = 0;
}

bool DialogRes::LoadFromStream(const MByteStreamEx& stream)
{
	if (stream.size() < sizeof(WORD) * 2)
		return false;

	if (*(WORD *)stream.ptr(0) == 1 &&
		*(WORD *)stream.ptr(2) == 0xFFFF)
	{
		// extended dialog
		if (_headerFromStreamEx(stream))
		{
			for (UINT i = 0; i < m_cItems; ++i)
			{
				DialogItem item;
				if (!item.LoadFromStreamEx(stream))
					return false;
				m_items.push_back(item);
			}
			return true;
		}
	}
	else
	{
		// normal dialog
		if (_headerFromStream(stream))
		{
			for (UINT i = 0; i < m_cItems; ++i)
			{
				DialogItem item;
				if (!item.LoadFromStream(stream))
					return false;
				m_items.push_back(item);
			}
			return true;
		}
	}
	return false;
}

bool DialogRes::SaveToStream(MByteStreamEx& stream) const
{
	if (IsExtended())
	{
		if (_headerToStreamEx(stream))
		{
			size_t i, count = m_items.size();
			for (i = 0; i < count; ++i)
			{
				if (!m_items[i].SaveToStreamEx(stream))
					return false;
			}
			stream.WriteByte(0);
			return true;
		}
	}
	else
	{
		if (_headerToStream(stream))
		{
			size_t i, count = m_items.size();
			for (i = 0; i < count; ++i)
			{
				if (!m_items[i].SaveToStream(stream))
					return false;
			}
			stream.WriteByte(0);
			return true;
		}
	}
	return false;
}

INT DialogRes::GetDlgItem(WORD id) const
{
	for (size_t k = 0; k < size(); ++k)
	{
		if ((*this)[k].m_id == id)
			return INT(k);
	}
	return -1;
}

bool DialogRes::LoadDlgInitData(const MByteStreamEx::data_type& data)
{
	MByteStreamEx stream(data);
	DlgInitRes dlginit;
	if (dlginit.LoadFromStream(stream))
	{
		for (auto& item : m_items)
		{
			item.m_str_list.clear();
		}

		for (size_t i = 0; i < dlginit.size(); ++i)
		{
			INT iItem = GetDlgItem(dlginit[i].wCtrl);
			if (iItem != -1 && iItem < INT(m_items.size()))
			{
				m_items[iItem].m_str_list.push_back(dlginit[i].strText);
			}
		}
		return true;
	}
	return false;
}

bool DialogRes::SaveDlgInitData(MByteStreamEx::data_type& data) const
{
	std::unordered_set<WORD> ids;
	for (auto& item : m_items)
	{
		ids.insert(item.m_id);
	}

	DlgInitRes dlginit;
	for (auto& id : ids)
	{
		INT iItem = GetDlgItem(id);
		if (iItem != -1 && iItem < INT(m_items.size()))
		{
			auto& item = m_items[iItem];
			for (auto& str : item.m_str_list)
			{
				if (item.IsListBox())
				{
					DlgInitEntry entry(id, LB_ADDSTRING, str);
					dlginit.push_back(entry);
				}
				else if (item.IsStdComboBox())
				{
					DlgInitEntry entry(id, CB_ADDSTRING, str);
					dlginit.push_back(entry);
				}
				else if (item.IsExtComboBox())
				{
					DlgInitEntry entry(id, CBEM_INSERTITEM, str);
					dlginit.push_back(entry);
				}
			}
		}
	}

	data = dlginit.data();
	return !dlginit.empty();
}

MStringW DialogRes::Dump(const MIdOrString& id_or_str, bool bAlwaysControl)
{
	MStringW ret;

	if (id_or_str.is_str())
	{
		ret += id_or_str.str();
	}
	else
	{
		ret += g_db.GetNameOfResID(IDTYPE_DIALOG, id_or_str.m_id);
	}

	if (IsExtended())
	{
		ret += L" DIALOGEX ";
	}
	else
	{
		ret += L" DIALOG ";
	}

	// NOTE: windres can't read negative x and y. It must be unsigned.
	ret += mstr_dec_word((WORD)m_pt.x);
	ret += L", ";
	ret += mstr_dec_word((WORD)m_pt.y);
	ret += L", ";

	ret += mstr_dec_short((SHORT)m_siz.cx);
	ret += L", ";
	ret += mstr_dec_short((SHORT)m_siz.cy);
	if (IsExtended() && m_help_id)
	{
		ret += L", ";
		ret += g_db.GetNameOfResID(IDTYPE_HELP, m_help_id);
	}
	ret += L"\r\n";

	if (!m_title.empty())
	{
		ret += L"CAPTION ";
		ret += m_title.quoted_wstr_with_wrap();
		ret += L"\r\n";
	}
	if (!m_class.empty())
	{
		ret += L"CLASS ";
		ret += m_class.quoted_wstr_with_wrap();
		ret += L"\r\n";
	}
	if (IsExtended() && !m_menu.empty())
	{
		ret += L"MENU ";
		ret += m_menu.str();
		ret += L"\r\n";
	}

	{
		DWORD value = m_style;
		if ((value & DS_SHELLFONT) == DS_SHELLFONT)
			;
		else if ((value & DS_SHELLFONT) == DS_FIXEDSYS)
			;
		else if ((value & DS_SHELLFONT) == DS_SETFONT)
			value &= ~DS_SETFONT;

		MStringW str;
		if ((value & WS_CAPTION) == WS_CAPTION)
		{
			str = g_db.DumpBitField(L"DIALOG", L"PARENT.STYLE", value);
		}
		else if ((value & WS_CAPTION) == WS_BORDER)
		{
			str = g_db.DumpBitField(L"DIALOG", L"PARENT.STYLE", value, WS_DLGFRAME);
		}
		else if ((value & WS_CAPTION) == WS_DLGFRAME)
		{
			str = g_db.DumpBitField(L"DIALOG", L"PARENT.STYLE", value, WS_BORDER);
		}
		else
		{
			str = g_db.DumpBitField(L"DIALOG", L"PARENT.STYLE", value, WS_CAPTION);
		}
		if (value)
		{
			if (!str.empty())
				str += L" | ";

			str = mstr_hex(value);
		}
		else
		{
			if (str.empty())
				str += L"0";
		}
		ret += L"STYLE ";
		ret += str;
		ret += L"\r\n";
	}

	if (m_ex_style)
	{
		DWORD value = m_ex_style;
		MStringW str = g_db.DumpBitField(L"EXSTYLE", L"", value);
		if (value)
		{
			if (!str.empty())
				str += L" | ";

			str = mstr_hex(value);
		}
		else
		{
			if (str.empty())
				str += L"0";
		}
		ret += L"EXSTYLE ";
		ret += str;
		ret += L"\r\n";
	}

	if (m_style & DS_SETFONT)
	{
		ret += L"FONT ";
		ret += mstr_dec_short(m_point_size);
		ret += L", ";
		ret += m_type_face.quoted_wstr_with_wrap();
		if (IsExtended())
		{
			ret += L", ";
			ret += mstr_dec_short(m_weight);
			ret += L", ";
			ret += mstr_dec_short(!!m_italic);
			ret += L", ";
			ret += mstr_dec_short(m_charset);
		}
		ret += L"\r\n";
	}

	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	for (UINT i = 0; i < m_cItems; ++i)
	{
		ret += L"    ";
		ret += m_items[i].Dump(!!g_settings.bAlwaysControl);
		ret += L"\r\n";
	}

	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";

	return ret;
}

void DialogRes::FixupForRad(bool bRevert)
{
	if (bRevert)
	{
		m_style = m_old_style;
		m_ex_style = m_old_ex_style;
		m_menu = m_old_menu;
		m_class = m_old_class;
		m_type_face = m_old_type_face;
	}
	else
	{
		m_old_style = m_style;
		m_old_ex_style = m_ex_style;
		m_old_menu = m_menu;
		m_old_class = m_class;

		m_style &= ~(WS_POPUP | DS_SYSMODAL | WS_DISABLED);
		m_style |= WS_VISIBLE | WS_CHILD | DS_NOIDLEMSG;

		// We don't know the non-flickering methods... Sorry...
		//m_style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

		m_ex_style &= ~(WS_EX_ACCEPTFILES | WS_EX_TOPMOST |
					 WS_EX_LAYERED | WS_EX_TRANSPARENT);
		m_ex_style |= WS_EX_NOACTIVATE;

		m_menu.clear();
		m_class.clear();

		m_old_type_face = m_type_face;
		m_type_face = m_replaced_type_face;
	}

	for (auto& item : m_items)
	{
		item.FixupForRad(bRevert);
	}
}

void DialogRes::FixupForTest(bool bRevert)
{
	for (auto& item : m_items)
	{
		item.FixupForTest(bRevert);
	}
}

LONG DialogRes::GetCharDimensions(HDC hdc, LONG *height) const
{
	SIZE sz;
	TEXTMETRICW tm;

	static const WCHAR alphabet[] =
		L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if (!GetTextMetricsW(hdc, &tm))
		return 0;

	if (!GetTextExtentPointW(hdc, alphabet, 52, &sz))
		return 0;

	if (height)
		*height = tm.tmHeight;

	return (sz.cx / 26 + 1) / 2;
}

INT DialogRes::GetBaseUnits(INT& y) const
{
	INT xBaseUnit, yBaseUnit;
	INT Units = GetDialogBaseUnits();
	xBaseUnit = LOWORD(Units);
	yBaseUnit = HIWORD(Units);

	HDC hDC = CreateCompatibleDC(NULL);
	HFONT hFont = NULL;
	switch (m_style & DS_SHELLFONT)
	{
	case DS_SETFONT:
		if (m_point_size == 0x7FFF)
		{
			NONCLIENTMETRICSW ncm;
			ncm.cbSize = sizeof(ncm);
			if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
			{
				hFont = CreateFontIndirectW(&ncm.lfMessageFont);
			}
		}
		else
		{
			int pixels = MulDiv(m_point_size, GetDeviceCaps(hDC, LOGPIXELSY), 72);

			LOGFONTW lf;
			ZeroMemory(&lf, sizeof(lf));
			lf.lfHeight = -pixels;
			lf.lfWeight = m_weight;
			lf.lfItalic = m_italic;
			lf.lfCharSet = DEFAULT_CHARSET;
			if (m_replaced_type_face.empty())
				lf.lfFaceName[0] = 0;
			else
				StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), m_replaced_type_face.c_str());

			hFont = CreateFontIndirectW(&lf);
		}
		break;
	case DS_SHELLFONT:
		if (m_point_size == 0x7FFF)
		{
			NONCLIENTMETRICSW ncm;
			ncm.cbSize = sizeof(ncm);
			if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
			{
				hFont = CreateFontIndirectW(&ncm.lfMessageFont);
			}
		}
		else
		{
			// check existence of "MS Shell Dlg 2"
			INT ret = TRUE;
			ret = EnumFontFamilies(hDC, L"MS Shell Dlg 2", (FONTENUMPROC)EnumFontFamProc, 0);

			LOGFONTW lf;
			ZeroMemory(&lf, sizeof(lf));
			int pixels = MulDiv(m_point_size, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			lf.lfHeight = -pixels;
			lf.lfWeight = m_weight;
			lf.lfItalic = m_italic;
			lf.lfCharSet = DEFAULT_CHARSET;

			if (!ret && lstrcmpiW(m_replaced_type_face.c_str(), L"MS Shell Dlg") == 0)
				StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), L"MS Shell Dlg 2");
			else
				StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), m_replaced_type_face.c_str());

			hFont = CreateFontIndirectW(&lf);
		}
		break;
	case DS_FIXEDSYS:
		hFont = HFONT(GetStockObject(SYSTEM_FIXED_FONT));
		break;
	default:
		hFont = HFONT(GetStockObject(SYSTEM_FONT));
		break;
	}

	if (hFont)
	{
		SIZE charSize;
		HGDIOBJ hOldFont = SelectObject(hDC, hFont);
		charSize.cx = GetCharDimensions(hDC, &charSize.cy);
		SelectObject(hDC, hOldFont);
		DeleteObject(hFont);

		if (charSize.cx)
		{
			xBaseUnit = charSize.cx;
			yBaseUnit = charSize.cy;
		}
	}
	DeleteDC(hDC);

	y = yBaseUnit;
	return xBaseUnit;
}

INT DialogRes::GetHeadLines() const
{
	INT ret = 0;

	++ret;  // "LANGUAGE ..."
	++ret;  // ""
	++ret;  // "... DIALOG ..."
	if (!m_title.empty())
	{
		++ret;  // "CAPTION ..."
	}
	if (!m_class.empty())
	{
		++ret;  // "CLASS ..."
	}
	if (IsExtended() && !m_menu.empty())
	{
		++ret;  // "MENU ..."
	}
	++ret;  // "STYLE ..."
	if (m_ex_style)
	{
		++ret;  // "EXSTYLE ..."
	}
	if (m_style & DS_SETFONT)
	{
		++ret;  // "FONT ..."
	}
	++ret;  // "{"
	return ret;
}

bool DialogRes::_headerFromStream(const MByteStreamEx& stream)
{
	stream.ReadDwordAlignment();

	DLGTEMPLATE tmp;
	if (!stream.ReadRaw(tmp))
		return false;

	m_version = 0;
	m_signature = 0;
	m_help_id = 0;
	m_style = tmp.style;
	m_ex_style = tmp.dwExtendedStyle;
	m_cItems = tmp.cdit;
	m_pt.x = tmp.x;
	m_pt.y = tmp.y;
	m_siz.cx = tmp.cx;
	m_siz.cy = tmp.cy;

	if (!stream.ReadString(m_menu) ||
		!stream.ReadString(m_class) ||
		!stream.ReadString(m_title))
	{
		return false;
	}

	m_point_size = 0;
	m_weight = FW_NORMAL;
	m_italic = 0;
	m_type_face.clear();
	m_items.clear();

	if (m_style & DS_SETFONT)
	{
		if (!stream.ReadWord(m_point_size) ||
			!stream.ReadString(m_type_face))
		{
			return false;
		}
		ReplaceFont();
	}

	return true;
}

bool DialogRes::_headerFromStreamEx(const MByteStreamEx& stream)
{
	stream.ReadDwordAlignment();

	DLGTEMPLATEEXHEAD TemplateEx;
	if (!stream.ReadRaw(TemplateEx))
		return false;

	if (TemplateEx.dlgVer != 1 || TemplateEx.signature != 0xFFFF)
		return false;

	m_version = TemplateEx.dlgVer;
	m_signature = TemplateEx.signature;
	m_help_id = TemplateEx.helpID;
	m_style = TemplateEx.style;
	m_ex_style = TemplateEx.exStyle;
	m_cItems = TemplateEx.cDlgItems;
	m_pt.x = TemplateEx.x;
	m_pt.y = TemplateEx.y;
	m_siz.cx = TemplateEx.cx;
	m_siz.cy = TemplateEx.cy;

	if (!stream.ReadString(m_menu) ||
		!stream.ReadString(m_class) ||
		!stream.ReadString(m_title))
	{
		return false;
	}

	m_point_size = 0;
	m_weight = FW_NORMAL;
	m_italic = FALSE;
	m_charset = DEFAULT_CHARSET;
	m_type_face.clear();
	m_items.clear();

	if (TemplateEx.style & DS_SETFONT)
	{
		if (!stream.ReadWord(m_point_size) ||
			!stream.ReadWord(m_weight) ||
			!stream.ReadByte(m_italic) ||
			!stream.ReadByte(m_charset) ||
			!stream.ReadString(m_type_face))
		{
			return false;
		}
		ReplaceFont();
	}

	return true;
}

bool DialogRes::_headerToStream(MByteStreamEx& stream) const
{
	stream.WriteDwordAlignment();

	DLGTEMPLATE tmp;

	tmp.style = m_style;
	tmp.dwExtendedStyle = m_ex_style;
	tmp.cdit = m_cItems;
	tmp.x = (SHORT)m_pt.x;
	tmp.y = (SHORT)m_pt.y;
	tmp.cx = (SHORT)m_siz.cx;
	tmp.cy = (SHORT)m_siz.cy;
	if (!stream.WriteRaw(tmp) ||
		!stream.WriteString(m_menu.ptr()) ||
		!stream.WriteString(m_class.ptr()) ||
		!stream.WriteString(m_title.ptr()))
	{
		return false;
	}

	if (tmp.style & DS_SETFONT)
	{
		if (!stream.WriteWord(m_point_size) ||
			!stream.WriteString(m_type_face.ptr()))
		{
			return false;
		}
	}

	return true;
}

bool DialogRes::_headerToStreamEx(MByteStreamEx& stream) const
{
	stream.WriteDwordAlignment();

	DLGTEMPLATEEXHEAD TemplateEx;
	TemplateEx.dlgVer = 1;
	TemplateEx.signature = 0xFFFF;
	TemplateEx.helpID = m_help_id;
	TemplateEx.exStyle = m_ex_style;
	TemplateEx.style = m_style;
	TemplateEx.cDlgItems = m_cItems;
	TemplateEx.x = (short)m_pt.x;
	TemplateEx.y = (short)m_pt.y;
	TemplateEx.cx = (short)m_siz.cx;
	TemplateEx.cy = (short)m_siz.cy;
	if (!stream.WriteRaw(TemplateEx) ||
		!stream.WriteString(m_menu.ptr()) ||
		!stream.WriteString(m_class.ptr()) ||
		!stream.WriteString(m_title.ptr()))
	{
		return false;
	}

	if (TemplateEx.style & DS_SETFONT)
	{
		if (!stream.WriteWord(m_point_size) ||
			!stream.WriteWord(m_weight) ||
			!stream.WriteByte(m_italic) ||
			!stream.WriteByte(m_charset) ||
			!stream.WriteString(m_type_face.ptr()))
		{
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////

DialogItemClipboard::DialogItemClipboard(DialogRes& dialog_res)
	: m_dialog_res(dialog_res),
	m_uCF_DIALOGITEMS(::RegisterClipboardFormat(TEXT("RisohEditor_DialogItem_ClipboardData")))
{
}

bool DialogItemClipboard::Copy(HWND hwndRad, const DialogItems& items)
{
	MByteStreamEx stream;

	for (size_t i = 0; i < items.size(); ++i)
	{
		items[i].SaveToStream(stream, m_dialog_res.IsExtended());
	}

	if (HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, DWORD(stream.size())))
	{
		if (LPVOID pv = GlobalLock(hGlobal))
		{
			CopyMemory(pv, &stream[0], stream.size());
			GlobalUnlock(hGlobal);

			if (OpenClipboard(hwndRad))
			{
				EmptyClipboard();
				SetClipboardData(m_uCF_DIALOGITEMS, hGlobal);
				return !!CloseClipboard();
			}
		}
		GlobalFree(hGlobal);
	}
	return false;
}

bool DialogItemClipboard::Paste(HWND hwndRad, DialogItems& items) const
{
	items.clear();

	if (!IsAvailable())
		return false;

	if (!OpenClipboard(hwndRad))
		return false;

	bool bOK = false;
	if (HGLOBAL hGlobal = GetClipboardData(m_uCF_DIALOGITEMS))
	{
		SIZE_T siz = GlobalSize(hGlobal);
		if (LPVOID pv = GlobalLock(hGlobal))
		{
			MByteStreamEx stream(siz);
			CopyMemory(&stream[0], pv, siz);

			DialogItem item;
			while (item.LoadFromStream(stream, m_dialog_res.IsExtended()))
			{
				items.push_back(item);
			}
			GlobalUnlock(hGlobal);

			if (items.size())
			{
				bOK = true;
			}
		}
	}
	CloseClipboard();
	return bOK;
}
