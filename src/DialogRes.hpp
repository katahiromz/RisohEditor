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

#ifndef DIALOG_RES_HPP_
#define DIALOG_RES_HPP_

#include <windows.h>
#include <vector>
#include "MByteStreamEx.hpp"
#include "ConstantsDB.hpp"

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

inline bool PredefClassToID(MStringW name, WORD& w)
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

inline bool IDToPredefClass(WORD w, MStringW& name)
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

inline void FixClassName(const ConstantsDB& db, MStringW& cls)
{
    ConstantsDB::TableType table = db.GetTable(L"CONTROL.CLASSES");
    for (size_t i = 0; i < table.size(); ++i)
    {
        if (lstrcmpiW(table[i].name.c_str(), cls.c_str()) == 0)
        {
            cls = table[i].name;
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

struct DialogItem
{
    DWORD               m_help_id;
    DWORD               m_style;
    DWORD               m_ex_style;
    POINT               m_pt;
    SIZE                m_siz;
    WORD                m_id;
    MIdOrString         m_class;
    MIdOrString         m_title;
    std::vector<BYTE>   m_extra;
    DWORD               m_old_style, m_old_ex_style;
    SIZE                m_sizOld;

    DialogItem()
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

    bool LoadFromStream(const MByteStreamEx& stream, bool extended = false)
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

    bool LoadFromStreamEx(const MByteStreamEx& stream)
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

    bool SaveToStream(MByteStreamEx& stream, bool extended = false) const
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

    bool SaveToStreamEx(MByteStreamEx& stream) const
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

    MStringW Dump(const ConstantsDB& db, bool bAlwaysControl = false)
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
                    return _do_AUTO3STATE(db);
                if ((m_style & BS_TYPEMASK) == BS_AUTOCHECKBOX)
                    return _do_AUTOCHECKBOX(db);
                if ((m_style & BS_TYPEMASK) == BS_AUTORADIOBUTTON)
                    return _do_AUTORADIOBUTTON(db);
                if ((m_style & BS_TYPEMASK) == BS_CHECKBOX)
                    return _do_CHECKBOX(db);
                if ((m_style & BS_TYPEMASK) == BS_DEFPUSHBUTTON)
                    return _do_DEFPUSHBUTTON(db);
                if ((m_style & BS_TYPEMASK) == BS_GROUPBOX)
                    return _do_GROUPBOX(db);
                if ((m_style & BS_TYPEMASK) == BS_PUSHBUTTON)
                    return _do_PUSHBUTTON(db);
                if ((m_style & BS_TYPEMASK) == BS_PUSHBOX ||
                    (m_style & BS_TYPEMASK) == 0xC)
                {
                    return _do_PUSHBOX(db);
                }
                if ((m_style & BS_TYPEMASK) == BS_RADIOBUTTON)
                    return _do_RADIOBUTTON(db);
                if ((m_style & BS_TYPEMASK) == BS_3STATE)
                    return _do_STATE3(db);
            }
            if (lstrcmpiW(cls.c_str(), L"STATIC") == 0)
            {
                if ((m_style & SS_TYPEMASK) == SS_LEFT)
                    return _do_LTEXT(db);
                if ((m_style & SS_TYPEMASK) == SS_CENTER)
                    return _do_CTEXT(db);
                if ((m_style & SS_TYPEMASK) == SS_RIGHT)
                    return _do_RTEXT(db);
                if ((m_style & SS_TYPEMASK) == SS_ICON && m_title.empty())
                    return _do_ICON(db);
            }
            if (m_title.empty())
            {
                if (lstrcmpiW(cls.c_str(), L"EDIT") == 0)
                    return _do_EDITTEXT(db);
                if (lstrcmpiW(cls.c_str(), L"COMBOBOX") == 0)
                    return _do_COMBOBOX(db);
                if (lstrcmpiW(cls.c_str(), L"LISTBOX") == 0)
                    return _do_LISTBOX(db);
                if (lstrcmpiW(cls.c_str(), L"SCROLLBAR") == 0)
                    return _do_SCROLLBAR(db);
            }
        }
        return DumpControl(db, cls);
    }

    MStringW DumpControl(const ConstantsDB& db, MStringW& cls)
    {
        MStringW ret;

        ret += L"CONTROL ";
        ret += m_title.quoted_wstr();
        ret += L", ";
        ret += db.GetNameOfResID(IDTYPE_CONTROL, m_id);
        ret += L", ";
        if (m_class.is_int())
        {
            if (IDToPredefClass(m_class.m_id, cls))
                ret += mstr_quote(cls);
            else
                ret += mstr_dec_short(m_class.m_id);
        }
        else
        {
            cls = m_class.str();
            FixClassName(db, cls);
            ret += mstr_quote(cls);
        }

        ret += L", ";
        {
            DWORD value = m_style;
            DWORD def_value = WS_CHILD | WS_VISIBLE;
            ret += db.DumpBitFieldOrZero(cls.c_str(), L"STYLE", value, def_value);
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
            ret += db.DumpBitFieldOrZero(L"EXSTYLE", L"", value);
        }
        if (m_help_id)
        {
            ret += L", ";
            ret += db.GetNameOfResID(IDTYPE_HELP, m_help_id);
        }
        if (m_extra.size() && m_extra.size() % 2 == 0)
        {
            size_t count = m_extra.size() / sizeof(WORD);
            const WORD *pw = (const WORD *)&m_extra[0];
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

    MStringW _do_CONTROL(bool bNeedsText,
                         const ConstantsDB& db,
                         const MStringW& ctrl,
                         const MStringW& cls,
                         DWORD DefStyle)
    {
        MStringW ret;
        ret += ctrl;
        ret += L" ";
        if (!m_title.empty() || bNeedsText)
        {
            ret += m_title.quoted_wstr();
            ret += L", ";
        }
        ret += db.GetNameOfResID(IDTYPE_CONTROL, m_id);
        ret += L", ";
        ret += mstr_dec_short((SHORT)m_pt.x);
        ret += L", ";
        ret += mstr_dec_short((SHORT)m_pt.y);
        ret += L", ";
        ret += mstr_dec_short((SHORT)m_siz.cx);
        ret += L", ";
        ret += mstr_dec_short((SHORT)m_siz.cy);
        if (m_style != DefStyle || m_ex_style || m_help_id)
        {
            ret += L", ";
            DWORD value = m_style;
            if (ctrl == L"PUSHBOX" && (value & BS_TYPEMASK) == 0xC)
            {
                value &= ~BS_TYPEMASK;
                value |= BS_PUSHBOX;
            }
            ret += db.DumpBitFieldOrZero(cls.c_str(), L"STYLE", value, DefStyle);
        }
        if (m_ex_style || m_help_id)
        {
            ret += L", ";
            DWORD value = m_ex_style;
            ret += db.DumpBitFieldOrZero(L"EXSTYLE", L"", value);
        }
        if (m_help_id)
        {
            ret += L", ";
            ret += db.GetNameOfResID(IDTYPE_HELP, m_help_id);
        }
        if (m_extra.size() && m_extra.size() % 2 == 0)
        {
            size_t count = m_extra.size() / sizeof(WORD);
            const WORD *pw = (const WORD *)&m_extra[0];
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

    MStringW _do_BUTTON(const ConstantsDB& db,
                            const MStringW& ctrl, DWORD DefStyle)
    {
        return _do_CONTROL(true, db, ctrl, L"BUTTON", DefStyle);
    }

    MStringW _do_TEXT(const ConstantsDB& db,
                          const MStringW& ctrl, DWORD DefStyle)
    {
        return _do_CONTROL(true, db, ctrl, L"STATIC", DefStyle);
    }

    MStringW _do_AUTO3STATE(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"AUTO3STATE", (BS_AUTO3STATE | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_AUTOCHECKBOX(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"AUTOCHECKBOX", (BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_AUTORADIOBUTTON(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"AUTORADIOBUTTON", (BS_AUTORADIOBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_CHECKBOX(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"CHECKBOX", (BS_CHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_DEFPUSHBUTTON(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"DEFPUSHBUTTON", (BS_DEFPUSHBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_GROUPBOX(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"GROUPBOX", (BS_GROUPBOX | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_PUSHBUTTON(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"PUSHBUTTON", (BS_PUSHBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_PUSHBOX(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"PUSHBOX", (BS_PUSHBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_RADIOBUTTON(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"RADIOBUTTON", (BS_RADIOBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_STATE3(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"STATE3", (BS_3STATE | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_LTEXT(const ConstantsDB& db)
    {
        return _do_TEXT(db, L"LTEXT", (SS_LEFT | WS_GROUP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_CTEXT(const ConstantsDB& db)
    {
        return _do_TEXT(db, L"CTEXT", (SS_CENTER | WS_GROUP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_RTEXT(const ConstantsDB& db)
    {
        return _do_TEXT(db, L"RTEXT", (SS_RIGHT | WS_GROUP | WS_CHILD | WS_VISIBLE));
    }
    MStringW _do_EDITTEXT(const ConstantsDB& db)
    {
        assert(m_title.empty());
        return _do_CONTROL(false, db, L"EDITTEXT", L"EDIT",
                           ES_LEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE);
    }
    MStringW _do_COMBOBOX(const ConstantsDB& db)
    {
        assert(m_title.empty());
        return _do_CONTROL(false, db, L"COMBOBOX", L"COMBOBOX", WS_CHILD | WS_VISIBLE);
    }
    MStringW _do_ICON(const ConstantsDB& db)
    {
        assert(m_title.empty());
        return _do_CONTROL(true, db, L"ICON", L"STATIC", SS_ICON | WS_CHILD | WS_VISIBLE);
    }
    MStringW _do_LISTBOX(const ConstantsDB& db)
    {
        assert(m_title.empty());
        return _do_CONTROL(false, db, L"LISTBOX", L"LISTBOX",
                           LBS_NOTIFY | WS_BORDER | WS_CHILD | WS_VISIBLE);
    }
    MStringW _do_SCROLLBAR(const ConstantsDB& db)
    {
        assert(m_title.empty());
        return _do_CONTROL(false, db, L"SCROLLBAR", L"SCROLLBAR", SBS_HORZ | WS_CHILD | WS_VISIBLE);
    }

    void Fixup(bool bRevert = false)
    {
        if (bRevert)
        {
            m_style = m_old_style;
            m_ex_style = m_old_ex_style;
            m_siz = m_sizOld;
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
            if (m_siz.cx == 0 && m_siz.cy == 0)
            {
                m_siz.cx = 20;
                m_siz.cy = 20;
            }
        }
    }
};
typedef std::vector<DialogItem> DialogItems;

//////////////////////////////////////////////////////////////////////////////

struct DialogRes
{
    const ConstantsDB&          m_db;
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
    MIdOrString                 m_type_face;
    DialogItems                 m_items;
    DWORD                       m_old_style, m_old_ex_style;
    LANGID                      m_lang_id;
    MIdOrString                 m_old_menu;
    MIdOrString                 m_old_class;

    DialogRes(const ConstantsDB& db) : m_db(db)
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
        m_lang_id = 0;
    }

    DialogItem& operator[](size_t i)
    {
        return m_items[i];
    }
    const DialogItem& operator[](size_t i) const
    {
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

    bool LoadFromStream(const MByteStreamEx& stream)
    {
        if (stream.size() < sizeof(WORD) * 2)
            return false;

        if (*(WORD *)stream.ptr(0) == 1 &&
            *(WORD *)stream.ptr(2) == 0xFFFF)
        {
            // extended dialog
            if (_headerFromStreamEx(stream))
            {
                for (WORD i = 0; i < m_cItems; ++i)
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
                for (WORD i = 0; i < m_cItems; ++i)
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

    bool SaveToStream(MByteStreamEx& stream) const
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
                return true;
            }
        }
        return false;
    }

    void Update()
    {
        if (m_type_face.empty())
            m_style &= ~DS_SETFONT;
        else
            m_style |= DS_SETFONT;
    }

    std::vector<BYTE> data() const
    {
        MByteStreamEx stream;
        SaveToStream(stream);
        return stream.data();
    }

    MStringW Dump(const MIdOrString& id_or_str, bool bAlwaysControl = false)
    {
        MStringW ret;

        if (id_or_str.is_str())
        {
            ret += id_or_str.str();
        }
        else
        {
            ret += m_db.GetNameOfResID(IDTYPE_DIALOG, id_or_str.m_id);
        }

        if (IsExtended())
        {
            ret += L" DIALOGEX ";
        }
        else
        {
            ret += L" DIALOG ";
        }

        ret += mstr_dec_short((SHORT)m_pt.x);
        ret += L", ";
        ret += mstr_dec_short((SHORT)m_pt.y);
        ret += L", ";
        ret += mstr_dec_short((SHORT)m_siz.cx);
        ret += L", ";
        ret += mstr_dec_short((SHORT)m_siz.cy);
        if (IsExtended() && m_help_id)
        {
            ret += L", ";
            ret += m_db.GetNameOfResID(IDTYPE_HELP, m_help_id);
        }
        ret += L"\r\n";

        if (!m_title.empty())
        {
            ret += L"CAPTION ";
            ret += m_title.quoted_wstr();
            ret += L"\r\n";
        }
        if (!m_class.empty())
        {
            ret += L"CLASS ";
            ret += m_class.quoted_wstr();
            ret += L"\r\n";
        }
        if (IsExtended() && !m_menu.empty())
        {
            ret += L"MENU ";
            ret += m_menu.str();
            ret += L"\r\n";
        }

        {
            DWORD value = (m_style & ~DS_SETFONT);
            MStringW str = m_db.DumpBitField(L"DIALOG", L"PARENT.STYLE", value);
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
            MStringW str = m_db.DumpBitField(L"EXSTYLE", L"", value);
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

        if (m_style & (DS_SETFONT | DS_SHELLFONT))
        {
            ret += L"FONT ";
            ret += mstr_dec_short(m_point_size);
            ret += L", ";
            ret += m_type_face.quoted_wstr();
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

        ret += L"{\r\n";

        for (WORD i = 0; i < m_cItems; ++i)
        {
            ret += L"    ";
            ret += m_items[i].Dump(m_db, bAlwaysControl);
            ret += L"\r\n";
        }

        ret += L"}\r\n";

        return ret;
    }

    void Fixup(bool bRevert = false)
    {
        if (bRevert)
        {
            m_style = m_old_style;
            m_ex_style = m_old_ex_style;
            m_menu = m_old_menu;
            m_class = m_old_class;
        }
        else
        {
            m_old_style = m_style;
            m_old_ex_style = m_ex_style;
            m_old_menu = m_menu;
            m_old_class = m_class;

            m_style &= ~(WS_POPUP | DS_SYSMODAL | WS_DISABLED);
            m_style |= WS_VISIBLE | WS_CHILD | DS_NOIDLEMSG;
            m_ex_style &= ~(WS_EX_ACCEPTFILES | WS_EX_TOPMOST |
                         WS_EX_LAYERED | WS_EX_TRANSPARENT);
            m_ex_style |= WS_EX_NOACTIVATE | WS_EX_MDICHILD;

            m_menu.clear();
            m_class.clear();
        }

        DialogItems::iterator it, end = m_items.end();
        for (it = m_items.begin(); it != end; ++it)
        {
            it->Fixup(bRevert);
        }
    }

    LONG GetCharDimensions(HDC hdc, LONG *height) const
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

    INT GetBaseUnits(INT& y) const
    {
        INT xBaseUnit, yBaseUnit;
        INT Units = GetDialogBaseUnits();
        xBaseUnit = LOWORD(Units);
        yBaseUnit = HIWORD(Units);

        HDC hDC = CreateCompatibleDC(NULL);
        HFONT hFont = NULL;
        if (m_style & DS_SETFONT)
        {
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
                if (m_type_face.empty())
                    lf.lfFaceName[0] = 0;
                else
                    StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), m_type_face.m_str.c_str());

                hFont = CreateFontIndirectW(&lf);
            }
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

protected:
    bool _headerFromStream(const MByteStreamEx& stream)
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

        if (m_style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.ReadWord(m_point_size) ||
                !stream.ReadString(m_type_face))
            {
                return false;
            }
        }

        return true;
    }

    bool _headerFromStreamEx(const MByteStreamEx& stream)
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

        if (TemplateEx.style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.ReadWord(m_point_size) ||
                !stream.ReadWord(m_weight) || 
                !stream.ReadByte(m_italic) ||
                !stream.ReadByte(m_charset) ||
                !stream.ReadString(m_type_face))
            {
                return false;
            }
        }

        m_items.clear();
        return true;
    }

    bool _headerToStream(MByteStreamEx& stream) const
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

        if (tmp.style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.WriteWord(m_point_size) ||
                !stream.WriteString(m_type_face.ptr()))
            {
                return false;
            }
        }

        return true;
    }

    bool _headerToStreamEx(MByteStreamEx& stream) const
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

        if (TemplateEx.style & (DS_SETFONT | DS_SHELLFONT))
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
};

//////////////////////////////////////////////////////////////////////////////

class DialogItemClipboard
{
public:
    DialogRes& m_dialog_res;
    UINT m_uCF_DIALOGITEMS;

    DialogItemClipboard(DialogRes& dialog_res)
        : m_dialog_res(dialog_res),
        m_uCF_DIALOGITEMS(::RegisterClipboardFormat(TEXT("RisohEditor_DialogItem_ClipboardData")))
    {
    }

    bool Copy(HWND hwndRad, const DialogItems& items)
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

    bool IsAvailable() const
    {
        return !!IsClipboardFormatAvailable(m_uCF_DIALOGITEMS);
    }

    bool Paste(HWND hwndRad, DialogItems& items) const
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
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef DIALOG_RES_HPP_

//////////////////////////////////////////////////////////////////////////////
