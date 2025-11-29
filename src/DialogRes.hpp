// DialogRes.hpp --- Dialog Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef _INC_WINDOWS
	#include <windows.h>
#endif
#include <vector>
#include "MByteStreamEx.hpp"
#include "DlgInitRes.hpp"

#ifndef BS_PUSHBOX
	#define BS_PUSHBOX  0x0000000A
#endif

//////////////////////////////////////////////////////////////////////////////

#include <pshpack2.h>

// the header of RT_DIALOG (DIALOGEX)
typedef struct DLGTEMPLATEEXHEAD
{
	WORD    dlgVer;
	WORD    signature;
	DWORD   helpID;
	DWORD   exStyle;
	DWORD   style;
	WORD    cDlgItems;
	short   x;
	short   y;
	short   cx;
	short   cy;
} DLGTEMPLATEEXHEAD, *PDLGTEMPLATEEXHEAD, *LPDLGTEMPLATEEXHEAD;

// the header of RT_DIALOG (DIALOGEX) item
typedef struct DLGITEMTEMPLATEEXHEAD
{
	DWORD   helpID;
	DWORD   exStyle;
	DWORD   style;
	short   x;
	short   y;
	short   cx;
	short   cy;
	WORD    id;
} DLGITEMTEMPLATEEXHEAD, *PDLGITEMTEMPLATEEXHEAD, *LPDLGITEMTEMPLATEEXHEAD;

#include <poppack.h>

//////////////////////////////////////////////////////////////////////////////

bool PredefClassToID(MStringW name, WORD& w);
bool IDToPredefClass(WORD w, MStringW& name);
void FixClassName(MStringW& cls);

//////////////////////////////////////////////////////////////////////////////

struct DialogItem
{
	DWORD                   m_help_id;
	DWORD                   m_style;
	DWORD                   m_ex_style;
	POINT                   m_pt;
	SIZE                    m_siz;
	WORD                    m_id;
	MIdOrString             m_class;
	MIdOrString             m_title;
	std::vector<BYTE>       m_extra;
	DWORD                   m_old_style, m_old_ex_style;
	MIdOrString             m_old_title;
	SIZE                    m_sizOld;
	MIdOrString             m_old_class;
	std::vector<MStringA>   m_str_list;

	DialogItem();

	bool LoadFromStream(const MByteStreamEx& stream, bool extended = false);
	bool LoadFromStreamEx(const MByteStreamEx& stream);

	bool SaveToStream(MByteStreamEx& stream, bool extended = false) const;
	bool SaveToStreamEx(MByteStreamEx& stream) const;
	MStringW Dump(bool bAlwaysControl = false) const;

	bool IsStdComboBox() const
	{
		if (lstrcmpiW(m_class.m_str.c_str(), L"COMBOBOX") == 0)
			return true;
		return (m_class.m_id == 0x0085);
	}
	bool IsListBox() const
	{
		if (lstrcmpiW(m_class.m_str.c_str(), L"LISTBOX") == 0)
			return true;
		return (m_class.m_id == 0x0083);
	}
	bool IsButton() const
	{
		if (lstrcmpiW(m_class.m_str.c_str(), L"BUTTON") == 0)
			return true;
		return (m_class.m_id == 0x0080);
	}
	bool IsGroupBox() const
	{
		if (!IsButton())
			return false;
		return (m_style & BS_TYPEMASK) == BS_GROUPBOX;
	}
	bool IsExtComboBox() const
	{
		return (lstrcmpiW(m_class.m_str.c_str(), L"ComboBoxEx32") == 0);
	}
	bool IsStaticIcon() const
	{
		if (m_title.is_int() &&
			(m_class.m_id == 0x0082 ||
			 lstrcmpiW(m_class.m_str.c_str(), L"STATIC") == 0) &&
			(m_style & SS_TYPEMASK) == SS_ICON)
		{
			return true;
		}
		return false;
	}

	MStringW DumpControl(MStringW& cls) const;

	MStringW _do_CONTROL(bool bNeedsText,
						 const MStringW& ctrl,
						 const MStringW& cls,
						 DWORD DefStyle) const;

	MStringW _do_BUTTON(const MStringW& ctrl, DWORD DefStyle) const
	{
		return _do_CONTROL(true, ctrl, L"BUTTON", DefStyle);
	}

	MStringW _do_TEXT(const MStringW& ctrl, DWORD DefStyle) const
	{
		return _do_CONTROL(true, ctrl, L"STATIC", DefStyle);
	}

	MStringW _do_AUTO3STATE() const
	{
		return _do_BUTTON(L"AUTO3STATE", (BS_AUTO3STATE | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_AUTOCHECKBOX() const
	{
		return _do_BUTTON(L"AUTOCHECKBOX", (BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_AUTORADIOBUTTON() const
	{
		// NOTE: RC won't add WS_TABSTOP. Microsoft document said a lie.
		return _do_BUTTON(L"AUTORADIOBUTTON", (BS_AUTORADIOBUTTON | /*WS_TABSTOP |*/ WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_CHECKBOX() const
	{
		return _do_BUTTON(L"CHECKBOX", (BS_CHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_DEFPUSHBUTTON() const
	{
		return _do_BUTTON(L"DEFPUSHBUTTON", (BS_DEFPUSHBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_GROUPBOX() const
	{
		return _do_BUTTON(L"GROUPBOX", (BS_GROUPBOX | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_PUSHBUTTON() const
	{
		return _do_BUTTON(L"PUSHBUTTON", (BS_PUSHBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_PUSHBOX() const
	{
		return _do_BUTTON(L"PUSHBOX", (BS_PUSHBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_RADIOBUTTON() const
	{
		return _do_BUTTON(L"RADIOBUTTON", (BS_RADIOBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_STATE3() const
	{
		return _do_BUTTON(L"STATE3", (BS_3STATE | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_LTEXT() const
	{
		return _do_TEXT(L"LTEXT", (SS_LEFT | WS_GROUP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_CTEXT() const
	{
		return _do_TEXT(L"CTEXT", (SS_CENTER | WS_GROUP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_RTEXT() const
	{
		return _do_TEXT(L"RTEXT", (SS_RIGHT | WS_GROUP | WS_CHILD | WS_VISIBLE));
	}
	MStringW _do_EDITTEXT() const
	{
		assert(m_title.empty());
		return _do_CONTROL(false, L"EDITTEXT", L"EDIT",
						   ES_LEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE);
	}
	MStringW _do_COMBOBOX() const
	{
		assert(m_title.empty());
		return _do_CONTROL(false, L"COMBOBOX", L"COMBOBOX", WS_CHILD | WS_VISIBLE);
	}
	MStringW _do_ICON() const
	{
		return _do_CONTROL(true, L"ICON", L"STATIC", SS_ICON | WS_CHILD | WS_VISIBLE);
	}
	MStringW _do_LISTBOX() const
	{
		assert(m_title.empty());
		return _do_CONTROL(false, L"LISTBOX", L"LISTBOX",
						   LBS_NOTIFY | WS_BORDER | WS_CHILD | WS_VISIBLE);
	}
	MStringW _do_SCROLLBAR() const
	{
		assert(m_title.empty());
		return _do_CONTROL(false, L"SCROLLBAR", L"SCROLLBAR", SBS_HORZ | WS_CHILD | WS_VISIBLE);
	}

	BOOL IsClassRegd(const WCHAR *name) const;

	void FixupForRad(bool bRevert = false);
	void FixupForTest(bool bRevert = false);
};
typedef std::vector<DialogItem> DialogItems;

//////////////////////////////////////////////////////////////////////////////

struct DialogRes
{
	WORD                        m_version;
	WORD                        m_signature;
	DWORD                       m_help_id;
	DWORD                       m_style;
	DWORD                       m_ex_style;
	WORD                        m_cItems;
	POINT                       m_pt;
	SIZE                        m_siz;
	MIdOrString                 m_menu;
	MIdOrString                 m_class;
	MIdOrString                 m_title;
	short                       m_point_size;
	short                       m_weight;
	BYTE                        m_italic;
	BYTE                        m_charset;
	DialogItems                 m_items;
	MIdOrString                 m_name;
	LANGID                      m_lang;
protected:
	DWORD                       m_old_style, m_old_ex_style;
	MIdOrString                 m_old_menu;
	MIdOrString                 m_old_class;
	MIdOrString                 m_type_face;
	MIdOrString                 m_replaced_type_face;
	MIdOrString                 m_old_type_face;
	void ReplaceFont();

public:
	void type_face(const MIdOrString& type_face)
	{
		m_type_face = type_face;
		ReplaceFont();
	}
	const MIdOrString& type_face() const
	{
		return m_type_face;
	}

	DialogRes();

	DialogItem& operator[](size_t i)
	{
		assert(i < size());
		return m_items[i];
	}
	const DialogItem& operator[](size_t i) const
	{
		assert(i < size());
		return m_items[i];
	}
	size_t size() const
	{
		return m_items.size();
	}

	bool IsExtended() const
	{
		return m_signature == 0xFFFF;
	}

	bool LoadFromStream(const MByteStreamEx& stream);
	bool SaveToStream(MByteStreamEx& stream) const;

	INT GetDlgItem(WORD id) const;

	bool LoadDlgInitData(const MByteStreamEx::data_type& data);
	bool SaveDlgInitData(MByteStreamEx::data_type& data) const;

	std::vector<BYTE> data() const
	{
		MByteStreamEx stream;
		SaveToStream(stream);
		return stream.data();
	}

	MStringW Dump(const MIdOrString& id_or_str, bool bAlwaysControl = false);

	void FixupForRad(bool bRevert = false);
	void FixupForTest(bool bRevert = false);

	LONG GetCharDimensions(HDC hdc, LONG *height) const;

	static INT CALLBACK
	EnumFontFamProc(ENUMLOGFONT *pelf, NEWTEXTMETRIC *pntm, INT nFontType, LPARAM lParam)
	{
		if (lstrcmpi(pelf->elfLogFont.lfFaceName, TEXT("MS Shell Dlg 2")) == 0)
			return FALSE;
		return TRUE;
	}

	INT GetBaseUnits(INT& y) const;
	INT GetHeadLines() const;
	BOOL HasActiveX() const
	{
		for (auto& item : m_items)
		{
			if (StrCmpNIW(item.m_class.c_str(), L"AtlAxWin", 8) == 0)
				return TRUE;
			if (item.m_class.c_str()[0] == L'{')
				return TRUE;
		}
		return FALSE;
	}

protected:
	bool _headerFromStream(const MByteStreamEx& stream);
	bool _headerFromStreamEx(const MByteStreamEx& stream);
	bool _headerToStream(MByteStreamEx& stream) const;
	bool _headerToStreamEx(MByteStreamEx& stream) const;
};

//////////////////////////////////////////////////////////////////////////////

class DialogItemClipboard
{
public:
	DialogRes& m_dialog_res;
	UINT m_uCF_DIALOGITEMS;

	DialogItemClipboard(DialogRes& dialog_res);

	bool IsAvailable() const
	{
		return !!IsClipboardFormatAvailable(m_uCF_DIALOGITEMS);
	}

	bool Copy(HWND hwndRad, const DialogItems& items);
	bool Paste(HWND hwndRad, DialogItems& items) const;
};
