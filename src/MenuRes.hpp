// MenuRes.hpp  --- Menu Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

MStringW GetMenuFlags(WORD fItemFlags);
void SetMenuFlags(WORD& fItemFlags, const MStringW& str);
MStringW GetMenuTypeAndState(DWORD dwType, DWORD dwState);
void SetMenuTypeAndState(DWORD& dwType, DWORD& dwState, const MStringW& str);

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
//     MF_END           0x0080   // Used internally

#ifndef MF_INACTIVE
    #define MF_INACTIVE     MF_DISABLED
#endif
#ifndef MF_END
    #define MF_END          0x0080
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
    MStringW szCaption;
    MStringW szFlags;
    MStringW szCommandID;
    MStringW szHelpID;
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

    bool LoadFromStream(const MByteStreamEx& stream);
    bool LoadFromStreamEx(const MByteStreamEx& stream);
    bool SaveToStream(MByteStreamEx& stream) const;
    bool SaveToStreamEx(MByteStreamEx& stream) const;
    string_type Dump(const MIdOrString& name) const;
    string_type DumpEx(const MIdOrString& name) const;
    std::vector<BYTE> data() const;

    void Update();

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

    bool IsParent(size_t iItem) const;
    bool IsLastItem(size_t iItem) const;
    string_type DumpFlags(WORD fItemFlags) const;
};
