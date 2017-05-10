#ifndef MENU_RES_HPP_
#define MENU_RES_HPP_

#include <windows.h>
#include <vector>
#include <stack>

#include "id_string.hpp"
#include "ConstantsDB.hpp"

// header of RT_MENU
typedef struct MENUHEADER
{
    WORD    wVersion;       // zero
    WORD    cbHeaderSize;   // zero
} MENUHEADER;

typedef struct NORMALMENUITEMHEAD
{
    WORD   fItemFlags;
    WORD   wMenuID;
    //WCHAR  szItemText[];
} NORMALMENUITEMHEAD;

typedef struct POPUPMENUITEMHEAD
{
    WORD    fItemFlags;
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

class MenuRes
{
public:
    typedef std::wstring    string_type;
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
        DWORD           dwType;
        DWORD           dwState;
        UINT            menuId;
        DWORD           bResInfo;
        string_type     text;
        DWORD           dwHelpId;
        WORD            wDepth;
    };
    typedef std::vector<ExMenuItem>  ExMenuItemsType;

    MenuRes()
    {
        ZeroMemory(&m_header, sizeof(m_header));
    }

    BOOL IsExtended() const
    {
        return (m_header.wVersion == 1);
    }

    BOOL LoadFromStream(const ByteStream& stream)
    {
        if (!stream.PeekWord(m_header.wVersion))
            return FALSE;

        if (m_header.wVersion == 1)
            return LoadFromStreamEx(stream);

        MENUHEADER header;
        if (!stream.ReadRaw(header) ||
            header.wVersion != 0 || header.cbHeaderSize != 0)
        {
            return FALSE;
        }

        std::stack<BYTE> flag_stack;
        flag_stack.push(TRUE);

        WORD wDepth = 0, fItemFlags;
        MenuItem item;
        while (stream.PeekWord(fItemFlags))
        {
            if (fItemFlags & MF_POPUP)
            {
                flag_stack.push(!!(fItemFlags & MF_ENDMENU));

                POPUPMENUITEMHEAD head;
                if (!stream.ReadRaw(head) || !stream.ReadSz(item.text))
                    return FALSE;

                item.fItemFlags = fItemFlags;
                item.wMenuID = 0;
                item.wDepth = wDepth++;
                m_items.push_back(item);
            }
            else
            {
                NORMALMENUITEMHEAD head;
                if (!stream.ReadRaw(head) || !stream.ReadSz(item.text))
                    return FALSE;

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

        return TRUE;
    }

    BOOL LoadFromStreamEx(const ByteStream& stream)
    {
        if (!stream.ReadRaw(m_header))
        {
            assert(0);
            return FALSE;
        }

        if (m_header.wVersion != 1 || m_header.wOffset < 4)
        {
            assert(0);
            return FALSE;
        }
        stream.pos(4 + m_header.wOffset);
        stream.ReadDwordAlignment();

        std::stack<BYTE> flag_stack;
        flag_stack.push(TRUE);

        WORD wDepth = 0;
        ExMenuItem exitem;
        MENUEX_TEMPLATE_ITEM_HEADER item_header;

        stream.ReadDwordAlignment();
        while (stream.ReadRaw(item_header))
        {
            if (!stream.ReadSz(exitem.text))
            {
                assert(0);
                return FALSE;
            }

            if (item_header.bResInfo & 0x01)
            {
                flag_stack.push(!!(item_header.bResInfo & 0x80));

                stream.ReadDwordAlignment();
                if (!stream.ReadRaw(exitem.dwHelpId))
                    return FALSE;

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
        return TRUE;
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

    string_type Dump(ID_OR_STRING name, const ConstantsDB& db)
    {
        if (IsExtended())
            return DumpEx(name, db);

        string_type ret;

        ret += name.wstr();
        ret += L" MENU\r\n";
        ret += L"{\r\n";

        WORD wDepth = 0;
        MenuItemsType::iterator it, end = m_items.end();
        for (it = m_items.begin(); it != end; ++it)
        {
            while (it->wDepth < wDepth)
            {
                --wDepth;
                ret += string_type((wDepth + 1) * 4, L' ');
                ret += L"}\r\n";
            }
            wDepth = it->wDepth;
            if (it->fItemFlags & MF_POPUP)
            {
                ret += string_type((it->wDepth + 1) * 4, L' ');
                ret += L"POPUP \"";
                ret += str_escape(it->text);
                ret += L"\"";
                ret += DumpFlags(it->fItemFlags);
                ret += L"\r\n";
                ret += string_type((it->wDepth + 1) * 4, L' ');
                ret += L"{\r\n";
            }
            else
            {
                ret += string_type((it->wDepth + 1) * 4, L' ');
                if (it->wMenuID == 0 && it->text.empty())
                {
                    ret += L"MENUITEM SEPARATOR\r\n";
                }
                else
                {
                    ret += L"MENUITEM \"";
                    ret += str_escape(it->text);
                    ret += L"\", ";
                    ret += str_dec(it->wMenuID);
                    ret += DumpFlags(it->fItemFlags);
                    ret += L"\r\n";
                }
            }
        }
        while (0 < wDepth)
        {
            --wDepth;
            ret += string_type((wDepth + 1) * 4, L' ');
            ret += L"}\r\n";
        }

        ret += L"}\r\n";

        return ret;
    }

    string_type DumpEx(ID_OR_STRING name, const ConstantsDB& db)
    {
        string_type ret;

        ret += name.wstr();
        ret += L" MENUEX\r\n";
        ret += L"{\r\n";

        WORD wDepth = 0;
        ExMenuItemsType::iterator it, end = m_exitems.end();
        for (it = m_exitems.begin(); it != end; ++it)
        {
            while (it->wDepth < wDepth)
            {
                --wDepth;
                ret += string_type((wDepth + 1) * 4, L' ');
                ret += L"}\r\n";
            }
            wDepth = it->wDepth;
            if (it->bResInfo & 0x01)
            {
                ret += string_type((it->wDepth + 1) * 4, L' ');
                ret += L"POPUP \"";
                ret += str_escape(it->text);
                ret += L"\"";
                if (it->menuId || it->dwType || it->dwState || it->dwHelpId)
                {
                    ret += L", ";
                    ret += str_dec(it->menuId);
                }
                if (it->dwType || it->dwState || it->dwHelpId)
                {
                    ret += L", ";
                    DWORD value = it->dwType;
                    ret += db.DumpBitField(L"MFT_", L"", value);
                }
                if (it->dwState || it->dwHelpId)
                {
                    ret += L", ";
                    DWORD value = it->dwState;
                    ret += db.DumpBitField(L"MFS_", L"", value);
                }
                if (it->dwHelpId)
                {
                    ret += L", ";
                    ret += str_hex(it->dwHelpId);
                }
                ret += L"\r\n";
                ret += string_type((it->wDepth + 1) * 4, L' ');
                ret += L"{\r\n";
            }
            else
            {
                ret += string_type((it->wDepth + 1) * 4, L' ');
                if (it->menuId == 0 && it->text.empty())
                {
                    ret += L"MENUITEM SEPARATOR\r\n";
                }
                else
                {
                    ret += L"MENUITEM \"";
                    ret += str_escape(it->text);
                    if (it->menuId || it->dwType || it->dwState)
                    {
                        ret += L", ";
                        ret += str_dec(it->menuId);
                    }
                    if (it->dwType || it->dwState)
                    {
                        ret += L", ";
                        DWORD value = it->dwType;
                        ret += db.DumpBitField(L"MFT_", L"", value);
                    }
                    if (it->dwState)
                    {
                        ret += L", ";
                        DWORD value = it->dwState;
                        ret += db.DumpBitField(L"MFS_", L"", value);
                    }
                    ret += L"\r\n";
                }
            }
        }
        while (0 < wDepth)
        {
            --wDepth;
            ret += string_type((wDepth + 1) * 4, L' ');
            ret += L"}\r\n";
        }

        ret += L"}\r\n";

        return ret;
    }

protected:
    MENUEX_TEMPLATE_HEADER  m_header;
    MenuItemsType           m_items;
    ExMenuItemsType         m_exitems;
};

#endif  // ndef MENU_RES_HPP_
