#ifndef DIALOG_RES_HPP_
#define DIALOG_RES_HPP_

#include <windows.h>
#include <vector>
#include "id_string.hpp"
#include "ByteStream.hpp"
#include "ConstantsDB.hpp"

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

struct DialogItem
{
    DWORD               HelpID;
    DWORD               Style;
    DWORD               ExStyle;
    POINT               pt;
    SIZE                siz;
    WORD                ID;
    ID_OR_STRING        Class;
    ID_OR_STRING        Title;
    std::vector<BYTE>   Extra;

    DialogItem()
    {
        HelpID = 0;
        Style = 0;
        ExStyle = 0;
        pt.x = 0;
        pt.y = 0;
        siz.cx = 0;
        siz.cy = 0;
        ID = 0;
    }

    BOOL LoadFromStream(const ByteStream& stream, BOOL Extended = FALSE)
    {
        if (Extended)
            return LoadFromStreamEx(stream);

        stream.ReadDwordAlignment();

        DLGITEMTEMPLATE Item;
        if (!stream.ReadRaw(Item))
            return FALSE;

        HelpID = 0;
        Style = Item.style;
        ExStyle = Item.dwExtendedStyle;
        pt.x = Item.x;
        pt.y = Item.y;
        siz.cx = Item.cx;
        siz.cy = Item.cy;
        ID = Item.id;

        if (!stream.ReadString(Class) ||
            !stream.ReadString(Title))
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

    BOOL LoadFromStreamEx(const ByteStream& stream)
    {
        stream.ReadDwordAlignment();

        DLGITEMTEMPLATEEXHEAD Item;
        if (!stream.ReadRaw(Item))
        {
            return FALSE;
        }

        HelpID = Item.helpID;
        Style = Item.style;
        ExStyle = Item.exStyle;
        pt.x = Item.x;
        pt.y = Item.y;
        siz.cx = Item.cx;
        siz.cy = Item.cy;
        ID = Item.id;

        stream.ReadDwordAlignment();

        if (!stream.ReadString(Class) || !stream.ReadString(Title))
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

    BOOL SaveToStream(ByteStream& stream, BOOL Extended = FALSE) const
    {
        if (Extended)
        {
            return SaveToStreamEx(stream);
        }

        stream.WriteDwordAlignment();

        DLGITEMTEMPLATE Item;
        Item.style = Style;
        Item.dwExtendedStyle = ExStyle;
        Item.x = (SHORT)pt.x;
        Item.y = (SHORT)pt.y;
        Item.cx = (SHORT)siz.cx;
        Item.cy = (SHORT)siz.cy;
        Item.id = ID;
        if (!stream.WriteData(&Item, sizeof(Item)))
            return FALSE;

        WORD w;
        if (!IS_INTRESOURCE(Class.Ptr()) && 
            PredefClassToID(Class.Ptr(), w))
        {
            if (!stream.WriteString(MAKEINTRESOURCEW(w)))
                return FALSE;
        }
        else
        {
            if (!stream.WriteString(Class.Ptr()))
                return FALSE;
        }

        if (!stream.WriteString(Title.Ptr()))
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

    BOOL SaveToStreamEx(ByteStream& stream) const
    {
        stream.WriteDwordAlignment();

        DLGITEMTEMPLATEEXHEAD ItemEx;
        ItemEx.helpID = HelpID;
        ItemEx.exStyle = ExStyle;
        ItemEx.style = Style;
        ItemEx.x = (short)pt.x;
        ItemEx.y = (short)pt.y;
        ItemEx.cx = (short)siz.cx;
        ItemEx.cy = (short)siz.cy;
        ItemEx.id = ID;
        if (!stream.WriteRaw(ItemEx))
            return FALSE;

        stream.WriteDwordAlignment();

        WORD w;
        if (!IS_INTRESOURCE(Class.Ptr()) && 
            PredefClassToID(Class.Ptr(), w))
        {
            if (!stream.WriteString(MAKEINTRESOURCEW(w)))
                return FALSE;
        }
        else
        {
            if (!stream.WriteString(Class.Ptr()))
                return FALSE;
        }

        if (!stream.WriteString(Title.Ptr()) ||
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

    std::wstring Dump(const ConstantsDB& db)
    {
        std::wstring ret, cls;

        if (Class.is_int())
        {
            if (!IDToPredefClass(Class.m_ID, cls))
                cls = str_hex(Class.m_ID);
        }
        else
        {
            cls = Class.wstr();
        }

        if (lstrcmpiW(cls.c_str(), L"BUTTON") == 0)
        {
#ifndef BS_TYPEMASK
    #define BS_TYPEMASK     0x0000000F
#endif
            if ((Style & BS_TYPEMASK) == BS_AUTO3STATE)
                return _do_AUTO3STATE(db);
            if ((Style & BS_TYPEMASK) == BS_AUTOCHECKBOX)
                return _do_AUTOCHECKBOX(db);
            if ((Style & BS_TYPEMASK) == BS_AUTORADIOBUTTON)
                return _do_AUTORADIOBUTTON(db);
            if ((Style & BS_TYPEMASK) == BS_CHECKBOX)
                return _do_CHECKBOX(db);
            if ((Style & BS_TYPEMASK) == BS_DEFPUSHBUTTON)
                return _do_DEFPUSHBUTTON(db);
            if ((Style & BS_TYPEMASK) == BS_GROUPBOX)
                return _do_GROUPBOX(db);
            if ((Style & BS_TYPEMASK) == BS_PUSHBUTTON)
                return _do_PUSHBUTTON(db);
            if ((Style & BS_TYPEMASK) == BS_RADIOBUTTON)
                return _do_RADIOBUTTON(db);
            if ((Style & BS_TYPEMASK) == BS_3STATE)
                return _do_STATE3(db);
        }
        if (lstrcmpiW(cls.c_str(), L"STATIC") == 0)
        {
            if ((Style & SS_TYPEMASK) == SS_LEFT)
                return _do_LTEXT(db);
            if ((Style & SS_TYPEMASK) == SS_CENTER)
                return _do_CTEXT(db);
            if ((Style & SS_TYPEMASK) == SS_RIGHT)
                return _do_RTEXT(db);
            if ((Style & SS_TYPEMASK) == SS_ICON)
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

        ret += L"CONTROL ";
        if (!Title.empty())
        {
            ret += Title.quoted_wstr();
            ret += L", ";
        }
        ret += db.GetCtrlID(ID);
        ret += L", ";
        if (Class.is_int())
        {
            if (IDToPredefClass(Class.m_ID, cls))
                ret += str_quote(cls);
            else
                ret += str_hex(Class.m_ID);
        }
        else
        {
            cls = Class.wstr();
            ret += Class.quoted_wstr();
        }

        ret += L", ";
        {
            DWORD value = Style;
            DWORD def_value = WS_CHILD | WS_VISIBLE;
            ret += db.DumpBitField(cls.c_str(), L"STYLE", value, def_value);
        }

        ret += L", ";
        ret += str_dec(pt.x);
        ret += L", ";
        ret += str_dec(pt.y);
        ret += L", ";
        ret += str_dec(siz.cx);
        ret += L", ";
        ret += str_dec(siz.cy);
        if (ExStyle || HelpID)
        {
            ret += L", ";
            DWORD value = ExStyle;
            ret += db.DumpBitField(L"EXSTYLE", L"", value);
        }
        if (HelpID)
        {
            ret += L", ";
            ret += str_hex(HelpID);
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
        if (!Title.empty())
        {
            ret += Title.quoted_wstr();
            ret += L", ";
        }
        ret += db.GetCtrlID(ID);
        ret += L", ";
        ret += str_dec(pt.x);
        ret += L", ";
        ret += str_dec(pt.y);
        ret += L", ";
        ret += str_dec(siz.cx);
        ret += L", ";
        ret += str_dec(siz.cy);
        if (Style != DefStyle || ExStyle)
        {
            ret += L", ";
            DWORD value = Style;
            ret += db.DumpBitField(cls.c_str(), L"STYLE", value, DefStyle);
        }
        if (ExStyle || HelpID)
        {
            ret += L", ";
            DWORD value = ExStyle;
            ret += db.DumpBitField(L"EXSTYLE", L"", value);
        }
        if (HelpID)
        {
            ret += L", ";
            ret += str_hex(HelpID);
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
};
typedef std::vector<DialogItem> DialogItems;

struct DialogRes
{
    WORD                        Version;
    WORD                        Signature;
    DWORD                       HelpID;
    DWORD                       Style;
    DWORD                       ExStyle;
    WORD                        cItems;
    POINT                       pt;
    SIZE                        siz;
    ID_OR_STRING                Menu;
    ID_OR_STRING                Class;
    ID_OR_STRING                Title;
    short                       PointSize;
    short                       Weight;
    BYTE                        Italic;
    BYTE                        CharSet;
    ID_OR_STRING                TypeFace;
    DialogItems                 Items;

    DialogRes()
    {
        Version = 0;
        Signature = 0;
        HelpID = 0;
        Style = 0;
        ExStyle = 0;
        cItems = 0;
        pt.x = 0;
        pt.y = 0;
        siz.cx = 0;
        siz.cy = 0;
        PointSize = 0;
        Weight = FW_NORMAL;
        Italic = FALSE;
        CharSet = DEFAULT_CHARSET;
    }

    BOOL IsExtended() const
    {
        return Signature == 0xFFFF;
    }

    BOOL LoadFromStream(const ByteStream& stream)
    {
        if (stream.size() < sizeof(WORD) * 2)
            return FALSE;

        if (*(WORD *)stream.ptr(0) == 1 &&
            *(WORD *)stream.ptr(2) == 0xFFFF)
        {
            // extended dialog
            if (_headerFromStreamEx(stream))
            {
                for (WORD i = 0; i < cItems; ++i)
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
                for (WORD i = 0; i < cItems; ++i)
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

    BOOL SaveToStream(ByteStream& stream) const
    {
        if (IsExtended())
        {
            if (_headerExToStream(stream))
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
        ;
    }

    std::vector<BYTE> data() const
    {
        ByteStream stream;
        SaveToStream(stream);
        return stream.data();
    }

    std::wstring Dump(const ID_OR_STRING& id_or_str, const ConstantsDB& db)
    {
        std::wstring ret;

        ret += id_or_str.wstr();
        if (IsExtended())
        {
            ret += L" DIALOGEX ";
        }
        else
        {
            ret += L" DIALOG ";
        }

        ret += str_dec(pt.x);
        ret += L", ";
        ret += str_dec(pt.y);
        ret += L", ";
        ret += str_dec(siz.cx);
        ret += L", ";
        ret += str_dec(siz.cy);
        ret += L"\r\n";

        if (!Title.empty())
        {
            ret += L"CAPTION ";
            ret += Title.quoted_wstr();
            ret += L"\r\n";
        }
        if (!Class.empty())
        {
            ret += L"CLASS ";
            ret += Class.quoted_wstr();
            ret += L"\r\n";
        }

        {
            DWORD value = (Style & ~DS_SETFONT);
            std::wstring str = db.DumpBitField(L"DIALOG", L"STYLE", value);
            if (value)
            {
                if (!str.empty())
                    str += L" | ";

                str = str_hex(value);
            }
            ret += L"STYLE ";
            ret += str;
            ret += L"\r\n";
        }

        if (ExStyle)
        {
            DWORD value = ExStyle;
            std::wstring str = db.DumpBitField(L"EXSTYLE", L"", value);
            if (value)
            {
                if (!str.empty())
                    str += L" | ";

                str = str_hex(value);
            }
            ret += L"EXSTYLE ";
            ret += str;
            ret += L"\r\n";
        }

        if (Style & (DS_SETFONT | DS_SHELLFONT))
        {
            ret += L"FONT ";
            ret += str_dec(PointSize);
            ret += L", ";
            ret += TypeFace.quoted_wstr();
            if (IsExtended())
            {
                ret += L", ";
                ret += str_dec(Weight);
                ret += L", ";
                ret += str_dec(!!Italic);
                ret += L", ";
                ret += str_dec(CharSet);
            }
            ret += L"\r\n";
        }

        ret += L"{\r\n";

        for (WORD i = 0; i < cItems; ++i)
        {
            ret += L"    ";
            ret += Items[i].Dump(db);
            ret += L"\r\n";
        }

        ret += L"}\r\n";

        return ret;
    }

protected:
    BOOL _headerFromStream(const ByteStream& stream)
    {
        stream.ReadDwordAlignment();

        DLGTEMPLATE Template;
        if (!stream.ReadRaw(Template))
            return FALSE;

        Version = 0;
        Signature = 0;
        HelpID = 0;
        Style = Template.style;
        ExStyle = Template.dwExtendedStyle;
        cItems = Template.cdit;
        pt.x = Template.x;
        pt.y = Template.y;
        siz.cx = Template.cx;
        siz.cy = Template.cy;

        if (!stream.ReadString(Menu) ||
            !stream.ReadString(Class) || 
            !stream.ReadString(Title))
        {
            return FALSE;
        }

        PointSize = 0;
        Weight = FW_NORMAL;
        Italic = 0;
        TypeFace.clear();
        Items.clear();

        if (Style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.ReadWord(PointSize) ||
                !stream.ReadString(TypeFace))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    BOOL _headerFromStreamEx(const ByteStream& stream)
    {
        stream.ReadDwordAlignment();

        DLGTEMPLATEEXHEAD TemplateEx;
        if (!stream.ReadRaw(TemplateEx))
            return FALSE;

        stream.ReadDwordAlignment();

        if (TemplateEx.dlgVer != 1 || TemplateEx.signature != 0xFFFF)
            return FALSE;

        Version = TemplateEx.dlgVer;
        Signature = TemplateEx.signature;
        HelpID = TemplateEx.helpID;
        Style = TemplateEx.style;
        ExStyle = TemplateEx.exStyle;
        cItems = TemplateEx.cDlgItems;
        pt.x = TemplateEx.x;
        pt.y = TemplateEx.y;
        siz.cx = TemplateEx.cx;
        siz.cy = TemplateEx.cy;

        if (!stream.ReadString(Menu) || !stream.ReadString(Title))
        {
            return FALSE;
        }

        if (TemplateEx.style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.ReadWord(PointSize) ||
                !stream.ReadWord(Weight) || 
                !stream.ReadByte(Italic) ||
                !stream.ReadByte(CharSet) ||
                !stream.ReadString(TypeFace))
            {
                return FALSE;
            }
        }

        Items.clear();
        return TRUE;
    }

    BOOL _headerToStream(ByteStream& stream) const
    {
        stream.WriteDwordAlignment();

        DLGTEMPLATE Template;

        Template.style = Style;
        Template.dwExtendedStyle = ExStyle;
        Template.cdit = cItems;
        Template.x = (SHORT)pt.x;
        Template.y = (SHORT)pt.y;
        Template.cx = (SHORT)siz.cx;
        Template.cy = (SHORT)siz.cy;
        if (!stream.WriteRaw(Template) ||
            !stream.WriteString(Menu.Ptr()) ||
            !stream.WriteString(Class.Ptr()) ||
            !stream.WriteString(Title.Ptr()))
        {
            return FALSE;
        }

        if (Template.style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.WriteWord(PointSize) ||
                !stream.WriteString(TypeFace.Ptr()))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    BOOL _headerExToStream(ByteStream& stream) const
    {
        stream.WriteDwordAlignment();

        DLGTEMPLATEEXHEAD TemplateEx;
        TemplateEx.dlgVer = 1;
        TemplateEx.signature = 0xFFFF;
        TemplateEx.helpID = HelpID;
        TemplateEx.exStyle = ExStyle;
        TemplateEx.style = Style;
        TemplateEx.cDlgItems = cItems;
        TemplateEx.x = (short)pt.x;
        TemplateEx.y = (short)pt.y;
        TemplateEx.cx = (short)siz.cx;
        TemplateEx.cy = (short)siz.cy;
        if (!stream.WriteRaw(TemplateEx))
        {
            return FALSE;
        }
        if (!stream.WriteString(Menu.Ptr()) ||
            !stream.WriteString(Class.Ptr()) ||
            !stream.WriteString(Title.Ptr()))
        {
            return FALSE;
        }

        if (TemplateEx.style & (DS_SETFONT | DS_SHELLFONT))
        {
            if (!stream.WriteWord(PointSize) ||
                !stream.WriteWord(Weight) ||
                !stream.WriteByte(Italic) ||
                !stream.WriteByte(CharSet) ||
                !stream.WriteString(TypeFace.Ptr()))
            {
                return FALSE;
            }
        }
        return TRUE;
    }
};

#endif  // ndef DIALOG_RES_HPP_
