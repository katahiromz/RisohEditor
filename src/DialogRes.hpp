// DialogRes --- Dialog Resources
//////////////////////////////////////////////////////////////////////////////

#ifndef DIALOG_RES_HPP_
#define DIALOG_RES_HPP_

#include <windows.h>
#include <vector>
#include "id_string.hpp"
#include "MByteStreamEx.hpp"
#include "ConstantsDB.hpp"

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

inline BOOL PredefClassToID(std::wstring name, WORD& w)
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

inline BOOL IDToPredefClass(WORD w, std::wstring& name)
{
    switch (w)
    {
    case 0x0080:
        name = L"BUTTON";
        return TRUE;
    case 0x0081:
        name = L"EDIT";
        return TRUE;
    case 0x0082:
        name = L"STATIC";
        return TRUE;
    case 0x0083:
        name = L"LISTBOX";
        return TRUE;
    case 0x0084:
        name = L"SCROLLBAR";
        return TRUE;
    case 0x0085:
        name = L"COMBOBOX";
        return TRUE;
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

struct DialogItem
{
    DWORD               m_HelpID;
    DWORD               m_Style;
    DWORD               m_ExStyle;
    POINT               m_pt;
    SIZE                m_siz;
    WORD                m_ID;
    ID_OR_STRING        m_Class;
    ID_OR_STRING        m_Title;
    std::vector<BYTE>   Extra;
    DWORD               m_OldStyle, m_OldExStyle;

    DialogItem()
    {
        m_HelpID = 0;
        m_Style = 0;
        m_ExStyle = 0;
        m_pt.x = 0;
        m_pt.y = 0;
        m_siz.cx = 0;
        m_siz.cy = 0;
        m_ID = 0;
    }

    BOOL LoadFromStream(const MByteStreamEx& stream, BOOL Extended = FALSE)
    {
        if (Extended)
            return LoadFromStreamEx(stream);

        stream.ReadDwordAlignment();

        DLGITEMTEMPLATE Item;
        if (!stream.ReadRaw(Item))
            return FALSE;

        m_HelpID = 0;
        m_Style = Item.style;
        m_ExStyle = Item.dwExtendedStyle;
        m_pt.x = Item.x;
        m_pt.y = Item.y;
        m_siz.cx = Item.cx;
        m_siz.cy = Item.cy;
        m_ID = Item.id;

        if (!stream.ReadString(m_Class) ||
            !stream.ReadString(m_Title))
        {
            return FALSE;
        }

        BYTE b;
        if (!stream.ReadByte(b))
            return FALSE;

        Extra.resize(b);
        if (b && !stream.ReadData(&Extra[0], b))
            return FALSE;

        return TRUE;
    }

    BOOL LoadFromStreamEx(const MByteStreamEx& stream)
    {
        stream.ReadDwordAlignment();

        DLGITEMTEMPLATEEXHEAD Item;
        if (!stream.ReadRaw(Item))
        {
            return FALSE;
        }

        m_HelpID = Item.helpID;
        m_Style = Item.style;
        m_ExStyle = Item.exStyle;
        m_pt.x = Item.x;
        m_pt.y = Item.y;
        m_siz.cx = Item.cx;
        m_siz.cy = Item.cy;
        m_ID = Item.id;

        stream.ReadDwordAlignment();

        if (!stream.ReadString(m_Class) || !stream.ReadString(m_Title))
        {
            return FALSE;
        }

        WORD extraCount;
        if (!stream.ReadWord(extraCount))
            return FALSE;

        if (extraCount)
        {
            stream.ReadDwordAlignment();
            Extra.resize(extraCount);
            if (extraCount && !stream.ReadData(&Extra[0], extraCount))
                return FALSE;
        }

        return TRUE;
    }

    BOOL SaveToStream(MByteStreamEx& stream, BOOL Extended = FALSE) const
    {
        if (Extended)
        {
            return SaveToStreamEx(stream);
        }

        stream.WriteDwordAlignment();

        DLGITEMTEMPLATE Item;
        Item.style = m_Style;
        Item.dwExtendedStyle = m_ExStyle;
        Item.x = (SHORT)m_pt.x;
        Item.y = (SHORT)m_pt.y;
        Item.cx = (SHORT)m_siz.cx;
        Item.cy = (SHORT)m_siz.cy;
        Item.id = m_ID;
        if (!stream.WriteData(&Item, sizeof(Item)))
            return FALSE;

        WORD w;
        if (!IS_INTRESOURCE(m_Class.Ptr()) && 
            PredefClassToID(m_Class.Ptr(), w))
        {
            if (!stream.WriteString(MAKEINTRESOURCEW(w)))
                return FALSE;
        }
        else
        {
            if (!stream.WriteString(m_Class.Ptr()))
                return FALSE;
        }

        if (!stream.WriteString(m_Title.Ptr()))
            return FALSE;

        BYTE b = BYTE(Extra.size());
        if (!stream.WriteRaw(b))
            return FALSE;

        if (b)
        {
            stream.WriteDwordAlignment();
            if (!stream.WriteData(&Extra[0], b))
                return FALSE;
        }

        return TRUE;
    }

    BOOL SaveToStreamEx(MByteStreamEx& stream) const
    {
        stream.WriteDwordAlignment();

        DLGITEMTEMPLATEEXHEAD ItemEx;
        ItemEx.helpID = m_HelpID;
        ItemEx.exStyle = m_ExStyle;
        ItemEx.style = m_Style;
        ItemEx.x = (short)m_pt.x;
        ItemEx.y = (short)m_pt.y;
        ItemEx.cx = (short)m_siz.cx;
        ItemEx.cy = (short)m_siz.cy;
        ItemEx.id = m_ID;
        if (!stream.WriteRaw(ItemEx))
            return FALSE;

        stream.WriteDwordAlignment();

        WORD w;
        if (!IS_INTRESOURCE(m_Class.Ptr()) && 
            PredefClassToID(m_Class.Ptr(), w))
        {
            if (!stream.WriteString(MAKEINTRESOURCEW(w)))
                return FALSE;
        }
        else
        {
            if (!stream.WriteString(m_Class.Ptr()))
                return FALSE;
        }

        if (!stream.WriteString(m_Title.Ptr()) ||
            !stream.WriteWord(WORD(Extra.size())))
        {
            return FALSE;
        }

        if (Extra.size() > 0)
        {
            stream.WriteDwordAlignment();
            WORD ExtraSize = WORD(Extra.size());
            if (!stream.WriteData(&Extra[0], ExtraSize))
                return FALSE;
        }
        return TRUE;
    }

    std::wstring Dump(const ConstantsDB& db, BOOL bAlwaysControl = FALSE)
    {
        std::wstring ret, cls;

        if (m_Class.is_int())
        {
            if (!IDToPredefClass(m_Class.m_ID, cls))
                cls = mstr_hex(m_Class.m_ID);
        }
        else
        {
            cls = m_Class.str();
        }

        if (!bAlwaysControl)
        {
            if (lstrcmpiW(cls.c_str(), L"BUTTON") == 0)
            {
#ifndef BS_TYPEMASK
    #define BS_TYPEMASK     0x0000000F
#endif
                if ((m_Style & BS_TYPEMASK) == BS_AUTO3STATE)
                    return _do_AUTO3STATE(db);
                if ((m_Style & BS_TYPEMASK) == BS_AUTOCHECKBOX)
                    return _do_AUTOCHECKBOX(db);
                if ((m_Style & BS_TYPEMASK) == BS_AUTORADIOBUTTON)
                    return _do_AUTORADIOBUTTON(db);
                if ((m_Style & BS_TYPEMASK) == BS_CHECKBOX)
                    return _do_CHECKBOX(db);
                if ((m_Style & BS_TYPEMASK) == BS_DEFPUSHBUTTON)
                    return _do_DEFPUSHBUTTON(db);
                if ((m_Style & BS_TYPEMASK) == BS_GROUPBOX)
                    return _do_GROUPBOX(db);
                if ((m_Style & BS_TYPEMASK) == BS_PUSHBUTTON)
                    return _do_PUSHBUTTON(db);
                if ((m_Style & BS_TYPEMASK) == BS_RADIOBUTTON)
                    return _do_RADIOBUTTON(db);
                if ((m_Style & BS_TYPEMASK) == BS_3STATE)
                    return _do_STATE3(db);
            }
            if (lstrcmpiW(cls.c_str(), L"STATIC") == 0)
            {
                if ((m_Style & SS_TYPEMASK) == SS_LEFT)
                    return _do_LTEXT(db);
                if ((m_Style & SS_TYPEMASK) == SS_CENTER)
                    return _do_CTEXT(db);
                if ((m_Style & SS_TYPEMASK) == SS_RIGHT)
                    return _do_RTEXT(db);
                if ((m_Style & SS_TYPEMASK) == SS_ICON)
                    return _do_ICON(db);
            }
            if (lstrcmpiW(cls.c_str(), L"EDIT") == 0)
            {
                return _do_EDITTEXT(db);
            }
            if (lstrcmpiW(cls.c_str(), L"COMBOBOX") == 0)
                return _do_COMBOBOX(db);
            if (lstrcmpiW(cls.c_str(), L"LISTBOX") == 0)
                return _do_LISTBOX(db);
            if (lstrcmpiW(cls.c_str(), L"SCROLLBAR") == 0)
                return _do_SCROLLBAR(db);
        }

        ret += L"CONTROL ";
        if (!m_Title.empty())
        {
            ret += m_Title.quoted_wstr();
            ret += L", ";
        }
        ret += db.GetCtrlID(m_ID);
        ret += L", ";
        if (m_Class.is_int())
        {
            if (IDToPredefClass(m_Class.m_ID, cls))
                ret += mstr_quote(cls);
            else
                ret += mstr_hex(m_Class.m_ID);
        }
        else
        {
            cls = m_Class.str();
            ret += m_Class.quoted_wstr();
        }

        ret += L", ";
        {
            DWORD value = m_Style;
            DWORD def_value = WS_CHILD | WS_VISIBLE;
            ret += db.DumpBitField(cls.c_str(), L"STYLE", value, def_value);
        }

        ret += L", ";
        ret += mstr_dec(m_pt.x);
        ret += L", ";
        ret += mstr_dec(m_pt.y);
        ret += L", ";
        ret += mstr_dec(m_siz.cx);
        ret += L", ";
        ret += mstr_dec(m_siz.cy);
        if (m_ExStyle || m_HelpID)
        {
            ret += L", ";
            DWORD value = m_ExStyle;
            ret += db.DumpBitField(L"EXSTYLE", L"", value);
        }
        if (m_HelpID)
        {
            ret += L", ";
            ret += mstr_hex(m_HelpID);
        }

        return ret;
    }

    std::wstring _do_CONTROL(const ConstantsDB& db,
                             const std::wstring& ctrl,
                             const std::wstring& cls,
                             DWORD DefStyle)
    {
        std::wstring ret;
        ret += ctrl;
        ret += L" ";
        if (!m_Title.empty())
        {
            ret += m_Title.quoted_wstr();
            ret += L", ";
        }
        ret += db.GetCtrlID(m_ID);
        ret += L", ";
        ret += mstr_dec(m_pt.x);
        ret += L", ";
        ret += mstr_dec(m_pt.y);
        ret += L", ";
        ret += mstr_dec(m_siz.cx);
        ret += L", ";
        ret += mstr_dec(m_siz.cy);
        if (m_Style != DefStyle || m_ExStyle)
        {
            ret += L", ";
            DWORD value = m_Style;
            ret += db.DumpBitField(cls.c_str(), L"STYLE", value, DefStyle);
        }
        if (m_ExStyle || m_HelpID)
        {
            ret += L", ";
            DWORD value = m_ExStyle;
            ret += db.DumpBitField(L"EXSTYLE", L"", value);
        }
        if (m_HelpID)
        {
            ret += L", ";
            ret += mstr_hex(m_HelpID);
        }
        return ret;
    }

    std::wstring _do_BUTTON(const ConstantsDB& db,
                            const std::wstring& ctrl, DWORD DefStyle)
    {
        return _do_CONTROL(db, ctrl, L"BUTTON", DefStyle);
    }

    std::wstring _do_TEXT(const ConstantsDB& db,
                          const std::wstring& ctrl, DWORD DefStyle)
    {
        return _do_CONTROL(db, ctrl, L"STATIC", DefStyle);
    }

    std::wstring _do_AUTO3STATE(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"AUTO3STATE", (BS_AUTO3STATE | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_AUTOCHECKBOX(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"AUTOCHECKBOX", (BS_AUTOCHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_AUTORADIOBUTTON(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"AUTORADIOBUTTON", (BS_AUTORADIOBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_CHECKBOX(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"CHECKBOX", (BS_CHECKBOX | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_DEFPUSHBUTTON(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"DEFPUSHBUTTON", (BS_DEFPUSHBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_GROUPBOX(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"GROUPBOX", (BS_GROUPBOX | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_PUSHBUTTON(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"PUSHBUTTON", (BS_PUSHBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_RADIOBUTTON(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"RADIOBUTTON", (BS_RADIOBUTTON | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_STATE3(const ConstantsDB& db)
    {
        return _do_BUTTON(db, L"STATE3", (BS_3STATE | WS_TABSTOP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_LTEXT(const ConstantsDB& db)
    {
        return _do_TEXT(db, L"LTEXT", (SS_LEFT | WS_GROUP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_CTEXT(const ConstantsDB& db)
    {
        return _do_TEXT(db, L"CTEXT", (SS_CENTER | WS_GROUP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_RTEXT(const ConstantsDB& db)
    {
        return _do_TEXT(db, L"RTEXT", (SS_RIGHT | WS_GROUP | WS_CHILD | WS_VISIBLE));
    }
    std::wstring _do_EDITTEXT(const ConstantsDB& db)
    {
        return _do_CONTROL(db, L"EDITTEXT", L"EDIT",
                           ES_LEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VISIBLE);
    }
    std::wstring _do_COMBOBOX(const ConstantsDB& db)
    {
        return _do_CONTROL(db, L"COMBOBOX", L"COMBOBOX",
                           CBS_SIMPLE | WS_TABSTOP | WS_CHILD | WS_VISIBLE);
    }
    std::wstring _do_ICON(const ConstantsDB& db)
    {
        return _do_CONTROL(db, L"ICON", L"STATIC", SS_ICON | WS_CHILD | WS_VISIBLE);
    }
    std::wstring _do_LISTBOX(const ConstantsDB& db)
    {
        return _do_CONTROL(db, L"LISTBOX", L"LISTBOX",
                           LBS_NOTIFY | WS_BORDER | WS_CHILD | WS_VISIBLE);
    }
    std::wstring _do_SCROLLBAR(const ConstantsDB& db)
    {
        return _do_CONTROL(db, L"SCROLLBAR", L"SCROLLBAR", SBS_HORZ | WS_CHILD | WS_VISIBLE);
    }

    void Fixup(BOOL bRevert = FALSE)
    {
        if (bRevert)
        {
            m_Style = m_OldStyle;
            m_ExStyle = m_OldExStyle;
        }
        else
        {
            m_OldStyle = m_Style;
            m_OldExStyle = m_ExStyle;

            m_Style &= ~WS_DISABLED;
            m_Style |= WS_CHILD | WS_VISIBLE;

            m_ExStyle &= ~(WS_EX_ACCEPTFILES | WS_EX_LAYERED | WS_EX_TRANSPARENT |
                         WS_EX_TOPMOST);
            m_ExStyle |= WS_EX_NOACTIVATE;
        }
    }
};
typedef std::vector<DialogItem> DialogItems;

//////////////////////////////////////////////////////////////////////////////

struct DialogRes
{
    WORD                        m_Version;
    WORD                        m_Signature;
    DWORD                       m_HelpID;
    DWORD                       m_Style;
    DWORD                       m_ExStyle;
    WORD                        m_cItems;
    POINT                       m_pt;
    SIZE                        m_siz;
    ID_OR_STRING                m_Menu;
    ID_OR_STRING                m_Class;
    ID_OR_STRING                m_Title;
    short                       m_PointSize;
    short                       m_Weight;
    BYTE                        m_Italic;
    BYTE                        m_CharSet;
    ID_OR_STRING                m_TypeFace;
    DialogItems                 Items;
    DWORD                       m_OldStyle, m_OldExStyle;
    LANGID                      m_LangID;
    ID_OR_STRING                m_OldMenu;
    ID_OR_STRING                m_OldClass;

    DialogRes()
    {
        m_Version = 0;
        m_Signature = 0;
        m_HelpID = 0;
        m_Style = 0;
        m_ExStyle = 0;
        m_cItems = 0;
        m_pt.x = 0;
        m_pt.y = 0;
        m_siz.cx = 0;
        m_siz.cy = 0;
        m_PointSize = 0;
        m_Weight = FW_NORMAL;
        m_Italic = FALSE;
        m_CharSet = DEFAULT_CHARSET;
        m_LangID = 0;
    }

    BOOL IsExtended() const
    {
        return m_Signature == 0xFFFF;
    }

    BOOL LoadFromStream(const MByteStreamEx& stream)
    {
        if (stream.size() < sizeof(WORD) * 2)
            return FALSE;

        if (*(WORD *)stream.ptr(0) == 1 &&
            *(WORD *)stream.ptr(2) == 0xFFFF)
        {
            // extended dialog
            if (_headerFromStreamEx(stream))
            {
                for (WORD i = 0; i < m_cItems; ++i)
                {
                    DialogItem Item;
                    if (!Item.LoadFromStreamEx(stream))
                        return FALSE;
                    Items.push_back(Item);
                }
                return TRUE;
            }
        }
        else
        {
            // normal dialog
            if (_headerFromStream(stream))
            {
                for (WORD i = 0; i < m_cItems; ++i)
                {
                    DialogItem Item;
                    if (!Item.LoadFromStream(stream))
                        return FALSE;
                    Items.push_back(Item);
                }
                return TRUE;
            }
        }
        return FALSE;
    }

    BOOL SaveToStream(MByteStreamEx& stream) const
    {
        if (IsExtended())
        {
            if (_headerToStreamEx(stream))
            {
                size_t i, count = Items.size();
                for (i = 0; i < count; ++i)
                {
                    if (!Items[i].SaveToStreamEx(stream))
                        return FALSE;
                }
                return TRUE;
            }
        }
        else
        {
            if (_headerToStream(stream))
            {
                size_t i, count = Items.size();
                for (i = 0; i < count; ++i)
                {
                    if (!Items[i].SaveToStream(stream))
                        return FALSE;
                }
                return TRUE;
            }
        }
        return FALSE;
    }

    void Update()
    {
        if (m_TypeFace.empty())
            m_Style &= ~DS_SETFONT;
        else
            m_Style |= DS_SETFONT;
    }

    std::vector<BYTE> data() const
    {
        MByteStreamEx stream;
        SaveToStream(stream);
        return stream.data();
    }

    std::wstring Dump(const ID_OR_STRING& id_or_str, const ConstantsDB& db,
                      BOOL bAlwaysControl = FALSE)
    {
        std::wstring ret;

        ret += id_or_str.str();
        if (IsExtended())
        {
            ret += L" DIALOGEX ";
        }
        else
        {
            ret += L" DIALOG ";
        }

        ret += mstr_dec(m_pt.x);
        ret += L", ";
        ret += mstr_dec(m_pt.y);
        ret += L", ";
        ret += mstr_dec(m_siz.cx);
        ret += L", ";
        ret += mstr_dec(m_siz.cy);
        ret += L"\r\n";

        if (!m_Title.empty())
        {
            ret += L"CAPTION ";
            ret += m_Title.quoted_wstr();
            ret += L"\r\n";
        }
        if (!m_Class.empty())
        {
            ret += L"CLASS ";
            ret += m_Class.quoted_wstr();
            ret += L"\r\n";
        }
        if (IsExtended() && !m_Menu.empty())
        {
            ret += L"MENU ";
            ret += m_Menu.str();
            ret += L"\r\n";
        }

        {
            DWORD value = (m_Style & ~DS_SETFONT);
            std::wstring str = db.DumpBitField(L"DIALOG", L"STYLE", value);
            if (value)
            {
                if (!str.empty())
                    str += L" | ";

                str = mstr_hex(value);
            }
            ret += L"STYLE ";
            ret += str;
            ret += L"\r\n";
        }

        if (m_ExStyle)
        {
            DWORD value = m_ExStyle;
            std::wstring str = db.DumpBitField(L"EXSTYLE", L"", value);
            if (value)
            {
                if (!str.empty())
                    str += L" | ";

                str = mstr_hex(value);
            }
            ret += L"EXSTYLE ";
            ret += str;
            ret += L"\r\n";
        }

        if (m_Style & (DS_SETFONT | DS_SHELLFONT))
        {
            ret += L"FONT ";
            ret += mstr_dec(m_PointSize);
            ret += L", ";
            ret += m_TypeFace.quoted_wstr();
            if (IsExtended())
            {
                ret += L", ";
                ret += mstr_dec(m_Weight);
                ret += L", ";
                ret += mstr_dec(!!m_Italic);
                ret += L", ";
                ret += mstr_dec(m_CharSet);
            }
            ret += L"\r\n";
        }

        ret += L"{\r\n";

        for (WORD i = 0; i < m_cItems; ++i)
        {
            ret += L"    ";
            ret += Items[i].Dump(db, bAlwaysControl);
            ret += L"\r\n";
        }

        ret += L"}\r\n";

        return ret;
    }

    void Fixup(BOOL bRevert = FALSE)
    {
        if (bRevert)
        {
            m_Style = m_OldStyle;
            m_ExStyle = m_OldExStyle;
            m_Menu = m_OldMenu;
            m_Class = m_OldClass;
        }
        else
        {
            m_OldStyle = m_Style;
            m_OldExStyle = m_ExStyle;
            m_OldMenu = m_Menu;
            m_OldClass = m_Class;

            m_Style &= ~(WS_POPUP | DS_SYSMODAL | WS_DISABLED);
            m_Style |= WS_VISIBLE | WS_CHILD | DS_NOIDLEMSG;
            m_ExStyle &= ~(WS_EX_ACCEPTFILES | WS_EX_TOPMOST |
                         WS_EX_LAYERED | WS_EX_TRANSPARENT);
            m_ExStyle |= WS_EX_NOACTIVATE | WS_EX_MDICHILD;

            m_Menu.clear();
            m_Class.clear();
        }

        DialogItems::iterator it, end = Items.end();
        for (it = Items.begin(); it != end; ++it)
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
        if (m_Style & DS_SETFONT)
        {
            if (m_PointSize == 0x7FFF)
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
                int pixels = MulDiv(m_PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);

                LOGFONTW lf;
                ZeroMemory(&lf, sizeof(lf));
                lf.lfHeight = -pixels;
                lf.lfWeight = m_Weight;
                lf.lfItalic = m_Italic;
                lf.lfCharSet = DEFAULT_CHARSET;
                if (m_TypeFace.empty())
                    lf.lfFaceName[0] = UNICODE_NULL;
                else
                    lstrcpyW(lf.lfFaceName, m_TypeFace.m_Str.c_str());

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
    BOOL _headerFromStream(const MByteStreamEx& stream)
    {
        stream.ReadDwordAlignment();

        DLGTEMPLATE Template;
        if (!stream.ReadRaw(Template))
            return FALSE;

        m_Version = 0;
        m_Signature = 0;
        m_HelpID = 0;
        m_Style = Template.style;
        m_ExStyle = Template.dwExtendedStyle;
        m_cItems = Template.cdit;
        m_pt.x = Template.x;
        m_pt.y = Template.y;
        m_siz.cx = Template.cx;
        m_siz.cy = Template.cy;

        if (!stream.ReadString(m_Menu) ||
            !stream.ReadString(m_Class) || 
            !stream.ReadString(m_Title))
        {
            return FALSE;
        }

        m_PointSize = 0;
        m_Weight = FW_NORMAL;
        m_Italic = 0;
        m_TypeFace.clear();
        Items.clear();

        if (m_Style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.ReadWord(m_PointSize) ||
                !stream.ReadString(m_TypeFace))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    BOOL _headerFromStreamEx(const MByteStreamEx& stream)
    {
        stream.ReadDwordAlignment();

        DLGTEMPLATEEXHEAD TemplateEx;
        if (!stream.ReadRaw(TemplateEx))
            return FALSE;

        if (TemplateEx.dlgVer != 1 || TemplateEx.signature != 0xFFFF)
            return FALSE;

        m_Version = TemplateEx.dlgVer;
        m_Signature = TemplateEx.signature;
        m_HelpID = TemplateEx.helpID;
        m_Style = TemplateEx.style;
        m_ExStyle = TemplateEx.exStyle;
        m_cItems = TemplateEx.cDlgItems;
        m_pt.x = TemplateEx.x;
        m_pt.y = TemplateEx.y;
        m_siz.cx = TemplateEx.cx;
        m_siz.cy = TemplateEx.cy;

        if (!stream.ReadString(m_Menu) ||
            !stream.ReadString(m_Class) ||
            !stream.ReadString(m_Title))
        {
            return FALSE;
        }

        if (TemplateEx.style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.ReadWord(m_PointSize) ||
                !stream.ReadWord(m_Weight) || 
                !stream.ReadByte(m_Italic) ||
                !stream.ReadByte(m_CharSet) ||
                !stream.ReadString(m_TypeFace))
            {
                return FALSE;
            }
        }

        Items.clear();
        return TRUE;
    }

    BOOL _headerToStream(MByteStreamEx& stream) const
    {
        stream.WriteDwordAlignment();

        DLGTEMPLATE Template;

        Template.style = m_Style;
        Template.dwExtendedStyle = m_ExStyle;
        Template.cdit = m_cItems;
        Template.x = (SHORT)m_pt.x;
        Template.y = (SHORT)m_pt.y;
        Template.cx = (SHORT)m_siz.cx;
        Template.cy = (SHORT)m_siz.cy;
        if (!stream.WriteRaw(Template) ||
            !stream.WriteString(m_Menu.Ptr()) ||
            !stream.WriteString(m_Class.Ptr()) ||
            !stream.WriteString(m_Title.Ptr()))
        {
            return FALSE;
        }

        if (Template.style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.WriteWord(m_PointSize) ||
                !stream.WriteString(m_TypeFace.Ptr()))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    BOOL _headerToStreamEx(MByteStreamEx& stream) const
    {
        stream.WriteDwordAlignment();

        DLGTEMPLATEEXHEAD TemplateEx;
        TemplateEx.dlgVer = 1;
        TemplateEx.signature = 0xFFFF;
        TemplateEx.helpID = m_HelpID;
        TemplateEx.exStyle = m_ExStyle;
        TemplateEx.style = m_Style;
        TemplateEx.cDlgItems = m_cItems;
        TemplateEx.x = (short)m_pt.x;
        TemplateEx.y = (short)m_pt.y;
        TemplateEx.cx = (short)m_siz.cx;
        TemplateEx.cy = (short)m_siz.cy;
        if (!stream.WriteRaw(TemplateEx) ||
            !stream.WriteString(m_Menu.Ptr()) ||
            !stream.WriteString(m_Class.Ptr()) ||
            !stream.WriteString(m_Title.Ptr()))
        {
            return FALSE;
        }

        if (TemplateEx.style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.WriteWord(m_PointSize) ||
                !stream.WriteWord(m_Weight) ||
                !stream.WriteByte(m_Italic) ||
                !stream.WriteByte(m_CharSet) ||
                !stream.WriteString(m_TypeFace.Ptr()))
            {
                return FALSE;
            }
        }
        return TRUE;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef DIALOG_RES_HPP_

//////////////////////////////////////////////////////////////////////////////
