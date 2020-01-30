// MenuRes.hpp  --- Menu Resources
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

#ifndef MENU_RES_HPP_
#define MENU_RES_HPP_

#include <windows.h>
#include <vector>
#include <stack>
#include <cassert>

#include "ConstantsDB.hpp"
#include "MByteStreamEx.hpp"

//////////////////////////////////////////////////////////////////////////////

// header of RT_MENU
typedef struct MENUHEADER
{
    WORD    wVersion;       // zero
    WORD    cbHeaderSize;   // zero
} MENUHEADER;

typedef struct NORMALMENUITEMHEAD
{
    WORD   fItemFlags;      // MF_
    WORD   wMenuID;
    //WCHAR  szItemText[];
} NORMALMENUITEMHEAD;

typedef struct POPUPMENUITEMHEAD
{
    WORD    fItemFlags;     // MF_
    //WCHAR  szItemText[];
} POPUPMENUITEMHEAD;

// You can use the following flags for fItemFlags:
//     MF_GRAYED        0x0001   // 'GRAYED' keyword 
//     MF_INACTIVE      0x0002   // 'INACTIVE' keyword 
//     MF_BITMAP        0x0004   // 'BITMAP' keyword 
//     MF_OWNERDRAW     0x0100   // 'OWNERDRAW' keyword 
//     MF_CHECKED       0x0008   // 'CHECKED' keyword 
//     MF_POPUP         0x0010   // Used internally 
//     MF_MENUBARBREAK  0x0020   // 'MENUBARBREAK' keyword 
//     MF_MENUBREAK     0x0040   // 'MENUBREAK' keyword 
//     MF_ENDMENU       0x0080   // Used internally 

#ifndef MF_INACTIVE
    #define MF_INACTIVE     MF_DISABLED
#endif
#ifndef MF_ENDMENU
    #define MF_ENDMENU      0x0080
#endif

// header of RT_MENU (MENUEX)
typedef struct MENUEX_TEMPLATE_HEADER
{
    WORD    wVersion;       // one
    WORD    wOffset;        // offset to items from this structure
    DWORD   dwHelpId;
} MENUEX_TEMPLATE_HEADER;

// item of MENUEX resource
#include <pshpack1.h>
typedef struct MENUEX_TEMPLATE_ITEM_HEADER
{
    DWORD   dwType;         // MFT_
    DWORD   dwState;        // MFS_
    DWORD   menuId;
    WORD    bResInfo;       // 0x80: is it last?, 0x01: popup
    //WCHAR szText[];
    //DWORD dwHelpId;       // only if popup
} MENUEX_TEMPLATE_ITEM_HEADER;
#include <poppack.h>

struct MENU_ENTRY
{
    WCHAR szCaption[128];
    WCHAR szFlags[64];
    WCHAR szCommandID[64];
    WCHAR szHelpID[64];
    WORD wDepth;
};

//////////////////////////////////////////////////////////////////////////////

class MenuRes
{
public:
    typedef MStringW    string_type;
    struct MenuItem
    {
        WORD            fItemFlags;
        WORD            wMenuID;
        WORD            wDepth;
        string_type     text;
    };
    typedef std::vector<MenuItem>  MenuItemsType;
    struct ExMenuItem
    {
        DWORD           dwType;     // MFT_
        DWORD           dwState;    // MFS_
        UINT            menuId;
        WORD            bResInfo;
        string_type     text;
        DWORD           dwHelpId;
        WORD            wDepth;
    };
    typedef std::vector<ExMenuItem>  ExMenuItemsType;

    MenuRes()
    {
        ZeroMemory(&m_header, sizeof(m_header));
    }

    bool IsExtended() const
    {
        return (m_header.wVersion == 1);
    }

    bool LoadFromStream(const MByteStreamEx& stream)
    {
        if (!stream.PeekWord(m_header.wVersion))
            return false;

        if (m_header.wVersion == 1)
            return LoadFromStreamEx(stream);

        MENUHEADER header;
        if (!stream.ReadRaw(header) ||
            header.wVersion != 0 || header.cbHeaderSize != 0)
        {
            return false;
        }

        std::stack<BYTE> flag_stack;
        flag_stack.push(true);

        WORD wDepth = 0, fItemFlags;
        MenuItem item;
        while (stream.PeekWord(fItemFlags))
        {
            if (fItemFlags & MF_POPUP)
            {
                flag_stack.push(!!(fItemFlags & MF_ENDMENU));

                POPUPMENUITEMHEAD head;
                if (!stream.ReadRaw(head) || !stream.ReadSz(item.text))
                    return false;

                item.fItemFlags = fItemFlags;
                item.wMenuID = 0;
                item.wDepth = wDepth++;
                m_items.push_back(item);
            }
            else
            {
                NORMALMENUITEMHEAD head;
                if (!stream.ReadRaw(head) || !stream.ReadSz(item.text))
                    return false;

                item.fItemFlags = fItemFlags;
                item.wMenuID = head.wMenuID;
                item.wDepth = wDepth;
                m_items.push_back(item);

                if (fItemFlags & MF_ENDMENU)
                {
                    --wDepth;
                    while (flag_stack.size() && flag_stack.top())
                    {
                        flag_stack.pop();
                        if (wDepth == 0)
                            break;
                        --wDepth;
                    }
                    if (flag_stack.empty())
                        break;
                }
            }
        }

        return true;
    }

    bool LoadFromStreamEx(const MByteStreamEx& stream)
    {
        if (!stream.ReadRaw(m_header))
        {
            assert(0);
            return false;
        }

        if (m_header.wVersion != 1 || m_header.wOffset < 4)
        {
            assert(0);
            return false;
        }
        stream.pos(4 + m_header.wOffset);
        stream.ReadDwordAlignment();

        std::stack<BYTE> flag_stack;
        flag_stack.push(true);

        WORD wDepth = 0;
        ExMenuItem exitem;
        MENUEX_TEMPLATE_ITEM_HEADER item_header;

        stream.ReadDwordAlignment();
        while (stream.ReadRaw(item_header))
        {
            if (!stream.ReadSz(exitem.text))
            {
                assert(0);
                return false;
            }

            if (item_header.bResInfo & 0x01)
            {
                flag_stack.push(!!(item_header.bResInfo & 0x80));

                stream.ReadDwordAlignment();
                if (!stream.ReadRaw(exitem.dwHelpId))
                    return false;

                exitem.dwType = item_header.dwType;
                exitem.dwState = item_header.dwState;
                exitem.menuId = item_header.menuId;
                exitem.bResInfo = item_header.bResInfo;
                exitem.wDepth = wDepth++;
                m_exitems.push_back(exitem);
            }
            else
            {
                exitem.dwHelpId = 0;
                exitem.dwType = item_header.dwType;
                exitem.dwState = item_header.dwState;
                exitem.menuId = item_header.menuId;
                exitem.bResInfo = item_header.bResInfo;
                exitem.wDepth = wDepth;
                m_exitems.push_back(exitem);

                if (item_header.bResInfo & 0x80)
                {
                    --wDepth;
                    while (flag_stack.size() && flag_stack.top())
                    {
                        flag_stack.pop();
                        if (wDepth == 0)
                            break;
                        --wDepth;
                    }
                    if (flag_stack.empty())
                        break;
                }
            }
            stream.ReadDwordAlignment();
        }
        return true;
    }

    string_type DumpFlags(WORD fItemFlags) const
    {
        string_type ret;
        if (fItemFlags & MF_GRAYED)
            ret += L", GRAYED";
        if (fItemFlags & MF_INACTIVE)
            ret += L", INACTIVE";
        if (fItemFlags & MF_BITMAP)
            ret += L", BITMAP";
        if (fItemFlags & MF_OWNERDRAW)
            ret += L", OWNERDRAW";
        if (fItemFlags & MF_CHECKED)
            ret += L", CHECKED";
        if (fItemFlags & MF_MENUBARBREAK)
            ret += L", MENUBARBREAK";
        if (fItemFlags & MF_MENUBREAK)
            ret += L", MENUBREAK";
        return ret;
    }

    std::vector<BYTE> data() const
    {
        MByteStreamEx stream;
        if (IsExtended())
            SaveToStreamEx(stream);
        else
            SaveToStream(stream);
        return stream.data();
    }

    bool SaveToStream(MByteStreamEx& stream) const
    {
        if (IsExtended())
            return SaveToStreamEx(stream);

        MENUHEADER header;
        header.wVersion = 0;
        header.cbHeaderSize = 0;
        if (!stream.WriteRaw(header))
            return false;

        for (auto& item : m_items)
        {
            WORD fItemFlags = item.fItemFlags;
            if (fItemFlags & MF_POPUP)
            {
                POPUPMENUITEMHEAD head;
                head.fItemFlags = fItemFlags;
                if (!stream.WriteRaw(head) || !stream.WriteSz(item.text))
                    return false;
            }
            else
            {
                NORMALMENUITEMHEAD head;
                head.fItemFlags = item.fItemFlags;
                head.wMenuID = item.wMenuID;
                if (!stream.WriteRaw(head) || !stream.WriteSz(item.text))
                    return false;
            }
        }

        return true;
    }

    bool SaveToStreamEx(MByteStreamEx& stream) const
    {
        if (!IsExtended())
            return false;

        MENUEX_TEMPLATE_HEADER header;
        header.wVersion = 1;
        header.wOffset = 4;
        header.dwHelpId = m_header.dwHelpId;
        if (!stream.WriteRaw(header))
        {
            assert(0);
            return false;
        }

        for (auto& exitem : m_exitems)
        {
            stream.WriteDwordAlignment();

            MENUEX_TEMPLATE_ITEM_HEADER item_header;
            item_header.dwType = exitem.dwType;
            item_header.dwState = exitem.dwState;
            item_header.menuId = exitem.menuId;
            item_header.bResInfo = exitem.bResInfo;
            if (!stream.WriteRaw(item_header) ||
                !stream.WriteSz(exitem.text))
            {
                assert(0);
                return false;
            }

            if (item_header.bResInfo & 0x01)
            {
                stream.WriteDwordAlignment();
                if (!stream.WriteRaw(exitem.dwHelpId))
                    return false;
            }
        }

        return true;
    }

    string_type Dump(MIdOrString name)
    {
        if (IsExtended())
            return DumpEx(name);

        string_type ret;

        if (name.m_id == 0)
        {
            ret += name.str();
        }
        else
        {
            ret += g_db.GetNameOfResID(IDTYPE_MENU, name.m_id);
        }

        ret += L" MENU\r\n";
        if (g_settings.bUseBeginEnd)
            ret += L"BEGIN\r\n";
        else
            ret += L"{\r\n";

        WORD wDepth = 0;
        for (auto& item : m_items)
        {
            while (item.wDepth < wDepth)
            {
                --wDepth;
                ret += string_type((wDepth + 1) * 4, L' ');
                if (g_settings.bUseBeginEnd)
                    ret += L"END\r\n";
                else
                    ret += L"}\r\n";
            }
            wDepth = item.wDepth;
            if (item.fItemFlags & MF_POPUP)
            {
                ret += string_type((item.wDepth + 1) * 4, L' ');
                ret += L"POPUP \"";
                ret += mstr_escape(item.text);
                ret += L"\"";
                ret += DumpFlags(item.fItemFlags);
                ret += L"\r\n";
                ret += string_type((item.wDepth + 1) * 4, L' ');
                if (g_settings.bUseBeginEnd)
                    ret += L"BEGIN\r\n";
                else
                    ret += L"{\r\n";
            }
            else
            {
                ret += string_type((item.wDepth + 1) * 4, L' ');
                if (item.wMenuID == 0 && item.text.empty())
                {
                    ret += L"MENUITEM SEPARATOR\r\n";
                }
                else
                {
                    ret += L"MENUITEM \"";
                    ret += mstr_escape(item.text);
                    ret += L"\", ";
                    if (0)
                    {
                        ret += mstr_dec_word(item.wMenuID);
                    }
                    else
                    {
                        ret += g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, item.wMenuID);
                    }
                    ret += DumpFlags(item.fItemFlags);
                    ret += L"\r\n";
                }
            }
        }
        while (0 < wDepth)
        {
            --wDepth;
            ret += string_type((wDepth + 1) * 4, L' ');
            if (g_settings.bUseBeginEnd)
                ret += L"END\r\n";
            else
                ret += L"}\r\n";
        }

        if (g_settings.bUseBeginEnd)
            ret += L"END\r\n";
        else
            ret += L"}\r\n";

        return ret;
    }

    string_type DumpEx(MIdOrString name)
    {
        string_type ret;

        if (name.m_id == 0)
        {
            ret += name.str();
        }
        else
        {
            ret += g_db.GetNameOfResID(IDTYPE_MENU, name.m_id);
        }

        ret += L" MENUEX\r\n";
        if (g_settings.bUseBeginEnd)
            ret += L"BEGIN\r\n";
        else
            ret += L"{\r\n";

        WORD wDepth = 0;
        for (auto& item : m_exitems)
        {
            while (item.wDepth < wDepth)
            {
                --wDepth;
                ret += string_type((wDepth + 1) * 4, L' ');
                if (g_settings.bUseBeginEnd)
                    ret += L"END\r\n";
                else
                    ret += L"}\r\n";
            }
            wDepth = item.wDepth;
            if (item.bResInfo & 0x01)
            {
                ret += string_type((item.wDepth + 1) * 4, L' ');
                ret += L"POPUP ";
                ret += mstr_quote(item.text);
                if (item.menuId || item.dwType || item.dwState || item.dwHelpId)
                {
                    ret += L", ";
                    if (0)
                    {
                        ret += mstr_dec_dword(item.menuId);
                    }
                    else
                    {
                        ret += g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, item.menuId);
                    }
                }
                if (item.dwType || item.dwState || item.dwHelpId)
                {
                    ret += L", ";
                    DWORD value = item.dwType;
                    ret += g_db.DumpBitFieldOrZero(L"MFT_", value);
                }
                if (item.dwState || item.dwHelpId)
                {
                    ret += L", ";
                    DWORD value = item.dwState;
                    ret += g_db.DumpBitFieldOrZero(L"MFS_", value);
                }
                if (item.dwHelpId)
                {
                    ret += L", ";
                    if (0)
                    {
                        ret += mstr_dec_dword(item.dwHelpId);
                    }
                    else
                    {
                        ret += g_db.GetNameOfResID(IDTYPE_HELP, item.dwHelpId);
                    }
                }
                ret += L"\r\n";
                ret += string_type((item.wDepth + 1) * 4, L' ');
                if (g_settings.bUseBeginEnd)
                    ret += L"BEGIN\r\n";
                else
                    ret += L"{\r\n";
            }
            else
            {
                ret += string_type((item.wDepth + 1) * 4, L' ');
                if (item.menuId == 0 && item.text.empty())
                {
                    ret += L"MENUITEM SEPARATOR\r\n";
                }
                else
                {
                    ret += L"MENUITEM ";
                    ret += mstr_quote(item.text);
                    if (item.menuId || item.dwType || item.dwState)
                    {
                        ret += L", ";
                        if (0)
                        {
                            ret += mstr_dec_dword(item.menuId);
                        }
                        else
                        {
                            ret += g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, item.menuId);
                        }
                    }
                    if (item.dwType || item.dwState)
                    {
                        ret += L", ";
                        DWORD value = item.dwType;
                        ret += g_db.DumpBitFieldOrZero(L"MFT_", value);
                    }
                    if (item.dwState)
                    {
                        ret += L", ";
                        DWORD value = item.dwState;
                        ret += g_db.DumpBitFieldOrZero(L"MFS_", value);
                    }
                    ret += L"\r\n";
                }
            }
        }
        while (0 < wDepth)
        {
            --wDepth;
            ret += string_type((wDepth + 1) * 4, L' ');
            if (g_settings.bUseBeginEnd)
                ret += L"END\r\n";
            else
                ret += L"}\r\n";
        }

        if (g_settings.bUseBeginEnd)
            ret += L"END\r\n";
        else
            ret += L"}\r\n";

        return ret;
    }

    void Update()
    {
        if (IsExtended())
        {
            for (size_t i = 0; i < m_exitems.size(); ++i)
            {
                if (IsLastItem(i))
                {
                    m_exitems[i].bResInfo |= 0x80;
                }
                else
                {
                    m_exitems[i].bResInfo &= ~0x80;
                }

                if (IsParent(i))
                {
                    m_exitems[i].bResInfo |= 0x01;
                }
                else
                {
                    m_exitems[i].bResInfo &= ~0x01;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < m_items.size(); ++i)
            {
                if (IsLastItem(i))
                {
                    m_items[i].fItemFlags |= MF_ENDMENU;
                }
                else
                {
                    m_items[i].fItemFlags &= ~MF_ENDMENU;
                }

                if (IsParent(i))
                {
                    m_items[i].fItemFlags |= MF_POPUP;
                }
                else
                {
                    m_items[i].fItemFlags &= ~MF_POPUP;
                }
            }
        }
    }

    MENUEX_TEMPLATE_HEADER& header()
    {
        return m_header;
    }
    const MENUEX_TEMPLATE_HEADER& header() const
    {
        return m_header;
    }

    MenuItemsType& items()
    {
        return m_items;
    }
    const MenuItemsType& items() const
    {
        return m_items;
    }

    ExMenuItemsType& exitems()
    {
        return m_exitems;
    }
    const ExMenuItemsType& exitems() const
    {
        return m_exitems;
    }

protected:
    MENUEX_TEMPLATE_HEADER  m_header;
    MenuItemsType           m_items;
    ExMenuItemsType         m_exitems;

    bool IsParent(size_t iItem) const
    {
        if (IsExtended())
        {
            WORD wDepth = m_exitems[iItem].wDepth;
            if (iItem + 1 < m_exitems.size() &&
                wDepth < m_exitems[iItem + 1].wDepth)
            {
                return true;
            }
        }
        else
        {
            WORD wDepth = m_items[iItem].wDepth;
            if (iItem + 1 < m_items.size() &&
                wDepth < m_items[iItem + 1].wDepth)
            {
                return true;
            }
        }
        return false;
    }

    bool IsLastItem(size_t iItem) const
    {
        bool Found = false;
        if (IsExtended())
        {
            WORD wDepth = m_exitems[iItem].wDepth;
            for (size_t i = iItem + 1; i < m_exitems.size(); ++i)
            {
                if (m_exitems[i].wDepth == wDepth)
                {
                    Found = true;
                }
                if (m_exitems[i].wDepth < wDepth)
                {
                    break;
                }
            }
        }
        else
        {
            WORD wDepth = m_items[iItem].wDepth;
            for (size_t i = iItem + 1; i < m_items.size(); ++i)
            {
                if (m_items[i].wDepth == wDepth)
                {
                    Found = true;
                }
                if (m_items[i].wDepth < wDepth)
                {
                    break;
                }
            }
        }
        return !Found;
    }
};

inline MStringW GetMenuFlags(WORD fItemFlags)
{
    MStringW str;

    if ((fItemFlags & MF_GRAYED) == MF_GRAYED)
        str += L"G ";

    if ((fItemFlags & MF_BITMAP) == MF_BITMAP)
        str += L"B ";

    if ((fItemFlags & MF_OWNERDRAW) == MF_OWNERDRAW)
        str += L"OD ";

    if ((fItemFlags & MF_CHECKED) == MF_CHECKED)
        str += L"C ";

    if ((fItemFlags & MF_MENUBARBREAK) == MF_MENUBARBREAK)
        str += L"MBB ";

    if ((fItemFlags & MF_MENUBREAK) == MF_MENUBREAK)
        str += L"MB ";

    return str;
}

inline void SetMenuFlags(WORD& fItemFlags, const MStringW& str)
{
    fItemFlags = 0;
    MStringW str2 = L" " + str;

    if (str2.find(L" G ") != MStringW::npos)
        fItemFlags |= MF_GRAYED;

    if (str2.find(L" B ") != MStringW::npos)
        fItemFlags |= MF_BITMAP;

    if (str2.find(L" OD ") != MStringW::npos)
        fItemFlags |= MF_OWNERDRAW;

    if (str2.find(L" C ") != MStringW::npos)
        fItemFlags |= MF_CHECKED;

    if (str2.find(L" MBB ") != MStringW::npos)
        fItemFlags |= MF_MENUBARBREAK;

    if (str2.find(L" MB ") != MStringW::npos)
        fItemFlags |= MF_MENUBREAK;
}

inline MStringW GetMenuTypeAndState(DWORD dwType, DWORD dwState)
{
    MStringW str;

    if ((dwState & MFS_GRAYED) == MFS_GRAYED)
        str += L"G ";

    if ((dwType & MFT_BITMAP) == MFT_BITMAP)
        str += L"B ";

    if ((dwType & MFT_OWNERDRAW) == MFT_OWNERDRAW)
        str += L"OD ";

    if ((dwState & MFS_CHECKED) == MFS_CHECKED)
        str += L"C ";

    if ((dwType & MFT_SEPARATOR) == MFT_SEPARATOR)
        str += L"S ";

    if ((dwType & MFT_MENUBARBREAK) == MFT_MENUBARBREAK)
        str += L"MBB ";

    if ((dwType & MFT_MENUBREAK) == MFT_MENUBREAK)
        str += L"MB ";

    if ((dwState & MFS_DEFAULT) == MFS_DEFAULT)
        str += L"D ";

    if ((dwState & MFS_HILITE) == MFS_HILITE)
        str += L"H ";

    if ((dwType & MFT_RADIOCHECK) == MFT_RADIOCHECK)
        str += L"RC ";

    if ((dwType & MFT_RIGHTORDER) == MFT_RIGHTORDER)
        str += L"RO ";

    if ((dwType & MFT_RIGHTJUSTIFY) == MFT_RIGHTJUSTIFY)
        str += L"RJ ";

    return str;
}

inline void SetMenuTypeAndState(DWORD& dwType, DWORD& dwState, const MStringW& str)
{
    dwType = dwState = 0;
    MStringW str2 = L" " + str;

    if (str2.find(L" G ") != MStringW::npos)
        dwState |= MFS_GRAYED;

    if (str2.find(L" B ") != MStringW::npos)
        dwType |= MFT_BITMAP;

    if (str2.find(L" OD ") != MStringW::npos)
        dwType |= MFT_OWNERDRAW;

    if (str2.find(L" C ") != MStringW::npos)
        dwState |= MFS_CHECKED;

    if (str2.find(L" S ") != MStringW::npos)
        dwType |= MFT_SEPARATOR;

    if (str2.find(L" MBB ") != MStringW::npos)
        dwType |= MFT_MENUBARBREAK;

    if (str2.find(L" MB ") != MStringW::npos)
        dwType |= MFT_MENUBREAK;

    if (str2.find(L" D ") != MStringW::npos)
        dwState |= MFS_DEFAULT;

    if (str2.find(L" H ") != MStringW::npos)
        dwState |= MFS_HILITE;

    if (str2.find(L" RC ") != MStringW::npos)
        dwType |= MFT_RADIOCHECK;

    if (str2.find(L" RO ") != MStringW::npos)
        dwType |= MFT_RIGHTORDER;

    if (str2.find(L" RJ ") != MStringW::npos)
        dwType |= MFT_RIGHTJUSTIFY;
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MENU_RES_HPP_
