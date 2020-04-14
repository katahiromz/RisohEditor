// RisohEditor.cpp --- RisohEditor
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

#include "RisohEditor.hpp"

//////////////////////////////////////////////////////////////////////////////
// constants

#define TV_WIDTH        250     // default m_hwndTV width
#define BV_WIDTH        160     // default m_hBmpView width
#define BE_HEIGHT       90      // default m_hBinEdit height
#define CX_STATUS_PART  80      // status bar part width

#define MYWM_POSTSEARCH (WM_USER + 200)

MString GetLanguageStatement(WORD langid, BOOL bOldStyle);

// the maximum number of captions to remember
static const DWORD s_nMaxCaptions = 10;

// the maximum number of backup
static const UINT s_nBackupMaxCount = 5;

// contents modified?
static BOOL s_bModified = FALSE;

void DoSetFileModified(BOOL bModified)
{
    s_bModified = bModified;
}

static HWND s_hMainWnd = NULL;

enum IMPORT_RESULT
{
    IMPORTED,
    IMPORT_CANCELLED,
    IMPORT_FAILED,
    NOT_IMPORTABLE
};

//////////////////////////////////////////////////////////////////////////////
// global variables

#ifdef USE_GLOBALS
    ConstantsDB g_db;           // constants database
    RisohSettings g_settings;   // settings
    EntrySet g_res;             // the set of resource items
#endif

typedef HRESULT (WINAPI *SETWINDOWTHEME)(HWND, LPCWSTR, LPCWSTR);
static SETWINDOWTHEME s_pSetWindowTheme = NULL;

//////////////////////////////////////////////////////////////////////////////
// useful global functions

BOOL IsFileLockedDx(LPCTSTR pszFileName)
{
    if (!PathFileExistsW(pszFileName))
        return FALSE;

    HANDLE hFile;
    hFile = CreateFileW(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        return FALSE;
    }

    return TRUE;
}

// "." or ".."
#define IS_DOTS(psz) ((psz)[0] == '.' && ((psz)[1] == '\0' || (psz)[1] == '.' && (psz)[2] == '\0'))

// delete a directory (a folder)
BOOL DeleteDirectoryDx(LPCTSTR pszDir)
{
    TCHAR szDirOld[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA find;

    GetCurrentDirectory(MAX_PATH, szDirOld);
    if (!SetCurrentDirectory(pszDir))
        return FALSE;

    hFind = FindFirstFile(TEXT("*"), &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!IS_DOTS(find.cFileName))
            {
                if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    DeleteDirectoryDx(find.cFileName);
                }
                else
                {
                    SetFileAttributes(find.cFileName, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(find.cFileName);
                }
            }
        } while(FindNextFile(hFind, &find));
        FindClose(hFind);
    }
    SetCurrentDirectory(szDirOld);

    SetFileAttributes(pszDir, FILE_ATTRIBUTE_DIRECTORY);
    return RemoveDirectory(pszDir);
}

// is the path an empty directory?
BOOL IsEmptyDirectoryDx(LPCTSTR pszPath)
{
    WCHAR sz[MAX_PATH];
    StringCchCopy(sz, _countof(sz), pszPath);
    StringCchCat(sz, _countof(sz), L"\\*");

    BOOL bFound = FALSE;
    WIN32_FIND_DATA find;
    HANDLE hFind = FindFirstFile(sz, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            MString str = find.cFileName;
            if (str != L"." && str != L"..")
            {
                bFound = TRUE;
                break;
            }
        } while (FindNextFile(hFind, &find));

        FindClose(hFind);
    }

    return !bFound;
}

// replace some fullwidth characters with halfwidth characters
void ReplaceFullWithHalf(LPWSTR pszText)
{
    MStringW strFullWidth = LoadStringDx(IDS_FULLWIDTH);
    MStringW strHalfWidth = LoadStringDx(IDS_HALFWIDTH);

    for (DWORD i = 0; pszText[i]; ++i)
    {
        size_t k = strFullWidth.find(pszText[i]);
        if (k != MStringW::npos)
        {
            pszText[i] = strHalfWidth[k];
        }
    }
}

// replace some fullwidth characters with halfwidth characters
void ReplaceFullWithHalf(MStringW& strText)
{
    ReplaceFullWithHalf(&strText[0]);
}

// get the path of a shortcut file
BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath)
{
    BOOL                bRes = FALSE;
    WIN32_FIND_DATAW    find;
    IShellLinkW*        pShellLink;
    IPersistFile*       pPersistFile;
    HRESULT             hRes;

    // NOTE: CoInitialize/CoInitializeEx call required before this
    pszPath[0] = 0;
    hRes = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLinkW, (void **)&pShellLink);
    if (SUCCEEDED(hRes))
    {
        hRes = pShellLink->QueryInterface(IID_IPersistFile,
                                          (void **)&pPersistFile);
        if (SUCCEEDED(hRes))
        {
            hRes = pPersistFile->Load(pszLnkFile, STGM_READ);
            if (SUCCEEDED(hRes))
            {
                pShellLink->Resolve(hwnd, SLR_NO_UI | SLR_UPDATE);

                hRes = pShellLink->GetPath(pszPath, MAX_PATH, &find, 0);
                if (SUCCEEDED(hRes) && 0 != pszPath[0])
                {
                    bRes = TRUE;
                }
            }
            pPersistFile->Release();
        }
        pShellLink->Release();
    }
    return bRes;
}

// dump a file
BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size)
{
    using namespace std;

    FILE *fp = _wfopen(filename, L"wb");        // open

    int n = (int)fwrite(pv, size, 1, fp);   // write

    fclose(fp);     // close the files

    return n == 1;  // success or not
}

// dump data as a text
MStringW DumpBinaryAsText(const std::vector<BYTE>& data)
{
    MStringW ret;
    WCHAR sz[64];
    DWORD addr, size = DWORD(data.size());

    // is it empty?
    if (data.empty())
    {
        return ret;
    }

    ret.reserve(data.size() * 3);   // for speed

    // add the head
    ret +=
        L"+ADDRESS  +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F  0123456789ABCDEF\r\n"
        L"--------  -----------------------  -----------------------  ----------------\r\n";

    // for all the addresses
    bool ending_flag = false;
    for (addr = 0; !ending_flag; ++addr)
    {
        if ((addr & 0xF) != 0)
            continue;

        // add the address
        StringCchPrintfW(sz, _countof(sz), L"%08lX  ", addr);
        ret += sz;

        ending_flag = false;

        // add the data
        for (DWORD i = 0; i < 16; ++i)
        {
            // add a space if the lowest digit was 8
            if (i == 8)
                ret += L' ';

            // add 3 characters
            DWORD offset = addr + i;    // the address to output
            if (offset < size)
            {
                StringCchPrintfW(sz, _countof(sz), L"%02X ", data[offset]);
                ret += sz;
            }
            else
            {
                ret += L"   ";
                ending_flag = true;
            }
        }

        // add the separation space
        ret += L' ';

        // add the characters
        for (DWORD i = 0; i < 16; ++i)
        {
            DWORD offset = addr + i;    // the address to output
            if (offset < size)
            {
                if (data[offset] == 0)
                    ret += L' ';        // the NUL character
                else if (data[offset] < 0x20 || data[offset] > 0x7F)
                    ret += L'.';        // invisible character
                else
                    ret += WCHAR(data[offset]);     // otherwise
            }
            else
            {
                ret += L' ';            // out of range
                ending_flag = true;
            }
        }

        // add a newline
        ret += L"\r\n";
    }

    return ret;     // the result
}

//////////////////////////////////////////////////////////////////////////////
// window styles

// store the window style info to a vector
void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel)
{
    for (size_t i = 0; i < sel.size(); ++i)
    {
        sel[i] = (ListBox_GetSel(hLst, (DWORD)i) > 0);
    }
}

// store the window style info to a vector
void GetStyleSelect(std::vector<BYTE>& sel,
                    const ConstantsDB::TableType& table, DWORD dwValue)
{
    sel.resize(table.size());
    for (size_t i = 0; i < table.size(); ++i)
    {
        if (table[i].name.find(L'|') != ConstantsDB::StringType::npos)
            continue;

        if ((dwValue & table[i].mask) == table[i].value)
            sel[i] = TRUE;
        else
            sel[i] = FALSE;
    }
}

// analyse the difference of two window styles
DWORD AnalyseStyleDiff(
    DWORD dwValue, ConstantsDB::TableType& table,
    std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel)
{
    assert(old_sel.size() == new_sel.size());
    for (size_t i = 0; i < old_sel.size(); ++i)
    {
        if (table[i].name.find(L'|') != ConstantsDB::StringType::npos)
            continue;

        if (old_sel[i] && !new_sel[i])
        {
            dwValue &= ~table[i].mask;
        }
        else if (!old_sel[i] && new_sel[i])
        {
            dwValue &= ~table[i].mask;
            dwValue |= table[i].value;
        }
    }
    return dwValue;
}

// initialize the style list box
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table)
{
    // clear all the items of listbox
    ListBox_ResetContent(hLst);

    for (auto& table_entry : table)
    {
        if (table_entry.name.find(L'|') != ConstantsDB::StringType::npos)
            continue;

        ListBox_AddString(hLst, table_entry.name.c_str());
    }

    ListBox_SetHorizontalExtent(hLst, 300);
}

//////////////////////////////////////////////////////////////////////////////
// font names

// the callback for InitFontComboBox
static int CALLBACK
EnumFontFamProc(ENUMLOGFONT *lpelf,
                NEWTEXTMETRIC *lpntm,
                INT FontType,
                LPARAM lParam)
{
    HWND hCmb = HWND(lParam);

    // ignore vertical fonts
    if (lpelf->elfLogFont.lfFaceName[0] != TEXT('@'))
        ComboBox_AddString(hCmb, lpelf->elfLogFont.lfFaceName);

    return TRUE;    // continue
}

// initialize the font combobox
void InitFontComboBox(HWND hCmb)
{
    HDC hDC = CreateCompatibleDC(NULL);
    EnumFontFamilies(hDC, NULL, (FONTENUMPROC)EnumFontFamProc, (LPARAM)hCmb);
    DeleteDC(hDC);
}

// character set information
typedef struct CharSetData
{
    BYTE CharSet;
    LPCTSTR name;
} CharSetData;

static const CharSetData s_charset_entries[] =
{
    { ANSI_CHARSET, TEXT("ANSI_CHARSET") },
    { DEFAULT_CHARSET, TEXT("DEFAULT_CHARSET") },
    { SYMBOL_CHARSET, TEXT("SYMBOL_CHARSET") },
    { SHIFTJIS_CHARSET, TEXT("SHIFTJIS_CHARSET") },
    { HANGEUL_CHARSET, TEXT("HANGEUL_CHARSET") },
    { HANGUL_CHARSET, TEXT("HANGUL_CHARSET") },
    { GB2312_CHARSET, TEXT("GB2312_CHARSET") },
    { CHINESEBIG5_CHARSET, TEXT("CHINESEBIG5_CHARSET") },
    { OEM_CHARSET, TEXT("OEM_CHARSET") },
    { JOHAB_CHARSET, TEXT("JOHAB_CHARSET") },
    { HEBREW_CHARSET, TEXT("HEBREW_CHARSET") },
    { ARABIC_CHARSET, TEXT("ARABIC_CHARSET") },
    { GREEK_CHARSET, TEXT("GREEK_CHARSET") },
    { TURKISH_CHARSET, TEXT("TURKISH_CHARSET") },
    { VIETNAMESE_CHARSET, TEXT("VIETNAMESE_CHARSET") },
    { THAI_CHARSET, TEXT("THAI_CHARSET") },
    { EASTEUROPE_CHARSET, TEXT("EASTEUROPE_CHARSET") },
    { RUSSIAN_CHARSET, TEXT("RUSSIAN_CHARSET") },
    { MAC_CHARSET, TEXT("MAC_CHARSET") },
    { BALTIC_CHARSET, TEXT("BALTIC_CHARSET") },
};

// initialize the charset combobox
void InitCharSetComboBox(HWND hCmb, BYTE CharSet)
{
    // clear all the items of combobox
    ComboBox_ResetContent(hCmb);

    // add entries to combobox
    for (auto& entry : s_charset_entries)
    {
        ComboBox_AddString(hCmb, entry.name);
    }

    // set data (charset values)
    for (UINT i = 0; i < _countof(s_charset_entries); ++i)
    {
        ComboBox_SetItemData(hCmb, i, s_charset_entries[i].CharSet);
    }

    // select DEFAULT_CHARSET
    ComboBox_SetCurSel(hCmb, 1);

    for (UINT i = 0; i < _countof(s_charset_entries); ++i)
    {
        if (s_charset_entries[i].CharSet == CharSet)
        {
            // charset was matched
            ComboBox_SetCurSel(hCmb, i);
            break;
        }
    }
}

// get charset value from the charset combobox
BYTE GetCharSetFromComboBox(HWND hCmb)
{
    // get current selection of combobox
    INT i = ComboBox_GetCurSel(hCmb);
    if (i == CB_ERR)    // not selected
        return DEFAULT_CHARSET;     // return the default value

    if (i < _countof(s_charset_entries))
        return s_charset_entries[i].CharSet;    // return the charset value

    return DEFAULT_CHARSET;     // return the default value
}

//////////////////////////////////////////////////////////////////////////////
// misc.

// initialize the caption combobox
void InitCaptionComboBox(HWND hCmb, LPCTSTR pszCaption)
{
    // clear all the items of combobox
    ComboBox_ResetContent(hCmb);

    // add captions from settings
    for (auto& cap : g_settings.captions)
    {
        ComboBox_AddString(hCmb, cap.c_str());
    }

    // set the text
    ComboBox_SetText(hCmb, pszCaption);
}

// initialize the control class combobox
void InitClassComboBox(HWND hCmb, LPCTSTR pszClass)
{
    // clear all the items of combobox
    ComboBox_ResetContent(hCmb);

    auto table = g_db.GetTable(TEXT("CONTROL.CLASSES"));

    for (auto& table_entry : table)
    {
        INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
        if (table_entry.name == pszClass)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

// initialize the window class combobox
void InitWndClassComboBox(HWND hCmb, LPCTSTR pszWndClass)
{
    // clear all the items of combobox
    ComboBox_ResetContent(hCmb);

    // get the control classes
    auto table = g_db.GetTable(TEXT("CONTROL.CLASSES"));

    for (auto& table_entry : table)
    {
        if (table_entry.value > 2)
            continue;   // not a window class

        // add the window class
        INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
        if (table_entry.name == pszWndClass)
        {
            // matched. select
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

// initialize the control ID combobox
void InitCtrlIDComboBox(HWND hCmb)
{
    // add the control IDs
    auto table = g_db.GetTable(TEXT("CTRLID"));
    for (auto& table_entry : table)
    {
        ComboBox_AddString(hCmb, table_entry.name.c_str());
    }

    // get the prefix of Control.ID
    table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_CONTROL].name;
    if (prefix.size())
    {
        // get the resource IDs by the prefix
        table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        for (auto& table_entry : table)
        {
            // add the resource IDs
            ComboBox_AddString(hCmb, table_entry.name.c_str());
        }
    }

    // get the prefix of Command.ID
    table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
    prefix = table[IDTYPE_COMMAND].name;
    if (prefix.size())
    {
        // get the resource IDs by the prefix
        table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        for (auto& table_entry : table)
        {
            // add the resource IDs
            ComboBox_AddString(hCmb, table_entry.name.c_str());
        }
    }

    // get the prefix of New.Command.ID
    table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
    prefix = table[IDTYPE_NEWCOMMAND].name;
    if (prefix.size())
    {
        // get the resource IDs by the prefix
        table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        for (auto& table_entry : table)
        {
            // add the resource IDs
            ComboBox_AddString(hCmb, table_entry.name.c_str());
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// resources

// switch between a resource ID and an IDTYPE_
void Res_ReplaceResTypeString(MString& str, bool bRevert = false)
{
    if (bRevert)
    {
        // revert
        if (str == L"Icon.ID")
            str = L"RT_GROUP_ICON";
        else if (str == L"Cursor.ID")
            str = L"RT_GROUP_CURSOR";
        else if (str == L"Accel.ID")
            str = L"RT_ACCELERATOR";
        else if (str == L"AniCursor.ID")
            str = L"RT_ANICURSOR";
        else if (str == L"AniIcon.ID")
            str = L"RT_ANIICON";
        else if (str == L"Dialog.ID")
            str = L"RT_DIALOG";
        else if (str == L"Menu.ID")
            str = L"RT_MENU";
        else if (str == L"Bitmap.ID")
            str = L"RT_BITMAP";
        else if (str == L"RCData.ID")
            str = L"RT_RCDATA";
        else if (str == L"Html.ID")
            str = L"RT_HTML";
    }
    else
    {
        // convert
        if (str == L"RT_GROUP_CURSOR")
            str = L"Cursor.ID";
        else if (str == L"RT_GROUP_ICON")
            str = L"Icon.ID";
        else if (str == L"RT_ACCELERATOR")
            str = L"Accel.ID";
        else if (str == L"RT_ANICURSOR")
            str = L"AniCursor.ID";
        else if (str == L"RT_ANIICON")
            str = L"AniIcon.ID";
        else if (str == L"RT_DIALOG")
            str = L"Dialog.ID";
        else if (str == L"RT_MENU")
            str = L"Menu.ID";
        else if (str == L"RT_BITMAP")
            str = L"Bitmap.ID";
        else if (str == L"RT_RCDATA")
            str = L"RCData.ID";
        else if (str == L"RT_HTML")
            str = L"Html.ID";
    }
}

MIdOrString ResourceTypeFromIDType(INT nIDTYPE_)
{
    MIdOrString type;
    switch (nIDTYPE_)
    {
    case IDTYPE_CURSOR:     type = RT_GROUP_CURSOR; break;
    case IDTYPE_BITMAP:     type = RT_BITMAP; break;
    case IDTYPE_MENU:       type = RT_MENU; break;
    case IDTYPE_DIALOG:     type = RT_DIALOG; break;
    case IDTYPE_ACCEL:      type = RT_ACCELERATOR; break;
    case IDTYPE_ICON:       type = RT_GROUP_ICON; break;
    case IDTYPE_ANICURSOR:  type = RT_ANICURSOR; break;
    case IDTYPE_ANIICON:    type = RT_ANIICON; break;
    case IDTYPE_HTML:       type = RT_HTML; break;
    case IDTYPE_RESOURCE:   type.clear(); break;
    case IDTYPE_RCDATA:     type = RT_RCDATA; break;
    default: break;
    }
    return type;
}

MString GetAssoc(const MString& name)
{
    if (name == L"IDC_STATIC")
        return L"Control.ID";

    MString ret;
    MString prefix = name.substr(0, name.find(L'_') + 1);
    if (prefix.empty())
        return L"";

    MIdOrString type;

    std::vector<INT> indexes = GetPrefixIndexes(prefix);
    for (size_t i = 0; i < indexes.size(); ++i)
    {
        auto nIDTYPE_ = IDTYPE_(indexes[i]);
        if (nIDTYPE_ == IDTYPE_UNKNOWN)
            continue;

        auto type = ResourceTypeFromIDType(nIDTYPE_);

        MIdOrString name_or_id;
        WORD wName = (WORD)g_db.GetValue(L"RESOURCE.ID", name);
        if (wName)
            name_or_id = wName;
        else
            name_or_id.m_str = name;

        EntrySetBase found;
        g_res.search(found, ET_LANG, type, name_or_id);

        if (found.size() && g_db.IsEntityIDType(nIDTYPE_))
        {
            for (auto e : found)    // enumerate the found entries
            {
                MString res_type;
                if (e->m_type.is_int()) // it's an integral name
                {
                    // get resource type name
                    res_type = g_db.GetName(L"RESOURCE", e->m_type.m_id);
                    if (res_type.empty())   // no name
                    {
                        res_type = mstr_dec(e->m_type.m_id);    // store numeric
                    }

                    // convert the resource type
                    Res_ReplaceResTypeString(res_type, false);
                }
                else
                {
                    res_type = e->m_type.str();
                }

                if (res_type.size())
                {
                    // add a resource type tag
                    if (ret.find(L"[" + res_type + L"]") == MString::npos)
                    {
                        ret += L"[";
                        ret += res_type;
                        ret += L"]";
                    }
                }
            }
        }
    }

    if (ret.empty())
    {
        for (size_t i = 0; i < indexes.size(); ++i)
        {
            auto nIDTYPE_ = IDTYPE_(indexes[i]);
            if (nIDTYPE_ == IDTYPE_UNKNOWN)
                continue;

            if (ret.empty())
            {
                ret = g_db.GetName(L"RESOURCE.ID.TYPE", nIDTYPE_);
            }
            else
            {
                ret += TEXT("/");
                ret += g_db.GetName(L"RESOURCE.ID.TYPE", nIDTYPE_);
            }
        }
    }

    // convert tags with slashes
    mstr_replace_all(ret, L"][", L"/");
    mstr_replace_all(ret, L"[", L"");
    mstr_replace_all(ret, L"]", L"");

    return ret;
}

// initialize the resource name combobox
void InitResNameComboBox(HWND hCmb, MIdOrString id, IDTYPE_ nIDTYPE_)
{
    // set the text of the ID
    SetWindowTextW(hCmb, id.c_str());

    if (g_settings.bHideID)
        return;     // don't use macro IDs

    INT k = -1;     // not matched yet
    MStringW prefix;
    if (nIDTYPE_ != IDTYPE_UNKNOWN)
    {
        // get the prefix from an IDTYPE_ value
        auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[nIDTYPE_].name;
        if (prefix.empty())
            return;     // unable to get

        // get the resource IDs by the prefix
        table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        for (auto& table_entry : table)
        {
            // add a resource ID to combobox
            INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
            if (table_entry.value == id.m_id)   // matched
            {
                k = i;  // matched index is k
                ComboBox_SetCurSel(hCmb, i);    // select its
                SetWindowTextW(hCmb, table_entry.name.c_str()); // set the text
            }
        }
    }

    if (k == -1 &&
        nIDTYPE_ != IDTYPE_RESOURCE && g_db.IsEntityIDType(nIDTYPE_))
    {
        // not found

        // get the prefix of Resource.ID
        auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[IDTYPE_RESOURCE].name;

        // get the resource IDs by the prefix
        table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        for (auto& table_entry : table)
        {
            // add the resource name to combobox
            INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
            if (table_entry.value == id.m_id)   // matched
            {
                ComboBox_SetCurSel(hCmb, i);    // selected
                SetWindowTextW(hCmb, table_entry.name.c_str());  // set the text
            }
        }
    }
}

// initialize the resource name combobox
void InitResNameComboBox(HWND hCmb, MIdOrString id, IDTYPE_ nIDTYPE_1, IDTYPE_ nIDTYPE_2)
{
    // set the ID text to combobox
    SetWindowTextW(hCmb, id.c_str());

    if (g_settings.bHideID)
        return;     // don't use the macro IDs

    INT k = -1; // not found yet
    MStringW prefix;
    if (nIDTYPE_1 != IDTYPE_UNKNOWN)
    {
        // get the prefix from nIDTYPE_1
        auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[nIDTYPE_1].name;
        if (prefix.empty())
            return;

        // get the resource IDs from the prefix
        table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

        // add the resource IDs
        for (auto& table_entry : table)
        {
            // add an item to combobox
            INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
            if (table_entry.value == id.m_id)   // matched
            {
                k = i;  // found

                // select it in combobox
                ComboBox_SetCurSel(hCmb, i);

                // set its text to combobox
                SetWindowTextW(hCmb, table_entry.name.c_str());
            }
        }
    }
    if (nIDTYPE_2 != IDTYPE_UNKNOWN)
    {
        // get the prefix from nIDTYPE_2
        auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[nIDTYPE_2].name;
        if (prefix.empty())
            return;

        // get the resource IDs from the prefix
        table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        for (auto& table_entry : table)
        {
            // add an item to combobox
            INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
            if (table_entry.value == id.m_id)   // matched
            {
                k = i;  // found

                // select it in combobox
                ComboBox_SetCurSel(hCmb, i);

                // set its text to combobox
                SetWindowTextW(hCmb, table_entry.name.c_str());
            }
        }
    }

    if (k == -1 &&
        nIDTYPE_1 != IDTYPE_RESOURCE && g_db.IsEntityIDType(nIDTYPE_1))
    {
        // not found

        // get the prefix from IDTYPE_RESOURCE
        auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[IDTYPE_RESOURCE].name;

        // get the resource IDs from the prefix
        table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        for (auto& table_entry : table)
        {
            // add an item to combobox
            INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
            if (table_entry.value == id.m_id)   // matched
            {
                // select it in combobox
                ComboBox_SetCurSel(hCmb, i);

                // set its text to combobox
                SetWindowTextW(hCmb, table_entry.name.c_str());
            }
        }
    }
}

// check the command ID text
BOOL CheckCommand(MString strCommand)
{
    // trim the string
    mstr_trim(strCommand);

    if (('0' <= strCommand[0] && strCommand[0] <= '9') ||
        strCommand[0] == '-' || strCommand[0] == '+')
    {
        // a numeric command ID
        return TRUE;    // OK
    }

    return g_db.HasResID(strCommand);   // is it resource ID name?
}

void InitConstantComboBox(HWND hCmb)
{
    auto table = g_db.GetWholeTable();

    // add the resource IDs
    for (auto& table_entry : table)
    {
        ComboBox_AddString(hCmb, table_entry.name.c_str());
    }

    for (auto& pair : g_settings.id_map)
    {
        MAnsiToWide wide(CP_ACP, pair.first.c_str());

        if (ComboBox_FindStringExact(hCmb, -1, wide.c_str()) != CB_ERR)
            continue;

        ComboBox_AddString(hCmb, wide.c_str());
    }
}

// initialize the resource string ID combobox
void InitStringComboBox(HWND hCmb, MString strString)
{
    // set the text to combobox
    SetWindowText(hCmb, strString.c_str());

    if (g_settings.bHideID)
        return;     // don't use macro IDs

    // get the prefix from IDTYPE_STRING
    auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_STRING].name;
    if (prefix.empty())
        return;

    // get the resource IDs from the prefix
    table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

    // add the resource IDs
    for (auto& table_entry : table)
    {
        // add an item to combobox
        INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
        if (table_entry.name == strString)  // matched
        {
            // select it
            ComboBox_SetCurSel(hCmb, i);
        }
    }

    // get the prefix from IDTYPE_PROMPT
    table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
    prefix = table[IDTYPE_PROMPT].name;

    // get the resource IDs from the prefix
    table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

    // add the resource IDs
    for (auto& table_entry : table)
    {
        // add an item to combobox
        INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
        if (table_entry.name == strString)  // matched
        {
            // select it
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

// initialize the resource message ID combobox
void InitMessageComboBox(HWND hCmb, MString strString)
{
    // set the text to combobox
    SetWindowText(hCmb, strString.c_str());

    if (g_settings.bHideID)
        return;     // don't use macro IDs

    // get the prefix from IDTYPE_MESSAGE
    auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_MESSAGE].name;
    if (prefix.empty())
        return;

    // get the resource IDs from the prefix
    table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

    // add the resource IDs
    for (auto& table_entry : table)
    {
        // add an item to combobox
        INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
        if (table_entry.name == strString)  // matched
        {
            ComboBox_SetCurSel(hCmb, i);    // select it
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// languages

// structure for language information
struct LANG_ENTRY
{
    WORD LangID;    // language ID
    MStringW str;   // string

    // for sorting
    bool operator<(const LANG_ENTRY& ent) const
    {
        return str < ent.str;
    }
};
std::vector<LANG_ENTRY> g_langs;

// initialize the language combobox
void InitLangComboBox(HWND hCmb3, LANGID langid)
{
    // for all the elements of g_langs
    for (auto& entry : g_langs)
    {
        // build the text
        WCHAR sz[MAX_PATH];
        StringCchPrintfW(sz, _countof(sz), L"%s (%u)", entry.str.c_str(), entry.LangID);

        // search the text
        if (ComboBox_FindStringExact(hCmb3, -1, sz) != CB_ERR)
            continue;   // found

        // add the text as a new item to combobox
        INT k = ComboBox_AddString(hCmb3, sz);
        if (langid == entry.LangID)     // matched
        {
            ComboBox_SetCurSel(hCmb3, k);   // select it
        }
    }

    auto table = g_db.GetTable(L"LANGUAGES");
    for (auto& table_entry : table)
    {
        // build the text
        WCHAR sz[MAX_PATH];
        StringCchPrintfW(sz, _countof(sz), L"%s (%lu)", table_entry.name.c_str(), table_entry.value);

        // search the text
        if (ComboBox_FindStringExact(hCmb3, -1, sz) != CB_ERR)
            continue;   // found

        // add the text as a new item to combobox
        ComboBox_AddString(hCmb3, sz);
    }
}

// initialize the language listview
void InitLangListView(HWND hLst1, LPCTSTR pszText)
{
    // delete all the items of listview
    ListView_DeleteAllItems(hLst1);

    WCHAR szText[128];
    if (pszText)
    {
        StringCbCopyW(szText, sizeof(szText), pszText);
        CharUpperW(szText);
    }

    WCHAR sz1[64], sz2[64];
    LV_ITEM item;
    INT iItem = 0;
    for (auto& entry : g_langs)     // for all the items of g_langs
    {
        // build two texts of an entry
        StringCchPrintfW(sz1, _countof(sz1), L"%s", entry.str.c_str());
        StringCchPrintfW(sz2, _countof(sz2), L"%u", entry.LangID);

        if (pszText)
        {
            // filtering by pszText
            MString str = sz1;
            CharUpperW(&str[0]);
            if (str.find(szText) == MString::npos)
            {
                str = sz2;
                CharUpperW(&str[0]);
                if (str.find(szText) == MString::npos)
                    continue;
            }
        }

        // if it exists, don't add it
        LV_FINDINFO find = { LVFI_STRING, sz1 };
        INT iFound = ListView_FindItem(hLst1, -1, &find);
        if (iFound != -1)
            continue;

        // add it
        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = sz1;
        ListView_InsertItem(hLst1, &item);

        item.iSubItem = 1;
        item.pszText = sz2;
        ListView_SetItem(hLst1, &item);

        ++iItem;    // next item index
    }

    auto table = g_db.GetTable(L"LANGUAGES");
    for (auto& table_entry : table)
    {
        // build two texts of an entry
        StringCchPrintfW(sz1, _countof(sz1), L"%s", table_entry.name.c_str());
        StringCchPrintfW(sz2, _countof(sz2), L"%lu", table_entry.value);

        if (pszText)
        {
            // filtering by pszText
            MString str = sz1;
            if (str.find(pszText) == MString::npos)
            {
                str = sz2;
                if (str.find(pszText) == MString::npos)
                    continue;
            }
        }

        // if it exists, don't add it
        LV_FINDINFO find = { LVFI_STRING, sz1 };
        INT iFound = ListView_FindItem(hLst1, -1, &find);
        if (iFound != -1)
            continue;

        // add it
        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = sz1;
        ListView_InsertItem(hLst1, &item);

        item.iSubItem = 1;
        item.pszText = sz2;
        ListView_SetItem(hLst1, &item);

        ++iItem;    // next item index
    }
}

// get the language ID from a text
WORD LangFromText(LPWSTR pszLang)
{
    WORD lang = BAD_LANG;     // not found yet

    // replace the fullwidth characters with halfwidth characters
    ReplaceFullWithHalf(pszLang);

    // trim and store to pszLang and strLang
    MStringW strLang = pszLang;
    mstr_trim(strLang);
    StringCchCopyW(pszLang, MAX_PATH, strLang.c_str());

    do
    {
        if (strLang[0] == 0)
            break;  // it's empty. invalid

        // is it American English?
        if (lstrcmpiW(pszLang, L"en") == 0 ||
            lstrcmpiW(pszLang, L"English") == 0 ||
            lstrcmpiW(pszLang, L"America") == 0 ||
            lstrcmpiW(pszLang, L"American") == 0 ||
            lstrcmpiW(pszLang, L"United States") == 0 ||
            lstrcmpiW(pszLang, L"USA") == 0 ||
            lstrcmpiW(pszLang, L"US") == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_AMERICA)) == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_ENGLISH)) == 0)
        {
            // American English
            lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            break;
        }

        // is it Chinese?
        if (lstrcmpiW(pszLang, L"Chinese") == 0 ||
            lstrcmpiW(pszLang, L"PRC") == 0 ||
            lstrcmpiW(pszLang, L"CHN") == 0 ||
            lstrcmpiW(pszLang, L"CN") == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_CHINA)) == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_CHINESE)) == 0)
        {
            // Chinese
            lang = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
            break;
        }

        // is it Russian?
        if (lstrcmpiW(pszLang, L"Russia") == 0 ||
            lstrcmpiW(pszLang, L"Russian") == 0 ||
            lstrcmpiW(pszLang, L"RUS") == 0 ||
            lstrcmpiW(pszLang, L"RU") == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_RUSSIA)) == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_RUSSIAN)) == 0)
        {
            // Russian
            lang = MAKELANGID(LANG_RUSSIAN, SUBLANG_RUSSIAN_RUSSIA);
            break;
        }

        // is it British?
        if (lstrcmpiW(pszLang, L"United Kingdom") == 0 ||
            lstrcmpiW(pszLang, L"Great Britain") == 0 ||
            lstrcmpiW(pszLang, L"British") == 0 ||
            lstrcmpiW(pszLang, L"GBR") == 0 ||
            lstrcmpiW(pszLang, L"GB") == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_UNITEDKINGDOM)) == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_GREATBRITAIN)) == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_BRITISH)) == 0)
        {
            // Russian
            lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK);
            break;
        }

        // is it French?
        if (lstrcmpiW(pszLang, L"French") == 0 ||
            lstrcmpiW(pszLang, L"France") == 0 ||
            lstrcmpiW(pszLang, L"FRA") == 0 ||
            lstrcmpiW(pszLang, L"FR") == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_FRANCE)) == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_FRENCH)) == 0)
        {
            // French
            lang = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
            break;
        }

        // is it Germany?
        if (lstrcmpiW(pszLang, L"Germany") == 0 ||
            lstrcmpiW(pszLang, L"German") == 0 ||
            lstrcmpiW(pszLang, L"DEU") == 0 ||
            lstrcmpiW(pszLang, L"DE") == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_GERMANY)) == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_GERMAN)) == 0)
        {
            // Germany
            lang = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
            break;
        }

        // is it Spanish?
        if (lstrcmpiW(pszLang, L"Spanish") == 0 ||
            lstrcmpiW(pszLang, L"Spain") == 0 ||
            lstrcmpiW(pszLang, L"ESP") == 0 ||
            lstrcmpiW(pszLang, L"ES") == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_SPANISH)) == 0 ||
            lstrcmpiW(pszLang, LoadStringDx(IDS_SPAIN)) == 0)
        {
            // Spanish
            lang = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
            break;
        }

        // maybe en_US, or jp_JP etc.
        if (INT nValue = g_db.GetValueI(L"LANGUAGES", strLang.c_str()))
        {
            lang = (WORD)nValue;    // found
            break;
        }

        // maybe en-US, or jp-JP etc.
        {
            MStringW str = strLang;

            // replace '-' with '_'
            auto i = str.find(L'-');
            if (i != MString::npos)
                str[i] = L'_';

            // maybe en_US, or jp_JP etc.
            if (INT nValue = g_db.GetValueI(L"LANGUAGES", str.c_str()))
            {
                lang = (WORD)nValue;    // found
                break;
            }
        }

        // is it numeric?
        if (mchr_is_digit(strLang[0]))
        {
            // strLang is numeric
            int nValue = mstr_parse_int(pszLang);
            if (nValue < 0 || BAD_LANG <= nValue)
                break;  // invalid

            lang = WORD(nValue);
        }

        if (lang == BAD_LANG)     // not found yet?
        {
            // whole match
            for (auto& entry : g_langs)
            {
                if (lstrcmpiW(entry.str.c_str(), pszLang) == 0) // matched
                {
                    lang = entry.LangID;
                    break;
                }
            }
        }

        if (lang == BAD_LANG)     // not found yet?
        {
            // numeric after parenthesis
            if (WCHAR *pch = wcsrchr(pszLang, L'('))
            {
                ++pch;
                if (mchr_is_digit(*pch))
                {
                    int nValue = mstr_parse_int(pch);
                    if (nValue < 0 || BAD_LANG <= nValue)
                        break;  // invalid

                    lang = WORD(nValue);
                }
            }
        }

        if (lang == BAD_LANG)     // not found yet?
        {
            // partial match
            for (auto& entry : g_langs)
            {
                if (wcsstr(entry.str.c_str(), pszLang) != NULL)
                {
                    lang = entry.LangID;
                    break;
                }
            }
        }

        if (lang == BAD_LANG)     // not found yet?
        {
            // ignore case, partial match
            CharUpperW(&strLang[0]);
            for (auto& entry : g_langs)
            {
                MStringW strEntry = entry.str;
                CharUpperW(&strEntry[0]);

                if (wcsstr(strEntry.c_str(), pszLang) != NULL)
                {
                    lang = entry.LangID;
                    break;
                }
            }
        }
    } while (0);

    return lang;
}

// verify the language combobox
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang)
{
    // get the text from combobox
    WCHAR szLang[MAX_PATH];
    GetWindowTextW(hCmb3, szLang, _countof(szLang));

    // get the language ID from texts
    lang = LangFromText(szLang);
    if (lang != BAD_LANG)
        return TRUE;    // success

    // error
    ComboBox_SetEditSel(hCmb3, 0, -1);
    SetFocus(hCmb3);
    MessageBoxW(GetParent(hCmb3), LoadStringDx(IDS_ENTERLANG),
                NULL, MB_ICONERROR);
    return FALSE;   // failure
}

// callback function for MMainWnd::DoLoadLangInfo
static BOOL CALLBACK
EnumLocalesProc(LPWSTR lpLocaleString)
{
    // get the locale ID from string
    LCID lcid = mstr_parse_int(lpLocaleString, false, 16);

    LANG_ENTRY entry;
    entry.LangID = LANGIDFROMLCID(lcid);    // store the language ID

    // get the localized language and country
    WCHAR sz[128] = L"";
    if (lcid == 0)
        return TRUE;    // continue
    if (!GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, sz, _countof(sz)))
        return TRUE;    // continue

    entry.str = sz;     // store the text

    // add it
    g_langs.push_back(entry);

    return TRUE;    // continue
}

// callback function for MMainWnd::DoLoadLangInfo
static BOOL CALLBACK
EnumEngLocalesProc(LPWSTR lpLocaleString)
{
    // get the locale ID from string
    LCID lcid = mstr_parse_int(lpLocaleString, false, 16);

    LANG_ENTRY entry;
    entry.LangID = LANGIDFROMLCID(lcid);    // store the language ID

    // get the language and country in English
    WCHAR sz1[128] = L"", sz2[128] = L"";
    if (lcid == 0)
        return TRUE;    // continue
    if (!GetLocaleInfoW(lcid, LOCALE_SENGLANGUAGE, sz1, _countof(sz1)))
        return TRUE;    // continue
    if (!GetLocaleInfoW(lcid, LOCALE_SENGCOUNTRY, sz2, _countof(sz2)))
        return TRUE;    // continue

    // join them and store it
    entry.str = sz1;
    entry.str += L" (";
    entry.str += sz2;
    entry.str += L")";

    // add it
    g_langs.push_back(entry);

    return TRUE;    // continue
}

// get the text from a language ID
MStringW TextFromLang(WORD lang)
{
    WCHAR sz[128], szLoc[128];

    // get the locale ID
    LCID lcid = MAKELCID(lang, SORT_DEFAULT);
    if (lcid == 0)
    {
        // neutral language
        StringCchPrintfW(sz, _countof(sz), L"%s (0)", LoadStringDx(IDS_NEUTRAL));
    }
    else
    {
        if (GetLocaleInfo(lcid, LOCALE_SLANGUAGE, szLoc, _countof(szLoc)))
        {
            // a valid language
            StringCchPrintfW(sz, _countof(sz), L"%s (%u)", szLoc, lang);
        }
        else
        {
            // invalid or unknown language. just store numeric
            StringCchPrintfW(sz, _countof(sz), L"%u", lang);
        }
    }

    return MStringW(sz);
}

//////////////////////////////////////////////////////////////////////////////
// the specialized toolbar

// buttons info #0
TBBUTTON g_buttons0[] =
{
    { 11, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_GUIEDIT },
    { 12, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TEST },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
    { 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
    { 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
    { 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
    { 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
    { 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
    { 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
    { 14, ID_EXTRACTBANG, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXTRACT },
};

// buttons info #1
TBBUTTON g_buttons1[] =
{
    { 12, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TEST },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
    { 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
    { 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
    { 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
    { 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
    { 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
    { 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
    { 14, ID_EXTRACTBANG, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXTRACT },
};

// buttons info #2
TBBUTTON g_buttons2[] =
{
    { 9, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_RECOMPILE },
    { 10, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CANCELEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
    { 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
    { 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
    { 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
    { 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
    { 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
    { 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
    { 14, ID_EXTRACTBANG, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXTRACT },
};

// buttons info #3
TBBUTTON g_buttons3[] =
{
    { 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
    { 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
    { 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
    { 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
    { 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
    { 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
    { 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
    { 14, ID_EXTRACTBANG, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXTRACT },
};

// buttons info #4
TBBUTTON g_buttons4[] =
{
    { 11, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_GUIEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
    { 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
    { 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
    { 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
    { 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
    { 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
    { 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
    { 14, ID_EXTRACTBANG, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXTRACT },
};

// store the toolbar strings
VOID ToolBar_StoreStrings(HWND hwnd, INT nCount, TBBUTTON *pButtons)
{
    for (INT i = 0; i < nCount; ++i)
    {
        if (pButtons[i].idCommand == 0 || (pButtons[i].fsStyle & BTNS_SEP))
            continue;   // ignore separators

        // replace the resource string ID with a toolbar string ID
        INT_PTR id = pButtons[i].iString;
        LPWSTR psz = LoadStringDx(INT(id));
        id = SendMessageW(hwnd, TB_ADDSTRING, 0, (LPARAM)psz);
        pButtons[i].iString = id;
    }
}

//////////////////////////////////////////////////////////////////////////////

// verify the resource type combobox
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type)
{
    // get the combobox text
    WCHAR szType[MAX_PATH];
    GetWindowTextW(hCmb1, szType, _countof(szType));

    // replace the fullwidth characters with halfwidth characters
    ReplaceFullWithHalf(szType);

    // trim and store it to str and szType
    MStringW str = szType;
    mstr_trim(str);
    StringCchCopyW(szType, _countof(szType), str.c_str());

    if (szType[0] == 0)  // an empty string
    {
        ComboBox_SetEditSel(hCmb1, 0, -1);  // select all
        SetFocus(hCmb1);    // set focus
        // show error message
        MessageBoxW(GetParent(hCmb1), LoadStringDx(IDS_ENTERTYPE),
                    NULL, MB_ICONERROR);
        return FALSE;   // failure
    }
    else if (mchr_is_digit(szType[0]) || szType[0] == L'-' || szType[0] == L'+')
    {
        // numeric type name
        type = WORD(mstr_parse_int(szType));
    }
    else
    {
        MStringW str = szType;
        size_t i = str.rfind(L'('); // ')'
        if (i != MStringW::npos && mchr_is_digit(str[i + 1]))
        {
            // numeric type name after the parenthesis
            type = WORD(mstr_parse_int(&str[i + 1]));
        }
        else
        {
            WORD nRT_ = (WORD)g_db.GetValue(L"RESOURCE", str);
            if (nRT_ != 0)
            {
                type = nRT_;
            }
            else
            {
                // a string type name
                type = szType;
            }
        }
    }

    return TRUE;    // success
}

// verify the resource name combobox
BOOL CheckNameComboBox(HWND hCmb2, MIdOrString& name)
{
    // get the combobox text
    WCHAR szName[MAX_PATH];
    GetWindowTextW(hCmb2, szName, _countof(szName));

    // replace the fullwidth characters with halfwidth characters
    ReplaceFullWithHalf(szName);

    // trim and store it to str and szType
    MStringW str = szName;
    mstr_trim(str);
    StringCchCopyW(szName, _countof(szName), str.c_str());

    if (szName[0] == 0) // an empty string
    {
        ComboBox_SetEditSel(hCmb2, 0, -1);  // select all
        SetFocus(hCmb2);    // set focus
        // show error message
        MessageBoxW(GetParent(hCmb2), LoadStringDx(IDS_ENTERNAME),
                    NULL, MB_ICONERROR);
        return FALSE;   // failure
    }
    else if (mchr_is_digit(szName[0]) || szName[0] == L'-' || szName[0] == L'+')
    {
        // a numeric name
        name = WORD(mstr_parse_int(szName));
    }
    else
    {
        // a non-numeric name
        if (g_db.HasResID(szName))
            name = (WORD)g_db.GetResIDValue(szName);    // a valued name
        else
            name = szName;  // a string name
    }

    return TRUE;    // success
}

// verify the file textbox
BOOL Edt1_CheckFile(HWND hEdt1, MStringW& file)
{
    // get the text from textbox
    WCHAR szFile[MAX_PATH];
    GetWindowTextW(hEdt1, szFile, _countof(szFile));

    // trim and store to str and szFile
    MStringW str = szFile;
    mstr_trim(str);
    StringCchCopyW(szFile, _countof(szFile), str.c_str());

    if (!PathFileExistsW(szFile))    // not exists
    {
        Edit_SetSel(hEdt1, 0, -1);  // select all
        SetFocus(hEdt1);    // set focus
        // show error message
        MessageBoxW(GetParent(hEdt1), LoadStringDx(IDS_FILENOTFOUND),
                    NULL, MB_ICONERROR);
        return FALSE;   // failure
    }

    // store
    file = szFile;

    return TRUE;    // success
}

// get the text from a command ID
MStringW GetKeyID(UINT wId)
{
    if (g_settings.bHideID) // don't use the macro IDs
        return mstr_dec_short((SHORT)wId);  // return the numeric ID string

    // convert the numeric ID value to the named ID name
    return g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, wId);
}

// initialize the virtual key combobox
void Cmb1_InitVirtualKeys(HWND hCmb1)
{
    // clear all the items of combobox
    ComboBox_ResetContent(hCmb1);

    // add items to combobox
    auto table = g_db.GetTable(L"VIRTUALKEYS");
    for (auto& table_entry : table)
    {
        // add an item to combobox
        ComboBox_AddString(hCmb1, table_entry.name.c_str());
    }
}

// verify the virtual key combobox
BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, MStringW& str)
{
    BOOL bOK;
    if (bVirtKey)
    {
        // a virtual key
        INT i = ComboBox_FindStringExact(hCmb1, -1, str.c_str());
        if (i == CB_ERR)
        {
            // not a string. is it numeric?
            i = GetDlgItemInt(hwnd, cmb1, &bOK, TRUE);
            if (!bOK)
            {
                return FALSE;   // not a string nor a numeric. invalid
            }
            str = mstr_dec(i);
        }
    }
    else
    {
        // a non-virtual key
        INT i = GetDlgItemInt(hwnd, cmb1, &bOK, TRUE);
        if (bOK)
        {
            // a numeric
            str = mstr_dec(i);
        }
        else
        {
            // not a numeric. is it a string?
            LPCWSTR pch = str.c_str();
            MStringW str2;
            if (!guts_quote(str2, pch) || str2.size() != 1)
            {
                return FALSE;   // invalid
            }
            str = mstr_quote(str2);
        }
    }

    return TRUE;    // success
}

//////////////////////////////////////////////////////////////////////////////
// STRING_ENTRY

// helper function for MAddStrDlg and MModifyStrDlg
BOOL StrDlg_GetEntry(HWND hwnd, STRING_ENTRY& entry)
{
    // get the text from combobox
    // replace the fullwidth characters with halfwidth characters
    MString str = MWindowBase::GetDlgItemText(hwnd, cmb1);
    ReplaceFullWithHalf(str);

    if (('0' <= str[0] && str[0] <= '9') || str[0] == '-' || str[0] == '+')
    {
        // numeric
        LONG n = mstr_parse_int(str.c_str());
        str = mstr_dec_word(WORD(n));
    }
    else if (!g_db.HasResID(str))
    {
        // non-numeric and not resource ID. invalid
        return FALSE;   // failure
    }

    // store the string to entry.StringID
    StringCchCopyW(entry.StringID, _countof(entry.StringID), str.c_str());

    // get the text from EDIT control
    str = MWindowBase::GetDlgItemText(hwnd, edt1);
    //mstr_trim(str);     // trim it

    // unquote if quoted
    if (str[0] == L'"')
    {
        mstr_unquote(str);
    }

    // store the text to entry.StringValue
    StringCchCopyW(entry.StringValue, _countof(entry.StringValue), str.c_str());

    return TRUE;    // success
}

// helper function for MAddStrDlg and MModifyStrDlg
void StrDlg_SetEntry(HWND hwnd, STRING_ENTRY& entry)
{
    // store entry.StringID to combobox
    SetDlgItemTextW(hwnd, cmb1, entry.StringID);

    // store the quoted entry.StringValue to the EDIT control
    MStringW str = entry.StringValue;
    str = mstr_quote(str);
    SetDlgItemTextW(hwnd, edt1, str.c_str());
}

//////////////////////////////////////////////////////////////////////////////
// MESSAGE_ENTRY

// helper function for MAddMsgDlg and MModifyMsgDlg
BOOL MsgDlg_GetEntry(HWND hwnd, MESSAGE_ENTRY& entry)
{
    // get the text from combobox
    // replace the fullwidth characters with halfwidth characters
    MString str = MWindowBase::GetDlgItemText(hwnd, cmb1);
    ReplaceFullWithHalf(str);

    if (('0' <= str[0] && str[0] <= '9') || str[0] == '-' || str[0] == '+')
    {
        // numeric
        LONG n = mstr_parse_int(str.c_str());
        str = mstr_hex(n);      // make it hexidemical
    }
    else if (!g_db.HasResID(str))
    {
        // non-numeric and not resource ID. invalid
        return FALSE;   // failure
    }

    // store the string to entry.MessageID
    StringCchCopyW(entry.MessageID, _countof(entry.MessageID), str.c_str());

    // get the text from EDIT control
    str = MWindowBase::GetDlgItemText(hwnd, edt1);

    // unquote if quoted
    if (str[0] == L'"')
    {
        mstr_unquote(str);
    }

    // store the text to entry.MessageValue
    StringCchCopyW(entry.MessageValue, _countof(entry.MessageValue), str.c_str());

    return TRUE;    // success
}

// helper function for MAddMsgDlg and MModifyMsgDlg
void MsgDlg_SetEntry(HWND hwnd, MESSAGE_ENTRY& entry)
{
    // get the text from combobox
    SetDlgItemTextW(hwnd, cmb1, entry.MessageID);

    // set the quoted entry.MessageValue to the EDIT control
    MStringW str = entry.MessageValue;
    str = mstr_quote(str);
    SetDlgItemTextW(hwnd, edt1, str.c_str());
}

//////////////////////////////////////////////////////////////////////////////
// MMainWnd --- the main window

// the file type
enum FileType
{
    FT_NONE,
    FT_EXECUTABLE,
    FT_RC,
    FT_RES
};

class MMainWnd : public MWindowBase
{
protected:
    INT         m_argc;         // number of command line parameters
    TCHAR **    m_targv;        // command line parameters
    BOOL        m_bLoading;     // loading now?

    // handles
    HINSTANCE   m_hInst;        // the instance handle
    HICON       m_hIcon;        // the icon handle
    HICON       m_hIconSm;      // the small icon handle
    HACCEL      m_hAccel;       // the accelerator handle
    HWND        m_hwndTV;       // the tree control
    HIMAGELIST  m_hImageList;   // the image list for m_hwndTV
    INT         m_nCommandLock; // the lock count of WM_COMMAND message
    HICON       m_hFileIcon;    // the file icon
    HICON       m_hFolderIcon;  // the folder icon
    HFONT       m_hSrcFont;     // the source font
    HFONT       m_hBinFont;     // the binary font
    HWND        m_hToolBar;     // the toolbar window handle
    HWND        m_hStatusBar;   // the status bar handle
    HWND        m_hFindReplaceDlg;  // the find/replace dialog handle
    HIMAGELIST  m_himlTools;        // the image list for the toolbar

    // data and sub-programs
    WCHAR       m_szDataFolder[MAX_PATH];       // the data folder location
    WCHAR       m_szConstantsFile[MAX_PATH];    // the Constants.txt file location
    WCHAR       m_szMCppExe[MAX_PATH];          // the mcpp.exe location
    WCHAR       m_szWindresExe[MAX_PATH];       // the windres.exe location
    WCHAR       m_szUpxExe[MAX_PATH];           // the upx.exe location
    WCHAR       m_szMcdxExe[MAX_PATH];          // the mcdx.exe location
    WCHAR       m_szDFMSC[MAX_PATH];            // the dfmsc.exe location
    WCHAR       m_szIncludeDir[MAX_PATH];       // the include directory

    // file info
    FileType    m_file_type;
    WCHAR       m_szFile[MAX_PATH];             // the file location
    WCHAR       m_szResourceH[MAX_PATH];        // the resource.h file location
    BOOL        m_bUpxCompressed;               // is the real file compressed?

    BOOL UpdateFileInfo(FileType ft, LPCWSTR pszFile, BOOL bCompressed);

    // selection
    MIdOrString     m_type;
    MIdOrString     m_name;
    WORD            m_lang;

    // classes
    MRadWindow      m_rad_window;               // the RADical window
    MEditCtrl       m_hBinEdit;                 // the EDIT control for binary
    MSrcEdit        m_hSrcEdit;                 // the EDIT control for source
    MBmpView        m_hBmpView;                 // the bitmap view
    MSplitterWnd    m_splitter1;                // 1st splitter window
    MSplitterWnd    m_splitter2;                // 2nd splitter window
    MSplitterWnd    m_splitter3;                // 3rd splitter window
    MIDListDlg      m_id_list_dlg;              // the ID List window
    ITEM_SEARCH     m_search;                   // the search options

    // find/replace
    FINDREPLACE     m_fr;                       // the find/replace structure
    TCHAR           m_szFindWhat[80];           // the source text for find/replace
    TCHAR           m_szReplaceWith[80];        // the destination text for replace

public:
    // constructor
    MMainWnd(int argc, TCHAR **targv, HINSTANCE hInst) :
        m_argc(argc), m_targv(targv), m_bLoading(FALSE),
        m_hInst(hInst), m_hIcon(NULL), m_hIconSm(NULL), m_hAccel(NULL),
        m_hwndTV(NULL), m_hImageList(NULL), m_nCommandLock(0),
        m_hFileIcon(NULL), m_hFolderIcon(NULL), m_hSrcFont(NULL), m_hBinFont(NULL),
        m_hToolBar(NULL), m_hStatusBar(NULL),
        m_hFindReplaceDlg(NULL), m_himlTools(NULL), m_file_type(FT_NONE)
    {
        m_szDataFolder[0] = 0;
        m_szConstantsFile[0] = 0;
        m_szMCppExe[0] = 0;
        m_szWindresExe[0] = 0;
        m_szUpxExe[0] = 0;
        m_szMcdxExe[0] = 0;
        m_szDFMSC[0] = 0;
        m_szIncludeDir[0] = 0;
        m_szFile[0] = 0;
        m_szResourceH[0] = 0;

        m_bUpxCompressed = FALSE;

        m_lang = BAD_LANG;

        ZeroMemory(&m_fr, sizeof(m_fr));
        m_fr.lStructSize = sizeof(m_fr);
        m_fr.Flags = FR_HIDEWHOLEWORD | FR_DOWN;

        m_szFindWhat[0] = 0;
        m_fr.lpstrFindWhat = m_szFindWhat;
        m_fr.wFindWhatLen = _countof(m_szFindWhat);

        m_szReplaceWith[0] = 0;
        m_fr.lpstrReplaceWith = m_szReplaceWith;
        m_fr.wReplaceWithLen = _countof(m_szReplaceWith);
    }

    // settings
    void SetDefaultSettings(HWND hwnd);
    BOOL LoadSettings(HWND hwnd);
    BOOL SaveSettings(HWND hwnd);
    void UpdatePrefixDB(HWND hwnd);
    BOOL ReCreateSrcEdit(HWND hwnd);

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        MWindowBase::ModifyWndClassDx(wcx);

        // set a class menu
        wcx.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);

        // change the window icon
        wcx.hIcon = m_hIcon;
        wcx.hIconSm = m_hIconSm;
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        // the window class name of the main window
        return TEXT("katahiromz's RisohEditor");
    }

    BOOL StartDx();
    INT_PTR RunDx();
    void DoEvents();
    void DoMsg(MSG& msg);

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    //////////////////////////////////////////////////////////////////////

    // status bar
    void ChangeStatusText(INT nID)
    {
        ChangeStatusText(LoadStringDx(nID));
    }
    void ChangeStatusText(LPCTSTR pszText)
    {
        SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)pszText);
    }

    // utilities
    BOOL CheckDataFolder(VOID);
    INT CheckData(VOID);

    void UpdateMenu();
    void SelectTV(EntryBase *entry, BOOL bDoubleClick);
    void SelectTV(EntryType et, const MIdOrString& type,
                  const MIdOrString& name, WORD lang, BOOL bDoubleClick);

    template <typename T_DIALOG>
    void SelectTV(EntryType et, const T_DIALOG& dialog, BOOL bDoubleClick)
    {
        SelectTV(et, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
    }

    BOOL CompileIfNecessary(BOOL bReopen = FALSE);
    BOOL ReCompileOnSelChange(BOOL bReopen = FALSE);
    void SelectString(void);
    void SelectMessage(void);
    BOOL CreateOurToolBar(HWND hwndParent, HIMAGELIST himlTools);
    void UpdateOurToolBarButtons(INT iType);
    void UpdateToolBarStatus();

    // ID list
    void OnIDList(HWND hwnd);
    void OnIdAssoc(HWND hwnd);
    void OnPredefMacros(HWND hwnd);
    void OnEditLabel(HWND hwnd);
    void OnSetPaths(HWND hwnd);
    void OnShowLangs(HWND hwnd);
    void OnShowHideToolBar(HWND hwnd);

    // show/hide
    void ShowIDList(HWND hwnd, BOOL bShow = TRUE);
    void ShowMovie(BOOL bShow = TRUE);
    void ShowBmpView(BOOL bShow = TRUE);
    void ShowStatusBar(BOOL bShow = TRUE);
    void ShowBinEdit(BOOL bShow = TRUE, BOOL bShowError = FALSE);

    // preview
    VOID HidePreview();
    BOOL Preview(HWND hwnd, const EntryBase *entry);

    // actions
    BOOL DoLoadResH(HWND hwnd, LPCTSTR pszFile);
    void DoLoadLangInfo(VOID);
    BOOL DoLoadFile(HWND hwnd, LPCWSTR pszFileName, DWORD nFilterIndex = 0, BOOL bForceDecompress = FALSE);
    BOOL DoLoadRC(HWND hwnd, LPCWSTR szRCFile, EntrySet& res);
    BOOL DoExtract(const EntryBase *entry, BOOL bExporting);
    BOOL DoExport(LPCWSTR pszRCFile, LPWSTR pszResHFile = NULL);
    void DoIDStat(UINT anValues[5]);
    BOOL DoBackupFile(LPCWSTR pszFileName, UINT nCount = 0);
    BOOL DoBackupFolder(LPCWSTR pszFileName, UINT nCount = 0);
    BOOL DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH);
    BOOL DoWriteRCLang(MFile& file, ResToText& res2text, WORD lang);
    BOOL DoWriteRCLangUTF8(MFile& file, ResToText& res2text, WORD lang);
    BOOL DoWriteRCLangUTF16(MFile& file, ResToText& res2text, WORD lang);
    BOOL DoWriteResH(LPCWSTR pszResH, LPCWSTR pszRCFile = NULL);
    BOOL DoWriteResHOfExe(LPCWSTR pszExeFile);
    BOOL DoSaveResAs(LPCWSTR pszExeFile);
    BOOL DoSaveAs(LPCWSTR pszExeFile);
    BOOL DoSaveAsCompression(LPCWSTR pszExeFile);
    BOOL DoSaveExeAs(LPCWSTR pszExeFile, BOOL bCompression = FALSE);
    BOOL DoSaveInner(LPCWSTR pszExeFile, BOOL bCompression = FALSE);
    IMPORT_RESULT DoImport(HWND hwnd, LPCWSTR pszFile, LPCWSTR pchDotExt);
    IMPORT_RESULT DoImportRes(HWND hwnd, LPCWSTR pszFile);
    BOOL DoUpxTest(LPCWSTR pszUpx, LPCWSTR pszFile);
    BOOL DoUpxDecompress(LPCWSTR pszUpx, LPCWSTR pszFile);
    BOOL DoUpxCompress(LPCWSTR pszUpx, LPCWSTR pszExeFile);
    void DoRenameEntry(LPWSTR pszText, EntryBase *entry, const MIdOrString& old_name, const MIdOrString& new_name);
    void DoRelangEntry(LPWSTR pszText, EntryBase *entry, WORD old_lang, WORD new_lang);
    void DoRefreshTV(HWND hwnd);
    void DoRefreshIDList(HWND hwnd);

    void ReCreateFonts(HWND hwnd);
    void ReSetPaths(HWND hwnd);
    BOOL DoItemSearch(ITEM_SEARCH& search);

    EGA::arg_t DoEgaResSearch(const EGA::args_t& args);
    EGA::arg_t DoEgaResDelete(const EGA::args_t& args);
    EGA::arg_t DoEgaResCloneByName(const EGA::args_t& args);
    EGA::arg_t DoEgaResCloneByLang(const EGA::args_t& args);
    EGA::arg_t DoEgaResUnloadResH(const EGA::args_t& args);

protected:
    // parsing resource IDs
    BOOL CompileParts(MStringA& strOutput, const MIdOrString& type, const MIdOrString& name,
                      WORD lang, const MStringW& strWide, BOOL bReopen = FALSE);
    BOOL CompileStringTable(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide);
    BOOL CompileMessageTable(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide);
    BOOL CompileRCData(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide);
    BOOL CheckResourceH(HWND hwnd, LPCTSTR pszPath);
    BOOL ParseResH(HWND hwnd, LPCTSTR pszFile, const char *psz, DWORD len);
    BOOL ParseMacros(HWND hwnd, LPCTSTR pszFile, const std::vector<MStringA>& macros, MStringA& str);
    BOOL UnloadResourceH(HWND hwnd);
    void SetErrorMessage(const MStringA& strOutput, BOOL bBox = FALSE);
    MStringW GetMacroDump() const;
    MStringW GetIncludesDump() const;
    void ReadResHLines(FILE *fp, std::vector<MStringA>& lines);
    void UpdateResHLines(std::vector<MStringA>& lines);

    void JoinLinesByBackslash(std::vector<MStringA>& lines);
    void DeleteIncludeGuard(std::vector<MStringA>& lines);
    void AddAdditionalMacroLines(std::vector<MStringA>& lines);
    void DeleteSpecificMacroLines(std::vector<MStringA>& lines);
    void AddApStudioBlock(std::vector<MStringA>& lines);
    void DeleteApStudioBlock(std::vector<MStringA>& lines);
    void AddHeadComment(std::vector<MStringA>& lines);
    void DeleteHeadComment(std::vector<MStringA>& lines);
    void DoAddRes(HWND hwnd, MAddResDlg& dialog);

    // preview
    void PreviewIcon(HWND hwnd, const EntryBase& entry);
    void PreviewCursor(HWND hwnd, const EntryBase& entry);
    void PreviewGroupIcon(HWND hwnd, const EntryBase& entry);
    void PreviewGroupCursor(HWND hwnd, const EntryBase& entry);
    void PreviewBitmap(HWND hwnd, const EntryBase& entry);
    void PreviewImage(HWND hwnd, const EntryBase& entry);
    void PreviewWAVE(HWND hwnd, const EntryBase& entry);
    void PreviewAVI(HWND hwnd, const EntryBase& entry);
    void PreviewAccel(HWND hwnd, const EntryBase& entry);
    void PreviewMessage(HWND hwnd, const EntryBase& entry);
    void PreviewString(HWND hwnd, const EntryBase& entry);
    void PreviewHtml(HWND hwnd, const EntryBase& entry);
    void PreviewMenu(HWND hwnd, const EntryBase& entry);
    void PreviewVersion(HWND hwnd, const EntryBase& entry);
    void PreviewDialog(HWND hwnd, const EntryBase& entry);
    void PreviewAniIcon(HWND hwnd, const EntryBase& entry, BOOL bIcon);
    void PreviewStringTable(HWND hwnd, const EntryBase& entry);
    void PreviewMessageTable(HWND hwnd, const EntryBase& entry);
    void PreviewRCData(HWND hwnd, const EntryBase& entry);
    void PreviewDlgInit(HWND hwnd, const EntryBase& entry);
    void PreviewUnknown(HWND hwnd, const EntryBase& entry);

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
    void OnSysColorChange(HWND hwnd);
    void OnPlay(HWND hwnd);
    void OnDropFiles(HWND hwnd, HDROP hdrop);
    void OnMove(HWND hwnd, int x, int y);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    void OnInitMenu(HWND hwnd, HMENU hMenu);
    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr);
    void OnClose(HWND hwnd);
    void OnDestroy(HWND hwnd);

    void OnCancelEdit(HWND hwnd);
    void OnCompile(HWND hwnd);
    void OnGuiEdit(HWND hwnd);
    void OnEdit(HWND hwnd);
    void OnCopyAsNewName(HWND hwnd);
    void OnCopyAsNewLang(HWND hwnd);
    void OnItemSearch(HWND hwnd);
    void OnItemSearchBang(HWND hwnd, MItemSearchDlg *pDialog);
    void OnExpandAll(HWND hwnd);
    void OnCollapseAll(HWND hwnd);
    void Expand(HTREEITEM hItem);
    void Collapse(HTREEITEM hItem);
    void OnWordWrap(HWND hwnd);
    void OnSrcEditSelect(HWND hwnd);
    void OnSaveAsWithCompression(HWND hwnd);
    void OnClone(HWND hwnd);
    void OnAddBang(HWND hwnd, NMTOOLBAR *pToolBar);
    void OnExtractBang(HWND hwnd);
    void OnJumpToMatome(HWND hwnd);
    void OnEncoding(HWND hwnd);
    void OnQueryConstant(HWND hwnd);
    void OnUseBeginEnd(HWND hwnd);

    LRESULT OnCompileCheck(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnMoveSizeReport(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnClearStatus(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnReopenRad(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnPostSearch(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnIDJumpBang(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnRadSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateDlgRes(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnGetHeadLines(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnDelphiDFMB2T(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void OnIDJumpBang2(HWND hwnd, const MString& name, MString& strType);

    void OnAddBitmap(HWND hwnd);
    void OnAddCursor(HWND hwnd);
    void OnAddDialog(HWND hwnd);
    void OnAddIcon(HWND hwnd);
    void OnAddMenu(HWND hwnd);
    void OnAddRes(HWND hwnd);
    void OnAddVerInfo(HWND hwnd);
    void OnAddManifest(HWND hwnd);
    void OnAddStringTable(HWND hwnd);
    void OnAddMessageTable(HWND hwnd);
    void OnAddHtml(HWND hwnd);
    void OnAddAccel(HWND hwnd);
    void OnDeleteRes(HWND hwnd);
    void OnExtractBin(HWND hwnd);
    void OnExtractDFM(HWND hwnd);
    void OnExtractBitmap(HWND hwnd);
    void OnExtractCursor(HWND hwnd);
    void OnExtractIcon(HWND hwnd);
    void OnReplaceBin(HWND hwnd);
    void OnReplaceBitmap(HWND hwnd);
    void OnReplaceCursor(HWND hwnd);
    void OnReplaceIcon(HWND hwnd);
    void OnUpdateResHBang(HWND hwnd);

    BOOL DoQuerySaveChange(HWND hwnd);

    void OnNew(HWND hwnd);
    void OnOpen(HWND hwnd);
    BOOL OnSave(HWND hwnd);
    BOOL OnSaveAs(HWND hwnd);
    void OnEga(HWND hwnd);
    void OnEgaProgram(HWND hwnd);
    void OnImport(HWND hwnd);
    void OnLoadResH(HWND hwnd);
    void OnLoadResHBang(HWND hwnd);
    void OnLoadWCLib(HWND hwnd);
    void OnExport(HWND hwnd);
    void OnFonts(HWND hwnd);
    void OnAbout(HWND hwnd);
    void OnConfig(HWND hwnd);
    void OnOpenReadMe(HWND hwnd);
    void OnOpenReadMeJp(HWND hwnd);
    void OnOpenReadMeIt(HWND hwnd);
    void OnOpenLicense(HWND hwnd);
    void OnOpenHyojunka(HWND hwnd);
    void OnDebugTreeNode(HWND hwnd);
    void OnAdviceResH(HWND hwnd);
    void OnUnloadResH(HWND hwnd);
    void OnHideIDMacros(HWND hwnd);
    void OnUseIDC_STATIC(HWND hwnd);
    void OnTest(HWND hwnd);

    // find/replace
    void OnFind(HWND hwnd);
    BOOL OnFindNext(HWND hwnd);
    BOOL OnFindPrev(HWND hwnd);
    BOOL OnReplaceNext(HWND hwnd);
    BOOL OnReplacePrev(HWND hwnd);
    BOOL OnReplace(HWND hwnd);
    BOOL OnReplaceAll(HWND hwnd);
    LRESULT OnFindMsg(HWND hwnd, WPARAM wParam, LPARAM lParam);

protected:
    MString GetLanguageStatement(WORD langid)
    {
        return ::GetLanguageStatement(langid, TRUE) + L"\r\n";
    }

    void UpdateNames(void);
    void UpdateEntryName(EntryBase *e, LPWSTR pszText = NULL);
    void UpdateEntryLang(EntryBase *e, LPWSTR pszText = NULL);
};

//////////////////////////////////////////////////////////////////////////////
// MMainWnd out-of-line functions

// WM_SYSCOLORCHANGE: system color settings was changed
void MMainWnd::OnSysColorChange(HWND hwnd)
{
    // notify the main window children
    m_splitter1.SendMessageDx(WM_SYSCOLORCHANGE);
    m_splitter2.SendMessageDx(WM_SYSCOLORCHANGE);
    m_splitter3.SendMessageDx(WM_SYSCOLORCHANGE);
    m_rad_window.SendMessageDx(WM_SYSCOLORCHANGE);
}

// check whether it needs compilation
LRESULT MMainWnd::OnCompileCheck(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
    {
        return FALSE;
    }
    return FALSE;
}

// reopen the RADical window
LRESULT MMainWnd::OnReopenRad(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    OnGuiEdit(hwnd);
    return 0;
}

// report the position and size to the status bar
LRESULT MMainWnd::OnMoveSizeReport(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // position
    INT x = (SHORT)LOWORD(wParam);
    INT y = (SHORT)HIWORD(wParam);

    // size
    INT cx = (SHORT)LOWORD(lParam);
    INT cy = (SHORT)HIWORD(lParam);

    // set the text to status bar
    ChangeStatusText(LoadStringPrintfDx(IDS_COORD, x, y, cx, cy));
    return 0;
}

// clear the status bar
LRESULT MMainWnd::OnClearStatus(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ChangeStatusText(TEXT(""));
    return 0;
}

// WM_ACTIVATE: if activated, then set focus to m_hwndTV
void MMainWnd::OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
    {
        // set focus to the treeview
        SetFocus(m_hwndTV);
    }

    // default processing
    FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, CallWindowProcDx);
}

// update the menu for recently used files
void MMainWnd::UpdateMenu()
{
    // get 'File' menu
    HMENU hMenu = GetMenu(m_hwnd);
    HMENU hFileMenu = GetSubMenu(hMenu, 0);

    // get Most Recently Used menu from 'File' menu
    HMENU hMruMenu = GetSubMenu(hFileMenu, GetMenuItemCount(hFileMenu) - 3);
    assert(hMruMenu);

    // delete all the menu items from MRU menu
    while (DeleteMenu(hMruMenu, 0, MF_BYPOSITION))
        ;

    TCHAR szText[MAX_PATH * 2];
    static const TCHAR szPrefix[] = TEXT("123456789ABCDEF0");

    // add the MRU menu items to the MRU menu
    INT i = 0;
    for (auto& recent : g_settings.vecRecentlyUsed)
    {
        // get the file title
        LPCTSTR pch = _tcsrchr(recent.c_str(), TEXT('\\'));
        if (pch == NULL)
            pch = _tcsrchr(recent.c_str(), TEXT('/'));
        if (pch == NULL)
            pch = recent.c_str();
        else
            ++pch;

        // build the text
        StringCchPrintf(szText, _countof(szText), TEXT("&%c  %s"), szPrefix[i], pch);

        // insert an item to the MRU menu
        InsertMenu(hMruMenu, i, MF_BYPOSITION | MF_STRING, ID_MRUFILE0 + i, szText);

        ++i;    // increment the index
    }

    if (g_settings.vecRecentlyUsed.empty())
    {
        // set the "(none)" item if empty
        InsertMenu(hMruMenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, -1, LoadStringDx(IDS_NONE));
    }
}

void MMainWnd::OnExtractDFM(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected language entry
    auto entry = g_res.get_lang_entry();
    if (!entry)
        return;

    WCHAR szFile[MAX_PATH] = L"";

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_DFMFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTDFM);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"dfm";

    // let the user choose the path
    if (GetSaveFileNameW(&ofn))
    {
        if (lstrcmpiW(PathFindExtensionW(szFile), L".txt") == 0)
        {
            auto ansi = dfm_text_from_binary(m_szDFMSC, entry->ptr(), entry->size());
            if (FILE *fp = _wfopen(szFile, L"wb"))
            {
                fwrite(ansi.c_str(), ansi.size(), 1, fp);
                fclose(fp);
            }
            else
            {
                ErrorBoxDx(IDS_CANTEXTRACTDFM);
            }
        }
        else
        {
            if (!g_res.extract_bin(ofn.lpstrFile, entry))
            {
                ErrorBoxDx(IDS_CANTEXTRACTDFM);
            }
        }
    }
}

// extract the binary as a file
void MMainWnd::OnExtractBin(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    // get the selected entry
    auto e = g_res.get_entry();
    if (!e)
        return;     // not selected

    if (e->is_delphi_dfm())
        return OnExtractDFM(hwnd);

    // initialize OPENFILENAME structure
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);

    // use the prefered filter by the entry
    if (e->m_et == ET_STRING || e->m_et == ET_MESSAGE)
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));

    if (e->m_et == ET_LANG)
    {
        if (e->m_type == L"PNG")
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_PNGRESBINFILTER));
        else if (e->m_type == L"JPEG")
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_JPEGRESBINFILTER));
        else if (e->m_type == L"GIF")
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_GIFRESBINFILTER));
        else if (e->m_type == L"TIFF")
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_TIFFRESBINFILTER));
        else if (e->m_type == L"AVI")
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_AVIRESBINFILTER));
        else if (e->m_type == L"WAVE")
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_WAVERESBINFILTER));
        else
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESBINFILTER));
    }
    else
    {
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
    }

    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"res";   // the default extension

    // let the user choose the path
    if (GetSaveFileNameW(&ofn))
    {
        // extract it to a file
        if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
        {
            // it was a *.res file
            if (!g_res.extract_res(ofn.lpstrFile, e))
            {
                ErrorBoxDx(IDS_CANNOTSAVE);
            }
        }
        else
        {
            // it was not a *.res file
            if (!g_res.extract_bin(ofn.lpstrFile, e))
            {
                ErrorBoxDx(IDS_CANNOTSAVE);
            }
        }
    }
}

// extract an icon as an *.ico file
void MMainWnd::OnExtractIcon(HWND hwnd)
{
    enum IconFilterIndex  // see also: IDS_ICOFILTER
    {
        IFI_NONE = 0,
        IFI_ICO = 1,
        IFI_ANI = 2,
        IFI_ALL = 3
    };

    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected language entry
    auto entry = g_res.get_lang_entry();
    if (!entry)
        return;

    WCHAR szFile[MAX_PATH] = L"";

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTICO);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    // use the prefered filter by the entry
    if (entry->m_type == RT_ANIICON)
    {
        ofn.nFilterIndex = IFI_ANI;
        ofn.lpstrDefExt = L"ani";   // the default extension
    }
    else
    {
        ofn.nFilterIndex = IFI_ICO;
        ofn.lpstrDefExt = L"ico";   // the default extension
    }

    // let the user choose the path
    if (GetSaveFileNameW(&ofn))
    {
        // extract it to an *.ico or *.ani file
        if (!g_res.extract_icon(ofn.lpstrFile, entry))
        {
            ErrorBoxDx(IDS_CANTEXTRACTICO);
        }
    }
}

// extract a cursor as an *.cur or *.ani file
void MMainWnd::OnExtractCursor(HWND hwnd)
{
    enum CursorFilterIndex      // see also: IDS_CURFILTER
    {
        CFI_NONE = 0,
        CFI_CUR = 1,
        CFI_ANI = 2,
        CFI_ALL = 3
    };

    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected language entry
    auto entry = g_res.get_lang_entry();
    if (!entry)     // not selected
        return;

    WCHAR szFile[MAX_PATH] = L"";

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTCUR);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    // use the prefered filter by the entry
    if (entry->m_type == RT_ANICURSOR)
    {
        ofn.nFilterIndex = CFI_ANI;
        ofn.lpstrDefExt = L"ani";   // the default extension
    }
    else
    {
        ofn.nFilterIndex = CFI_CUR;
        ofn.lpstrDefExt = L"cur";   // the default extension
    }

    // let the user choose the path
    if (GetSaveFileNameW(&ofn))
    {
        // extract it to an *.cur or *.ani file
        if (!g_res.extract_cursor(ofn.lpstrFile, entry))
        {
            ErrorBoxDx(IDS_CANTEXTRACTCUR);
        }
    }
}

// extract a bitmap as an *.bmp file
void MMainWnd::OnExtractBitmap(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected language entry
    auto entry = g_res.get_lang_entry();
    if (!entry)     // not selected
        return;

    WCHAR szFile[MAX_PATH] = L"";

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_BMPFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTBMP);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"bmp";   // the default extension

    // let the user choose the path
    if (GetSaveFileNameW(&ofn))
    {
        // extract a bitmap as an *.bmp or *.png file
        BOOL bPNG = (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"png") == 0);
        if (!PackedDIB_Extract(ofn.lpstrFile, &(*entry)[0], (*entry).size(), bPNG))
        {
            ErrorBoxDx(IDS_CANTEXTRACTBMP);
        }
    }
}

// replace the resource data by a binary file
void MMainWnd::OnReplaceBin(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    // get the selected language entry
    auto entry = g_res.get_lang_entry();
    if (!entry)
        return;

    // show the dialog
    MReplaceBinDlg dialog(entry);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // select the entry
        SelectTV(ET_LANG, dialog, FALSE);
    }

    DoSetFileModified(TRUE);
}

// version info
void MMainWnd::OnAbout(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MVersionInfoDlg dialog;
    dialog.DialogBoxDx(hwnd);
}

// show the MFontsDlg to allow the user to change the font settings
void MMainWnd::OnFonts(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the fonts dialog
    MFontsDlg dialog;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // update m_hBinFont and set it to m_hBinEdit
        DeleteObject(m_hBinFont);
        m_hBinFont = dialog.DetachBinFont();
        SetWindowFont(m_hBinEdit, m_hBinFont, TRUE);

        // update m_hSrcFont and set it to m_hSrcEdit
        DeleteObject(m_hSrcFont);
        m_hSrcFont = dialog.DetachSrcFont();
        SetWindowFont(m_hSrcEdit, m_hSrcFont, TRUE);
    }
}

// export all the resource items to an RC file
void MMainWnd::OnExport(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    // show the "export options" dialog
    MExportOptionsDlg dialog;
    if (dialog.DialogBoxDx(hwnd) != IDOK)
        return;

    WCHAR file[MAX_PATH] = TEXT("");

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RCFILTER));
    ofn.lpstrFile = file;
    ofn.nMaxFile = _countof(file);
    ofn.lpstrTitle = LoadStringDx(IDS_EXPORT);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"rc";    // the default extension

    // let the user choose the path
    if (GetSaveFileNameW(&ofn))
    {
        // do export!
        if (!DoExport(file))
        {
            ErrorBoxDx(IDS_CANTEXPORT);
        }
    }
}

// the window class libraries
typedef std::unordered_set<HMODULE> wclib_t;
wclib_t s_wclib;

// is there a window class that is named pszName?
BOOL IsThereWndClass(const WCHAR *pszName)
{
    if (!pszName || pszName[0] == 0)
        return FALSE;   // failure

    WNDCLASSEX cls;
    if (GetClassInfoEx(NULL, pszName, &cls) ||
        GetClassInfoEx(GetModuleHandle(NULL), pszName, &cls))
    {
        return TRUE;    // already exists
    }

    // in the window class libraries?
    for (auto& library : s_wclib)
    {
        if (GetClassInfoEx(library, pszName, &cls))
            return TRUE;    // found
    }

    // CLSID?
    if (pszName[0] == L'{' &&
        pszName[9] == L'-' && pszName[14] == L'-' &&
        pszName[19] == L'-' && pszName[24] == L'-' &&
        pszName[37] == L'}')
    {
        return TRUE;        // it's a CLSID
    }

    // ATL OLE control?
    if (std::wstring(pszName).find(L"AtlAxWin") == 0)
        return TRUE;        // it's an ATL OLE control

    return FALSE;   // failure
}

// release all the window class libraries
void FreeWCLib()
{
    for (auto& library : s_wclib)
    {
        FreeLibrary(library);
        //library = NULL;
    }
    s_wclib.clear();
}

// load a window class library
void MMainWnd::OnLoadWCLib(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    WCHAR file[MAX_PATH] = TEXT("");

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_DLLFILTER));
    ofn.lpstrFile = file;
    ofn.nMaxFile = _countof(file);
    ofn.lpstrTitle = LoadStringDx(IDS_LOADWCLIB);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"dll";       // the default extension

    // let the user choose the path
    if (GetOpenFileNameW(&ofn))
    {
        // load the window class library
        HMODULE hMod = LoadLibraryW(file);
        if (hMod)
        {
            // success. add it
            s_wclib.insert(hMod);
        }
        else
        {
            ErrorBoxDx(IDS_CANNOTLOAD);
        }
    }
}

// import the resource data additionally
void MMainWnd::OnImport(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    WCHAR file[MAX_PATH] = TEXT("");

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_IMPORTFILTER));
    ofn.lpstrFile = file;
    ofn.nMaxFile = _countof(file);
    ofn.lpstrTitle = LoadStringDx(IDS_IMPORTRES);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"res";       // the default extension

    // let the user choose the path
    if (GetOpenFileNameW(&ofn))
    {
        // find the file title
        LPCWSTR pch = wcsrchr(file, L'\\');
        if (pch == NULL)
            pch = wcsrchr(file, L'/');
        if (pch == NULL)
            pch = file;
        else
            ++pch;

        // find the dot extension
        pch = wcsrchr(pch, L'.');

        if (IMPORT_FAILED == DoImport(hwnd, file, pch))
        {
            ErrorBoxDx(IDS_CANNOTIMPORT);
        }
        DoSetFileModified(TRUE);
    }
}

// open a file
void MMainWnd::OnOpen(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    if (!DoQuerySaveChange(hwnd))
        return;

    // store the nominal path
    WCHAR szFile[MAX_PATH];
    StringCchCopyW(szFile, _countof(szFile), m_szFile);

    // if path was not valid, make it empty
    if (!PathFileExistsW(szFile))
        szFile[0] = 0;

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXERESRCFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_OPEN);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"exe";       // the default extension

    // let the user choose the path
    if (GetOpenFileNameW(&ofn))
    {
        // load the file
        DoLoadFile(hwnd, szFile, ofn.nFilterIndex);
    }
}

BOOL MMainWnd::DoQuerySaveChange(HWND hwnd)
{
    if (!s_bModified)
        return TRUE;

    INT id = MessageBoxW(hwnd, LoadStringDx(IDS_QUERYSAVECHANGE),
                         LoadStringDx(IDS_APPNAME),
                         MB_ICONINFORMATION | MB_YESNOCANCEL);
    if (id == IDCANCEL)
        return FALSE;

    if (id == IDYES)
        return OnSave(hwnd);

    return TRUE;
}

// clear all the resource data
void MMainWnd::OnNew(HWND hwnd)
{
    if (!DoQuerySaveChange(hwnd))
        return;

    // close preview
    HidePreview();

    // unload the resource.h file
    OnUnloadResH(hwnd);

    // update the file info
    UpdateFileInfo(FT_NONE, NULL, FALSE);

    // unselect
    SelectTV(NULL, FALSE);

    // clean up
    g_res.delete_all();

    DoSetFileModified(FALSE);
}

enum ResFileFilterIndex     // see also: IDS_EXERESFILTER
{
    RFFI_NONE = 0,
    RFFI_EXECUTABLE = 1,
    RFFI_RC = 2,
    RFFI_RES = 3,
    RFFI_ALL = 4
};

// save as a file or files
BOOL MMainWnd::OnSaveAs(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return FALSE;

    // store m_szFile to szFile
    WCHAR szFile[MAX_PATH];
    StringCchCopyW(szFile, _countof(szFile), m_szFile);

    // if not found, then make it empty
    if (!PathFileExistsW(szFile))
        szFile[0] = 0;

    // was it an executable?
    BOOL bWasExecutable = (m_file_type == FT_EXECUTABLE);

    // get and delete the filename extension
    WCHAR szExt[32] = L"";
    LPWSTR pch = wcsrchr(szFile, L'.');
    static const LPCWSTR s_DotExts[] =
    {
        L".exe", L".dll", L".ocx", L".cpl", L".scr", L".mui", L".rc", L".res"
    };
    for (auto ext : s_DotExts)
    {
        if (lstrcmpiW(pch, ext) == 0)
        {
            StringCbCopyW(szExt, sizeof(szExt), ext + 1);
            *pch = 0;
            break;
        }
    }

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXERESFILTER));

    // use the prefered filter by the entry
    ofn.nFilterIndex = g_settings.nSaveFilterIndex;
    if (bWasExecutable)
    {
        if (ofn.nFilterIndex != RFFI_EXECUTABLE)
            ofn.nFilterIndex = RFFI_EXECUTABLE;
    }
    else
    {
        if (ofn.nFilterIndex == RFFI_EXECUTABLE)
            ofn.nFilterIndex = RFFI_RC;
    }

    // use the preferred extension
    switch (ofn.nFilterIndex)
    {
    case RFFI_EXECUTABLE:
        if (szExt[0])
        {
            ofn.lpstrDefExt = szExt;
        }
        else
        {
            ofn.lpstrDefExt = L"exe";       // the default extension
        }
        break;

    case RFFI_RC:
        ofn.lpstrDefExt = L"rc";        // the default extension
        break;

    case RFFI_RES:
    default:
        ofn.lpstrDefExt = L"res";       // the default extension
        break;
    }

    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_SAVEAS);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    // let the user choose the path
    if (GetSaveFileNameW(&ofn))
    {
        // save the filter index to the settings
        g_settings.nSaveFilterIndex = ofn.nFilterIndex;

        if (ofn.nFilterIndex == RFFI_ALL)
        {
            ofn.nFilterIndex = RFFI_EXECUTABLE;
        }

        switch (ofn.nFilterIndex)
        {
        case RFFI_EXECUTABLE:
            // save it
            if (DoSaveAs(szFile))
                return TRUE;

            ErrorBoxDx(IDS_CANNOTSAVE);
            break;

        case RFFI_RC:
            // export and save it
            {
                // show "save options" dialog
                MSaveOptionsDlg save_options;
                if (save_options.DialogBoxDx(hwnd) != IDOK)
                    return FALSE;

                // export
                WCHAR szResH[MAX_PATH] = L"";
                if (DoExport(szFile, szResH))   // succeeded
                {
                    // save the resource.h path
                    StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);

                    // update the file info
                    UpdateFileInfo(FT_RC, szFile, FALSE);

                    return TRUE;
                }
                else
                {
                    ErrorBoxDx(IDS_CANNOTSAVE);
                }
            }
            break;

        case RFFI_RES:
            // save the *.res file
            if (DoSaveResAs(szFile))
                return TRUE;

            ErrorBoxDx(IDS_CANNOTSAVE);
            break;

        default:
            assert(0);
            break;
        }
    }

    return FALSE;
}

void MMainWnd::OnEga(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    MEgaDlg dialog(NULL);
    dialog.DialogBoxDx(hwnd);
}

void MMainWnd::OnEgaProgram(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    WCHAR szFile[MAX_PATH] = L"";
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EGAFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = ARRAYSIZE(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_LOADEGAPROGRAM);
    ofn.lpstrDefExt = L"ega";
    if (GetOpenFileNameW(&ofn))
    {
        MEgaDlg dialog(szFile);
        dialog.DialogBoxDx(hwnd);
    }
}

BOOL MMainWnd::OnSave(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return FALSE;

    if (!m_szFile[0])
    {
        return OnSaveAs(hwnd);
    }

    LPWSTR pchDotExt = PathFindExtensionW(m_szFile);
    if (lstrcmpiW(pchDotExt, L".res") == 0)
    {
        g_settings.nSaveFilterIndex = RFFI_RES;
    }
    else if (lstrcmpiW(pchDotExt, L".rc") == 0)
    {
        g_settings.nSaveFilterIndex = RFFI_RC;
    }
    else
    {
        g_settings.nSaveFilterIndex = RFFI_EXECUTABLE;
    }

    switch (g_settings.nSaveFilterIndex)
    {
    case RFFI_EXECUTABLE:
        // save it
        if (DoSaveAs(m_szFile))
        {
            return TRUE;
        }
        ErrorBoxDx(IDS_CANNOTSAVE);
        break;

    case RFFI_RC:
        // export and save it
        {
            // show "save options" dialog
            MSaveOptionsDlg save_options;
            if (save_options.DialogBoxDx(hwnd) != IDOK)
                return FALSE;

            // export
            WCHAR szResH[MAX_PATH] = L"";
            if (DoExport(m_szFile, szResH))   // succeeded
            {
                // save the resource.h path
                StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);

                // update the file info
                UpdateFileInfo(FT_RC, m_szFile, FALSE);

                return TRUE;
            }
            else
            {
                ErrorBoxDx(IDS_CANNOTSAVE);
            }
        }
        break;

    case RFFI_RES:
        // save the *.res file
        if (DoSaveResAs(m_szFile))
        {
            return TRUE;
        }
        ErrorBoxDx(IDS_CANNOTSAVE);
        break;

    default:
        assert(0);
        break;
    }

    return FALSE;
}

// update the fonts by the font settings
void MMainWnd::ReCreateFonts(HWND hwnd)
{
    // delete the fonts
    if (m_hBinFont)
    {
        DeleteObject(m_hBinFont);
        m_hBinFont = NULL;
    }
    if (m_hSrcFont)
    {
        DeleteObject(m_hSrcFont);
        m_hSrcFont = NULL;
    }

    // initialize LOGFONT structures
    LOGFONTW lfBin, lfSrc;
    ZeroMemory(&lfBin, sizeof(lfBin));
    ZeroMemory(&lfSrc, sizeof(lfSrc));

    // set lfFaceName from settings
    StringCchCopy(lfBin.lfFaceName, _countof(lfBin.lfFaceName), g_settings.strBinFont.c_str());
    StringCchCopy(lfSrc.lfFaceName, _countof(lfSrc.lfFaceName), g_settings.strSrcFont.c_str());

    // calculate the height
    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        lfBin.lfHeight = -MulDiv(g_settings.nBinFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        lfSrc.lfHeight = -MulDiv(g_settings.nSrcFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        DeleteDC(hDC);
    }

    // create the fonts
    m_hBinFont = CreateFontIndirectW(&lfBin);
    assert(m_hBinFont);
    m_hSrcFont = ::CreateFontIndirectW(&lfSrc);
    assert(m_hSrcFont);

    // set the fonts to the controls
    SetWindowFont(m_hBinEdit, m_hBinFont, TRUE);
    SetWindowFont(m_hSrcEdit, m_hSrcFont, TRUE);
}

// check the text for item search
static bool CheckTextForSearch(ITEM_SEARCH *pSearch, EntryBase *entry, MString text)
{
    // make the text uppercase to ignore case
    if (pSearch->bIgnoreCases)
        CharUpperW(&text[0]);

    // find?
    if (text.find(pSearch->strText) == MString::npos)
        return false;   // not found

    if (pSearch->bDownward)     // go downward
    {
        // check the position
        if (pSearch->pCurrent == NULL || *pSearch->pCurrent < *entry)
        {
            if (pSearch->pFound)    // already found
            {
                if (*entry < *pSearch->pFound)  // compare with the found one
                {
                    pSearch->pFound = entry;
                    return true;    // found
                }
            }
            else    // not found yet
            {
                pSearch->pFound = entry;    // found
                return true;    // found
            }
        }
    }
    else    // go upward
    {
        // check the position
        if (pSearch->pCurrent == NULL || *entry < *pSearch->pCurrent)
        {
            if (pSearch->pFound)    // already found
            {
                if (*pSearch->pFound < *entry)  // compare with the found one
                {
                    pSearch->pFound = entry;
                    return true;    // found
                }
            }
            else
            {
                pSearch->pFound = entry;
                return true;    // found
            }
        }
    }

    return false;    // not found
}

// a function for item search
static unsigned __stdcall
search_proc(void *arg)
{
    auto pSearch = (ITEM_SEARCH *)arg;
    ResToText& res2text = pSearch->res2text;
    MString text;

    for (auto entry : g_res)
    {
        if (!entry->valid())
            continue;

        EntryBase e = *entry;

        // check label
        text = e.m_strLabel;
        if (CheckTextForSearch(pSearch, entry, text))
        {
            //MessageBoxW(NULL, e.m_strLabel.c_str(), L"OK", 0);
            continue;
        }

        // check internal text
        if (pSearch->bInternalText)
        {
            switch (e.m_et)
            {
            case ET_LANG:
            case ET_MESSAGE:
                break;
            case ET_STRING:
                // ignore the name
                e.m_name.clear();
                break;
            default:
                continue;
            }

            text = pSearch->res2text.DumpEntry(e);
            if (CheckTextForSearch(pSearch, entry, text))
            {
                //MessageBoxW(NULL, (e.m_strLabel + L"<>" + text).c_str(), NULL, 0);
                continue;
            }
        }
    }

    pSearch->bRunning = FALSE;  // finish
    return 0;
}

// do item search
BOOL MMainWnd::DoItemSearch(ITEM_SEARCH& search)
{
    MWaitCursor wait;

    if (search.bIgnoreCases)
        CharUpperW(&search.strText[0]);

    search_proc(&search);

    return search.pFound != NULL;
}

// clone the resource item in new name
void MMainWnd::OnCopyAsNewName(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected name entry
    auto entry = g_res.get_entry();
    if (!entry || entry->m_et != ET_NAME)
        return;

    // show the dialog
    MCloneInNewNameDlg dialog(entry);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // search the ET_LANG entries
        EntrySetBase found;
        g_res.search(found, ET_LANG, entry->m_type, entry->m_name);

        if (entry->m_type == RT_GROUP_ICON)     // group icon
        {
            for (auto e : found)
            {
                g_res.copy_group_icon(e, dialog.m_name, e->m_lang);
            }
        }
        else if (entry->m_type == RT_GROUP_CURSOR)  // group cursor
        {
            for (auto e : found)
            {
                g_res.copy_group_cursor(e, dialog.m_name, e->m_lang);
            }
        }
        else    // otherwise
        {
            for (auto e : found)
            {
                g_res.add_lang_entry(e->m_type, dialog.m_name, e->m_lang, e->m_data);
            }
        }

        // select the entry
        SelectTV(ET_NAME, dialog.m_type, dialog.m_name, BAD_LANG, FALSE);
        DoSetFileModified(TRUE);
    }
}

// clone the resource item in new language
void MMainWnd::OnCopyAsNewLang(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected entry
    auto entry = g_res.get_entry();
    if (!entry)
        return;

    switch (entry->m_et)
    {
    case ET_LANG: case ET_STRING: case ET_MESSAGE:
        break;      // ok

    default:
        return;     // unable to copy the language
    }

    // show the dialog
    MCloneInNewLangDlg dialog(entry);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        if (entry->m_type == RT_GROUP_ICON)     // group icon
        {
            // search the group icons
            EntrySetBase found;
            g_res.search(found, ET_LANG, RT_GROUP_ICON, entry->m_name, entry->m_lang);

            // copy them
            for (auto e : found)
            {
                g_res.copy_group_icon(e, e->m_name, dialog.m_lang);
            }

            // select the entry
            SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
        }
        else if (entry->m_type == RT_GROUP_CURSOR)
        {
            // search the group cursors
            EntrySetBase found;
            g_res.search(found, ET_LANG, RT_GROUP_CURSOR, entry->m_name, entry->m_lang);

            // copy them
            for (auto e : found)
            {
                g_res.copy_group_cursor(e, e->m_name, dialog.m_lang);
            }

            // select the entry
            SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
        }
        else if (entry->m_et == ET_STRING)
        {
            // search the strings
            EntrySetBase found;
            g_res.search(found, ET_LANG, RT_STRING, WORD(0), entry->m_lang);

            // copy them
            for (auto e : found)
            {
                g_res.add_lang_entry(e->m_type, e->m_name, dialog.m_lang, e->m_data);
            }

            // select the entry
            SelectTV(ET_STRING, dialog.m_type, WORD(0), dialog.m_lang, FALSE);
        }
        else if (entry->m_et == ET_MESSAGE)
        {
            // search the messagetables
            EntrySetBase found;
            g_res.search(found, ET_LANG, RT_MESSAGETABLE, WORD(0), entry->m_lang);

            // copy them
            for (auto e : found)
            {
                g_res.add_lang_entry(e->m_type, e->m_name, dialog.m_lang, e->m_data);
            }

            // select the entry
            SelectTV(ET_MESSAGE, dialog.m_type, WORD(0), dialog.m_lang, FALSE);
        }
        else
        {
            // search the entries
            EntrySetBase found;
            g_res.search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);

            // copy them
            for (auto e : found)
            {
                g_res.add_lang_entry(e->m_type, e->m_name, dialog.m_lang, e->m_data);
            }

            // select the entry
            SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
        }
        DoSetFileModified(TRUE);
    }
}

// show the item search dialog
void MMainWnd::OnItemSearch(HWND hwnd)
{
    // is there "item search" dialogs?
    if (!MItemSearchDlg::Dialogs().empty())
    {
        // bring it to the top
        HWND hDlg = **MItemSearchDlg::Dialogs().begin();
        SetForegroundWindow(hDlg);
        SetFocus(hDlg);
        return;
    }

    // create dialog
    MItemSearchDlg *pDialog = new MItemSearchDlg(m_search);
    pDialog->CreateDialogDx(hwnd);

    // set the window handles to m_search.res2text
    m_search.res2text.m_hwnd = hwnd;
    m_search.res2text.m_hwndDialog = *pDialog;

    // show it
    ShowWindow(*pDialog, SW_SHOWNORMAL);
    UpdateWindow(*pDialog);
}

// do item search
void MMainWnd::OnItemSearchBang(HWND hwnd, MItemSearchDlg *pDialog)
{
    // is it visible?
    if (!IsWindowVisible(pDialog->m_hwnd))
    {
        assert(0);
        return;
    }

    // get the selected entry
    auto entry = g_res.get_entry();

    if (m_search.bIgnoreCases)
    {
        CharUpperW(&m_search.strText[0]);
    }

    // initialize
    m_search.bCancelled = FALSE;
    m_search.pFound = NULL;
    m_search.pCurrent = entry;

    // start searching
    if (DoItemSearch(m_search) && m_search.pFound)
    {
        pDialog->Done();    // uninitialize

        // select the found one
        TreeView_SelectItem(m_hwndTV, m_search.pFound->m_hItem);
        TreeView_EnsureVisible(m_hwndTV, m_search.pFound->m_hItem);

        // recalculate the splitter
        PostMessageDx(MYWM_POSTSEARCH);
    }
    else
    {
        pDialog->Done();    // uninitialize

        // is it not cancelled?
        if (!m_search.bCancelled)
        {
            // "no more item" message
            EnableWindow(*pDialog, FALSE);
            MsgBoxDx(IDS_NOMOREITEM, MB_ICONINFORMATION);
            EnableWindow(*pDialog, TRUE);
        }

        // set focus to the dialog
        SetFocus(*pDialog);
    }
}

// delete a resource item
void MMainWnd::OnDeleteRes(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected entry
    if (auto entry = g_res.get_entry())
    {
        // delete it
        if (g_res.super()->find(entry) != g_res.end())
            TreeView_DeleteItem(m_hwndTV, entry->m_hItem);
    }
    DoSetFileModified(TRUE);
}

// play the sound
void MMainWnd::OnPlay(HWND hwnd)
{
    // is it a WAVE sound?
    auto entry = g_res.get_lang_entry();
    if (entry && entry->m_type == L"WAVE")
    {
        // play the sound
        PlaySound((LPCTSTR)&(*entry)[0], NULL, SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
    }
}

// cancel edit
void MMainWnd::OnCancelEdit(HWND hwnd)
{
    // clear modification flag
    Edit_SetModify(m_hSrcEdit, FALSE);
    Edit_SetReadOnly(m_hSrcEdit, FALSE);

    // reselect to update the m_hSrcEdit
    auto entry = g_res.get_entry();
    SelectTV(entry, FALSE);
}

// set error message
void MMainWnd::SetErrorMessage(const MStringA& strOutput, BOOL bBox)
{
    if (bBox)
    {
        // show the message box
        if (strOutput.empty())
        {
            MWideToAnsi ansi(CP_ACP, LoadStringDx(IDS_COMPILEERROR));
            MessageBoxA(m_hwnd, ansi.c_str(), NULL, MB_ICONERROR);
        }
        else
        {
            MessageBoxA(m_hwnd, strOutput.c_str(), NULL, MB_ICONERROR);
        }
    }
    else
    {
        // show the message in m_hBinEdit
        if (strOutput.empty())
        {
            SetWindowTextW(m_hBinEdit, LoadStringDx(IDS_COMPILEERROR));
        }
        else
        {
            SetWindowTextA(m_hBinEdit, (char *)&strOutput[0]);
        }

        // show m_hBinEdit
        ShowBinEdit(TRUE, TRUE);
    }
}

// compile the source text
void MMainWnd::OnCompile(HWND hwnd)
{
    // needs reopen?
    BOOL bReopen = IsWindowVisible(m_rad_window);

    // get the selected entry
    auto entry = g_res.get_entry();
    if (!entry)
        return;

    // is it not modified?
    if (!Edit_GetModify(m_hSrcEdit))
    {
        // select the entry
        SelectTV(entry, FALSE);
        return;
    }

    ChangeStatusText(IDS_COMPILING);

    // m_hSrcEdit --> strWide
    MStringW strWide = MWindowBase::GetWindowTextW(m_hSrcEdit);

    // compile the strWide text
    MStringA strOutput;
    if (CompileParts(strOutput, entry->m_type, entry->m_name, entry->m_lang, strWide, bReopen))
    {
        // clear the control selection
        MRadCtrl::GetTargets().clear();
        m_hSrcEdit.ClearIndeces();

        // clear the modification flag
        Edit_SetModify(m_hSrcEdit, FALSE);

        // select the entry
        SelectTV(entry, FALSE);
    }
    else
    {
        // failed
        SetErrorMessage(strOutput);
    }
}

// do GUI edit
void MMainWnd::OnGuiEdit(HWND hwnd)
{
    // get the selected entry
    auto entry = g_res.get_entry();
    if (!entry->is_editable())
        return;     // not editable

    if (!entry->can_gui_edit())
        return;     // unable to edit by GUI?

    // compile if necessary
    if (!CompileIfNecessary(FALSE))
    {
        return;
    }

    DoSetFileModified(TRUE);

    if (entry->m_type == RT_ACCELERATOR)
    {
        // entry->m_data --> accel_res
        AccelRes accel_res;
        MByteStreamEx stream(entry->m_data);
        if (accel_res.LoadFromStream(stream))
        {
            // editing...
            ChangeStatusText(IDS_EDITINGBYGUI);

            // show the dialog
            MEditAccelDlg dialog(accel_res);
            INT nID = (INT)dialog.DialogBoxDx(hwnd);
            if (nID == IDOK)
            {
                // update accel_res
                accel_res.Update();

                // accel_res --> entry->m_data
                entry->m_data = accel_res.data();
            }
        }

        // make it non-read-only
        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        // select the entry
        SelectTV(entry, FALSE);

        // ready
        ChangeStatusText(IDS_READY);
    }
    else if (entry->m_type == RT_MENU)
    {
        // entry->m_data --> menu_res
        MenuRes menu_res;
        MByteStreamEx stream(entry->m_data);
        if (menu_res.LoadFromStream(stream))
        {
            // editing...
            ChangeStatusText(IDS_EDITINGBYGUI);

            // show the dialog
            MEditMenuDlg dialog(menu_res);
            INT nID = (INT)dialog.DialogBoxDx(hwnd);
            if (nID == IDOK)
            {
                // update menu_res
                menu_res.Update();

                // menu_res --> entry->m_data
                entry->m_data = menu_res.data();
            }
        }

        // make it non-read-only
        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        // select the entry
        SelectTV(entry, FALSE);

        // ready
        ChangeStatusText(IDS_READY);
    }
    else if (entry->m_type == RT_DIALOG)
    {
        // entry->m_data --> m_rad_window.m_dialog_res
        MByteStreamEx stream(entry->m_data);
        m_rad_window.m_dialog_res.LoadFromStream(stream);
        m_rad_window.m_dialog_res.m_lang = entry->m_lang;
        m_rad_window.m_dialog_res.m_name = entry->m_name;
        m_rad_window.clear_maps();
        m_rad_window.create_maps(entry->m_lang);

        // load RT_DLGINIT
        if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, entry->m_lang))
        {
            m_rad_window.m_dialog_res.LoadDlgInitData(e->m_data);
        }
        else if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, BAD_LANG))
        {
            m_rad_window.m_dialog_res.LoadDlgInitData(e->m_data);
        }

        if (::IsWindowVisible(m_rad_window) &&
            ::IsWindowVisible(m_rad_window.m_rad_dialog))
        {
            // recreate the RADical dialog
            m_rad_window.ReCreateRadDialog(m_rad_window);
        }
        else
        {
            // create the RADical dialog
            if (!m_rad_window.CreateDx(m_hwnd))
            {
                ErrorBoxDx(IDS_DLGFAIL);
            }
        }

        // make it non-read-only
        Edit_SetReadOnly(m_hSrcEdit, FALSE);
    }
    else if (entry->m_type == RT_DLGINIT)
    {
        // entry->m_data --> dlginit_res
        DlgInitRes dlginit_res;
        MByteStreamEx stream(entry->m_data);
        if (dlginit_res.LoadFromStream(stream))
        {
            // editing...
            ChangeStatusText(IDS_EDITINGBYGUI);

            // show the dialog
            MDlgInitDlg dialog(dlginit_res);
            INT nID = (INT)dialog.DialogBoxDx(hwnd);
            if (nID == IDOK)
            {
                // dlginit_res --> entry->m_data
                entry->m_data = dlginit_res.data();
            }
        }

        // make it non-read-only
        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        // select the entry
        SelectTV(entry, FALSE);

        // ready
        ChangeStatusText(IDS_READY);
    }
    else if (entry->m_type == RT_STRING && entry->m_et == ET_STRING)
    {
        // g_res --> found
        WORD lang = entry->m_lang;
        EntrySetBase found;
        g_res.search(found, ET_LANG, RT_STRING, WORD(0), lang);

        // found --> str_res
        StringRes str_res;
        for (auto e : found)
        {
            MByteStreamEx stream(e->m_data);
            if (!str_res.LoadFromStream(stream, e->m_name.m_id))
            {
                ErrorBoxDx(IDS_CANNOTLOAD);
                return;
            }
        }

        // editing...
        ChangeStatusText(IDS_EDITINGBYGUI);

        // show the dialog
        MStringsDlg dialog(str_res);
        INT nID = (INT)dialog.DialogBoxDx(hwnd);
        if (nID == IDOK)
        {
            // dialog --> str_res
            str_res = dialog.m_str_res;

            // dump (with disabling macro IDs)
            bool shown = !g_settings.bHideID;
            g_settings.bHideID = false;
            MStringW strWide = str_res.Dump();
            g_settings.bHideID = !shown;

            // compile the dumped result
            MStringA strOutput;
            if (CompileParts(strOutput, RT_STRING, WORD(0), lang, strWide))
            {
                // select the entry to update the source
                SelectTV(ET_STRING, RT_STRING, WORD(0), lang, FALSE);
            }
            else
            {
                SetErrorMessage(strOutput);
            }
        }

        // make it non-read-only
        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        // ready
        ChangeStatusText(IDS_READY);
    }
    else if (entry->m_type == RT_MESSAGETABLE && entry->m_et == ET_MESSAGE)
    {
        // g_res --> found
        WORD lang = entry->m_lang;
        EntrySetBase found;
        g_res.search(found, ET_LANG, RT_MESSAGETABLE, WORD(0), lang);

        // found --> msg_res
        MessageRes msg_res;
        for (auto e : found)
        {
            MByteStreamEx stream(e->m_data);
            if (!msg_res.LoadFromStream(stream, e->m_name.m_id))
            {
                ErrorBoxDx(IDS_CANNOTLOAD);
                return;
            }
        }

        // editing...
        ChangeStatusText(IDS_EDITINGBYGUI);

        // show the dialog
        MMessagesDlg dialog(msg_res);
        INT nID = (INT)dialog.DialogBoxDx(hwnd);
        if (nID == IDOK)
        {
            // dialog --> msg_res
            msg_res = dialog.m_msg_res;

            // dump (with disabling macro IDs)
            bool shown = !g_settings.bHideID;
            g_settings.bHideID = false;
            MStringW strWide = msg_res.Dump();
            g_settings.bHideID = !shown;

            // compile the dumped result
            MStringA strOutput;
            if (CompileParts(strOutput, RT_MESSAGETABLE, WORD(1), lang, strWide))
            {
                // select the entry
                SelectTV(ET_MESSAGE, RT_MESSAGETABLE, (WORD)0, lang, FALSE);
            }
            else
            {
                SetErrorMessage(strOutput);
            }
        }

        // make it non-read-only
        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        // ready
        ChangeStatusText(IDS_READY);
    }
}

MStringW
GetResTypeEncoding(const MIdOrString& type)
{
    MStringW name;

    if (type.m_id)
    {
        name = g_db.GetName(L"RESOURCE", type.m_id);
        if (name.empty())
            name = mstr_dec_word(type.m_id);
    }
    else
    {
        name = type.str();
    }

    auto it = g_settings.encoding_map.find(name);
    if (it != g_settings.encoding_map.end())
        return it->second;

    return L"";
}

bool IsEntryTextEditable(const EntryBase *entry)
{
    if (!entry)
        return false;

    if (entry->is_editable())
        return true;

    auto enc = GetResTypeEncoding(entry->m_type);
    if (enc.size() && enc != L"bin")
        return true;

    return false;
}

// do text edit
void MMainWnd::OnEdit(HWND hwnd)
{
    // get the selected entry
    auto entry = g_res.get_entry();
    if (!IsEntryTextEditable(entry))
        return;

    // make it non-read-only
    Edit_SetReadOnly(m_hSrcEdit, FALSE);

    // select the entry
    SelectTV(entry, TRUE);
    DoSetFileModified(TRUE);
}

// open README
void MMainWnd::OnOpenReadMe(HWND hwnd)
{
    // get the module path filename of this application module
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    // find the last '\\' or '/'
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(szPath, L'/');
    if (pch == NULL)
        pch = szPath;
    else
        ++pch;

    // find the "README.txt" file
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"README.txt");
    if (!PathFileExistsW(szPath))
    {
        StringCchCopyW(pch, diff, L"..\\README.txt");
        if (!PathFileExistsW(szPath))
        {
            StringCchCopyW(pch, diff, L"..\\..\\README.txt");
            if (!PathFileExistsW(szPath))
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\README.txt");
                if (!PathFileExistsW(szPath))
                {
                    return;
                }
            }
        }
    }

    // open it
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

// Open READMEJP (Japanese)
void MMainWnd::OnOpenReadMeJp(HWND hwnd)
{
    // get the module path filename of this application module
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    // find the last '\\' or '/'
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(szPath, L'/');
    if (pch == NULL)
        pch = szPath;
    else
        ++pch;

    // find the "READMEJP.txt" file
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"READMEJP.txt");
    if (!PathFileExistsW(szPath))
    {
        StringCchCopyW(pch, diff, L"..\\READMEJP.txt");
        if (!PathFileExistsW(szPath))
        {
            StringCchCopyW(pch, diff, L"..\\..\\READMEJP.txt");
            if (!PathFileExistsW(szPath))
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\READMEJP.txt");
                if (!PathFileExistsW(szPath))
                {
                    return;
                }
            }
        }
    }

    // open it
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

// Open READMEIT (Italian)
void MMainWnd::OnOpenReadMeIt(HWND hwnd)
{
    // get the module path filename of this application module
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    // find the last '\\' or '/'
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(szPath, L'/');
    if (pch == NULL)
        pch = szPath;
    else
        ++pch;

    // find the "READMEIT.txt" file
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"READMEIT.txt");
    if (!PathFileExistsW(szPath))
    {
        StringCchCopyW(pch, diff, L"..\\READMEIT.txt");
        if (!PathFileExistsW(szPath))
        {
            StringCchCopyW(pch, diff, L"..\\..\\READMEIT.txt");
            if (!PathFileExistsW(szPath))
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\READMEIT.txt");
                if (!PathFileExistsW(szPath))
                {
                    return;
                }
            }
        }
    }

    // open it
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

// Open HYOJUNKA.txt (Japanese)
void MMainWnd::OnOpenHyojunka(HWND hwnd)
{
    // get the module path filename of this application module
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    // find the last '\\' or '/'
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(szPath, L'/');
    if (pch == NULL)
        pch = szPath;
    else
        ++pch;

    // find the "HYOJUNKA.txt" file
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"HYOJUNKA.txt");
    if (!PathFileExistsW(szPath))
    {
        StringCchCopyW(pch, diff, L"..\\HYOJUNKA.txt");
        if (!PathFileExistsW(szPath))
        {
            StringCchCopyW(pch, diff, L"..\\..\\HYOJUNKA.txt");
            if (!PathFileExistsW(szPath))
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\HYOJUNKA.txt");
                if (!PathFileExistsW(szPath))
                {
                    return;
                }
            }
        }
    }

    // open it
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

// open the license text file
void MMainWnd::OnOpenLicense(HWND hwnd)
{
    // get the module path filename of this application module
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    // find the last '\\' or '/'
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(szPath, L'/');
    if (pch == NULL)
        pch = szPath;
    else
        ++pch;

    // find the "LICENSE.txt" file
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"LICENSE.txt");
    if (!PathFileExistsW(szPath))
    {
        StringCchCopyW(pch, diff, L"..\\LICENSE.txt");
        if (!PathFileExistsW(szPath))
        {
            StringCchCopyW(pch, diff, L"..\\..\\LICENSE.txt");
            if (!PathFileExistsW(szPath))
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\LICENSE.txt");
                if (!PathFileExistsW(szPath))
                {
                    return;
                }
            }
        }
    }

    // open it
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

// do UPX test to check whether the file is compressed or not
BOOL MMainWnd::DoUpxTest(LPCWSTR pszUpx, LPCWSTR pszFile)
{
    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L"\"";
    strCmdLine += pszUpx;
    strCmdLine += L"\" -t \"";
    strCmdLine += pszFile;
    strCmdLine += L"\"";
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    // create an upx.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        MStringA strOutput;
        pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
        pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

        if (pmaker.GetExitCode() == 0)
        {
            if (strOutput.find("[OK]") != MStringA::npos)
            {
                // success
                bOK = TRUE;
            }
        }
    }

    return bOK;
}

// do UPX extract to decompress the file
BOOL MMainWnd::DoUpxDecompress(LPCWSTR pszUpx, LPCWSTR pszFile)
{
    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L"\"";
    strCmdLine += pszUpx;
    strCmdLine += L"\" -d \"";
    strCmdLine += pszFile;
    strCmdLine += L"\"";
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    // create the upx.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        MStringA strOutput;
        bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
        pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

        if (pmaker.GetExitCode() == 0 && bOK)
        {
            DebugPrintDx("%s\n", strOutput.c_str());
            bOK = (strOutput.find("Unpacked") != MStringA::npos);
        }
    }

    return bOK;
}

// for debugging purpose
void MMainWnd::OnDebugTreeNode(HWND hwnd)
{
    WCHAR sz[MAX_PATH * 2 + 32];

    // show the file paths
    StringCchPrintfW(sz, _countof(sz), L"%s\n\n%s", m_szFile, m_szResourceH);
    MsgBoxDx(sz, MB_ICONINFORMATION);

    // get the selected entry
    auto entry = g_res.get_entry();
    if (!entry)
        return;

    static const LPCWSTR apszI_[] =
    {
        L"ET_ANY",
        L"ET_TYPE",
        L"ET_STRING",
        L"ET_MESSAGE",
        L"ET_NAME",
        L"ET_LANG"
    };

    MStringW type = entry->m_type.str();
    MStringW name = entry->m_name.str();
    StringCchPrintfW(sz, _countof(sz),
        L"%s: type:%s, name:%s, lang:0x%04X, entry:%p, hItem:%p, strLabel:%s", apszI_[entry->m_et],
        type.c_str(), name.c_str(), entry->m_lang, entry, entry->m_hItem, entry->m_strLabel.c_str());
    MsgBoxDx(sz, MB_ICONINFORMATION);
}

// show the movie or not
void MMainWnd::ShowMovie(BOOL bShow/* = TRUE*/)
{
    if (bShow)
    {
        // show the movie
        ShowWindow(m_hBmpView, SW_SHOWNOACTIVATE);
        ShowWindow(m_hSrcEdit, SW_HIDE);
        m_splitter3.SetPaneCount(1);
        m_splitter3.SetPane(0, m_hBmpView);
    }
    else
    {
        // hide the movie
        ShowBmpView(FALSE);
    }
}

// show the image file or not
void MMainWnd::ShowBmpView(BOOL bShow/* = TRUE*/)
{
    ShowWindow(m_hSrcEdit, SW_SHOWNOACTIVATE);
    if (bShow)
    {
        // show m_hBmpView
        ShowWindow(m_hBmpView, SW_SHOWNOACTIVATE);
        m_splitter3.SetPaneCount(2);
        m_splitter3.SetPane(0, m_hSrcEdit);
        m_splitter3.SetPane(1, m_hBmpView);

        // resume the width
        m_splitter3.SetPaneExtent(1, g_settings.nBmpViewWidth);
    }
    else
    {
        if (m_splitter3.GetPaneCount() >= 2)
        {
            // remember the m_hBmpView width
            g_settings.nBmpViewWidth = m_splitter3.GetPaneExtent(1);
        }

        // hide m_hBmpView
        ShowWindow(m_hBmpView, SW_HIDE);
        m_splitter3.SetPaneCount(1);
        m_splitter3.SetPane(0, m_hSrcEdit);
    }

    // update m_hBmpView's scroll info
    SendMessageW(m_hBmpView, WM_COMMAND, 999, 0);
}

// show the status bar or not
void MMainWnd::ShowStatusBar(BOOL bShow/* = TRUE*/)
{
    if (bShow)
        ShowWindow(m_hStatusBar, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hStatusBar, SW_HIDE);
}

// show the binary/error EDIT control or not
void MMainWnd::ShowBinEdit(BOOL bShow/* = TRUE*/, BOOL bShowError/* = FALSE*/)
{
    if (bShow && (g_settings.bShowBinEdit || bShowError))
    {
        // show the binary EDIT control
        ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);
        m_splitter2.SetPaneCount(2);
        m_splitter2.SetPane(0, m_splitter3);
        m_splitter2.SetPane(1, m_hBinEdit);

        // resume the height
        m_splitter2.SetPaneExtent(1, g_settings.nBinEditHeight);
    }
    else
    {
        if (m_splitter2.GetPaneCount() >= 2)
        {
            // remember the binary EDIT control's height
            g_settings.nBinEditHeight = m_splitter2.GetPaneExtent(1);
        }

        // hide the binary EDIT control
        ShowWindow(m_hBinEdit, SW_HIDE);
        m_splitter2.SetPaneCount(1);
        m_splitter2.SetPane(0, m_splitter3);
    }
}

// WM_MOVE: the main window has moved
void MMainWnd::OnMove(HWND hwnd, int x, int y)
{
    // is the window maximized or minimized?
    if (!IsZoomed(hwnd) && !IsIconic(hwnd))
    {
        // if so, then remember the position
        RECT rc;
        GetWindowRect(hwnd, &rc);
        g_settings.nWindowLeft = rc.left;
        g_settings.nWindowTop = rc.top;
    }
}

// WM_SIZE: the main window has changed in size
void MMainWnd::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    // auto move and resize the toolbar
    SendMessageW(m_hToolBar, TB_AUTOSIZE, 0, 0);

    // auto move and resize the status bar
    SendMessageW(m_hStatusBar, WM_SIZE, 0, 0);

    RECT rc, ClientRect;

    // is the main window maximized?
    if (IsZoomed(hwnd))
    {
        g_settings.bMaximized = TRUE;
    }
    else if (IsIconic(hwnd))   // is it minimized?
    {
        ;
    }
    else
    {
        // not maximized nor minimized
        // remember the size
        GetWindowRect(hwnd, &rc);
        g_settings.nWindowWidth = rc.right - rc.left;
        g_settings.nWindowHeight = rc.bottom - rc.top;
        g_settings.bMaximized = FALSE;
    }

    // get the client rectangle from the main window
    GetClientRect(hwnd, &ClientRect);
    SIZE sizClient = SizeFromRectDx(&ClientRect);

    // currently, the upper left corner of the client rectangle is (0, 0)
    INT x = 0, y = 0;

    // reserve the toolbar space
    if (::IsWindowVisible(m_hToolBar))
    {
        GetWindowRect(m_hToolBar, &rc);
        y += rc.bottom - rc.top;
        sizClient.cy -= rc.bottom - rc.top;
    }

    // reserve the status bar space
    if (::IsWindowVisible(m_hStatusBar))
    {
        INT anWidths[] = { ClientRect.right - CX_STATUS_PART, -1 };
        SendMessage(m_hStatusBar, SB_SETPARTS, 2, (LPARAM)anWidths);
        GetWindowRect(m_hStatusBar, &rc);
        sizClient.cy -= rc.bottom - rc.top;
    }

    // notify the size change to m_splitter1
    MoveWindow(m_splitter1, x, y, sizClient.cx, sizClient.cy, TRUE);
}

// WM_INITMENU: update the menus
void MMainWnd::OnInitMenu(HWND hwnd, HMENU hMenu)
{
    if (g_settings.bWordWrap)
        CheckMenuItem(hMenu, ID_WORD_WRAP, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_WORD_WRAP, MF_BYCOMMAND | MF_UNCHECKED);

    // search the language entries
    EntrySetBase found;
    g_res.search(found, ET_LANG);

    if (found.empty())
        EnableMenuItem(hMenu, ID_ITEMSEARCH, MF_GRAYED);
    else
        EnableMenuItem(hMenu, ID_ITEMSEARCH, MF_ENABLED);

    if (g_settings.bShowToolBar)
        CheckMenuItem(hMenu, ID_SHOWHIDETOOLBAR, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_SHOWHIDETOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);

    if (g_settings.bUseBeginEnd)
        CheckMenuItem(hMenu, ID_USEBEGINEND, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_USEBEGINEND, MF_BYCOMMAND | MF_UNCHECKED);

    BOOL bCanEditLabel = TRUE;

    // get the selected entry
    auto entry = g_res.get_entry();
    if (!entry || entry->m_et == ET_TYPE)
    {
        bCanEditLabel = FALSE;
    }

    if (bCanEditLabel)
    {
        if (entry)
        {
            if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
            {
                if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
                {
                    bCanEditLabel = FALSE;
                }
            }
        }
        else
        {
            bCanEditLabel = FALSE;
        }
    }

    if (bCanEditLabel)
        EnableMenuItem(hMenu, ID_EDITLABEL, MF_BYCOMMAND | MF_ENABLED);
    else
        EnableMenuItem(hMenu, ID_EDITLABEL, MF_BYCOMMAND | MF_GRAYED);

    if (IsWindowVisible(m_hStatusBar))
        CheckMenuItem(hMenu, ID_STATUSBAR, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_STATUSBAR, MF_UNCHECKED);

    if (IsWindowVisible(m_hBinEdit))
        CheckMenuItem(hMenu, ID_BINARYPANE, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_BINARYPANE, MF_UNCHECKED);

    if (g_settings.bAlwaysControl)
        CheckMenuItem(hMenu, ID_ALWAYSCONTROL, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_ALWAYSCONTROL, MF_UNCHECKED);

    if (g_settings.bHideID)
        CheckMenuItem(hMenu, ID_HIDEIDMACROS, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_HIDEIDMACROS, MF_UNCHECKED);

    if (g_settings.bUseIDC_STATIC)
        CheckMenuItem(hMenu, ID_USEIDC_STATIC, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_USEIDC_STATIC, MF_UNCHECKED);

    if (GetWindowTextLength(m_hSrcEdit) == 0 ||
        !IsWindowVisible(m_hSrcEdit))
    {
        EnableMenuItem(hMenu, ID_FIND, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hMenu, ID_FIND, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_ENABLED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_ENABLED);
    }

    if (!entry || !entry->m_hItem)
    {
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EDIT, MF_GRAYED);
        EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        return;
    }

    BOOL bEditable = entry && entry->is_editable();
    if (bEditable)
    {
        EnableMenuItem(hMenu, ID_EDIT, MF_ENABLED);
        if (entry->can_gui_edit())
        {
            EnableMenuItem(hMenu, ID_GUIEDIT, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
        }
    }
    else
    {
        EnableMenuItem(hMenu, ID_EDIT, MF_GRAYED);
        EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
    }

    switch (entry ? entry->m_et : ET_ANY)
    {
    case ET_TYPE:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FIND, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_GRAYED);
        break;
    case ET_NAME:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_ENABLED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FIND, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_GRAYED);
        break;

    case ET_LANG:
        EnableMenuItem(hMenu, ID_FIND, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_ENABLED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_ENABLED);
        if (entry->m_type == RT_GROUP_ICON || entry->m_type == RT_ICON ||
            entry->m_type == RT_ANIICON)
        {
            EnableMenuItem(hMenu, ID_EXTRACTICON, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        }
        if (entry->m_type == RT_GROUP_ICON || entry->m_type == RT_ANIICON)
        {
            EnableMenuItem(hMenu, ID_REPLACEICON, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        }

        if (entry->m_type == RT_BITMAP)
        {
            EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_ENABLED);
            EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        }

        if (entry->m_type == RT_GROUP_CURSOR || entry->m_type == RT_CURSOR ||
            entry->m_type == RT_ANICURSOR)
        {
            EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        }
        if (entry->m_type == RT_GROUP_CURSOR || entry->m_type == RT_ANICURSOR)
        {
            EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        }

        if (entry->is_testable())
        {
            EnableMenuItem(hMenu, ID_TEST, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        }

        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
            EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        else
            EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_ENABLED);
        break;

    case ET_STRING: case ET_MESSAGE:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FIND, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_ENABLED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_ENABLED);
        break;

    default:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FIND, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_GRAYED);
        break;
    }
}

// WM_CONTEXTMENU: the context menu is shown
void MMainWnd::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    if (hwndContext != m_hwndTV)
        return;     // ignore

    // if RADical window is displayed
    if (IsWindowVisible(m_rad_window))
    {
        // destroy it
        DestroyWindow(m_rad_window);
    }

    // get screen coordinates from xPos and yPos
    POINT pt = {(INT)xPos, (INT)yPos};
    HTREEITEM hItem;
    if (xPos == -1 && yPos == -1)
    {
        // context menu from keyboard
        hItem = TreeView_GetSelection(hwndContext);

        RECT rc;
        TreeView_GetItemRect(hwndContext, hItem, &rc, FALSE);
        pt.x = (rc.left + rc.right) / 2;
        pt.y = (rc.top + rc.bottom) / 2;
    }
    else
    {
        // context menu from mouse
        ScreenToClient(hwndContext, &pt);

        TV_HITTESTINFO HitTest;
        ZeroMemory(&HitTest, sizeof(HitTest));
        HitTest.pt = pt;
        TreeView_HitTest(hwndContext, &HitTest);

        hItem = HitTest.hItem;
    }

    // select the item
    TreeView_SelectItem(hwndContext, hItem);

    // load the menu
    HMENU hMenu = LoadMenuW(m_hInst, MAKEINTRESOURCEW(IDR_POPUPMENUS));
    OnInitMenu(hwnd, hMenu);
    HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
    if (hMenu == NULL || hSubMenu == NULL)
        return;

    // convert the client coordinates to the screen coordinates
    ClientToScreen(hwndContext, &pt);

    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
    SetForegroundWindow(hwndContext);

    UINT Flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
    INT id = TrackPopupMenu(hSubMenu, Flags, pt.x, pt.y, 0, hwndContext, NULL);

    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
    PostMessageW(hwndContext, WM_NULL, 0, 0);

    // destroy the menu
    DestroyMenu(hMenu);

    if (id)
    {
        // execute the command
        SendMessageW(hwnd, WM_COMMAND, id, 0);
    }
}

// preview the icon resource
void MMainWnd::PreviewIcon(HWND hwnd, const EntryBase& entry)
{
    // create a bitmap object from the entry and set it to m_hBmpView
    BITMAP bm;
    m_hBmpView.SetBitmap(CreateBitmapFromIconOrPngDx(hwnd, entry, bm));

    // create the icon
    MStringW str;
    HICON hIcon = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, TRUE);

    // dump info to m_hSrcEdit
    if (hIcon)
    {
        str = DumpIconInfo(bm, TRUE);
    }
    else
    {
        str = DumpBitmapInfo(m_hBmpView.m_hBitmap);
    }
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // destroy the icon
    DestroyIcon(hIcon);

    // show m_hBmpView
    ShowBmpView(TRUE);
}

// preview the cursor resource
void MMainWnd::PreviewCursor(HWND hwnd, const EntryBase& entry)
{
    // create a cursor object from the entry and set it to m_hBmpView
    BITMAP bm;
    HCURSOR hCursor = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, FALSE);
    m_hBmpView.SetBitmap(CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE));

    // dump info to m_hSrcEdit
    MStringW str = DumpIconInfo(bm, FALSE);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // destroy the cursor
    DestroyCursor(hCursor);

    // show m_hBmpView
    ShowBmpView(TRUE);
}

// preview the group icon resource
void MMainWnd::PreviewGroupIcon(HWND hwnd, const EntryBase& entry)
{
    // create a bitmap object from the entry and set it to m_hBmpView
    m_hBmpView.SetBitmap(CreateBitmapFromIconsDx(hwnd, entry));

    // dump the text to m_hSrcEdit
    ResToText res2text;
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // show m_hBmpView
    ShowBmpView(TRUE);
}

// preview the group cursor resource
void MMainWnd::PreviewGroupCursor(HWND hwnd, const EntryBase& entry)
{
    // create a bitmap object from the entry and set it to m_hBmpView
    m_hBmpView.SetBitmap(CreateBitmapFromCursorsDx(hwnd, entry));
    assert(m_hBmpView);

    // dump the text to m_hSrcEdit
    ResToText res2text;
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // show m_hBmpView
    ShowBmpView(TRUE);
}

// preview the bitmap resource
void MMainWnd::PreviewBitmap(HWND hwnd, const EntryBase& entry)
{
    // create a bitmap object from the entry and set it to m_hBmpView
    HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&entry[0], entry.size());
    m_hBmpView.SetBitmap(hbm);

    // dump the text to m_hSrcEdit
    ResToText res2text;
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // show m_hBmpView
    ShowBmpView(TRUE);
}

// preview the image resource
void MMainWnd::PreviewImage(HWND hwnd, const EntryBase& entry)
{
    // dump the text to m_hSrcEdit
    ResToText res2text;
    MStringW str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // set the entry image to m_hBmpView
    m_hBmpView.SetImage(&entry[0], entry.size());

    // show m_hBmpView
    ShowBmpView(TRUE);
}

// preview the WAVE resource
void MMainWnd::PreviewWAVE(HWND hwnd, const EntryBase& entry)
{
    // dump the text to m_hSrcEdit
    ResToText res2text;
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // make it playable
    m_hBmpView.SetPlay();

    // show m_hBmpView
    ShowBmpView(TRUE);
}

// preview the AVI resource
void MMainWnd::PreviewAVI(HWND hwnd, const EntryBase& entry)
{
    // dump the text to m_hSrcEdit
    ResToText res2text;
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // set the AVI
    m_hBmpView.SetMedia(&entry[0], entry.size());

    // show m_hBmpView
    ShowMovie(TRUE);
}

// preview the RT_ACCELERATOR resource
void MMainWnd::PreviewAccel(HWND hwnd, const EntryBase& entry)
{
    // entry.m_data --> stream --> accel
    AccelRes accel;
    MByteStreamEx stream(entry.m_data);
    if (accel.LoadFromStream(stream))
    {
        // dump the text to m_hSrcEdit
        MString str = GetLanguageStatement(entry.m_lang);
        str += accel.Dump(entry.m_name);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

// preview the message table resource
void MMainWnd::PreviewMessage(HWND hwnd, const EntryBase& entry)
{
    // entry.m_data --> stream --> mes
    MessageRes mes;
    MByteStreamEx stream(entry.m_data);
    WORD nNameID = entry.m_name.m_id;
    if (mes.LoadFromStream(stream, nNameID))
    {
        // dump the text to m_hSrcEdit
        MStringW str = mes.Dump(nNameID);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

// preview the string resource
void MMainWnd::PreviewString(HWND hwnd, const EntryBase& entry)
{
    // entry.m_data --> stream --> str_res
    StringRes str_res;
    MByteStreamEx stream(entry.m_data);
    WORD nNameID = entry.m_name.m_id;
    if (str_res.LoadFromStream(stream, nNameID))
    {
        // dump the text to m_hSrcEdit
        MStringW str = str_res.Dump(nNameID);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

// preview the HTML resource
void MMainWnd::PreviewHtml(HWND hwnd, const EntryBase& entry)
{
    // load a text file
    MTextType type;
    type.nNewLine = MNEWLINE_CRLF;
    MStringW str;
    if (entry.size())
        str = mstr_from_bin(&entry.m_data[0], entry.m_data.size(), &type);

    // dump the text to m_hSrcEdit
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

// preview the menu resource
void MMainWnd::PreviewMenu(HWND hwnd, const EntryBase& entry)
{
    // entry.m_data --> stream --> menu_res
    MenuRes menu_res;
    MByteStreamEx stream(entry.m_data);
    if (menu_res.LoadFromStream(stream))
    {
        // dump the text to m_hSrcEdit
        MString str = GetLanguageStatement(entry.m_lang);
        str += menu_res.Dump(entry.m_name);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

// preview the version resource
void MMainWnd::PreviewVersion(HWND hwnd, const EntryBase& entry)
{
    // entry.m_data --> ver_res
    VersionRes ver_res;
    if (ver_res.LoadFromData(entry.m_data))
    {
        // dump the text to m_hSrcEdit
        MString str = GetLanguageStatement(entry.m_lang);
        str += ver_res.Dump(entry.m_name);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

// preview the unknown resource
void MMainWnd::PreviewUnknown(HWND hwnd, const EntryBase& entry)
{
    // dump the text to m_hSrcEdit
    ResToText res2text;
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

// preview the RT_RCDATA resource
void MMainWnd::PreviewRCData(HWND hwnd, const EntryBase& entry)
{
    // dump the text to m_hSrcEdit
    ResToText res2text;
    res2text.m_hwnd = m_hwnd;
    res2text.m_bHumanReadable = TRUE;
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

// preview the DLGINIT resource
void MMainWnd::PreviewDlgInit(HWND hwnd, const EntryBase& entry)
{
    // dump the text to m_hSrcEdit
    ResToText res2text;
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

// preview the dialog template resource
void MMainWnd::PreviewDialog(HWND hwnd, const EntryBase& entry)
{
    // entry.m_data --> stream --> dialog_res
    DialogRes dialog_res;
    MByteStreamEx stream(entry.m_data);
    if (dialog_res.LoadFromStream(stream))
    {
        // dump the text to m_hSrcEdit
        MString str = GetLanguageStatement(entry.m_lang);
        str += dialog_res.Dump(entry.m_name, !!g_settings.bAlwaysControl);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

// preview the animation icon resource
void MMainWnd::PreviewAniIcon(HWND hwnd, const EntryBase& entry, BOOL bIcon)
{
    HICON hIcon = NULL;

    {
        WCHAR szPath[MAX_PATH], szTempFile[MAX_PATH];
        GetTempPathW(_countof(szPath), szPath);
        GetTempFileNameW(szPath, L"ani", 0, szTempFile);

        MFile file;
        DWORD cbWritten = 0;
        if (file.OpenFileForOutput(szTempFile) &&
            file.WriteFile(&entry[0], entry.size(), &cbWritten))
        {
            file.FlushFileBuffers();    // flush
            file.CloseHandle();         // close the handle
            Sleep(FILE_WAIT_TIME);      // wait for the file operation

            if (bIcon)
            {
                hIcon = (HICON)LoadImage(NULL, szTempFile, IMAGE_ICON,
                    0, 0, LR_LOADFROMFILE);
            }
            else
            {
                hIcon = (HICON)LoadImage(NULL, szTempFile, IMAGE_CURSOR,
                    0, 0, LR_LOADFROMFILE);
            }
        }
        DeleteFileW(szTempFile);
    }

    if (hIcon)
    {
        m_hBmpView.SetIcon(hIcon, bIcon);

        ResToText res2text;
        MString str = res2text.DumpEntry(entry);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
    else
    {
        m_hBmpView.DestroyView();
    }
    ShowBmpView(TRUE);
}

// preview the string table resource
void MMainWnd::PreviewStringTable(HWND hwnd, const EntryBase& entry)
{
    // search the strings
    EntrySetBase found;
    g_res.search(found, ET_LANG, RT_STRING, (WORD)0, entry.m_lang);

    // found --> str_res
    StringRes str_res;
    for (auto e : found)
    {
        MByteStreamEx stream(e->m_data);
        if (!str_res.LoadFromStream(stream, e->m_name.m_id))
            return;
    }

    // dump the text to m_hSrcEdit
    MString str = GetLanguageStatement(entry.m_lang);
    str += str_res.Dump();
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

// preview the message table resource
void MMainWnd::PreviewMessageTable(HWND hwnd, const EntryBase& entry)
{
    // search the message tables
    EntrySetBase found;
    g_res.search(found, ET_LANG, RT_MESSAGETABLE, (WORD)0, entry.m_lang);

    // found --> msg_res
    MessageRes msg_res;
    for (auto e : found)
    {
        MByteStreamEx stream(e->m_data);
        if (!msg_res.LoadFromStream(stream, e->m_name.m_id))
            return;
    }

    // dump the text to m_hSrcEdit
    MString str;
    str += GetLanguageStatement(entry.m_lang);
    str += L"#ifdef APSTUDIO_INVOKED\r\n";
    str += L"    #error Ap Studio cannot edit this message table.\r\n";
    str += L"#endif\r\n";
    str += L"#ifdef MCDX_INVOKED\r\n";
    str += msg_res.Dump();
    str += L"#endif\r\n\r\n";
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

// close the preview
VOID MMainWnd::HidePreview()
{
    // destroy the RADical window if any
    if (IsWindow(m_rad_window))
    {
        DestroyWindow(m_rad_window);
    }

    // clear m_hBinEdit
    SetWindowTextW(m_hBinEdit, NULL);
    Edit_SetModify(m_hBinEdit, FALSE);

    // clear m_hSrcEdit
    SetWindowTextW(m_hSrcEdit, NULL);
    Edit_SetModify(m_hSrcEdit, FALSE);

    // close and hide m_hBmpView
    m_hBmpView.DestroyView();
    ShowBmpView(FALSE);

    // recalculate the splitter
    PostMessageDx(WM_SIZE);
}

// do preview the resource item
BOOL MMainWnd::Preview(HWND hwnd, const EntryBase *entry)
{
    // close the preview
    HidePreview();

    // show the binary
    MStringW str = DumpBinaryAsText(entry->m_data);
    SetWindowTextW(m_hBinEdit, str.c_str());

    // do preview the resource item
    if (entry->m_type.m_id != 0)
    {
        WORD wType = entry->m_type.m_id;
        if (wType == (WORD)(UINT_PTR)RT_ICON)
        {
            PreviewIcon(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_CURSOR)
        {
            PreviewCursor(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_GROUP_ICON)
        {
            PreviewGroupIcon(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_GROUP_CURSOR)
        {
            PreviewGroupCursor(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_BITMAP)
        {
            PreviewBitmap(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_ACCELERATOR)
        {
            PreviewAccel(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_STRING)
        {
            PreviewString(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_MENU)
        {
            PreviewMenu(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_DIALOG)
        {
            PreviewDialog(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_ANIICON)
        {
            PreviewAniIcon(hwnd, *entry, TRUE);
        }
        else if (wType == (WORD)(UINT_PTR)RT_ANICURSOR)
        {
            PreviewAniIcon(hwnd, *entry, FALSE);
        }
        else if (wType == (WORD)(UINT_PTR)RT_MESSAGETABLE)
        {
            PreviewMessage(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_MANIFEST || wType == (WORD)(UINT_PTR)RT_HTML)
        {
            PreviewHtml(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_VERSION)
        {
            PreviewVersion(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_RCDATA)
        {
            PreviewRCData(hwnd, *entry);
        }
        else if (wType == (WORD)(UINT_PTR)RT_DLGINIT)
        {
            PreviewDlgInit(hwnd, *entry);
        }
        else
        {
            PreviewUnknown(hwnd, *entry);
        }
    }
    else
    {
        if (entry->m_type == L"PNG" || entry->m_type == L"GIF" ||
            entry->m_type == L"JPEG" || entry->m_type == L"TIFF" ||
            entry->m_type == L"JPG" || entry->m_type == L"TIF" ||
            entry->m_type == L"EMF" || entry->m_type == L"ENHMETAFILE" ||
            entry->m_type == L"WMF" || entry->m_type == L"IMAGE")
        {
            PreviewImage(hwnd, *entry);
        }
        else if (entry->m_type == L"WAVE")
        {
            PreviewWAVE(hwnd, *entry);
        }
        else if (entry->m_type == L"AVI")
        {
            PreviewAVI(hwnd, *entry);
        }
        else
        {
            PreviewUnknown(hwnd, *entry);
        }
    }

    // recalculate the splitter
    PostMessageDx(WM_SIZE);

    return IsEntryTextEditable(entry);
}

// create the toolbar
BOOL MMainWnd::CreateOurToolBar(HWND hwndParent, HIMAGELIST himlTools)
{
    // create a toolbar
    HWND hwndTB = CreateWindowW(TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_LIST | TBSTYLE_TOOLTIPS,
        0, 0, 0, 0, hwndParent, (HMENU)1, GetWindowInstance(hwndParent), NULL);
    if (hwndTB == NULL)
        return FALSE;

    // initialize
    SendMessageW(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(0, 0));
    SendMessageW(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himlTools);
    SendMessageW(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    ToolBar_StoreStrings(hwndTB, _countof(g_buttons0), g_buttons0);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons1), g_buttons1);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons2), g_buttons2);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons3), g_buttons3);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons4), g_buttons4);

    m_hToolBar = hwndTB;
    UpdateOurToolBarButtons(3);

    return TRUE;
}

void MMainWnd::UpdateToolBarStatus()
{
    if (TreeView_GetCount(m_hwndTV) == 0)
    {
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPAND_ALL, 0);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_COLLAPSE_ALL, 0);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXTRACTBANG, 0);
    }
    else
    {
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPAND_ALL, TBSTATE_ENABLED);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_COLLAPSE_ALL, TBSTATE_ENABLED);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXTRACTBANG, TBSTATE_ENABLED);
    }

    BOOL bCanEditLabel = TRUE;

    // get the selected entry
    auto entry = g_res.get_entry();
    if (entry && !entry->valid())
        entry = NULL;
    if (!entry || entry->m_et == ET_TYPE)
    {
        bCanEditLabel = FALSE;
    }

    if (bCanEditLabel)
    {
        if (entry)
        {
            if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
            {
                if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
                {
                    bCanEditLabel = FALSE;
                }
            }
        }
        else
        {
            bCanEditLabel = FALSE;
        }
    }

    if (bCanEditLabel)
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_EDITLABEL, TBSTATE_ENABLED);
    else
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_EDITLABEL, 0);

    if (!entry || !entry->m_hItem)
    {
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, 0);
        return;
    }

    BOOL bEditable = entry && entry->is_editable();
    if (bEditable)
    {
        if (entry->can_gui_edit())
        {
            SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, TBSTATE_ENABLED);
        }
        else
        {
            SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
        }
    }
    else
    {
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
    }

    switch (entry ? entry->m_et : ET_ANY)
    {
    case ET_TYPE:
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
        break;

    case ET_NAME:
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
        break;

    case ET_LANG:
        if (entry->is_testable())
        {
            SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, TBSTATE_ENABLED);
        }
        else
        {
            SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
        }

        SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
        if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
            SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
        else
            SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
        break;

    case ET_STRING: case ET_MESSAGE:
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
        break;

    default:
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, 0);
        SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
        break;
    }
}

// update the toolbar buttons
void MMainWnd::UpdateOurToolBarButtons(INT iType)
{
    // delete all the buttons of toolbar
    while (SendMessageW(m_hToolBar, TB_DELETEBUTTON, 0, 0))
        ;

    // add the buttons
    switch (iType)
    {
    case 0:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons0), (LPARAM)g_buttons0);
        break;
    case 1:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons1), (LPARAM)g_buttons1);
        break;
    case 2:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons2), (LPARAM)g_buttons2);
        break;
    case 3:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons3), (LPARAM)g_buttons3);
        break;
    case 4:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons4), (LPARAM)g_buttons4);
        break;
    }

    UpdateToolBarStatus();

    // show/hide the toolbar by settings
    if (g_settings.bShowToolBar)
        ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hToolBar, SW_HIDE);
}

// select an item in the tree control
void
MMainWnd::SelectTV(EntryType et, const MIdOrString& type,
                   const MIdOrString& name, WORD lang, BOOL bDoubleClick)
{
    // close the preview
    HidePreview();

    // find the entry
    if (auto entry = g_res.find(et, type, name, lang))
    {
        // select it
        SelectTV(entry, bDoubleClick);
    }
}

// select an item in the tree control
void MMainWnd::SelectTV(EntryBase *entry, BOOL bDoubleClick)
{
    // close the preview
    HidePreview();

    if (!entry)     // not selected
    {
        UpdateOurToolBarButtons(3);
        return;
    }

    // expand the parent and ensure visible
    auto parent = g_res.get_parent(entry);
    while (parent && parent->m_hItem)
    {
        TreeView_Expand(m_hwndTV, parent->m_hItem, TVE_EXPAND);
        parent = g_res.get_parent(parent);
    }
    TreeView_SelectItem(m_hwndTV, entry->m_hItem);
    TreeView_EnsureVisible(m_hwndTV, entry->m_hItem);

    m_type = entry->m_type;
    m_name = entry->m_name;
    m_lang = entry->m_lang;

    BOOL bEditable, bSelectNone = FALSE;
    switch (entry->m_et)
    {
    case ET_LANG:
        // do preview
        bEditable = Preview(m_hwnd, entry);
        // show the binary EDIT control
        ShowBinEdit(TRUE);
        break;

    case ET_STRING:
        // clean up m_hBmpView
        m_hBmpView.DestroyView();
        m_hBmpView.DeleteTempFile();

        // show the string table
        PreviewStringTable(m_hwnd, *entry);

        // hide the binary EDIT control
        SetWindowTextW(m_hBinEdit, NULL);
        ShowBinEdit(FALSE);

        // it's editable
        bEditable = TRUE;
        break;

    case ET_MESSAGE:
        // clean up m_hBmpView
        m_hBmpView.DestroyView();
        m_hBmpView.DeleteTempFile();

        // show the message table
        PreviewMessageTable(m_hwnd, *entry);

        // hide the binary EDIT control
        SetWindowTextW(m_hBinEdit, NULL);
        ShowBinEdit(FALSE);

        // it's editable
        bEditable = TRUE;
        break;

    default:
        // otherwise
        // clean up m_hBmpView
        m_hBmpView.DestroyView();
        m_hBmpView.DeleteTempFile();

        // hide the binary EDIT control
        SetWindowTextW(m_hBinEdit, NULL);
        ShowBinEdit(FALSE);

        // it's non editable
        bEditable = FALSE;

        // select none
        bSelectNone = TRUE;
    }

    if (bEditable)  // editable
    {
        // make it not read-only
        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        // update the toolbar
        if (Edit_GetModify(m_hSrcEdit))
        {
            UpdateOurToolBarButtons(2);
        }
        else if (entry->is_testable())
        {
            UpdateOurToolBarButtons(0);
        }
        else if (entry->can_gui_edit())
        {
            UpdateOurToolBarButtons(4);
        }
        else
        {
            UpdateOurToolBarButtons(3);
        }
    }
    else
    {
        // make it read-only
        Edit_SetReadOnly(m_hSrcEdit, TRUE);

        // update the toolbar
        UpdateOurToolBarButtons(3);
    }

    // recalculate the splitter
    PostMessageDx(WM_SIZE);
}

// dump all the macros
MStringW MMainWnd::GetMacroDump() const
{
    MStringW ret;
    for (auto& macro : g_settings.macros)   // for each predefined macros
    {
        // " -Dmacro=contents"
        ret += L" -D";
        ret += macro.first;
        if (macro.second.size())
        {
            ret += L"=";
            ret += macro.second;
        }
    }
    ret += L" ";
    return ret;
}

// dump all the #include's
MStringW MMainWnd::GetIncludesDump() const
{
    MStringW ret;
    for (auto& path : g_settings.includes)
    {
        // " -Ipath"
        auto& str = path;
        if (str.empty())
            continue;

        ret += L" -I";
        ret += str;
    }
    ret += L" ";
    return ret;
}

// compile the string table
BOOL MMainWnd::CompileStringTable(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide)
{
    // convert strWide to UTF-8
    MStringA strUtf8 = MWideToAnsi(CP_UTF8, strWide).c_str();

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

    // Source file #1
    StringCchCopyW(szPath1, MAX_PATH, GetTempFileNameDx(L"R1"));
    MFile r1(szPath1, TRUE);

    // Source file #2 (#included)
    StringCchCopyW(szPath2, MAX_PATH, GetTempFileNameDx(L"R2"));
    MFile r2(szPath2, TRUE);

    // Output resource object file (imported)
    StringCchCopyW(szPath3, MAX_PATH, GetTempFileNameDx(L"R3"));
    MFile r3(szPath3, TRUE);    // create
    r3.CloseHandle();   // close the handle

    // dump the head to Source file #1
    if (m_szResourceH[0])
        r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, m_szResourceH).c_str());
    r1.WriteFormatA("#include <windows.h>\r\n");
    r1.WriteFormatA("#include <commctrl.h>\r\n");
    r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n", PRIMARYLANGID(lang), SUBLANGID(lang));
    r1.WriteFormatA("#pragma code_page(65001) // UTF-8\r\n");

    // dump the macros
    for (auto& pair : g_settings.id_map)
    {
        if (pair.first == "IDC_STATIC")
        {
            r1.WriteFormatA("#undef IDC_STATIC\r\n");
            r1.WriteFormatA("#define IDC_STATIC -1\r\n");
        }
        else
        {
            r1.WriteFormatA("#undef %s\r\n", pair.first.c_str());
            r1.WriteFormatA("#define %s %s\r\n", pair.first.c_str(), pair.second.c_str());
        }
    }

    r1.WriteFormatA("#include \"%S\"\r\n", szPath2);
    r1.CloseHandle();   // close the handle

    // write the UTF-8 file to Source file #2
    DWORD cbWrite = DWORD(strUtf8.size() * sizeof(char));
    DWORD cbWritten;
    r2.WriteFile(strUtf8.c_str(), cbWrite, &cbWritten);
    r2.CloseHandle();   // close the handle

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szWindresExe;
    strCmdLine += L"\" --use-temp-file -DRC_INVOKED ";
    strCmdLine += GetMacroDump();
    strCmdLine += GetIncludesDump();
    strCmdLine += L" -I \"";
    strCmdLine += m_szIncludeDir;
    strCmdLine += L"\" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res -F pe-i386 --preprocessor=\"";
    strCmdLine += m_szMCppExe;
    strCmdLine += L"\" --preprocessor-arg=\"\" \"";
    strCmdLine += szPath1;
    strCmdLine += '\"';
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    // wait for the file operation
    Sleep(FILE_WAIT_TIME);

    // create a windres.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
        pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

        if (pmaker.GetExitCode() == 0 && bOK)
        {
            bOK = FALSE;

            // wait for the file operation
            Sleep(FILE_WAIT_TIME);

            // import res
            EntrySet res;
            if (res.import_res(szPath3))
            {
                // resource type check
                bOK = TRUE;
                for (auto entry : res)
                {
                    if (entry->m_type != RT_STRING)
                    {
                        ErrorBoxDx(IDS_RESMISMATCH);
                        bOK = FALSE;
                        break;
                    }
                }

                if (bOK)
                {
                    // merge
                    g_res.search_and_delete(ET_LANG, RT_STRING, (WORD)0, lang);
                    g_res.merge(res);
                }
            }

            // clean res up
            res.delete_all();
        }
        else
        {
            bOK = FALSE;
            // error message
            strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR));
        }
    }
    else
    {
        // error message
        strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
    }

#ifdef NDEBUG
    DeleteFileW(szPath1);
    DeleteFileW(szPath2);
    DeleteFileW(szPath3);
#endif

    // recalculate the splitter
    PostMessageDx(WM_SIZE);

    if (bOK)
        DoSetFileModified(TRUE);
    return bOK;
}

BOOL MMainWnd::CompileRCData(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide)
{
    EntryBase *entry = g_res.find(ET_LANG, RT_RCDATA, name, lang);
    if (!entry || !entry->is_delphi_dfm())
        return FALSE;

    MWideToAnsi w2a(CP_UTF8, strWide.c_str());
    EntryBase::data_type data = dfm_binary_from_text(m_szDFMSC, w2a.c_str());
    if (data.empty())
        return FALSE;

    entry->m_data = data;
    DoSetFileModified(TRUE);
    return TRUE;
}

// compile the message table
BOOL MMainWnd::CompileMessageTable(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide)
{
    // convert strWide to UTF-8
    MStringA strUtf8 = MWideToAnsi(CP_UTF8, strWide).c_str();

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

    // Source file #1
    StringCchCopyW(szPath1, MAX_PATH, GetTempFileNameDx(L"R1"));
    MFile r1(szPath1, TRUE);

    // Source file #2 (#included)
    StringCchCopyW(szPath2, MAX_PATH, GetTempFileNameDx(L"R2"));
    MFile r2(szPath2, TRUE);

    // Output resource object file (imported)
    StringCchCopyW(szPath3, MAX_PATH, GetTempFileNameDx(L"R3"));

    MFile r3(szPath3, TRUE);    // create the file
    r3.CloseHandle();   // close the handle

    // dump the head to Source file #1
    if (m_szResourceH[0])
        r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, m_szResourceH).c_str());
    r1.WriteFormatA("#include <windows.h>\r\n");
    r1.WriteFormatA("#include <commctrl.h>\r\n");
    r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n", PRIMARYLANGID(lang), SUBLANGID(lang));
    r1.WriteFormatA("#pragma code_page(65001) // UTF-8\r\n");
    r1.WriteFormatA("#include \"%S\"\r\n", szPath2);
    r1.CloseHandle();       // close the handle

    // write the UTF-8 file to Source file #2
    DWORD cbWrite = DWORD(strUtf8.size() * sizeof(char));
    DWORD cbWritten;
    r2.WriteFile(strUtf8.c_str(), cbWrite, &cbWritten);
    r2.FlushFileBuffers();  // flush
    r2.CloseHandle();       // close the handle

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szMcdxExe;
    strCmdLine += L"\" ";
    strCmdLine += GetMacroDump();
    strCmdLine += GetIncludesDump();
    strCmdLine += L" --include-dir=\"";
    strCmdLine += m_szIncludeDir;
    strCmdLine += L"\" --preprocessor=\"";
    strCmdLine += m_szMCppExe;
    strCmdLine += L"\" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res \"";
    strCmdLine += szPath1;
    strCmdLine += L'\"';
    //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    // wait for the file operation
    Sleep(FILE_WAIT_TIME);

    // create the mcdx.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
        pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

        if (pmaker.GetExitCode() == 0 && bOK)
        {
            bOK = FALSE;

            // wait for the file operation
            Sleep(FILE_WAIT_TIME);

            // import res
            EntrySet res;
            if (res.import_res(szPath3))
            {
                // resource type check
                bOK = TRUE;
                for (auto entry : res)
                {
                    if (entry->m_type != RT_MESSAGETABLE)
                    {
                        ErrorBoxDx(IDS_RESMISMATCH);
                        bOK = FALSE;
                        break;
                    }
                }

                if (bOK)
                {
                    // merge
                    g_res.search_and_delete(ET_LANG, RT_MESSAGETABLE, (WORD)0, lang);
                    g_res.merge(res);
                }
            }

            // clean res up
            res.delete_all();
        }
        else
        {
            bOK = FALSE;
            // error message
            strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR));
        }
    }
    else
    {
        // error message
        MStringA msg;
        strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
    }

#ifdef NDEBUG
    DeleteFileW(szPath1);
    DeleteFileW(szPath2);
    DeleteFileW(szPath3);
#endif

    // recalculate the splitter
    PostMessageDx(WM_SIZE);

    if (bOK)
        DoSetFileModified(TRUE);

    return bOK;
}

// compile a resource item source
BOOL MMainWnd::CompileParts(MStringA& strOutput, const MIdOrString& type, const MIdOrString& name,
                            WORD lang, const MStringW& strWide, BOOL bReopen)
{
    if (type == RT_STRING)
    {
        return CompileStringTable(strOutput, name, lang, strWide);
    }
    if (type == RT_MESSAGETABLE)
    {
        return CompileMessageTable(strOutput, name, lang, strWide);
    }
    if (type == RT_RCDATA)
    {
        return CompileRCData(strOutput, name, lang, strWide);
    }

    // add a UTF-8 BOM to data
    static const BYTE bom[] = {0xEF, 0xBB, 0xBF, 0};

    // strWide --> strUtf8 (in UTF-8)
    MStringA strUtf8 = MWideToAnsi(CP_UTF8, strWide).c_str();
    auto enc = GetResTypeEncoding(type);
    BOOL bDataOK = FALSE;
    EntryBase::data_type data;
    if (enc.size())
    {
        bDataOK = TRUE;

        if (enc == L"utf8" || enc == L"utf8n")
        {
            data.assign(strUtf8.begin(), strUtf8.end());

            if (enc != L"utf8n")
            {
                // add a UTF-8 BOM to data
                data.insert(data.begin(), &bom[0], &bom[3]);
            }
        }
        else if (enc == L"ansi")
        {
            // strWide --> data (in ANSI)
            MStringA TextAnsi = MWideToAnsi(CP_ACP, strWide).c_str();
            data.assign(TextAnsi.begin(), TextAnsi.end());
        }
        else if (enc == L"wide")
        {
            data.assign((BYTE *)&strWide[0], (BYTE *)&strWide.c_str()[strWide.size()]);
        }
        else if (enc == L"sjis")
        {
            MStringA TextSjis = MWideToAnsi(932, strWide).c_str();
            data.assign(TextSjis.begin(), TextSjis.end());
        }
        else
        {
            bDataOK = FALSE;
        }
    }
    else if (type == RT_HTML || type == RT_MANIFEST ||
             type == L"RISOHTEMPLATE")
    {
        data.assign(strUtf8.begin(), strUtf8.end());
        data.insert(data.begin(), &bom[0], &bom[3]);

        bDataOK = TRUE;
    }

    if (bDataOK)
    {
        if (data.empty())
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return FALSE;
        }

        // add a language entry
        auto entry = g_res.add_lang_entry(type, name, lang, data);

        // select the added entry
        SelectTV(entry, FALSE);

        DoSetFileModified(TRUE);

        return TRUE;    // success
    }

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

    // Source file #1
    StringCchCopyW(szPath1, MAX_PATH, GetTempFileNameDx(L"R1"));
    MFile r1(szPath1, TRUE);

    // Source file #2 (#included)
    StringCchCopyW(szPath2, MAX_PATH, GetTempFileNameDx(L"R2"));
    MFile r2(szPath2, TRUE);

    // Output resource object file (imported)
    StringCchCopyW(szPath3, MAX_PATH, GetTempFileNameDx(L"R3"));
    MFile r3(szPath3, TRUE);
    r3.CloseHandle();   // close the handle

    // dump the head to Source file #1
    if (m_szResourceH[0])
        r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, m_szResourceH).c_str());
    r1.WriteSzA("#include <windows.h>\r\n");
    r1.WriteSzA("#include <commctrl.h>\r\n");
    r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n", PRIMARYLANGID(lang), SUBLANGID(m_lang));
    r1.WriteSzA("#pragma code_page(65001) // UTF-8\r\n\r\n");
    r1.WriteSzA("#ifndef IDC_STATIC\r\n");
    r1.WriteSzA("    #define IDC_STATIC (-1)\r\n");
    r1.WriteSzA("#endif\r\n\r\n");

    r1.WriteFormatA("#include \"%S\"\r\n", szPath2);
    r1.CloseHandle();   // close the handle

    // write the UTF-8 file to Source file #2
    DWORD cbWrite = DWORD(strUtf8.size() * sizeof(char));
    DWORD cbWritten;
    r2.WriteFile(strUtf8.c_str(), cbWrite, &cbWritten);
    r2.CloseHandle();   // close the handle

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szWindresExe;
    strCmdLine += L"\" -DRC_INVOKED ";
    strCmdLine += GetMacroDump();
    strCmdLine += GetIncludesDump();
    strCmdLine += L" -I \"";
    strCmdLine += m_szIncludeDir;
    strCmdLine += L"\" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res -F pe-i386 --preprocessor=\"";
    strCmdLine += m_szMCppExe;
    strCmdLine += L"\" --preprocessor-arg=\"\" \"";
    strCmdLine += szPath1;
    strCmdLine += '\"';
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    // wait for the file operation
    Sleep(FILE_WAIT_TIME);

    // create a windres.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
        pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

        // check the exit code
        if (pmaker.GetExitCode() == 0 && bOK)
        {
            bOK = FALSE;

            // wait for the file operation
            Sleep(FILE_WAIT_TIME);

            // import res
            EntrySet res;
            if (res.import_res(szPath3))
            {
                bOK = TRUE;
                for (auto entry : res)
                {
                    if (entry->m_et != ET_LANG)
                        continue;

                    // resource type check
                    if (entry->m_type != type)
                    {
                        ErrorBoxDx(IDS_RESMISMATCH);
                        bOK = FALSE;
                        break;
                    }

                    // adjust names and languages
                    entry->m_name = name;
                    entry->m_lang = lang;
                }

                if (bOK)
                {
                    // merge
                    g_res.search_and_delete(ET_LANG, type, name, lang);
                    g_res.merge(res);
                }

                // clean res up
                res.delete_all();
            }
        }
        else
        {
            bOK = FALSE;
            // error message
            strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR));
        }
    }
    else
    {
        // error message
        strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
    }

#ifdef NDEBUG
    DeleteFileW(szPath1);
    DeleteFileW(szPath2);
    DeleteFileW(szPath3);
#endif

    if (bOK)
    {
        if (bReopen && type == RT_DIALOG)
        {
            PostMessageDx(MYWM_REOPENRAD);
        }

        DoSetFileModified(TRUE);
    }

    // recalculate the splitter
    PostMessageDx(WM_SIZE);

    return bOK;
}

// recompile the resource item on selection change.
// reopen if necessary
BOOL MMainWnd::ReCompileOnSelChange(BOOL bReopen/* = FALSE*/)
{
    MStringW strWide = GetWindowTextW(m_hSrcEdit);

    // get the selected entry
    auto entry = g_res.get_entry();
    if (!entry)
        return FALSE;   // no selection

    // compile the entry
    MStringA strOutput;
    if (!CompileParts(strOutput, entry->m_type, entry->m_name, entry->m_lang, strWide))
    {
        SetErrorMessage(strOutput);
        return FALSE;   // failure
    }

    // compiled. clear the modification flag
    Edit_SetModify(m_hSrcEdit, FALSE);

    // destroy the RADical window if any
    if (IsWindow(m_rad_window))
    {
        DestroyWindow(m_rad_window);
    }

    // reopen if necessary
    if (bReopen)
    {
        if (m_type == entry->m_type &&
            m_name == entry->m_name &&
            m_lang == entry->m_lang)
        {
            if (entry->m_et == ET_LANG && entry->m_type == RT_DIALOG)
            {
                MByteStreamEx stream(entry->m_data);
                m_rad_window.m_dialog_res.LoadFromStream(stream);
                m_rad_window.CreateDx(m_hwnd);
            }
        }
    }

    return TRUE;
}

// compile the source if necessary
BOOL MMainWnd::CompileIfNecessary(BOOL bReopen/* = FALSE*/)
{
    if (Edit_GetModify(m_hSrcEdit))
    {
        // the modification flag is set. query to compile
        INT id = MsgBoxDx(IDS_COMPILENOW, MB_ICONINFORMATION | MB_YESNOCANCEL);
        switch (id)
        {
        case IDYES:
            // ok, let's compile
            return ReCompileOnSelChange(bReopen);

        case IDNO:
            // clear the modification flag
            Edit_SetModify(m_hSrcEdit, FALSE);

            // destroy the RADical window if any
            if (IsWindow(m_rad_window))
                DestroyWindow(m_rad_window);
            break;

        case IDCANCEL:
            return FALSE;   // cancelled
        }
    }

    return TRUE;    // go
}

// check the data folder
BOOL MMainWnd::CheckDataFolder(VOID)
{
    // get the module path filename of this application module
    WCHAR szPath[MAX_PATH], *pch;
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    // find the last '\\' or '/'
    pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(szPath, L'/');
    if (pch == NULL)
        return FALSE;

    // find the data folder
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"\\data");
    if (!PathFileExistsW(szPath))
    {
        StringCchCopyW(pch, diff, L"\\..\\data");
        if (!PathFileExistsW(szPath))
        {
            StringCchCopyW(pch, diff, L"\\..\\..\\data");
            if (!PathFileExistsW(szPath))
            {
                StringCchCopyW(pch, diff, L"\\..\\..\\..\\data");
                if (!PathFileExistsW(szPath))
                {
                    StringCchCopyW(pch, diff, L"\\..\\..\\..\\..\\data");
                    if (!PathFileExistsW(szPath))
                    {
                        return FALSE;   // not found
                    }
                }
            }
        }
    }

    // found. store to m_szDataFolder
    StringCchCopyW(m_szDataFolder, _countof(m_szDataFolder), szPath);

    // get the PATH environment variable
    MStringW env, str;
    DWORD cch = GetEnvironmentVariableW(L"PATH", NULL, 0);
    env.resize(cch);
    GetEnvironmentVariableW(L"PATH", &env[0], cch);

    // add "data/bin" to the PATH variable
    str = m_szDataFolder;
    str += L"\\bin;";
    str += env;
    SetEnvironmentVariableW(L"PATH", str.c_str());

    return TRUE;
}

// check the data and the helper programs
INT MMainWnd::CheckData(VOID)
{
    if (!CheckDataFolder())
    {
        ErrorBoxDx(TEXT("ERROR: data folder was not found!"));
        return -1;  // failure
    }

    // Constants.txt
    StringCchCopyW(m_szConstantsFile, _countof(m_szConstantsFile), m_szDataFolder);
    StringCchCatW(m_szConstantsFile, _countof(m_szConstantsFile), L"\\Constants.txt");
    if (!g_db.LoadFromFile(m_szConstantsFile))
    {
        ErrorBoxDx(TEXT("ERROR: Unable to load Constants.txt file."));
        return -2;  // failure
    }
    g_db.m_map[L"CTRLID"].emplace_back(L"IDC_STATIC", (WORD)-1);

    // mcpp.exe
    StringCchCopyW(m_szMCppExe, _countof(m_szMCppExe), m_szDataFolder);
    StringCchCatW(m_szMCppExe, _countof(m_szMCppExe), L"\\bin\\mcpp.exe");
    if (!PathFileExistsW(m_szMCppExe))
    {
        ErrorBoxDx(TEXT("ERROR: No mcpp.exe found."));
        return -3;  // failure
    }

    // windres.exe
    StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
    StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");
    if (!PathFileExistsW(m_szWindresExe))
    {
        ErrorBoxDx(TEXT("ERROR: No windres.exe found."));
        return -4;  // failure
    }

    // upx.exe
    StringCchCopyW(m_szUpxExe, _countof(m_szUpxExe), m_szDataFolder);
    StringCchCatW(m_szUpxExe, _countof(m_szUpxExe), L"\\bin\\upx.exe");
    if (!PathFileExistsW(m_szUpxExe))
    {
        ErrorBoxDx(TEXT("ERROR: No upx.exe found."));
        return -5;  // failure
    }

    // include directory
    StringCchCopyW(m_szIncludeDir, _countof(m_szIncludeDir), m_szDataFolder);
    StringCchCatW(m_szIncludeDir, _countof(m_szIncludeDir), L"\\lib\\gcc\\i686-w64-mingw32\\7.3.0\\include");
    if (!PathFileExistsW(m_szIncludeDir))
    {
        ErrorBoxDx(TEXT("ERROR: No include directory found."));
        return -6;  // failure
    }

    // dfmsc.exe
    StringCchCopyW(m_szDFMSC, _countof(m_szDFMSC), m_szDataFolder);
    StringCchCatW(m_szDFMSC, _countof(m_szDFMSC), L"\\bin\\dfmsc.exe");
    if (!PathFileExistsW(m_szDFMSC))
    {
        ErrorBoxDx(TEXT("ERROR: No dfmsc.exe found."));
        return -7;  // failure
    }

    // get the module path filename of this application module
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    // mcdx.exe
    PathRemoveFileSpecW(szPath);
    PathAppendW(szPath, L"mcdx.exe");
    if (PathFileExistsW(szPath))
    {
        StringCchCopyW(m_szMcdxExe, _countof(m_szMcdxExe), szPath);
    }
    else
    {
        StringCchCopyW(m_szMcdxExe, _countof(m_szMcdxExe), m_szDataFolder);
        StringCchCatW(m_szMcdxExe, _countof(m_szMcdxExe), L"\\bin\\mcdx.exe");
        if (!PathFileExistsW(m_szMcdxExe))
        {
            ErrorBoxDx(TEXT("ERROR: No mcdx.exe found."));
            return -8;  // failure
        }
    }

    return 0;   // success
}

// load the language information
void MMainWnd::DoLoadLangInfo(VOID)
{
    // enumerate localized languages
    EnumSystemLocalesW(EnumLocalesProc, LCID_SUPPORTED);

    // enumerate English languages
    EnumSystemLocalesW(EnumEngLocalesProc, LCID_SUPPORTED);

    // add the neutral language
    {
        LANG_ENTRY entry;
        entry.LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        entry.str = LoadStringDx(IDS_NEUTRAL);
        g_langs.push_back(entry);
    }

    // sort
    std::sort(g_langs.begin(), g_langs.end());
}

// load a file
BOOL MMainWnd::DoLoadFile(HWND hwnd, LPCWSTR pszFileName, DWORD nFilterIndex, BOOL bForceDecompress)
{
    MWaitCursor wait;
    WCHAR szPath[MAX_PATH], szResolvedPath[MAX_PATH], *pchPart;

    enum LoadFilterIndex        // see also: IDS_EXERESRCFILTER
    {
        LFI_NONE = 0,
        LFI_EXECUTABLE = 1,
        LFI_RES = 2,
        LFI_RC = 3,
        LFI_ALL = 4
    };

    if (nFilterIndex == LFI_ALL)
        nFilterIndex = LFI_NONE;

    // if it was a shortcut file, then resolve it.
    // pszFileName --> szPath
    if (GetPathOfShortcutDx(hwnd, pszFileName, szResolvedPath))
    {
        GetFullPathNameW(szResolvedPath, _countof(szPath), szPath, &pchPart);
    }
    else
    {
        GetFullPathNameW(pszFileName, _countof(szPath), szPath, &pchPart);
    }

    // find the dot extension
    LPWSTR pch = wcsrchr(szPath, L'.');
    if (nFilterIndex == LFI_NONE || nFilterIndex == LFI_EXECUTABLE)
    {
        if (pch && lstrcmpiW(pch, L".res") == 0)
            nFilterIndex = LFI_RES;
        else if (pch && lstrcmpiW(pch, L".rc") == 0)
            nFilterIndex = LFI_RC;
    }

    if (nFilterIndex == LFI_RES)     // .res files
    {
        // reload the resource.h if necessary
        UnloadResourceH(hwnd);
        if (g_settings.bAutoLoadNearbyResH)
            CheckResourceH(hwnd, szPath);

        // do import to the res variable
        EntrySet res;
        if (!res.import_res(szPath))
        {
            ErrorBoxDx(IDS_CANNOTOPEN);
            return FALSE;
        }

        // load it now
        m_bLoading = TRUE;
        {
            // renewal
            g_res.delete_all();
            g_res.merge(res);

            // clean up
            res.delete_all();
        }
        m_bLoading = FALSE;

        // update the file info
        UpdateFileInfo(FT_RES, szPath, FALSE);

        // show ID list if necessary
        if (m_szResourceH[0] && g_settings.bAutoShowIDList)
        {
            ShowIDList(hwnd, TRUE);
        }

        // select none
        SelectTV(NULL, FALSE);

        DoSetFileModified(FALSE);
        return TRUE;
    }

    if (nFilterIndex == LFI_RC)  // .rc files
    {
        // reload the resource.h if necessary
        UnloadResourceH(hwnd);
        if (g_settings.bAutoLoadNearbyResH)
            CheckResourceH(hwnd, szPath);

        // load the RC file to the res variable
        EntrySet res;
        if (!DoLoadRC(hwnd, szPath, res))
        {
            ErrorBoxDx(IDS_CANNOTOPEN);
            return FALSE;
        }

        // load it now
        m_bLoading = TRUE;
        {
            // renewal
            g_res.delete_all();
            g_res.merge(res);

            // clean up
            res.delete_all();
        }
        m_bLoading = FALSE;

        // update the file info
        UpdateFileInfo(FT_RC, szPath, FALSE);

        // show ID list if necessary
        if (m_szResourceH[0] && g_settings.bAutoShowIDList)
        {
            ShowIDList(hwnd, TRUE);
        }

        // select none
        SelectTV(NULL, FALSE);

        DoSetFileModified(FALSE);
        return TRUE;
    }

    LPWSTR pszPath = szPath;        // the real path

    // check whether it was compressed
    MStringW strToOpen = pszPath;
    BOOL bCompressed = DoUpxTest(m_szUpxExe, pszPath);
    if (bCompressed)   // it was compressed
    {
        INT nID;
        if (bForceDecompress)
        {
            nID = IDYES;    // no veto for you
        }
        else
        {
            // confirm to the user to decompress
            LPWSTR szMsg = LoadStringPrintfDx(IDS_FILEISUPXED, pszPath);
            nID = MsgBoxDx(szMsg, MB_ICONINFORMATION | MB_YESNOCANCEL);
            if (nID == IDCANCEL)
                return FALSE;   // cancelled
        }

        if (nID == IDYES)   // try to decompress
        {
            // build a temporary file path
            WCHAR szTempFile[MAX_PATH];
            StringCchCopyW(szTempFile, _countof(szTempFile), GetTempFileNameDx(L"UPX"));

            // pszPath --> szTempFile (to be decompressed)
            if (!CopyFileW(pszPath, szTempFile, FALSE) ||
                !DoUpxDecompress(m_szUpxExe, szTempFile))
            {
                DeleteFileW(szTempFile);
                ErrorBoxDx(IDS_CANTUPXEXTRACT);
                return FALSE;   // failure
            }

            // decompressed
            strToOpen = szTempFile;
        }
        else
        {
            // consider as uncompressed
            bCompressed = FALSE;
        }
    }

    // load an executable files
    HMODULE hMod = LoadLibraryExW(strToOpen.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE |
                                  LOAD_LIBRARY_AS_IMAGE_RESOURCE |
                                  LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hMod == NULL)
    {
        // replace the path
        #ifdef _WIN64
            mstr_replace_all(strToOpen,
                L"C:\\Program Files\\",
                L"C:\\Program Files (x86)\\");
        #else
            mstr_replace_all(strToOpen,
                L"C:\\Program Files (x86)\\",
                L"C:\\Program Files\\");
        #endif

        // retry to load
        hMod = LoadLibraryExW(strToOpen.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE |
            LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_WITH_ALTERED_SEARCH_PATH);
        if (hMod)
        {
            // ok, succeeded
            StringCchCopy(pszPath, _countof(szPath), strToOpen.c_str());
        }
        else
        {
            // retry again
            hMod = LoadLibraryW(strToOpen.c_str());
            if (hMod == NULL)
            {
                ErrorBoxDx(IDS_CANNOTOPEN);

                // delete the decompressed file if any
                if (bCompressed)
                {
                    ::DeleteFileW(strToOpen.c_str());
                }

                return FALSE;       // failure
            }
        }
    }

    // unload the resource.h file
    UnloadResourceH(hwnd);
    if (g_settings.bAutoLoadNearbyResH)
        CheckResourceH(hwnd, pszPath);

    // load all the resource items from the executable
    m_bLoading = TRUE;
    {
        g_res.delete_all();
        g_res.from_res(hMod);
    }
    m_bLoading = FALSE;

    // free the executable
    FreeLibrary(hMod);

    // update the file info (using the real path)
    UpdateFileInfo(FT_EXECUTABLE, pszPath, bCompressed);

    // delete the decompressed file if any
    if (bCompressed)
    {
        ::DeleteFileW(strToOpen.c_str());
    }

    // open the ID list window if necessary
    if (m_szResourceH[0] && g_settings.bAutoShowIDList)
    {
        ShowIDList(hwnd, TRUE);
    }

    // select none
    SelectTV(NULL, FALSE);

    DoSetFileModified(FALSE);

    return TRUE;    // success
}

// unload resource.h
BOOL MMainWnd::UnloadResourceH(HWND hwnd)
{
    // delete all the macro IDs
    auto it = g_db.m_map.find(L"RESOURCE.ID");
    if (it != g_db.m_map.end())
    {
        it->second.clear();
    }

    // reset the settings of the resource.h file
    g_settings.AddIDC_STATIC();
    g_settings.id_map.clear();
    g_settings.added_ids.clear();
    g_settings.removed_ids.clear();
    m_szResourceH[0] = 0;

    // update the names
    UpdateNames();

    // select the selected entry
    auto entry = g_res.get_entry();
    SelectTV(entry, FALSE);

    // hide the ID list window
    ShowIDList(hwnd, FALSE);

    return TRUE;
}

// check the resource.h file
BOOL MMainWnd::CheckResourceH(HWND hwnd, LPCTSTR pszPath)
{
    // unload the resource.h file
    UnloadResourceH(hwnd);

    // pszPath --> szPath
    TCHAR szPath[MAX_PATH];
    StringCchCopy(szPath, _countof(szPath), pszPath);

    // find the last '\\' or '/'
    TCHAR *pch = _tcsrchr(szPath, TEXT('\\'));
    if (pch == NULL)
        pch = _tcsrchr(szPath, TEXT('/'));
    if (pch == NULL)
        pch = szPath;
    else
        ++pch;

    // find the nearest resource.h file
    size_t diff = pch - szPath;
    StringCchCopy(pch, diff, TEXT("resource.h"));
    if (!PathFileExistsW(szPath))
    {
        StringCchCopy(pch, diff, TEXT("..\\resource.h"));
        if (!PathFileExistsW(szPath))
        {
            StringCchCopy(pch, diff, TEXT("..\\..\\resource.h"));
            if (!PathFileExistsW(szPath))
            {
                StringCchCopy(pch, diff, TEXT("..\\..\\..\\resource.h"));
                if (!PathFileExistsW(szPath))
                {
                    StringCchCopy(pch, diff, TEXT("..\\src\\resource.h"));
                    if (!PathFileExistsW(szPath))
                    {
                        StringCchCopy(pch, diff, TEXT("..\\..\\src\\resource.h"));
                        if (!PathFileExistsW(szPath))
                        {
                            StringCchCopy(pch, diff, TEXT("..\\..\\..\\src\\resource.h"));
                            if (!PathFileExistsW(szPath))
                            {
                                return FALSE;   // not found
                            }
                        }
                    }
                }
            }
        }
    }

    // load the resource.h file
    return DoLoadResH(hwnd, szPath);
}

// load an RC file
BOOL MMainWnd::DoLoadRC(HWND hwnd, LPCWSTR szRCFile, EntrySet& res)
{
    // load the RC file to the res variable
    MStringA strOutput;
    BOOL bOK = res.load_rc(szRCFile, strOutput, m_szWindresExe,
                           m_szMCppExe, m_szMcdxExe, GetMacroDump(),
                           GetIncludesDump(), m_szIncludeDir);
    if (!bOK)
    {
        // failed. show error message
        if (strOutput.empty())
        {
            SetWindowTextW(m_hBinEdit, LoadStringDx(IDS_COMPILEERROR));
            ShowBinEdit(FALSE);
        }
        else
        {
            MAnsiToWide a2w(CP_ACP, strOutput.c_str());
            ErrorBoxDx(a2w.c_str());

            SetWindowTextA(m_hBinEdit, (char *)&strOutput[0]);
            ShowBinEdit(TRUE, TRUE);
        }
    }

    // close the preview
    HidePreview();

    // recalculate the splitter
    PostMessageDx(WM_SIZE);

    return bOK;
}

// find the text
void MMainWnd::OnFind(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return;     // there is no text in m_hSrcEdit

    if (!IsWindowVisible(m_hSrcEdit))
        return;     // m_hSrcEdit was not visible

    // close the find/replace dialog if any
    if (IsWindow(m_hFindReplaceDlg))
    {
        SendMessage(m_hFindReplaceDlg, WM_CLOSE, 0, 0);
        m_hFindReplaceDlg = NULL;
    }

    // show the find dialog
    m_fr.hwndOwner = hwnd;
    m_fr.Flags = FR_HIDEWHOLEWORD | FR_DOWN;
    m_hFindReplaceDlg = FindText(&m_fr);
}

// find next
BOOL MMainWnd::OnFindNext(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;   // there is no text in m_hSrcEdit

    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;   // m_hSrcEdit was not visible

    // if the text to find was empty, then show the dialog
    if (m_szFindWhat[0] == 0)
    {
        OnFind(hwnd);
        return FALSE;
    }

    // get the selection
    DWORD ibegin, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&ibegin, (LPARAM)&iend);

    // m_szFindWhat --> szText
    TCHAR szText[_countof(m_szFindWhat)];
    StringCchCopy(szText, _countof(szText), m_szFindWhat);
    if (szText[0] == 0)
        return FALSE;

    // get the text of m_hSrcEdit
    MString str = GetWindowText(m_hSrcEdit);
    if (str.empty())
        return FALSE;

    // make the text uppercase if necessary
    if (!(m_fr.Flags & FR_MATCHCASE))
    {
        CharUpperW(szText);
        CharUpperW(&str[0]);
    }

    // get the selection text
    MString substr = str.substr(ibegin, iend - ibegin);
    if (substr == szText)
    {
        // if the selected text was szText, move the starting position
        ibegin += (DWORD)substr.size();
    }

    // find the string
    size_t i = str.find(szText, ibegin);
    if (i == MString::npos)
        return FALSE;   // not found

    // found
    ibegin = (DWORD)i;
    iend = ibegin + lstrlen(m_szFindWhat);

    // set the text selection
    SendMessage(m_hSrcEdit, EM_SETSEL, (WPARAM)ibegin, (LPARAM)iend);

    // ensure the text visible
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);

    return TRUE;
}

// find previous
BOOL MMainWnd::OnFindPrev(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;   // there is no text in m_hSrcEdit
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;   // m_hSrcEdit was not visible

    // if the text to find was empty, then show the dialog
    if (m_szFindWhat[0] == 0)
    {
        OnFind(hwnd);
        return FALSE;
    }

    // get the text selection
    DWORD ibegin, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&ibegin, (LPARAM)&iend);

    // m_szFindWhat --> szText
    TCHAR szText[_countof(m_szFindWhat)];
    StringCchCopy(szText, _countof(szText), m_szFindWhat);
    if (szText[0] == 0)
        return FALSE;

    // get the text of m_hSrcEdit
    MString str = GetWindowText(m_hSrcEdit);
    if (str.empty())
        return FALSE;

    // make the text uppercase if necessary
    if (!(m_fr.Flags & FR_MATCHCASE))
    {
        CharUpperW(szText);
        CharUpperW(&str[0]);
    }

    // get the selection text
    MString substr = str.substr(ibegin, iend - ibegin);
    if (substr == szText)
    {
        // if the selected text was szText, move the starting position
        --ibegin;
    }

    // find the string barkward
    size_t i = str.rfind(szText, ibegin);
    if (i == MString::npos)
        return FALSE;

    // found
    ibegin = (DWORD)i;
    iend = ibegin + lstrlen(m_szFindWhat);

    // set the text selection
    SendMessage(m_hSrcEdit, EM_SETSEL, (WPARAM)ibegin, (LPARAM)iend);

    // ensure the text visible
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);

    return TRUE;
}

// replace next
BOOL MMainWnd::OnReplaceNext(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;   // there is no text in m_hSrcEdit
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;   // m_hSrcEdit was not visible
    if (GetWindowStyle(m_hSrcEdit) & ES_READONLY)
        return FALSE;   // m_hSrcEdit was read-only

    // if the text to find was empty, then show the dialog
    if (m_szFindWhat[0] == 0)
    {
        OnReplace(hwnd);
        return FALSE;
    }

    // get the text selection
    DWORD ibegin, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&ibegin, (LPARAM)&iend);

    // m_szFindWhat --> szText
    TCHAR szText[_countof(m_szFindWhat)];
    StringCchCopy(szText, _countof(szText), m_szFindWhat);
    if (szText[0] == 0)
        return FALSE;

    // get the text of m_hSrcEdit
    MString str = GetWindowText(m_hSrcEdit);
    if (str.empty())
        return FALSE;

    // make the text uppercase if necessary
    if (!(m_fr.Flags & FR_MATCHCASE))
    {
        CharUpperW(szText);
        CharUpperW(&str[0]);
    }

    // get the selection text
    MString substr = str.substr(ibegin, iend - ibegin);
    if (substr == szText)
    {
        // if the selected text was szText, replace it and move the starting position
        SendMessage(m_hSrcEdit, EM_REPLACESEL, TRUE, (LPARAM)m_szReplaceWith);
        str.replace(ibegin, iend - ibegin, m_szReplaceWith);
        ibegin += lstrlen(m_szReplaceWith);

        // make it modified
        Edit_SetModify(m_hSrcEdit, TRUE);
    }

    // find the text
    size_t i = str.find(szText, ibegin);
    if (i == MString::npos)
        return FALSE;   // not found

    // found
    ibegin = (DWORD)i;
    iend = ibegin + lstrlen(m_szFindWhat);

    // set the text selection
    SendMessage(m_hSrcEdit, EM_SETSEL, (WPARAM)ibegin, (LPARAM)iend);

    // ensure the text visible
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);

    return TRUE;
}

// replace previous
BOOL MMainWnd::OnReplacePrev(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;   // there is no text in m_hSrcEdit
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;   // m_hSrcEdit was not visible
    if (GetWindowStyle(m_hSrcEdit) & ES_READONLY)
        return FALSE;   // m_hSrcEdit was read-only

    // if the text to find was empty, then show the dialog
    if (m_szFindWhat[0] == 0)
    {
        OnReplace(hwnd);
        return FALSE;
    }

    // get the text selection
    DWORD ibegin, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&ibegin, (LPARAM)&iend);

    // m_szFindWhat --> szText
    TCHAR szText[_countof(m_szFindWhat)];
    StringCchCopy(szText, _countof(szText), m_szFindWhat);
    if (szText[0] == 0)
        return FALSE;

    // get the text of m_hSrcEdit
    MString str = GetWindowText(m_hSrcEdit);
    if (str.empty())
        return FALSE;

    // make the text to find uppercase if necessary
    if (!(m_fr.Flags & FR_MATCHCASE))
    {
        CharUpperW(szText);
        CharUpperW(&str[0]);
    }

    // get the selection text
    MString substr = str.substr(ibegin, iend - ibegin);
    if (substr == szText)
    {
        // if the selected text was szText, replace it and move the starting position
        SendMessage(m_hSrcEdit, EM_REPLACESEL, TRUE, (LPARAM)m_szReplaceWith);
        str.replace(ibegin, iend - ibegin, m_szReplaceWith);
        --ibegin;

        // make it modified
        Edit_SetModify(m_hSrcEdit, TRUE);
    }

    // find the string barkward
    size_t i = str.rfind(szText, ibegin);
    if (i == MString::npos)
        return FALSE;

    // found
    ibegin = (DWORD)i;
    iend = ibegin + lstrlen(m_szFindWhat);

    // set the text selection
    SendMessage(m_hSrcEdit, EM_SETSEL, (WPARAM)ibegin, (LPARAM)iend);

    // ensure the text visible
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);

    return TRUE;
}

// do replace
BOOL MMainWnd::OnReplace(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;   // there is no text in m_hSrcEdit
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;   // m_hSrcEdit was not visible
    if (GetWindowStyle(m_hSrcEdit) & ES_READONLY)
        return FALSE;   // m_hSrcEdit was read-only

    // close the find/replace dialog if any
    if (IsWindow(m_hFindReplaceDlg))
    {
        SendMessage(m_hFindReplaceDlg, WM_CLOSE, 0, 0);
        m_hFindReplaceDlg = NULL;
    }

    // replace the text
    m_fr.hwndOwner = hwnd;
    m_fr.Flags = FR_HIDEWHOLEWORD | FR_DOWN;
    m_hFindReplaceDlg = ReplaceText(&m_fr);

    return TRUE;
}

// replace all
BOOL MMainWnd::OnReplaceAll(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;   // there is no text in m_hSrcEdit
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;   // m_hSrcEdit was not visible
    if (GetWindowStyle(m_hSrcEdit) & ES_READONLY)
        return FALSE;   // m_hSrcEdit was read-only

    // get the text selection
    DWORD istart, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&istart, (LPARAM)&iend);

    // move the caret to the top
    SendMessage(m_hSrcEdit, EM_SETSEL, 0, 0);

    // repeat replacing until failure
    while (OnReplaceNext(hwnd))
        ;

    // restore the text selection
    SendMessage(m_hSrcEdit, EM_SETSEL, istart, iend);

    // ensure the text visible
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);

    return TRUE;
}

// do the find message for FindText/ReplaceText API
LRESULT MMainWnd::OnFindMsg(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (m_fr.Flags & FR_DIALOGTERM)     // to be destroyed
    {
        m_hFindReplaceDlg = NULL;
        SetFocus(m_hSrcEdit);
        return 0;
    }

    if (m_fr.Flags & FR_REPLACEALL)     // do replace all
    {
        OnReplaceAll(hwnd);
    }
    else if (m_fr.Flags & FR_REPLACE)   // do replace once
    {
        if (m_fr.Flags & FR_DOWN)
            OnReplaceNext(hwnd);
        else
            OnReplacePrev(hwnd);
    }
    else if (m_fr.Flags & FR_FINDNEXT)  // do find
    {
        if (m_fr.Flags & FR_DOWN)       // find downward
        {
            OnFindNext(hwnd);
        }
        else    // find upward
        {
            OnFindPrev(hwnd);
        }
    }
    return 0;
}

BOOL MMainWnd::DoWriteRCLangUTF8(MFile& file, ResToText& res2text, WORD lang)
{
    MTextToAnsi comment_sep(CP_UTF8, LoadStringDx(IDS_COMMENT_SEP));

    if (!g_settings.bSepFilesByLang && g_settings.bRedundantComments)
    {
        file.WriteSzA(comment_sep.c_str());
        file.WriteSzA("\r\n");
    }

    // dump a comment and a LANGUAGE statement
    MString strLang = ::GetLanguageStatement(lang, TRUE);
    strLang += L"\r\n";
    file.WriteSzA(MWideToAnsi(CP_ACP, strLang.c_str()).c_str());

    // search the language entries
    EntrySetBase found;
    g_res.search(found, ET_LANG, WORD(0), WORD(0), lang);

    std::vector<EntryBase *> vecFound(found.begin(), found.end());

    std::sort(vecFound.begin(), vecFound.end(),
        [](const EntryBase *a, const EntryBase *b) {
            if (a->m_type < b->m_type)
                return true;
            if (a->m_type > b->m_type)
                return false;
            return a->m_name < b->m_name;
        }
    );

    MIdOrString type, old_type;

    // for all found entries
    for (auto entry : vecFound)
    {
        old_type = type;
        type = entry->m_type;

        // ignore the string or message tables or font dir
        if (type == RT_STRING || type == RT_MESSAGETABLE || type == RT_FONTDIR)
        {
            continue;
        }

        // dump the entry
        MString str = res2text.DumpEntry(*entry);
        if (!str.empty())
        {
            // output redundant comments
            if (type != old_type && g_settings.bRedundantComments)
            {
                file.WriteSzA(comment_sep.c_str());
                MStringW strType = res2text.GetResTypeName(type);
                MWideToAnsi utf8(CP_UTF8, strType.c_str());
                file.WriteSzA("// ");
                file.WriteSzA(utf8.c_str());
                file.WriteSzA("\r\n\r\n");
            }

            mstr_trim(str);     // trim

            // convert the text to UTF-8
            MTextToAnsi t2a(CP_UTF8, str.c_str());
            file.WriteSzA(t2a.c_str());

            // add newlines
            file.WriteSzA("\r\n\r\n");
        }
    }

    // search the string tables
    found.clear();
    g_res.search(found, ET_LANG, RT_STRING, (WORD)0, lang);
    if (found.size())
    {
        if (g_settings.bRedundantComments)
        {
            file.WriteSzA(comment_sep.c_str());
            file.WriteSzA("// RT_STRING\r\n\r\n");
        }

        // found --> str_res
        StringRes str_res;
        for (auto e : found)
        {
            if (e->m_lang != lang)
                continue;       // must be same language

            MByteStreamEx stream(e->m_data);
            if (!str_res.LoadFromStream(stream, e->m_name.m_id))
                return FALSE;
        }

        // dump
        MString str = str_res.Dump();

        // trim
        mstr_trim(str);

        // append newlines
        str += L"\r\n\r\n";

        // convert the text to UTF-8
        MTextToAnsi t2a(CP_UTF8, str.c_str());

        // write it
        file.WriteSzA(t2a.c_str());
    }

    // search the message tables
    found.clear();
    g_res.search(found, ET_LANG, RT_MESSAGETABLE, (WORD)0, lang);
    if (found.size())
    {
        if (g_settings.bRedundantComments)
        {
            file.WriteSzA(comment_sep.c_str());
            file.WriteSzA("// RT_MESSAGETABLE\r\n\r\n");
        }

        // found --> msg_res
        MessageRes msg_res;
        for (auto e : found)
        {
            if (e->m_lang != lang)
                continue;       // must be same language

            MByteStreamEx stream(e->m_data);
            if (!msg_res.LoadFromStream(stream, e->m_name.m_id))
                return FALSE;
        }

        // dump it
        MString str;
        str += L"#ifdef APSTUDIO_INVOKED\r\n";
        str += L"    #error Ap Studio cannot edit this message table.\r\n";
        str += L"#endif\r\n";
        str += L"#ifdef MCDX_INVOKED\r\n";
        str += msg_res.Dump();
        str += L"#endif\r\n\r\n";

        // convert it to UTF-8
        MTextToAnsi t2a(CP_UTF8, str.c_str());

        // write it
        file.WriteSzA(t2a.c_str());
    }

    return TRUE;
}

BOOL MMainWnd::DoWriteRCLangUTF16(MFile& file, ResToText& res2text, WORD lang)
{
    MString comment_sep(LoadStringDx(IDS_COMMENT_SEP));

    if (!g_settings.bSepFilesByLang && g_settings.bRedundantComments)
    {
        file.WriteSzW(comment_sep.c_str());
        file.WriteSzW(L"\r\n");
    }

    // dump a comment and a LANGUAGE statement
    MString strLang = ::GetLanguageStatement(lang, TRUE);
    strLang += L"\r\n";
    file.WriteSzW(strLang.c_str());

    // search the language entries
    EntrySetBase found;
    g_res.search(found, ET_LANG, WORD(0), WORD(0), lang);

    std::vector<EntryBase *> vecFound(found.begin(), found.end());

    std::sort(vecFound.begin(), vecFound.end(),
        [](const EntryBase *a, const EntryBase *b) {
            if (a->m_type < b->m_type)
                return true;
            if (a->m_type > b->m_type)
                return false;
            return a->m_name < b->m_name;
        }
    );

    MIdOrString type, old_type;

    // for all found entries
    for (auto entry : vecFound)
    {
        old_type = type;
        type = entry->m_type;

        // ignore the string or message tables or font dir 
        if (type == RT_STRING || type == RT_MESSAGETABLE || type == RT_FONTDIR)
        {
            continue;
        }

        // dump the entry
        MString str = res2text.DumpEntry(*entry);
        if (!str.empty())
        {
            // output redundant comments
            if (type != old_type && g_settings.bRedundantComments)
            {
                file.WriteSzW(comment_sep.c_str());
                MStringW strType = res2text.GetResTypeName(type);
                file.WriteSzW(L"// ");
                file.WriteSzW(strType.c_str());
                file.WriteSzW(L"\r\n\r\n");
            }

            mstr_trim(str);     // trim

            // convert the text to UTF-8
            file.WriteSzW(str.c_str());

            // add newlines
            file.WriteSzW(L"\r\n\r\n");
        }
    }

    // search the string tables
    found.clear();
    g_res.search(found, ET_LANG, RT_STRING, (WORD)0, lang);
    if (found.size())
    {
        if (g_settings.bRedundantComments)
        {
            file.WriteSzW(comment_sep.c_str());
            file.WriteSzW(L"// RT_STRING\r\n\r\n");
        }

        // found --> str_res
        StringRes str_res;
        for (auto e : found)
        {
            if (e->m_lang != lang)
                continue;       // must be same language

            MByteStreamEx stream(e->m_data);
            if (!str_res.LoadFromStream(stream, e->m_name.m_id))
                return FALSE;
        }

        // dump
        MString str = str_res.Dump();

        // trim
        mstr_trim(str);

        // append newlines
        str += L"\r\n\r\n";

        // write it
        file.WriteSzW(str.c_str());
    }

    // search the message tables
    found.clear();
    g_res.search(found, ET_LANG, RT_MESSAGETABLE, (WORD)0, lang);
    if (found.size())
    {
        if (g_settings.bRedundantComments)
        {
            file.WriteSzW(comment_sep.c_str());
            file.WriteSzW(L"// RT_MESSAGETABLE\r\n\r\n");
        }

        // found --> msg_res
        MessageRes msg_res;
        for (auto e : found)
        {
            if (e->m_lang != lang)
                continue;       // must be same language

            MByteStreamEx stream(e->m_data);
            if (!msg_res.LoadFromStream(stream, e->m_name.m_id))
                return FALSE;
        }

        // dump it
        MString str;
        str += L"#ifdef APSTUDIO_INVOKED\r\n";
        str += L"    #error Ap Studio cannot edit this message table.\r\n";
        str += L"#endif\r\n";
        str += L"#ifdef MCDX_INVOKED\r\n";
        str += msg_res.Dump();
        str += L"#endif\r\n\r\n";

        // write it
        file.WriteSzW(str.c_str());
    }

    return TRUE;
}

// write a language-specific RC text
BOOL MMainWnd::DoWriteRCLang(MFile& file, ResToText& res2text, WORD lang)
{
    if (g_settings.bRCFileUTF16)
        return DoWriteRCLangUTF16(file, res2text, lang);
    else
        return DoWriteRCLangUTF8(file, res2text, lang);
}

// do backup a folder
BOOL MMainWnd::DoBackupFolder(LPCWSTR pszPath, UINT nCount)
{
    if (!PathIsDirectoryW(pszPath))
        return TRUE;    // no files to be backup'ed

    if (nCount < s_nBackupMaxCount)     // less than max count
    {
        MString strPath = pszPath;
        strPath += g_settings.strBackupSuffix;

        // do backup the "old" folder (recursively)
        DoBackupFolder(strPath.c_str(), nCount + 1);

        // rename the current folder as an "old" folder
        return MoveFileExW(pszPath, strPath.c_str(),
                           MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
    }
    else
    {
        // unable to create one more backup. delete it
        DeleteDirectoryDx(pszPath);
    }

    return TRUE;
}

// do backup a file
BOOL MMainWnd::DoBackupFile(LPCWSTR pszPath, UINT nCount)
{
    if (!PathFileExistsW(pszPath))
        return TRUE;

    if (nCount < s_nBackupMaxCount)     // less than max count
    {
        MString strPath = pszPath;
        strPath += g_settings.strBackupSuffix;

        // do backup the "old" file (recursively)
        DoBackupFile(strPath.c_str(), nCount + 1);

        // copy the current file as an "old" file
        return CopyFileW(pszPath, strPath.c_str(), FALSE);
    }
    else
    {
        // otherwise overwritten
    }

    return TRUE;
}

// write a RC file
BOOL MMainWnd::DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH)
{
    ResToText res2text;
    res2text.m_bHumanReadable = FALSE;  // it's not human-friendly
    res2text.m_bNoLanguage = TRUE;      // no LANGUAGE statements generated

    MWideToAnsi comment_sep(CP_UTF8, LoadStringDx(IDS_COMMENT_SEP));

    // check not locking
    if (IsFileLockedDx(pszFileName))
    {
        WCHAR szMsg[MAX_PATH + 256];
        StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), pszFileName);
        ErrorBoxDx(szMsg);
        return FALSE;
    }

    // at first, backup it
    if (g_settings.bBackup)
        DoBackupFile(pszFileName);

    // create a RC file
    MFile file(pszFileName, TRUE);
    if (!file)
        return FALSE;

    BOOL bRCFileUTF16 = g_settings.bRCFileUTF16;

    WCHAR szTitle[MAX_PATH];
    GetFileTitleW(pszFileName, szTitle, _countof(szTitle));

    // dump heading
    if (bRCFileUTF16)
    {
        file.WriteFormatW(L"// %s\r\n", szTitle);

        file.WriteSzW(LoadStringDx(IDS_NOTICE));
        file.WriteSzW(LoadStringDx(IDS_DAGGER));
        file.WriteSzW(L"\r\n");

        if (pszResH && pszResH[0])
            file.WriteSzW(L"#include \"resource.h\"\r\n");
        file.WriteSzW(L"#define APSTUDIO_HIDDEN_SYMBOLS\r\n");
        file.WriteSzW(L"#include <windows.h>\r\n");
        file.WriteSzW(L"#include <commctrl.h>\r\n");
        file.WriteSzW(L"#undef APSTUDIO_HIDDEN_SYMBOLS\r\n");
        file.WriteSzW(L"#pragma code_page(65001) // UTF-8\r\n\r\n");

        if (g_settings.bUseIDC_STATIC && !g_settings.bHideID)
        {
            file.WriteSzW(L"#ifndef IDC_STATIC\r\n");
            file.WriteSzW(L"    #define IDC_STATIC (-1)\r\n");
            file.WriteSzW(L"#endif\r\n\r\n");
        }
    }
    else
    {
        MWideToAnsi utf8(CP_UTF8, szTitle);
        file.WriteFormatA("// %s\r\n", utf8.c_str());

        MWideToAnsi utf8Notice(CP_UTF8, LoadStringDx(IDS_NOTICE));
        MWideToAnsi utf8Dagger(CP_UTF8, LoadStringDx(IDS_DAGGER));
        file.WriteSzA(utf8Notice.c_str());
        file.WriteSzA(utf8Dagger.c_str());
        file.WriteSzA("\r\n");

        if (pszResH && pszResH[0])
            file.WriteSzA("#include \"resource.h\"\r\n");
        file.WriteSzA("#define APSTUDIO_HIDDEN_SYMBOLS\r\n");
        file.WriteSzA("#include <windows.h>\r\n");
        file.WriteSzA("#include <commctrl.h>\r\n");
        file.WriteSzA("#undef APSTUDIO_HIDDEN_SYMBOLS\r\n");
        file.WriteSzA("#pragma code_page(65001) // UTF-8\r\n\r\n");

        if (g_settings.bUseIDC_STATIC && !g_settings.bHideID)
        {
            file.WriteSzA("#ifndef IDC_STATIC\r\n");
            file.WriteSzA("    #define IDC_STATIC (-1)\r\n");
            file.WriteSzA("#endif\r\n\r\n");
        }
    }

    // get the used languages
    std::unordered_set<WORD> langs;
    typedef std::pair<WORD, MStringW> lang_pair;
    std::vector<lang_pair> lang_vec;

    EntrySetBase found;
    g_res.search(found, ET_LANG);

    for (auto res : found)
    {
        WORD lang = res->m_lang;
        if (langs.insert(lang).second)
        {
            MString lang_name = g_db.GetLangName(lang);
            lang_vec.push_back(std::make_pair(lang, lang_name));
        }
    }

    // sort by lang_name
    std::sort(lang_vec.begin(), lang_vec.end(),
        [](const lang_pair& a, const lang_pair& b) {
            return (a.second < b.second);
        }
    );

    // add "res/" to the prefix if necessary
    if (g_settings.bStoreToResFolder)
        res2text.m_strFilePrefix = L"res/";

    // use the "lang" folder?
    if (g_settings.bSepFilesByLang)
    {
        // dump neutral
        if (langs.count(0) > 0)
        {
            if (!DoWriteRCLang(file, res2text, 0))
                return FALSE;
        }

        // create "lang" directory path
        WCHAR szLangDir[MAX_PATH];
        StringCchCopyW(szLangDir, _countof(szLangDir), pszFileName);

        // find the last '\\' or '/'
        WCHAR *pch = wcsrchr(szLangDir, L'\\');
        if (pch == NULL)
            pch = mstrrchr(szLangDir, L'/');
        if (pch == NULL)
            return FALSE;

        // build the lang directory path
        *pch = 0;
        StringCchCatW(szLangDir, _countof(szLangDir), TEXT("/lang"));

        // backup and create "lang" directory
        for (auto lang_pair : lang_vec)
        {
            if (!lang_pair.first)
                continue;

            if (g_settings.bBackup)
                DoBackupFolder(szLangDir);

            CreateDirectory(szLangDir, NULL);
            break;
        }

        // for each language
        for (auto lang_pair : lang_vec)
        {
            auto lang = lang_pair.first;
            if (!lang)
                continue;

            // create lang/XX_XX.rc file
            WCHAR szLangFile[MAX_PATH];
            StringCchCopyW(szLangFile, _countof(szLangFile), szLangDir);
            StringCchCatW(szLangFile, _countof(szLangFile), TEXT("/"));
            MStringW lang_name = lang_pair.second;
            StringCchCatW(szLangFile, _countof(szLangFile), lang_name.c_str());
            StringCchCatW(szLangFile, _countof(szLangFile), TEXT(".rc"));
            //MessageBox(NULL, szLangFile, NULL, 0);

            if (g_settings.bBackup)
                DoBackupFile(szLangFile);

            // dump to lang/XX_XX.rc file
            MFile lang_file(szLangFile, TRUE);
            if (bRCFileUTF16)
            {
                lang_file.WriteSzW(LoadStringDx(IDS_NOTICE));
                lang_file.WriteSzW(LoadStringDx(IDS_DAGGER));
                lang_file.WriteSzW(L"\r\n");
                lang_file.WriteSzW(L"#pragma code_page(65001) // UTF-8\r\n\r\n");
            }
            else
            {
                MWideToAnsi utf8Notice(CP_UTF8, LoadStringDx(IDS_NOTICE));
                MWideToAnsi utf8Dagger(CP_UTF8, LoadStringDx(IDS_DAGGER));
                lang_file.WriteSzA(utf8Notice.c_str());
                lang_file.WriteSzA(utf8Dagger.c_str());
                lang_file.WriteSzA("\r\n");
                lang_file.WriteSzA("#pragma code_page(65001) // UTF-8\r\n\r\n");
            }
            if (!lang_file)
                return FALSE;
            if (!DoWriteRCLang(lang_file, res2text, lang))
                return FALSE;

            if (g_settings.bRedundantComments)
            {
                if (bRCFileUTF16)
                {
                    lang_file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
                }
                else
                {
                    lang_file.WriteSzA(comment_sep.c_str());
                }
            }
        }
    }
    else
    {
        // don't use the "lang" folder
        for (auto lang_pair : lang_vec)
        {
            auto lang = lang_pair.first;
            // write it for each language
            if (!DoWriteRCLang(file, res2text, lang))
                return FALSE;
        }
    }

    // dump language includes
    if (g_settings.bSepFilesByLang)
    {
        // write a C++ comment to make a section
        if (g_settings.bRedundantComments)
        {
            if (bRCFileUTF16)
            {
                file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
                file.WriteSzW(L"// Languages\r\n\r\n");
            }
            else
            {
                file.WriteSzA(comment_sep.c_str());
                file.WriteSzA("// Languages\r\n\r\n");
            }
        }

        if (g_settings.bSelectableByMacro)
        {
            for (auto lang_pair : lang_vec)     // for each language
            {
                auto lang = lang_pair.first;
                if (!lang)
                    continue;       // ignore neutral language

                // get the language name (such as en_US, ja_JP, etc.) from database
                MString lang_name1 = g_db.GetLangName(lang);

                // make uppercase one
                MString lang_name2 = lang_name1;
                CharUpperW(&lang_name2[0]);

                if (bRCFileUTF16)
                {
                    // write "#ifdef LANGUAGE_...\r\n"
                    file.WriteSzW(L"#ifdef LANGUAGE_");
                    file.WriteSzW(lang_name2.c_str());
                    file.WriteSzW(L"\r\n");

                    // write "#define \"lang/....rc\"\r\n"
                    file.WriteSzW(L"    #include \"lang/");
                    file.WriteSzW(lang_name1.c_str());
                    file.WriteSzW(L".rc\"\r\n");

                    // write "#endif\r\n"
                    file.WriteSzW(L"#endif\r\n");
                }
                else
                {
                    // write "#ifdef LANGUAGE_...\r\n"
                    file.WriteSzA("#ifdef LANGUAGE_");
                    MWideToAnsi lang2_w2a(CP_ACP, lang_name2.c_str());
                    file.WriteSzA(lang2_w2a.c_str());
                    file.WriteSzA("\r\n");

                    // write "#define \"lang/....rc\"\r\n"
                    file.WriteSzA("    #include \"lang/");
                    MWideToAnsi lang1_w2a(CP_ACP, lang_name1.c_str());
                    file.WriteSzA(lang1_w2a.c_str());
                    file.WriteSzA(".rc\"\r\n");

                    // write "#endif\r\n"
                    file.WriteSzA("#endif\r\n");
                }
            }
        }
        else
        {
            for (auto lang_pair : lang_vec)
            {
                auto lang = lang_pair.first;
                if (!lang)
                    continue;   // ignore the neutral language

                // get the language name (such as en_US, ja_JP, etc.) from database
                MString lang_name1 = g_db.GetLangName(lang);

                if (bRCFileUTF16)
                {
                    // write "#include \"lang/....rc\"\r\n"
                    file.WriteSzW(L"#include \"lang/");
                    file.WriteSzW(lang_name1.c_str());
                    file.WriteSzW(L".rc\"\r\n");
                }
                else
                {
                    // write "#include \"lang/....rc\"\r\n"
                    file.WriteSzA("#include \"lang/");
                    file.WriteSzA(MWideToAnsi(CP_ACP, lang_name1.c_str()).c_str());
                    file.WriteSzA(".rc\"\r\n");
                }
            }
        }

        if (bRCFileUTF16)
            file.WriteSzW(L"\r\n");
        else
            file.WriteSzA("\r\n");
    }

    if (g_settings.bRedundantComments)
    {
        if (bRCFileUTF16)
        {
            file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
            file.WriteSzW(L"// TEXTINCLUDE\r\n\r\n");
        }
        else
        {
            file.WriteSzA(comment_sep.c_str());
            file.WriteSzA("// TEXTINCLUDE\r\n\r\n");
        }
    }

    if (bRCFileUTF16)
    {
        file.WriteSzW(L"#ifdef APSTUDIO_INVOKED\r\n\r\n");

        // write three TEXTINCLUDE's
        file.WriteSzW(L"1 TEXTINCLUDE\r\n");
        file.WriteSzW(L"BEGIN\r\n");
        file.WriteSzW(L"    \"resource.h\\0\"\r\n");
        file.WriteSzW(L"END\r\n\r\n");

        file.WriteSzW(L"2 TEXTINCLUDE\r\n");
        file.WriteSzW(L"BEGIN\r\n");
        file.WriteSzW(L"    \"#define APSTUDIO_HIDDEN_SYMBOLS\\r\\n\"\r\n");
        file.WriteSzW(L"    \"#include <windows.h>\\r\\n\"\r\n");
        file.WriteSzW(L"    \"#include <commctrl.h>\\r\\n\"\r\n");
        file.WriteSzW(L"    \"#undef APSTUDIO_HIDDEN_SYMBOLS\\r\\n\"\r\n");
        file.WriteSzW(L"    \"\\0\"\r\n");
        file.WriteSzW(L"END\r\n\r\n");

        file.WriteSzW(L"3 TEXTINCLUDE\r\n");
        file.WriteSzW(L"BEGIN\r\n");
        file.WriteSzW(L"    \"\\r\\n\"\r\n");
        file.WriteSzW(L"    \"\\0\"\r\n");
        file.WriteSzW(L"END\r\n\r\n");

        file.WriteSzW(L"#endif    // APSTUDIO_INVOKED\r\n");

        if (g_settings.bRedundantComments)
        {
            file.WriteSzW(L"\r\n");
            file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
        }
    }
    else
    {
        file.WriteSzA("#ifdef APSTUDIO_INVOKED\r\n\r\n");

        // write three TEXTINCLUDE's
        file.WriteSzA("1 TEXTINCLUDE\r\n");
        file.WriteSzA("BEGIN\r\n");
        file.WriteSzA("    \"resource.h\\0\"\r\n");
        file.WriteSzA("END\r\n\r\n");

        file.WriteSzA("2 TEXTINCLUDE\r\n");
        file.WriteSzA("BEGIN\r\n");
        file.WriteSzA("    \"#define APSTUDIO_HIDDEN_SYMBOLS\\r\\n\"\r\n");
        file.WriteSzA("    \"#include <windows.h>\\r\\n\"\r\n");
        file.WriteSzA("    \"#include <commctrl.h>\\r\\n\"\r\n");
        file.WriteSzA("    \"#undef APSTUDIO_HIDDEN_SYMBOLS\\r\\n\"\r\n");
        file.WriteSzA("    \"\\0\"\r\n");
        file.WriteSzA("END\r\n\r\n");

        file.WriteSzA("3 TEXTINCLUDE\r\n");
        file.WriteSzA("BEGIN\r\n");
        file.WriteSzA("    \"\\r\\n\"\r\n");
        file.WriteSzA("    \"\\0\"\r\n");
        file.WriteSzA("END\r\n\r\n");

        file.WriteSzA("#endif    // APSTUDIO_INVOKED\r\n");

        if (g_settings.bRedundantComments)
        {
            file.WriteSzA("\r\n");
            file.WriteSzA(comment_sep.c_str());
        }
    }

    return TRUE;
}

struct MACRO_DEF
{
    MStringA prefix;
    MStringA name;
    DWORD value;
    MStringA definition;
};

void WriteMacroLine(MFile& file, const MStringA& name, const MStringA& definition)
{
    MStringA strSpace(" ");
    if (name.size() < 35)
        strSpace.assign(35 - name.size(), ' ');
    file.WriteFormatA("#define %s %s%s\r\n",
        name.c_str(), strSpace.c_str(), definition.c_str());
}

// write the resource.h file
BOOL MMainWnd::DoWriteResH(LPCWSTR pszResH, LPCWSTR pszRCFile)
{
    // check not locking
    if (IsFileLockedDx(pszResH))
    {
        WCHAR szMsg[MAX_PATH + 256];
        StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), pszResH);
        ErrorBoxDx(szMsg);
        return FALSE;
    }

    // do backup the resource.h file
    if (g_settings.bBackup)
        DoBackupFile(pszResH);

    // create the resource.h file
    MFile file(pszResH, TRUE);
    if (!file)
        return FALSE;

    // write a heading
    file.WriteSzA("//{{NO_DEPENDENCIES}}\r\n");
    file.WriteSzA("// Microsoft Visual C++ Compatible\r\n");

    MWideToAnsi utf8Notice(CP_UTF8, LoadStringDx(IDS_NOTICE));
    file.WriteFormatA(utf8Notice.c_str());

    // write the RC file info if necessary
    if (pszRCFile)
    {
        // get file title
        TCHAR szFileTitle[64];
        GetFileTitle(pszRCFile, szFileTitle, _countof(szFileTitle));

        // change extension to .rc
        LPTSTR pch = mstrrchr(szFileTitle, TEXT('.'));
        if (pch)
        {
            *pch = 0;
            StringCchCatW(szFileTitle, _countof(szFileTitle), TEXT(".rc"));
        }

        // write file title
        file.WriteSzA("// ");
        file.WriteSzA(MTextToAnsi(CP_ACP, szFileTitle).c_str());
        file.WriteSzA("\r\n");
    }

    // sort macro definitions
    std::vector<MACRO_DEF> defs;
    for (auto& pair : g_settings.id_map)
    {
        if (pair.first == "IDC_STATIC")
            continue;

        MACRO_DEF def;
        def.name = pair.first;
        def.definition = pair.second;
        def.value = mstr_parse_int(pair.second.c_str(), true, 0);
        size_t i = pair.first.find('_');
        if (i != MStringA::npos)
        {
            def.prefix = pair.first.substr(0, i + 1);
        }
        defs.push_back(def);
    }
    std::sort(defs.begin(), defs.end(),
        [](const MACRO_DEF& a, const MACRO_DEF& b) {
            if (a.prefix.empty() && b.prefix.empty())
                return a.value < b.value;
            if (a.prefix.empty())
                return false;
            if (b.prefix.empty())
                return true;
            if (a.prefix < b.prefix)
                return true;
            if (a.prefix > b.prefix)
                return false;
            if (a.definition.empty() && b.definition.empty())
                return false;
            if (a.definition.empty())
                return true;
            if (b.definition.empty())
                return false;
            if (a.definition[0] == '"' && b.definition[0] == '"')
                return a.definition < b.definition;
            if (a.definition[0] == '"')
                return false;
            if (b.definition[0] == '"')
                return true;
            if (a.value < b.value)
                return true;
            if (a.value > b.value)
                return false;
            return false;
        }
    );

    if (g_settings.bUseIDC_STATIC)
    {
        // write the macro definitions
        file.WriteSzA("\r\n");
        WriteMacroLine(file, "IDC_STATIC", "(-1)");
    }

    MStringA prefix;
    bool first = true;
    file.WriteFormatA("\r\n");
    for (auto& def : defs)
    {
        if (def.name == "IDC_STATIC")
            continue;

        if (!first && prefix != def.prefix)
            file.WriteFormatA("\r\n");

        WriteMacroLine(file, def.name, def.definition);

        prefix = def.prefix;
        first = false;
    }

    // do statistics about resource IDs
    UINT anValues[5];
    DoIDStat(anValues);

    // dump the statistics
    file.WriteFormatA("\r\n");
    file.WriteFormatA("#ifdef APSTUDIO_INVOKED\r\n");
    file.WriteFormatA("    #ifndef APSTUDIO_READONLY_SYMBOLS\r\n");
    file.WriteFormatA("        #define _APS_NO_MFC                 %u\r\n", anValues[0]);
    file.WriteFormatA("        #define _APS_NEXT_RESOURCE_VALUE    %u\r\n", anValues[1]);
    file.WriteFormatA("        #define _APS_NEXT_COMMAND_VALUE     %u\r\n", anValues[2]);
    file.WriteFormatA("        #define _APS_NEXT_CONTROL_VALUE     %u\r\n", anValues[3]);
    file.WriteFormatA("        #define _APS_NEXT_SYMED_VALUE       %u\r\n", anValues[4]);
    file.WriteFormatA("    #endif\r\n");
    file.WriteFormatA("#endif\r\n");

    return TRUE;
}

// write the resource.h file
BOOL MMainWnd::DoWriteResHOfExe(LPCWSTR pszExeFile)
{
    assert(pszExeFile);

    // pszExeFile --> szResH
    WCHAR szResH[MAX_PATH];
    StringCchCopyW(szResH, _countof(szResH), pszExeFile);

    // find the last '\\' or '/'
    LPWSTR pch = wcsrchr(szResH, L'\\');
    if (!pch)
        pch = wcsrchr(szResH, L'/');
    if (!pch)
        return FALSE;

    // build the "resource.h" file path
    ++pch;
    *pch = 0;
    StringCchCatW(szResH, _countof(szResH), L"resource.h");

    // write the "resource.h" file
    if (DoWriteResH(szResH))
    {
        // szResH --> m_szResourceH
        StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);
        return TRUE;
    }

    return FALSE;   // failure
}

// do statistics for resource IDs
void MMainWnd::DoIDStat(UINT anValues[5])
{
    const size_t count = 4;

    INT anNext[count];
    MString prefixes[count];
    prefixes[0] = g_settings.assoc_map[L"Resource.ID"];
    prefixes[1] = g_settings.assoc_map[L"Command.ID"];
    prefixes[2] = g_settings.assoc_map[L"New.Command.ID"];
    prefixes[3] = g_settings.assoc_map[L"Control.ID"];

    for (size_t i = 0; i < count; ++i)
    {
        auto table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefixes[i]);

        UINT nMax = 0;
        for (auto& table_entry : table)
        {
            if (table_entry.name == L"IDC_STATIC")
                continue;

            if (i == 3)
            {
                if (auto e = g_res.find(ET_LANG, RT_CURSOR, WORD(table_entry.value)))
                    continue;   // it was Cursor.ID, not Control.ID
            }

            if (nMax < table_entry.value)
                nMax = table_entry.value;
        }

        anNext[i] = nMax + 1;
    }

    anValues[0] = 1;
    anValues[1] = anNext[0];
#undef max
    anValues[2] = std::max(anNext[1], anNext[2]);
    anValues[3] = anNext[3];
    anValues[4] = 300;

    // fix for preferable values
    if (anValues[1] < 100)
        anValues[1] = 100;
    if (anValues[2] < 100)
        anValues[2] = 100;
    if (anValues[3] < 1000)
        anValues[3] = 1000;
}

// extract the resource data to a file
inline BOOL MMainWnd::DoExtract(const EntryBase *entry, BOOL bExporting)
{
    ResToText res2text;

    if (bExporting && g_settings.bStoreToResFolder)
    {
        // add "res\\" to the prefix if necessary
        res2text.m_strFilePrefix = L"res\\";
    }

    // get the entry file name
    MString filename = res2text.GetEntryFileName(*entry);
    if (filename.empty())
        return TRUE;        // no need to extract

    //MessageBox(NULL, filename.c_str(), NULL, 0);

    if (entry->m_type.is_int())
    {
        WORD wType = entry->m_type.m_id;
        if (wType == (WORD)(UINT_PTR)RT_CURSOR)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_BITMAP)
        {
            return PackedDIB_Extract(filename.c_str(), &(*entry)[0], entry->size(), FALSE);
        }
        if (wType == (WORD)(UINT_PTR)RT_ICON)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_MENU)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_DIALOG)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_STRING)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_FONTDIR)
        {
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_FONT)
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_ACCELERATOR)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_RCDATA)
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_MESSAGETABLE)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_GROUP_CURSOR)
        {
            return g_res.extract_cursor(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_GROUP_ICON)
        {
            return g_res.extract_icon(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_VERSION)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_DLGINIT)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_DLGINCLUDE)
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_PLUGPLAY)
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_VXD)
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_ANICURSOR)
        {
            return g_res.extract_cursor(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_ANIICON)
        {
            return g_res.extract_icon(filename.c_str(), entry);
        }
    }
    else
    {
        if (entry->m_type == L"AVI")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"PNG")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"GIF")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"JPEG")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"JPG")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"TIFF")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"TIF")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"EMF")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"ENHMETAFILE")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"WMF")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"WAVE")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
        if (entry->m_type == L"IMAGE")
        {
            return g_res.extract_bin(filename.c_str(), entry);
        }
    }

    return g_res.extract_bin(filename.c_str(), entry);
}

// do export the resource data to an RC file and related files
BOOL MMainWnd::DoExport(LPCWSTR pszRCFile, LPWSTR pszResHFile)
{
    if (g_res.empty())
    {
        // unable to export the empty data
        ErrorBoxDx(IDS_DATAISEMPTY);
        return FALSE;
    }

    // pszRCFile --> szPath
    WCHAR szPath[MAX_PATH];
    StringCchCopy(szPath, _countof(szPath), pszRCFile);

    // find the '\\' or '/' character
    WCHAR *pch = mstrrchr(szPath, L'\\');
    if (!pch)
        pch = mstrrchr(szPath, L'/');
    if (!pch)
        return FALSE;   // failure

    *pch = 0;

    // search the language entries
    EntrySetBase found;
    g_res.search(found, ET_LANG);

    // check whether there is an external file to be extracted
    BOOL bHasExternFile = FALSE;
    for (auto e : found)
    {
        ResToText res2text;
        MString filename = res2text.GetEntryFileName(*e);
        if (filename.size())
        {
            bHasExternFile = TRUE;
            break;
        }
    }

    // if g_settings.bStoreToResFolder is set, then check the folder is not empty
    if (!IsEmptyDirectoryDx(szPath))
    {
        if (bHasExternFile && !g_settings.bStoreToResFolder)
        {
            ErrorBoxDx(IDS_MUSTBEEMPTYDIR);
            return FALSE;
        }
    }

    // save the current directory and move the current directory
    WCHAR szCurDir[MAX_PATH];
    GetCurrentDirectory(_countof(szCurDir), szCurDir);
    if (!SetCurrentDirectory(szPath))
        return FALSE;

    if (bHasExternFile)
    {
        // create the "res" folder (with backuping) if necessary
        if (g_settings.bStoreToResFolder)
        {
            MString strResDir = szPath;
            strResDir += TEXT("\\res");

            if (g_settings.bBackup)
                DoBackupFolder(strResDir.c_str());

            CreateDirectory(strResDir.c_str(), NULL);
        }

        // search the language entries
        EntrySetBase found;
        g_res.search(found, ET_LANG);

        // extract each data if necessary
        for (auto e : found)
        {
            if (e->m_type == RT_STRING || e->m_type == RT_MESSAGETABLE ||
                e->m_type == RT_FONTDIR)
            {
                continue;
            }
            if (!DoExtract(e, TRUE))
                return FALSE;
        }
    }

    BOOL bOK = FALSE;
    if ((m_szResourceH[0] || !g_settings.IsIDMapEmpty()) &&
        !g_settings.bHideID)
    {
        // build the resource.h file path
        *pch = 0;
        StringCchCatW(szPath, _countof(szPath), L"\\resource.h");

        // write the resource.h file and the RC file
        bOK = DoWriteResH(szPath, pszRCFile) && DoWriteRC(pszRCFile, szPath);

        // szPath --> pszResHFile
        if (bOK && pszResHFile)
            StringCchCopyW(pszResHFile, MAX_PATH, szPath);
    }
    else
    {
        // write the RC file
        bOK = DoWriteRC(pszRCFile, NULL);
    }

    // resume the current directory
    SetCurrentDirectory(szCurDir);

    if (bOK)
        DoSetFileModified(FALSE);

    return bOK;
}

// save the resource data as a *.res file
BOOL MMainWnd::DoSaveResAs(LPCWSTR pszExeFile)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return FALSE;

    if (g_res.extract_res(pszExeFile, g_res))
    {
        UpdateFileInfo(FT_RES, pszExeFile, FALSE);
        DoSetFileModified(FALSE);
        return TRUE;
    }
    return FALSE;
}

// save the file
BOOL MMainWnd::DoSaveAs(LPCWSTR pszExeFile)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return TRUE;

    return DoSaveExeAs(pszExeFile);
}

BOOL MMainWnd::DoSaveAsCompression(LPCWSTR pszExeFile)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return TRUE;

    return DoSaveExeAs(pszExeFile, TRUE);
}

BOOL IsExeOrDll(LPCWSTR pszFileName)
{
    BYTE ab[2] = { 0, 0 };
    if (FILE *fp = _wfopen(pszFileName, L"rb"))
    {
        fread(ab, 2, 1, fp);
        fclose(fp);
    }

    if (ab[0] == 'M' && ab[1] == 'Z')
        return TRUE;
    if (ab[0] == 'P' && ab[1] == 'E')
        return TRUE;
    return FALSE;
}

BOOL IsDotExe(LPCWSTR pszFileName)
{
    return lstrcmpiW(PathFindExtensionW(pszFileName), L".exe") == 0;
}

BOOL DumpTinyExeOrDll(HINSTANCE hInst, LPCWSTR pszFileName, INT nID)
{
    if (HRSRC hRsrc = FindResourceW(hInst, MAKEINTRESOURCEW(nID), RT_RCDATA))
    {
        if (HGLOBAL hGlobal = LoadResource(hInst, hRsrc))
        {
            if (LPVOID pvData = LockResource(hGlobal))
            {
                DWORD cbData = SizeofResource(hInst, hRsrc);
                if (FILE *fp = _wfopen(pszFileName, L"wb"))
                {
                    int nOK = fwrite(pvData, cbData, 1, fp);
                    fclose(fp);

                    return !!nOK;
                }
            }
        }
    }
    return FALSE;
}

BOOL MMainWnd::DoSaveInner(LPCWSTR pszExeFile, BOOL bCompression)
{
    if (!g_res.update_exe(pszExeFile))
    {
        return FALSE;
    }

    // update file info
    UpdateFileInfo(FT_EXECUTABLE, pszExeFile, m_bUpxCompressed);

    // do compress by UPX
    if (g_settings.bCompressByUPX || bCompression)
    {
        DoUpxCompress(m_szUpxExe, pszExeFile);
    }

    // is there any resource ID?
    if (m_szResourceH[0] || !g_settings.id_map.empty())
    {
        // query
        if (MsgBoxDx(IDS_WANNAGENRESH, MB_ICONINFORMATION | MB_YESNO) == IDYES)
        {
            // write the resource.h file
            return DoWriteResHOfExe(pszExeFile);
        }
    }

    DoSetFileModified(FALSE);

    return TRUE;    // success
}

struct AutoDeleteFile
{
    LPCWSTR m_file;
    AutoDeleteFile(LPCWSTR file) : m_file(file)
    {
    }
    ~AutoDeleteFile()
    {
        DeleteFileW(m_file);
    }
};

// open the dialog to save the EXE file
BOOL MMainWnd::DoSaveExeAs(LPCWSTR pszExeFile, BOOL bCompression)
{
    LPCWSTR src = m_szFile;
    LPCWSTR dest = pszExeFile;
    WCHAR szTempFile[MAX_PATH] = L"";
    AutoDeleteFile auto_delete(szTempFile);

    // check not locking
    if (IsFileLockedDx(dest))
    {
        WCHAR szMsg[MAX_PATH + 256];
        StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), dest);
        ErrorBoxDx(szMsg);
        return FALSE;
    }

    // is the file compressed?
    if (m_bUpxCompressed)
    {
        // build a temporary file path
        StringCchCopyW(szTempFile, _countof(szTempFile), GetTempFileNameDx(L"UPX"));

        // src --> szTempFile (decompressed)
        if (!CopyFileW(src, szTempFile, FALSE) ||
            !DoUpxDecompress(m_szUpxExe, szTempFile))
        {
            ErrorBoxDx(IDS_CANTUPXEXTRACT);
            return FALSE;   // failure
        }

        src = szTempFile;
    }

    // do backup the dest
    if (g_settings.bBackup)
    {
        DoBackupFile(dest);
    }

    // check whether it is an executable or not
    BOOL bSrcExecutable = IsExeOrDll(src);
    BOOL bDestExecutable = IsExeOrDll(dest);

    if (bSrcExecutable)
    {
        // copy src to dest (if src and dest are not same), then update resource
        if (lstrcmpiW(src, dest) == 0 ||
            CopyFileW(src, dest, FALSE))
        {
            return DoSaveInner(dest, bCompression);
        }
    }
    else if (bDestExecutable)
    {
        // src is not exe and dest exe is respected
        return DoSaveInner(dest, bCompression);
    }
    else
    {
        // if src and dest are non-executable, then dump tiny exe or dll to dest
        if (IsDotExe(dest))
        {
            if (DumpTinyExeOrDll(m_hInst, dest, IDR_TINYEXE))
            {
                return DoSaveInner(dest, bCompression);
            }
        }
        else
        {
            if (DumpTinyExeOrDll(m_hInst, dest, IDR_TINYDLL))
            {
                return DoSaveInner(dest, bCompression);
            }
        }
    }

    return FALSE;   // failure
}

// do compress the file by UPX.exe
BOOL MMainWnd::DoUpxCompress(LPCWSTR pszUpx, LPCWSTR pszExeFile)
{
    // build the command line
    MStringW strCmdLine;
    strCmdLine += L"\"";
    strCmdLine += pszUpx;
    strCmdLine += L"\" -9 \"";
    strCmdLine += pszExeFile;
    strCmdLine += L"\"";
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    // create a UPX process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all output
        MStringA strOutput;
        bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
        pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

        if (pmaker.GetExitCode() == 0 && bOK)
        {
            bOK = (strOutput.find("Packed") != MStringA::npos);
        }
    }

    return bOK;
}

IMPORT_RESULT MMainWnd::DoImportRes(HWND hwnd, LPCWSTR pszFile)
{
    // do import to the res variable
    EntrySet res;
    if (!res.import_res(pszFile))
    {
        return IMPORT_FAILED;
    }

    // is it overlapped?
    if (g_res.intersect(res))
    {
        // query overwrite
        INT nID = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
        switch (nID)
        {
        case IDYES:
            // delete the overlapped entries
            for (auto entry : res)
            {
                if (entry->m_et != ET_LANG)
                    continue;

                g_res.search_and_delete(ET_LANG, entry->m_type, entry->m_name, entry->m_lang);
            }
            break;

        case IDNO:
        case IDCANCEL:
            // clean up
            res.delete_all();
            return IMPORT_CANCELLED;
        }
    }

    // load it now
    m_bLoading = TRUE;
    {
        // renewal
        g_res.merge(res);

        // clean up
        res.delete_all();
    }
    m_bLoading = FALSE;

    // refresh the ID list window
    DoRefreshIDList(hwnd);

    return IMPORTED;
}

IMPORT_RESULT MMainWnd::DoImport(HWND hwnd, LPCWSTR pszFile, LPCWSTR pchDotExt)
{
    if (!pchDotExt)
        return NOT_IMPORTABLE;

    if (lstrcmpiW(pchDotExt, L".res") == 0)
    {
        return DoImportRes(hwnd, pszFile);
    }
    else if (lstrcmpiW(pchDotExt, L".ico") == 0)
    {
        // show the dialog
        MAddIconDlg dialog;
        dialog.m_file = pszFile;
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // refresh the ID list window
            DoRefreshIDList(hwnd);

            // select the entry
            SelectTV(ET_LANG, dialog, FALSE);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".cur") == 0 || lstrcmpiW(pchDotExt, L".ani") == 0)
    {
        // show the dialog
        MAddCursorDlg dialog;
        dialog.m_file = pszFile;
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // refresh the ID list window
            DoRefreshIDList(hwnd);

            // select the entry
            SelectTV(ET_LANG, dialog, FALSE);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".wav") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_type = L"WAVE";
        dialog.m_file = pszFile;
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".bmp") == 0 || lstrcmpiW(pchDotExt, L".dib") == 0)
    {
        // show the dialog
        MAddBitmapDlg dialog;
        dialog.m_file = pszFile;
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // refresh the ID list window
            DoRefreshIDList(hwnd);

            // select the entry
            SelectTV(ET_LANG, dialog, FALSE);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".png") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_file = pszFile;
        dialog.m_type = L"PNG";
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".gif") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_file = pszFile;
        dialog.m_type = L"GIF";
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".jpg") == 0 || lstrcmpiW(pchDotExt, L".jpeg") == 0 ||
             lstrcmpiW(pchDotExt, L".jpe") == 0 || lstrcmpiW(pchDotExt, L".jfif") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_file = pszFile;
        dialog.m_type = L"JPEG";
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".tif") == 0 || lstrcmpiW(pchDotExt, L".tiff") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_file = pszFile;
        dialog.m_type = L"TIFF";
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".avi") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_file = pszFile;
        dialog.m_type = L"AVI";
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".wmf") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_file = pszFile;
        dialog.m_type = L"WMF";
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".emf") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_file = pszFile;
        dialog.m_type = L"EMF";
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }
    else if (lstrcmpiW(pchDotExt, L".dfm") == 0)
    {
        // show the dialog
        MAddResDlg dialog;
        dialog.m_file = pszFile;
        dialog.m_type = RT_RCDATA;
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // add a resource item
            DoAddRes(hwnd, dialog);

            return IMPORTED;
        }
        return IMPORT_CANCELLED;
    }

    return NOT_IMPORTABLE;
}

// WM_DROPFILES: file(s) has been dropped
void MMainWnd::OnDropFiles(HWND hwnd, HDROP hdrop)
{
    MWaitCursor wait;
    ChangeStatusText(IDS_EXECUTINGCMD);     // executing command

    // add the command lock
    ++m_nCommandLock;

    WCHAR file[MAX_PATH], *pch;

    // get the dropped file path
    DragQueryFileW(hdrop, 0, file, _countof(file));

    // free hdrop
    DragFinish(hdrop);

    // make the window foreground
    SetForegroundWindow(hwnd);

    // find the file title
    pch = wcsrchr(file, L'\\');
    if (pch == NULL)
        pch = wcsrchr(file, L'/');
    if (pch == NULL)
        pch = file;
    else
        ++pch;

    // find the dot extension
    pch = wcsrchr(pch, L'.');

    IMPORT_RESULT result = NOT_IMPORTABLE;
    if (pch)
    {
        result = DoImport(hwnd, file, pch);
    }

    if (result == IMPORTED || result == IMPORT_CANCELLED)
    {
        // do nothing
    }
    else if (result == IMPORT_FAILED)
    {
        ErrorBoxDx(IDS_CANNOTIMPORT);
    }
    else if (pch && lstrcmpiW(pch, L".h") == 0)
    {
        // unload the resource.h file
        UnloadResourceH(hwnd);

        CheckResourceH(hwnd, file);
        if (m_szResourceH[0] && g_settings.bAutoShowIDList)
        {
            ShowIDList(hwnd, TRUE);
        }

        // update the names
        UpdateNames();
    }
    else
    {
        if (!DoQuerySaveChange(hwnd))
            return;

        // otherwise, load the file
        DoLoadFile(hwnd, file);
    }

    // remove the command lock
    --m_nCommandLock;

    // show "ready" if just unlocked
    if (m_nCommandLock == 0 && !::IsWindow(m_rad_window))
        ChangeStatusText(IDS_READY);
}

// open the dialog to load the resource.h
void MMainWnd::OnLoadResH(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
    {
        return;
    }

    // (resource.h file path) --> szFile
    WCHAR szFile[MAX_PATH];
    if (m_szResourceH[0])
        StringCchCopyW(szFile, _countof(szFile), m_szResourceH);
    else
        StringCchCopyW(szFile, _countof(szFile), L"resource.h");

    // if it does not exist, clear the file path
    if (!PathFileExistsW(szFile))
        szFile[0] = 0;

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_LOADRESH);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"h";     // the default extension

    // let the user choose the path
    if (GetOpenFileNameW(&ofn))
    {
        // load the resource.h file
        DoLoadResH(hwnd, szFile);

        // is the resource.h file loaded?
        if (m_szResourceH[0])
        {
            // show the ID list window
            ShowIDList(hwnd, TRUE);
        }

        // update the names
        UpdateNames();
    }
}

// load the resource.h file
void MMainWnd::OnLoadResHBang(HWND hwnd)
{
    if (m_szResourceH[0])
    {
        MString strFile = m_szResourceH;
        DoLoadResH(hwnd, strFile.c_str());

        if (m_szResourceH[0])
        {
            ShowIDList(hwnd, TRUE);
        }
    }
}

void MMainWnd::OnClose(HWND hwnd)
{
    if (DoQuerySaveChange(hwnd))
        DestroyWindow(hwnd);
}

// WM_DESTROY: the main window has been destroyed
void MMainWnd::OnDestroy(HWND hwnd)
{
    // close preview
    HidePreview();

    // unload the resource.h file
    OnUnloadResH(hwnd);

    // update the file info
    UpdateFileInfo(FT_NONE, NULL, FALSE);

    // unselect
    SelectTV(NULL, FALSE);

    // clean up
    g_res.delete_all();

    // save the settings
    SaveSettings(hwnd);

    //DestroyIcon(m_hIcon);     // LR_SHARED
    DestroyIcon(m_hIconSm);
    DestroyAcceleratorTable(m_hAccel);
    ImageList_Destroy(m_hImageList);
    ImageList_Destroy(m_himlTools);
    DestroyIcon(m_hFileIcon);
    DestroyIcon(m_hFolderIcon);
    DeleteObject(m_hSrcFont);
    DeleteObject(m_hBinFont);

    //DestroyIcon(MRadCtrl::Icon());    // LR_SHARED
    DeleteObject(MRadCtrl::Bitmap());
    DestroyCursor(MSplitterWnd::CursorNS());
    DestroyCursor(MSplitterWnd::CursorWE());

    // clean up
    g_res.delete_all();
    g_res.delete_invalid();

    // destroy the window's
    DestroyWindow(m_rad_window);
    DestroyWindow(m_hBinEdit);
    DestroyWindow(m_hSrcEdit);
    m_hBmpView.DestroyView();
    DestroyWindow(m_hBmpView);
    DestroyWindow(m_id_list_dlg);

    DestroyWindow(m_hwndTV);
    DestroyWindow(m_hToolBar);
    DestroyWindow(m_hStatusBar);
    DestroyWindow(m_hFindReplaceDlg);

    DestroyWindow(m_splitter1);
    DestroyWindow(m_splitter2);
    DestroyWindow(m_splitter3);

    s_hMainWnd = NULL;

    // post WM_QUIT message to quit the application
    PostQuitMessage(0);
}

// parse the macros
BOOL MMainWnd::ParseMacros(HWND hwnd, LPCTSTR pszFile,
                           const std::vector<MStringA>& macros, MStringA& str)
{
    // split text to lines by "\n"
    std::vector<MStringA> lines;
    mstr_trim(str);
    mstr_split(lines, str, "\n");

    // erase the first line (the special pragma)
    lines.erase(lines.begin());

    // check the line count
    size_t line_count = lines.size();
    if (macros.size() < line_count)
        line_count = macros.size();

    for (size_t i = 0; i < line_count; ++i)
    {
        auto& macro = macros[i];
        auto& line = lines[i];

        // scan the line lexically
        using namespace MacroParser;
        StringScanner scanner(line);

        // tokenize it
        MacroParser::TokenStream stream(scanner);
        stream.read_tokens();

        // and parse it
        Parser parser(stream);
        if (parser.parse())     // successful
        {
            if (is_str(parser.ast()))
            {
                // it's a string value macro
                string_type value;
                if (eval_string(parser.ast(), value))   // evaluate
                {
                    // add an ID mapping's pair
                    g_settings.id_map[macro] = value;
                }
            }
            else
            {
                // it's an integer value macro
                int value = 0;
                if (eval_int(parser.ast(), value))  // evaluate
                {
                    // convert to a string by base m_id_list_dlg.m_nBase
                    char sz[32];
                    if (m_id_list_dlg.m_nBase == 16)
                        StringCchPrintfA(sz, _countof(sz), "0x%X", value);
                    else
                        StringCchPrintfA(sz, _countof(sz), "%d", value);

                    // ignore some special macros
                    if (macro != "WIN32" && macro != "WINNT" && macro != "i386")
                    {
                        // add an ID mapping's pair
                        g_settings.id_map[macro] = sz;
                    }
                }
            }
        }
    }

    // clear the resource IDs in the constants database
    auto& table = g_db.m_map[L"RESOURCE.ID"];
    table.clear();

    // add the resource ID entries to the "RESOURCE.ID" table from the ID mapping
    for (auto& pair : g_settings.id_map)
    {
        MStringW str1 = MAnsiToWide(CP_ACP, pair.first).c_str();
        MStringW str2 = MAnsiToWide(CP_ACP, pair.second).c_str();
        DWORD value2 = mstr_parse_int(str2.c_str());
        ConstantsDB::EntryType entry(str1, value2);
        table.push_back(entry);
    }

    // add IDC_STATIC macro
    g_settings.AddIDC_STATIC();

    // save the resource.h file location to m_szResourceH
    StringCchCopyW(m_szResourceH, _countof(m_szResourceH), pszFile);

    return TRUE;
}

// parse the resource.h file
BOOL MMainWnd::ParseResH(HWND hwnd, LPCTSTR pszFile, const char *psz, DWORD len)
{
    // split text to lines by "\n"
    MStringA str(psz, len);
    std::vector<MStringA> lines, macros;
    mstr_split(lines, str, "\n");

    for (auto& line : lines)
    {
        // trim
        mstr_trim(line);
        if (line.empty())
            continue;   // empty ones are ignored

        // the macro that begins with "_" will be ignored
        if (line.find("#define _") != MStringA::npos)
            continue;

        // find "#define "
        size_t found0 = line.find("#define");
        if (found0 == MStringA::npos)
            continue;

        // parse the line
        line = line.substr(strlen("#define"));
        mstr_trim(line);
        size_t found1 = line.find_first_of(" \t");
        size_t found2 = line.find('(');
        if (found1 == MStringA::npos)
            continue;   // without space is an invalid #define
        if (found2 != MStringA::npos && found2 < found1)
            continue;   // we ignore the function-like macros

        // push the macro
        macros.push_back(line.substr(0, found1));
    }

    g_settings.id_map.clear();

    if (macros.empty())
    {
        // no macros to 
        return TRUE;
    }

    // (new temporary file path) --> szTempFile1
    WCHAR szTempFile1[MAX_PATH];
    StringCchCopyW(szTempFile1, _countof(szTempFile1), GetTempFileNameDx(L"R1"));

    // create the temporary file
    MFile file1(szTempFile1, TRUE);

    // pszFile --> szFile
    WCHAR szFile[MAX_PATH];
    StringCchCopyW(szFile, _countof(szFile), pszFile);

    // write the heading
    DWORD cbWritten;
    file1.WriteSzA("#include \"", &cbWritten);
    file1.WriteSzA(MTextToAnsi(CP_ACP, szFile).c_str(), &cbWritten);
    file1.WriteSzA("\"\n", &cbWritten);
    file1.WriteSzA("#pragma RisohEditor\n", &cbWritten);    // the special pragma

    // write the macro names (in order to retrieve the value after)
    char buf[MAX_PATH + 64];
    for (size_t i = 0; i < macros.size(); ++i)
    {
        StringCchPrintfA(buf, _countof(buf), "%s\n", macros[i].c_str());
        file1.WriteSzA(buf, &cbWritten);
    }
    file1.CloseHandle();    // close the handle

    // build the command line text
    MString strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szMCppExe;       // mcpp.exe
    strCmdLine += L"\" ";
    strCmdLine += GetIncludesDump();
    strCmdLine += GetMacroDump();
    strCmdLine += L" \"";
    strCmdLine += szTempFile1;
    strCmdLine += L'\"';
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    // wait for the file operation
    Sleep(FILE_WAIT_TIME);

    // create a cpp.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MStringA strOutput;
    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
        pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

        if (bOK)
        {
            bOK = FALSE;

            // find the special pragma
            size_t pragma_found = strOutput.find("#pragma RisohEditor");
            if (pragma_found != MStringA::npos)
            {
                // get text after the special pragma
                strOutput = strOutput.substr(pragma_found);

                // parse macros
                bOK = ParseMacros(hwnd, pszFile, macros, strOutput);
            }
        }
    }

#ifdef NDEBUG
    // delete the temporary file
    DeleteFileW(szTempFile1);
#endif

    return bOK;
}

// load the resource.h
BOOL MMainWnd::DoLoadResH(HWND hwnd, LPCTSTR pszFile)
{
    // unload the resource.h file
    UnloadResourceH(hwnd);

    // (new temporary file path) --> szTempFile
    WCHAR szTempFile[MAX_PATH];
    StringCchCopyW(szTempFile, _countof(szTempFile), GetTempFileNameDx(L"R1"));

    // create a temporary file
    MFile file(szTempFile, TRUE);
    file.CloseHandle();     // close the handle

    // build a command line
    MString strCmdLine;
    strCmdLine += L'"';
    strCmdLine += m_szMCppExe;
    strCmdLine += L"\" -dM -DRC_INVOKED -o \"";
    strCmdLine += szTempFile;
    strCmdLine += L"\" -I \"";
    strCmdLine += m_szIncludeDir;
    strCmdLine += L"\" \"";
    strCmdLine += pszFile;
    strCmdLine += L"\"";
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    // create a cpp.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MStringA strOutput;
    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
        pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

        if (bOK && pmaker.GetExitCode() == 0)
        {
            // read all from szTempFile
            MStringA data;
            MFile file(szTempFile);
            bOK = file.ReadAll(data);
            file.CloseHandle();

            if (bOK)
            {
                // parse the resource.h
                bOK = ParseResH(hwnd, pszFile, &data[0], DWORD(data.size()));
            }
        }
    }

#ifdef NDEBUG
    // delete the temporary file
    DeleteFileW(szTempFile);
#endif

    return bOK;
}

// refresh the ID list window
void MMainWnd::DoRefreshIDList(HWND hwnd)
{
    ShowIDList(hwnd, IsWindow(m_id_list_dlg));
}

// refresh the treeview
void MMainWnd::DoRefreshTV(HWND hwnd)
{
    // refresh the resource items
    EntrySet res;
    res.merge(g_res);
    g_res.delete_all();
    g_res.merge(res);

    // clean up
    res.delete_all();

    // redraw
    InvalidateRect(m_hwndTV, NULL, TRUE);
}

// advice the change for resource.h file
void MMainWnd::OnAdviceResH(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    MString str;

    if (!g_settings.removed_ids.empty() &&
        (g_settings.removed_ids.size() != 1 ||
         g_settings.removed_ids.find("IDC_STATIC") == g_settings.removed_ids.end()))
    {
        str += LoadStringDx(IDS_DELETENEXTIDS);

        for (auto& pair : g_settings.removed_ids)
        {
            if (pair.first == "IDC_STATIC")
                continue;

            str += TEXT("#define ");
            str += MAnsiToText(CP_ACP, pair.first).c_str();
            str += TEXT(" ");
            str += MAnsiToText(CP_ACP, pair.second).c_str();
            str += TEXT("\r\n");
        }
        str += TEXT("\r\n");
    }

    if (!g_settings.added_ids.empty() &&
        (g_settings.added_ids.size() != 1 ||
         g_settings.added_ids.find("IDC_STATIC") == g_settings.added_ids.end() ||
         !g_settings.bUseIDC_STATIC))
    {
        str += LoadStringDx(IDS_ADDNEXTIDS);

        for (auto& pair : g_settings.added_ids)
        {
            str += TEXT("#define ");
            str += MAnsiToText(CP_ACP, pair.first).c_str();
            str += TEXT(" ");
            str += MAnsiToText(CP_ACP, pair.second).c_str();
            str += TEXT("\r\n");
        }
        str += TEXT("\r\n");
    }

    if (str.empty())
    {
        str += LoadStringDx(IDS_NOCHANGE);
        str += TEXT("\r\n");
    }

    // show the dialog
    MAdviceResHDlg dialog(str);
    dialog.DialogBoxDx(hwnd);
}

// unload the resource.h info
void MMainWnd::OnUnloadResH(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    // unload the resource.h file
    UnloadResourceH(hwnd);
}

// the configuration dialog
void MMainWnd::OnConfig(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MConfigDlg dialog;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // refresh PATHs
        ReSetPaths(hwnd);

        // update word wrapping
        ReCreateSrcEdit(hwnd);

        // update the fonts
        ReCreateFonts(hwnd);

        // update the labels of the entries
        UpdateNames();

        // select the entry to update the text
        auto entry = g_res.get_entry();
        SelectTV(entry, FALSE);
    }
}

// reset the path settings
void MMainWnd::ReSetPaths(HWND hwnd)
{
    // windres.exe
    if (g_settings.strWindResExe.size())
    {
        // g_settings.strWindResExe --> m_szWindresExe
        StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), g_settings.strWindResExe.c_str());
    }
    else
    {
        // g_settings.m_szDataFolder + L"\\bin\\windres.exe" --> m_szWindresExe
        StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
        StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");
    }

    // cpp.exe
    if (g_settings.strCppExe.size())
    {
        // g_settings.strCppExe --> m_szMCppExe
        StringCchCopy(m_szMCppExe, _countof(m_szMCppExe), g_settings.strCppExe.c_str());
    }
    else
    {
        // m_szDataFolder + "\\bin\\cpp.exe" --> m_szMCppExe
        StringCchCopyW(m_szMCppExe, _countof(m_szMCppExe), m_szDataFolder);
        StringCchCatW(m_szMCppExe, _countof(m_szMCppExe), L"\\bin\\mcpp.exe");
    }
}

// use IDC_STATIC macro or not
void MMainWnd::OnUseIDC_STATIC(HWND hwnd)
{
    // toggle the flag
    g_settings.bUseIDC_STATIC = !g_settings.bUseIDC_STATIC;

    // select the entry to update the text
    auto entry = g_res.get_entry();
    SelectTV(entry, FALSE);
}

// update the name of the tree control
void MMainWnd::UpdateNames(void)
{
    EntrySetBase found;
    g_res.search(found, ET_NAME);

    for (auto entry : found)
    {
        UpdateEntryName(entry);
    }

    auto entry = g_res.get_entry();
    SelectTV(entry, FALSE);

    DoSetFileModified(TRUE);
}

void MMainWnd::UpdateEntryName(EntryBase *e, LPWSTR pszText)
{
    // update name label
    e->m_strLabel = e->get_name_label();

    // set the label text
    TV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TVIF_TEXT | TVIF_HANDLE;
    item.hItem = e->m_hItem;
    item.pszText = &e->m_strLabel[0];
    TreeView_SetItem(m_hwndTV, &item);

    // update pszText if any
    if (pszText)
        StringCchCopyW(pszText, MAX_PATH, item.pszText);

    DoSetFileModified(TRUE);
}

void MMainWnd::UpdateEntryLang(EntryBase *e, LPWSTR pszText)
{
    // update lang label
    e->m_strLabel = e->get_lang_label();

    // set the label text
    TV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TVIF_TEXT | TVIF_HANDLE;
    item.hItem = e->m_hItem;
    item.pszText = &e->m_strLabel[0];
    TreeView_SetItem(m_hwndTV, &item);

    // update pszText if any
    if (pszText)
        StringCchCopyW(pszText, MAX_PATH, item.pszText);

    DoSetFileModified(TRUE);
}

// show/hide the ID macros
void MMainWnd::OnHideIDMacros(HWND hwnd)
{
    BOOL bListOpen = IsWindow(m_id_list_dlg);

    // toggle the flag
    g_settings.bHideID = !g_settings.bHideID;

    UpdateNames();

    ShowIDList(hwnd, bListOpen);

    // select the entry to update the text
    auto entry = g_res.get_entry();
    SelectTV(entry, FALSE);
}

// show/hide the ID list window
void MMainWnd::ShowIDList(HWND hwnd, BOOL bShow/* = TRUE*/)
{
    if (bShow)
    {
        if (IsWindow(m_id_list_dlg))
            DestroyWindow(m_id_list_dlg);
        m_id_list_dlg.CreateDialogDx(hwnd);
        ShowWindow(m_id_list_dlg, SW_SHOWNOACTIVATE);
        UpdateWindow(m_id_list_dlg);
    }
    else
    {
        ShowWindow(m_id_list_dlg, SW_HIDE);
        DestroyWindow(m_id_list_dlg);
    }
}

// show the ID list window
void MMainWnd::OnIDList(HWND hwnd)
{
    ShowIDList(hwnd, TRUE);
}

// show the ID association dialog
void MMainWnd::OnIdAssoc(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MIdAssocDlg dialog;
    dialog.DialogBoxDx(hwnd);

    // update the prefix database
    UpdatePrefixDB(hwnd);
}

// show the language list
void MMainWnd::OnShowLangs(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MLangsDlg dialog;
    dialog.DialogBoxDx(hwnd);
}

// show/hide the toolbar
void MMainWnd::OnShowHideToolBar(HWND hwnd)
{
    // toggle the flag
    g_settings.bShowToolBar = !g_settings.bShowToolBar;

    if (g_settings.bShowToolBar)
        ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hToolBar, SW_HIDE);

    // recalculate the splitter
    PostMessageDx(WM_SIZE);
}

// the paths dialog
void MMainWnd::OnSetPaths(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MPathsDlg dialog;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        ReSetPaths(hwnd);
    }
}

// start changing the resource name/language
void MMainWnd::OnEditLabel(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected type entry
    auto entry = g_res.get_entry();
    if (!entry || entry->m_et == ET_TYPE)
    {
        return;
    }

    if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
    {
        if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
        {
            return;
        }
    }

    HTREEITEM hItem = TreeView_GetSelection(m_hwndTV);
    TreeView_EditLabel(m_hwndTV, hItem);
}

// the predefined macro dialog
void MMainWnd::OnPredefMacros(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MMacrosDlg dialog;
    INT_PTR nID = dialog.DialogBoxDx(hwnd);
    switch (INT(nID))
    {
    case IDOK:
        g_settings.macros = dialog.m_map;
        break;
    case psh6:
        g_settings.ResetMacros();
        break;
    }
}

// expand all the tree control items
void MMainWnd::OnExpandAll(HWND hwnd)
{
    // get the selected entry
    auto entry = g_res.get_entry();

    HTREEITEM hItem = TreeView_GetRoot(m_hwndTV);
    do
    {
        Expand(hItem);
        hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
    } while (hItem);

    // select the entry
    SelectTV(entry, FALSE);
}

// unexpand all the tree control items
void MMainWnd::OnCollapseAll(HWND hwnd)
{
    HTREEITEM hItem = TreeView_GetRoot(m_hwndTV);
    do
    {
        Collapse(hItem);
        hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
    } while (hItem);

    SelectTV(NULL, FALSE);
}

void MMainWnd::OnSrcEditSelect(HWND hwnd)
{
    INT iItem = m_hSrcEdit.m_iItemToBeSelected;
    if (iItem != -1)
    {
        MRadCtrl::DeselectSelection();
        MRadCtrl::SelectByIndex(iItem);

        m_hSrcEdit.m_iItemToBeSelected = -1;
    }
}

void MMainWnd::OnAddBang(HWND hwnd, NMTOOLBAR *pToolBar)
{
    // TODO: If you edited "Edit" menu, then you may have to update the below codes
    HMENU hMenu = GetMenu(hwnd);
    HMENU hEditMenu = GetSubMenu(hMenu, 1);
    HMENU hAddMenu = GetSubMenu(hEditMenu, 2);

    // the button rectangle
    RECT rcItem = pToolBar->rcButton;

    // get the hot point
    POINT pt;
    pt.x = rcItem.left;
    pt.y = rcItem.bottom;
    ClientToScreen(m_hToolBar, &pt);

    // get the monitor info
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMon, &mi);

    // by the hot point, change the menu alignment
    UINT uFlags = TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL;
    if (pt.y >= (mi.rcWork.top + mi.rcWork.bottom) / 2)
    {
        uFlags |= TPM_BOTTOMALIGN;
        pt.x = rcItem.left;
        pt.y = rcItem.top;
        ClientToScreen(m_hToolBar, &pt);
    }
    else
    {
        uFlags |= TPM_TOPALIGN;
    }

    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
    SetForegroundWindow(m_hwnd);

    // show the popup menu
    TPMPARAMS params;
    ZeroMemory(&params, sizeof(params));
    params.cbSize = sizeof(params);
    params.rcExclude = rcItem;
    TrackPopupMenuEx(hAddMenu, uFlags, pt.x, pt.y, m_hwnd, &params);

    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
    SendMessageDx(WM_NULL);
}

void MMainWnd::OnClone(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    auto entry = g_res.get_entry();
    if (!entry)
        return;

    switch (entry->m_et)
    {
    case ET_TYPE:
        break;

    case ET_NAME:
        OnCopyAsNewName(hwnd);
        break;

    case ET_LANG:
    case ET_STRING:
    case ET_MESSAGE:
        OnCopyAsNewLang(hwnd);
        break;

    default:
        break;
    }
}

void MMainWnd::OnJumpToMatome(HWND hwnd)
{
    static const WCHAR szURL[] =
        L"https://katahiromz.web.fc2.com/colony3rd/risoheditor";

    ShellExecuteW(hwnd, NULL, szURL, NULL, NULL, SW_SHOWNORMAL);
}

void MMainWnd::OnEncoding(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    MEncodingDlg dialog;
    if (IDOK == dialog.DialogBoxDx(hwnd))
    {
        // select the entry
        auto entry = g_res.get_entry();
        SelectTV(entry, FALSE);
    }
}

void MMainWnd::OnQueryConstant(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    MConstantDlg dialog;
    dialog.DialogBoxDx(hwnd);
}

void MMainWnd::OnExtractBang(HWND hwnd)
{
    auto entry = g_res.get_entry();
    if (!entry)
        return;

    switch (entry->m_et)
    {
    case ET_TYPE:
    case ET_NAME:
    case ET_STRING:
    case ET_MESSAGE:
        OnExtractBin(hwnd);
        break;

    case ET_LANG:
        if (entry->m_type == RT_ICON || entry->m_type == RT_GROUP_ICON ||
            entry->m_type == RT_ANIICON)
        {
            OnExtractIcon(hwnd);
        }
        else if (entry->m_type == RT_CURSOR || entry->m_type == RT_GROUP_CURSOR ||
                 entry->m_type == RT_ANICURSOR)
        {
            OnExtractCursor(hwnd);
        }
        else if (entry->m_type == RT_BITMAP)
        {
            OnExtractBitmap(hwnd);
        }
        else if (entry->m_type == RT_RCDATA && entry->is_delphi_dfm())
        {
            OnExtractDFM(hwnd);
        }
        else
        {
            OnExtractBin(hwnd);
        }
        break;

    default:
        break;
    }
}

void MMainWnd::OnSaveAsWithCompression(HWND hwnd)
{
    enum ResFileFilterIndex     // see also: IDS_EXEFILTER
    {
        RFFI2_NONE = 0,
        RFFI2_EXECUTABLE = 1,
        RFFI2_ALL = 2
    };

    // compile if necessary
    if (!CompileIfNecessary(TRUE))
        return;

    // store m_szFile to szFile
    WCHAR szFile[MAX_PATH];
    StringCchCopyW(szFile, _countof(szFile), m_szFile);

    // if not found, then make it empty
    if (!PathFileExistsW(szFile))
        szFile[0] = 0;

    // initialize OPENFILENAME structure
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXEFILTER));

    // use the prefered filter by the entry
    ofn.nFilterIndex = RFFI2_EXECUTABLE;

    // use the preferred extension
    WCHAR szExt[32];
    LPWSTR pchDotExt = PathFindExtensionW(m_szFile);
    if (pchDotExt && *pchDotExt == L'.')
    {
        StringCbCopyW(szExt, sizeof(szExt), pchDotExt + 1);
        ofn.lpstrDefExt = szExt;
    }
    else
    {
        ofn.lpstrDefExt = L"exe";       // the default extension
    }

    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_SAVEASCOMPRESS);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    // let the user choose the path
    if (GetSaveFileNameW(&ofn))
    {
        // save it
        if (!DoSaveAsCompression(szFile))
        {
            ErrorBoxDx(IDS_CANNOTSAVE);
        }
    }
}

// toggle the word wrapping of the source EDIT control
void MMainWnd::OnWordWrap(HWND hwnd)
{
    // switch the flag
    g_settings.bWordWrap = !g_settings.bWordWrap;

    // create the source EDIT control
    ReCreateSrcEdit(hwnd);

    // reset fonts
    ReCreateFonts(hwnd);

    // select the entry to refresh
    auto entry = g_res.get_entry();
    SelectTV(entry, FALSE);
}

void MMainWnd::OnUseBeginEnd(HWND hwnd)
{
    g_settings.bUseBeginEnd = !g_settings.bUseBeginEnd;

    // create the source EDIT control
    ReCreateSrcEdit(hwnd);

    // reset fonts
    ReCreateFonts(hwnd);

    // select the entry to refresh
    auto entry = g_res.get_entry();
    SelectTV(entry, FALSE);
}

// expand the treeview items
void MMainWnd::Expand(HTREEITEM hItem)
{
    TreeView_Expand(m_hwndTV, hItem, TVE_EXPAND);
    hItem = TreeView_GetChild(m_hwndTV, hItem);
    if (hItem == NULL)
        return;
    do
    {
        Expand(hItem);
        hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
    } while (hItem);
}

// unexpand the treeview items
void MMainWnd::Collapse(HTREEITEM hItem)
{
    TreeView_Expand(m_hwndTV, hItem, TVE_COLLAPSE);
    hItem = TreeView_GetChild(m_hwndTV, hItem);
    if (hItem == NULL)
        return;
    do
    {
        Collapse(hItem);
        hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
    } while (hItem);
}

// WM_COMMAND
void MMainWnd::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    MWaitCursor wait;

    if (codeNotify == EN_CHANGE && m_hSrcEdit == hwndCtl)
    {
        // the source EDIT control was modified.
        // change the toolbar
        UpdateOurToolBarButtons(2);

        // show "ready" status
        ChangeStatusText(IDS_READY);
        return;
    }

    // show "executing command..." status
    if (!::IsWindow(m_rad_window) && id >= 100)
        ChangeStatusText(IDS_EXECUTINGCMD);

    // add a command lock
    ++m_nCommandLock;

    // execute the command
    BOOL bUpdateStatus = TRUE;
    switch (id)
    {
    case ID_NEW:
        OnNew(hwnd);
        break;
    case ID_OPEN:
        OnOpen(hwnd);
        break;
    case ID_SAVEAS:
        OnSaveAs(hwnd);
        break;
    case ID_IMPORT:
        OnImport(hwnd);
        break;
    case ID_EXIT:
        DestroyWindow(hwnd);
        break;
    case ID_ADDICON:
        OnAddIcon(hwnd);
        break;
    case ID_ADDCURSOR:
        OnAddCursor(hwnd);
        break;
    case ID_ADDBITMAP:
        OnAddBitmap(hwnd);
        break;
    case ID_ADDRES:
        OnAddRes(hwnd);
        break;
    case ID_REPLACEICON:
        OnReplaceIcon(hwnd);
        break;
    case ID_REPLACECURSOR:
        OnReplaceCursor(hwnd);
        break;
    case ID_REPLACEBITMAP:
        OnReplaceBitmap(hwnd);
        break;
    case ID_REPLACEBIN:
        OnReplaceBin(hwnd);
        break;
    case ID_DELETERES:
        OnDeleteRes(hwnd);
        break;
    case ID_EDIT:
        OnEdit(hwnd);
        break;
    case ID_EXTRACTICON:
        OnExtractIcon(hwnd);
        break;
    case ID_EXTRACTCURSOR:
        OnExtractCursor(hwnd);
        break;
    case ID_EXTRACTBITMAP:
        OnExtractBitmap(hwnd);
        break;
    case ID_EXTRACTBIN:
        OnExtractBin(hwnd);
        break;
    case ID_ABOUT:
        OnAbout(hwnd);
        break;
    case ID_TEST:
        OnTest(hwnd);
        break;
    case ID_CANCELEDIT:
        OnCancelEdit(hwnd);
        break;
    case ID_COMPILE:
        OnCompile(hwnd);
        break;
    case ID_GUIEDIT:
        OnGuiEdit(hwnd);
        break;
    case ID_DESTROYRAD:
        OnCancelEdit(hwnd);
        break;
    case ID_DELCTRL:
        MRadCtrl::DeleteSelection();
        m_hSrcEdit.ClearIndeces();
        break;
    case ID_ADDCTRL:
        m_rad_window.OnAddCtrl(m_rad_window);
        break;
    case ID_CTRLPROP:
        m_rad_window.OnCtrlProp(m_rad_window);
        break;
    case ID_DLGPROP:
        m_rad_window.OnDlgProp(m_rad_window);
        break;
    case ID_CTRLINDEXTOP:
        m_rad_window.IndexTop(m_rad_window);
        m_hSrcEdit.ClearIndeces();
        break;
    case ID_CTRLINDEXBOTTOM:
        m_rad_window.IndexBottom(m_rad_window);
        m_hSrcEdit.ClearIndeces();
        break;
    case ID_CTRLINDEXMINUS:
        m_rad_window.IndexMinus(m_rad_window);
        m_hSrcEdit.ClearIndeces();
        break;
    case ID_CTRLINDEXPLUS:
        m_rad_window.IndexPlus(m_rad_window);
        m_hSrcEdit.ClearIndeces();
        break;
    case ID_SHOWHIDEINDEX:
        m_rad_window.OnShowHideIndex(m_rad_window);
        break;
    case ID_TOPALIGN:
        m_rad_window.OnTopAlign(m_rad_window);
        break;
    case ID_BOTTOMALIGN:
        m_rad_window.OnBottomAlign(m_rad_window);
        break;
    case ID_LEFTALIGN:
        m_rad_window.OnLeftAlign(m_rad_window);
        break;
    case ID_RIGHTALIGN:
        m_rad_window.OnRightAlign(m_rad_window);
        break;
    case ID_FITTOGRID:
        m_rad_window.OnFitToGrid(m_rad_window);
        bUpdateStatus = FALSE;
        break;
    case ID_STATUSBAR:
        // toggle the flag
        g_settings.bShowStatusBar = !g_settings.bShowStatusBar;

        // show/hide the scroll bar
        ShowStatusBar(g_settings.bShowStatusBar);

        // relayout
        PostMessageDx(WM_SIZE);
        break;
    case ID_BINARYPANE:
        // toggle the flag
        g_settings.bShowBinEdit = !g_settings.bShowBinEdit;

        // show/hide the binary EDIT control
        ShowBinEdit(g_settings.bShowBinEdit);
        break;
    case ID_ALWAYSCONTROL:
        {
            // toggle the flag
            g_settings.bAlwaysControl = !g_settings.bAlwaysControl;

            // select the entry to update the text
            auto entry = g_res.get_entry();
            SelectTV(entry, FALSE);
        }
        break;
    case ID_MRUFILE0:
    case ID_MRUFILE1:
    case ID_MRUFILE2:
    case ID_MRUFILE3:
    case ID_MRUFILE4:
    case ID_MRUFILE5:
    case ID_MRUFILE6:
    case ID_MRUFILE7:
    case ID_MRUFILE8:
    case ID_MRUFILE9:
    case ID_MRUFILE10:
    case ID_MRUFILE11:
    case ID_MRUFILE12:
    case ID_MRUFILE13:
    case ID_MRUFILE14:
    case ID_MRUFILE15:
        {
            DWORD i = id - ID_MRUFILE0;
            if (i < g_settings.vecRecentlyUsed.size())
            {
                DoLoadFile(hwnd, g_settings.vecRecentlyUsed[i].c_str());
            }
        }
        break;
    case ID_PLAY:
        OnPlay(hwnd);
        break;
    case ID_READY:
        break;
    case ID_IDASSOC:
        OnIdAssoc(hwnd);
        break;
    case ID_LOADRESH:
        OnLoadResH(hwnd);
        break;
    case ID_IDLIST:
        OnIDList(hwnd);
        break;
    case ID_UNLOADRESH:
        OnUnloadResH(hwnd);
        break;
    case ID_HIDEIDMACROS:
        OnHideIDMacros(hwnd);
        break;
    case ID_USEIDC_STATIC:
        OnUseIDC_STATIC(hwnd);
        break;
    case ID_CONFIG:
        OnConfig(hwnd);
        break;
    case ID_ADVICERESH:
        OnAdviceResH(hwnd);
        break;
    case ID_UPDATEID:
        UpdateNames();
        break;
    case ID_OPENREADME:
        OnOpenReadMe(hwnd);
        break;
    case ID_OPENREADMEJP:
        OnOpenReadMeJp(hwnd);
        break;
    case ID_LOADWCLIB:
        OnLoadWCLib(hwnd);
        break;
    case ID_FIND:
        OnFind(hwnd);
        break;
    case ID_FINDDOWNWARD:
        OnFindNext(hwnd);
        break;
    case ID_FINDUPWARD:
        OnFindPrev(hwnd);
        break;
    case ID_REPLACE:
        OnReplace(hwnd);
        break;
    case ID_ADDMENU:
        OnAddMenu(hwnd);
        break;
    case ID_ADDVERINFO:
        OnAddVerInfo(hwnd);
        break;
    case ID_ADDMANIFEST:
        OnAddManifest(hwnd);
        break;
    case ID_ADDDIALOG:
        OnAddDialog(hwnd);
        break;
    case ID_ADDSTRINGTABLE:
        OnAddStringTable(hwnd);
        break;
    case ID_ADDMESSAGETABLE:
        OnAddMessageTable(hwnd);
        break;
    case ID_ADDHTML:
        OnAddHtml(hwnd);
        break;
    case ID_ADDACCEL:
        OnAddAccel(hwnd);
        break;
    case ID_COPYASNEWNAME:
        OnCopyAsNewName(hwnd);
        break;
    case ID_COPYASNEWLANG:
        OnCopyAsNewLang(hwnd);
        break;
    case ID_ITEMSEARCH:
        OnItemSearch(hwnd);
        break;
    case ID_ITEMSEARCHBANG:
        OnItemSearchBang(hwnd, reinterpret_cast<MItemSearchDlg *>(hwndCtl));
        break;
    case ID_UPDATERESHBANG:
        OnUpdateResHBang(hwnd);
        break;
    case ID_OPENLICENSE:
        OnOpenLicense(hwnd);
        break;
    case ID_OPENHYOJUNKA:
        OnOpenHyojunka(hwnd);
        break;
    case ID_DEBUGTREENODE:
        OnDebugTreeNode(hwnd);
        break;
    case ID_LOADRESHBANG:
        OnLoadResHBang(hwnd);
        break;
    case ID_REFRESHDIALOG:
        m_rad_window.OnRefresh(m_rad_window);
        break;
    case ID_REFRESHALL:
        DoRefreshTV(hwnd);
        DoRefreshIDList(hwnd);
        break;
    case ID_EXPORT:
        OnExport(hwnd);
        break;
    case ID_FONTS:
        OnFonts(hwnd);
        break;
    case ID_REFRESH:
        DoRefreshTV(hwnd);
        break;
    case ID_PREDEFMACROS:
        OnPredefMacros(hwnd);
        break;
    case ID_EDITLABEL:
        OnEditLabel(hwnd);
        break;
    case ID_SETPATHS:
        OnSetPaths(hwnd);
        break;
    case ID_SETDEFAULTS:
        SetDefaultSettings(hwnd);
        break;
    case ID_SHOWLANGS:
        OnShowLangs(hwnd);
        break;
    case ID_SHOWHIDETOOLBAR:
        OnShowHideToolBar(hwnd);
        break;
    case ID_EXPAND_ALL:
        OnExpandAll(hwnd);
        break;
    case ID_COLLAPSE_ALL:
        OnCollapseAll(hwnd);
        break;
    case ID_WORD_WRAP:
        OnWordWrap(hwnd);
        break;
    case ID_SRCEDITSELECT:
        OnSrcEditSelect(hwnd);
        break;
    case ID_SAVEASCOMPRESS:
        OnSaveAsWithCompression(hwnd);
        break;
    case ID_CLONE:
        OnClone(hwnd);
        break;
    case ID_ADDBANG:
        break;
    case ID_EXTRACTBANG:
        OnExtractBang(hwnd);
        break;
    case ID_JUMPTOMATOME:
        OnJumpToMatome(hwnd);
        break;
    case ID_ENCODING:
        OnEncoding(hwnd);
        break;
    case ID_QUERYCONSTANT:
        OnQueryConstant(hwnd);
        break;
    case ID_USEBEGINEND:
        OnUseBeginEnd(hwnd);
        break;
    case ID_SAVE:
        OnSave(hwnd);
        break;
    case ID_EGA:
        OnEga(hwnd);
        break;
    case ID_EGA_PROGRAM:
        OnEgaProgram(hwnd);
        break;
    case ID_OPENREADMEIT:
        OnOpenReadMeIt(hwnd);
        break;
    default:
        bUpdateStatus = FALSE;
        break;
    }

    // remove the command lock
    --m_nCommandLock;

    if (m_nCommandLock == 0)
        g_res.delete_invalid();     // clean up invalids

    UpdateToolBarStatus();

    // show "ready" status if ready
    if (m_nCommandLock == 0 && bUpdateStatus && !::IsWindow(m_rad_window))
        ChangeStatusText(IDS_READY);

#if !defined(NDEBUG) && (WINVER >= 0x0500)
    // show object counts (for debugging purpose)
    HANDLE hProcess = GetCurrentProcess();
    TCHAR szText[64];
    StringCchPrintf(szText, _countof(szText), TEXT("GDI:%ld, USER:%ld"),
             GetGuiResources(hProcess, GR_GDIOBJECTS),
             GetGuiResources(hProcess, GR_USEROBJECTS));
    ChangeStatusText(szText);
#endif
}

// get the resource name from text
MIdOrString GetNameFromText(const WCHAR *pszText)
{
    // pszText --> szText
    WCHAR szText[128];
    StringCchCopyW(szText, _countof(szText), pszText);

    // replace the fullwidth characters with halfwidth characters
    ReplaceFullWithHalf(szText);

    if (szText[0] == 0)
    {
        return (WORD)0;     // empty
    }
    else if (mchr_is_digit(szText[0]) || szText[0] == L'-' || szText[0] == L'+')
    {
        // numeric
        return WORD(mstr_parse_int(szText));
    }
    else
    {
        // string value
        MStringW str = szText;

        // is there parenthesis?
        size_t i = str.rfind(L'('); // ')'
        if (i != MStringW::npos && mchr_is_digit(str[i + 1]))
        {
            // parse the text after the last parenthesis
            return WORD(mstr_parse_int(&str[i + 1]));
        }

        // string
        return MIdOrString(szText);
    }
}

// get the IDTYPE_ values by the specified prefix
std::vector<INT> GetPrefixIndexes(const MString& prefix)
{
    std::vector<INT> ret;
    for (auto& pair : g_settings.assoc_map)
    {
        if (prefix == pair.second && !pair.second.empty())
        {
            auto nIDTYPE_ = IDTYPE_(g_db.GetValue(L"RESOURCE.ID.TYPE", pair.first));
            ret.push_back(nIDTYPE_);
        }
    }
    return ret;
}

// WM_NOTIFY
LRESULT MMainWnd::OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    // get the selected entry
    auto entry = g_res.get_entry();

    if (pnmhdr->code == MSplitterWnd::NOTIFY_CHANGED)
    {
        MWaitCursor wait;
        if (pnmhdr->hwndFrom == m_splitter1)
        {
            if (m_splitter1.GetPaneCount() >= 1)
                g_settings.nTreeViewWidth = m_splitter1.GetPaneExtent(0);
        }
        else if (pnmhdr->hwndFrom == m_splitter2)
        {
            if (m_splitter2.GetPaneCount() >= 2)
                g_settings.nBinEditHeight = m_splitter2.GetPaneExtent(1);
        }
        else if (pnmhdr->hwndFrom == m_splitter3)
        {
            if (m_splitter3.GetPaneCount() >= 2)
                g_settings.nBmpViewWidth = m_splitter3.GetPaneExtent(1);
        }
    }
    else if (pnmhdr->code == TVN_DELETEITEM)
    {
        MWaitCursor wait;
        auto ptv = (NM_TREEVIEW *)pnmhdr;
        auto entry = (EntryBase *)ptv->itemOld.lParam;
        g_res.on_delete_item(entry);
        DoSetFileModified(TRUE);
    }
    else if (pnmhdr->code == NM_DBLCLK)
    {
        MWaitCursor wait;
        if (pnmhdr->hwndFrom == m_hwndTV)
        {
            switch (entry->m_et)
            {
            case ET_LANG:
                OnEdit(hwnd);
                if (g_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            case ET_STRING:
                OnEdit(hwnd);
                if (g_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            case ET_MESSAGE:
                OnEdit(hwnd);
                if (g_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            }
        }
    }
    else if (pnmhdr->code == TVN_SELCHANGING)
    {
        MWaitCursor wait;
        if (!m_bLoading)
        {
            // compile if necessary
            if (!CompileIfNecessary(FALSE))
                return TRUE;
        }
        if (IsWindow(m_rad_window))
        {
            DestroyWindow(m_rad_window);
        }
    }
    else if (pnmhdr->code == TVN_SELCHANGED)
    {
        MWaitCursor wait;
        if (!m_bLoading && entry)
        {
            NM_TREEVIEWW *pTV = (NM_TREEVIEWW *)pnmhdr;

            // select the entry to update the text
            SelectTV(entry, FALSE);
        }
    }
    else if (pnmhdr->code == NM_RETURN)
    {
        MWaitCursor wait;
        if (pnmhdr->hwndFrom == m_hwndTV)
        {
            switch (entry->m_et)
            {
            case ET_LANG:
                OnEdit(hwnd);
                if (g_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            case ET_STRING:
                OnEdit(hwnd);
                if (g_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            case ET_MESSAGE:
                OnEdit(hwnd);
                if (g_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            }
        }
    }
    else if (pnmhdr->code == TVN_KEYDOWN)
    {
        MWaitCursor wait;
        auto pTVKD = (TV_KEYDOWN *)pnmhdr;
        switch (pTVKD->wVKey)
        {
        case VK_DELETE:
            PostMessageW(hwnd, WM_COMMAND, ID_DELETERES, 0);
            DoSetFileModified(TRUE);
            return TRUE;
        case VK_F2:
            {
                // compile if necessary
                if (!CompileIfNecessary(FALSE))
                    return TRUE;

                // get the selected type entry
                auto entry = g_res.get_entry();
                if (!entry || entry->m_et == ET_TYPE)
                {
                    return TRUE;
                }

                if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
                {
                    if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
                    {
                        return TRUE;
                    }
                }

                HTREEITEM hItem = TreeView_GetSelection(m_hwndTV);
                TreeView_EditLabel(m_hwndTV, hItem);
            }
            return TRUE;
        }
    }
    else if (pnmhdr->code == TVN_GETINFOTIP)
    {
        MWaitCursor wait;
        auto pGetInfoTip = (NMTVGETINFOTIP *)pnmhdr;
        auto entry = (EntryBase *)pGetInfoTip->lParam;
        if (g_res.super()->find(entry) != g_res.super()->end())
        {
            StringCchCopyW(pGetInfoTip->pszText, pGetInfoTip->cchTextMax,
                           entry->m_strLabel.c_str());
        }
    }
    else
    {
        static WORD old_lang = BAD_LANG;
        static WCHAR szOldText[MAX_PATH] = L"";

        if (pnmhdr->code == TBN_DROPDOWN)
        {
            auto pToolBar = (NMTOOLBAR *)pnmhdr;
            OnAddBang(hwnd, pToolBar);
        }
        else if (pnmhdr->code == TVN_BEGINLABELEDIT)
        {
            MWaitCursor wait;
            auto pInfo = (TV_DISPINFO *)pnmhdr;
            LPARAM lParam = pInfo->item.lParam;
            HTREEITEM hItem = pInfo->item.hItem;
            LPWSTR pszOldText = pInfo->item.pszText;
            //MessageBoxW(NULL, pszOldText, NULL, 0);

            auto entry = (EntryBase *)lParam;

            if (entry->m_et == ET_TYPE)
            {
                return TRUE;    // prevent
            }

            if (entry->m_et == ET_NAME || entry->m_et== ET_LANG)
            {
                if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
                {
                    return TRUE;    // prevent
                }
            }

            StringCchCopyW(szOldText, _countof(szOldText), pszOldText);
            mstr_trim(szOldText);

            if (entry->m_et == ET_LANG || entry->m_et == ET_STRING ||
                entry->m_et == ET_MESSAGE)
            {
                old_lang = LangFromText(szOldText);
                if (old_lang == BAD_LANG)
                {
                    return TRUE;    // prevent
                }
            }

            return FALSE;       // accept
        }
        else if (pnmhdr->code == TVN_ENDLABELEDIT)
        {
            MWaitCursor wait;
            auto pInfo = (TV_DISPINFO *)pnmhdr;
            LPARAM lParam = pInfo->item.lParam;
            HTREEITEM hItem = pInfo->item.hItem;
            LPWSTR pszNewText = pInfo->item.pszText;
            if (pszNewText == NULL)
                return FALSE;   // reject

            auto entry = (EntryBase *)lParam;

            if (!entry || entry->m_et == ET_TYPE)
            {
                return FALSE;   // reject
            }

            if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
            {
                if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
                {
                    return FALSE;   // reject
                }
            }

            WCHAR szNewText[MAX_PATH];
            StringCchCopyW(szNewText, _countof(szNewText), pszNewText);
            mstr_trim(szNewText);

            if (entry->m_et == ET_NAME)
            {
                // rename the name
                MIdOrString old_name = GetNameFromText(szOldText);
                MIdOrString new_name = GetNameFromText(szNewText);

                if (old_name.empty())
                    return FALSE;   // reject

                if (new_name.empty())
                {
                    ErrorBoxDx(IDS_INVALIDNAME);
                    return FALSE;   // reject
                }

                if (old_name.is_str())
                    CharUpperW(&old_name.m_str[0]);
                if (new_name.is_str())
                    CharUpperW(&new_name.m_str[0]);

                if (old_name == new_name)
                    return FALSE;   // reject

                // check if it already exists
                if (auto e = g_res.find(ET_LANG, entry->m_type, new_name))
                {
                    ErrorBoxDx(IDS_ALREADYEXISTS);
                    return FALSE;   // reject
                }

                DoRenameEntry(pszNewText, entry, old_name, new_name);
                DoSetFileModified(TRUE);
                return TRUE;   // accept
            }
            else if (entry->m_et == ET_LANG)
            {
                old_lang = LangFromText(szOldText);
                if (old_lang == BAD_LANG)
                    return FALSE;   // reject

                WORD new_lang = LangFromText(szNewText);
                if (new_lang == BAD_LANG)
                {
                    ErrorBoxDx(IDS_INVALIDLANG);
                    return FALSE;   // reject
                }

                if (old_lang == new_lang)
                    return FALSE;   // reject

                // check if it already exists
                if (auto e = g_res.find(ET_LANG, entry->m_type, entry->m_name, new_lang))
                {
                    ErrorBoxDx(IDS_ALREADYEXISTS);
                    return FALSE;   // reject
                }

                DoRelangEntry(pszNewText, entry, old_lang, new_lang);
                DoSetFileModified(TRUE);
                return TRUE;   // accept
            }
            else if (entry->m_et == ET_STRING || entry->m_et == ET_MESSAGE)
            {
                old_lang = LangFromText(szOldText);
                if (old_lang == BAD_LANG)
                    return FALSE;   // reject

                WORD new_lang = LangFromText(szNewText);
                if (new_lang == BAD_LANG)
                {
                    ErrorBoxDx(IDS_INVALIDLANG);
                    return FALSE;   // reject
                }

                if (old_lang == new_lang)
                    return FALSE;   // reject

                // check if it already exists
                if (auto e = g_res.find(ET_LANG, entry->m_type, WORD(0), new_lang))
                {
                    ErrorBoxDx(IDS_ALREADYEXISTS);
                    return FALSE;   // reject
                }

                DoRelangEntry(pszNewText, entry, old_lang, new_lang);
                DoSetFileModified(TRUE);
                return TRUE;   // accept
            }

            return FALSE;   // reject
        }
    }
    return 0;
}

// change the name of the resource entries
void MMainWnd::DoRenameEntry(LPWSTR pszText, EntryBase *entry, const MIdOrString& old_name, const MIdOrString& new_name)
{
    // search the old named language entries
    EntrySetBase found;
    g_res.search(found, ET_LANG, entry->m_type, old_name);

    // rename them
    for (auto e : found)
    {
        assert(e->m_name == old_name);
        e->m_name = new_name;
    }

    // update the entry name
    entry->m_name = new_name;
    UpdateEntryName(entry, pszText);

    // select the entry to update the text
    SelectTV(entry, FALSE);

    DoSetFileModified(TRUE);
}

// change the language of the resource entries
void MMainWnd::DoRelangEntry(LPWSTR pszText, EntryBase *entry, WORD old_lang, WORD new_lang)
{
    EntrySetBase found;

    switch (entry->m_et)
    {
    case ET_STRING:
        // serach the resource strings
        g_res.search(found, ET_LANG, entry->m_type, (WORD)0, old_lang);

        // replace the language
        for (auto e : found)
        {
            assert(e->m_lang == old_lang);
            e->m_lang = new_lang;
            UpdateEntryLang(e, pszText);
        }
        found.clear();
        break;

    case ET_MESSAGE:
        // serach the resource messages
        g_res.search(found, ET_LANG, entry->m_type, (WORD)0, old_lang);

        // replace the language
        for (auto e : found)
        {
            assert(e->m_lang == old_lang);
            e->m_lang = new_lang;
            UpdateEntryLang(e, pszText);
        }
        found.clear();
        break;

    case ET_LANG:
        break;

    default:
        return;
    }

    // search the old named old language entries
    g_res.search(found, ET_ANY, entry->m_type, entry->m_name, old_lang);
    for (auto e : found)
    {
        // replace the language
        assert(e->m_lang == old_lang);
        e->m_lang = new_lang;

        // update the language
        UpdateEntryLang(e, pszText);
    }

    // select the entry
    SelectTV(entry, FALSE);

    DoSetFileModified(TRUE);
}

// do resource test
void MMainWnd::OnTest(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected entry
    auto entry = g_res.get_entry();
    if (!entry)
        return;

    if (entry->m_type == RT_DIALOG)
    {
        // entry->m_data --> stream --> dialog_res
        DialogRes dialog_res;
        MByteStreamEx stream(entry->m_data);
        dialog_res.LoadFromStream(stream);

        if (!dialog_res.m_class.empty())
        {
            // TODO: support the classed dialogs
            ErrorBoxDx(IDS_CANTTESTCLASSDLG);
        }
        else
        {
            // detach the dialog template menu (it will be used after)
            MIdOrString menu = dialog_res.m_menu;
            dialog_res.m_menu.clear();
            stream.clear();

            // fixup for "MOleCtrl", "AtlAxWin*" and/or "{...}" window classes.
            // see also: DialogRes::FixupForTest
            dialog_res.FixupForTest(false);
            dialog_res.SaveToStream(stream);
            dialog_res.FixupForTest(true);

            // load RT_DLGINIT if any
            std::vector<BYTE> dlginit_data;
            if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, entry->m_lang))
            {
                dlginit_data = e->m_data;
            }

            // show test dialog
            if (dialog_res.m_style & WS_CHILD)
            {
                // dialog_res is a child dialog. create its parent (with menu if any)
                auto window = new MTestParentWnd(dialog_res, menu, entry->m_lang,
                                                 stream, dlginit_data);
                window->CreateWindowDx(hwnd, LoadStringDx(IDS_PARENTWND),
                    WS_DLGFRAME | WS_POPUPWINDOW, WS_EX_APPWINDOW);

                // show it
                ShowWindow(*window, SW_SHOWNORMAL);
                UpdateWindow(*window);
            }
            else
            {
                // it's a non-child dialog. show the test dialog (with menu if any)
                MTestDialog dialog(dialog_res, menu, entry->m_lang, dlginit_data);
                dialog.DialogBoxIndirectDx(hwnd, stream.ptr());
            }
        }
    }
    else if (entry->m_type == RT_MENU)
    {
        // load a menu from memory
        HMENU hMenu = LoadMenuIndirect(&(*entry)[0]);
        if (hMenu)
        {
            // show the dialog
            MTestMenuDlg dialog(hMenu);
            dialog.DialogBoxDx(hwnd, IDD_MENUTEST);

            // unload the menu
            DestroyMenu(hMenu);
        }
    }
}

// join the lines by '\\'
void MMainWnd::JoinLinesByBackslash(std::vector<MStringA>& lines)
{
    // join by '\\'
    for (size_t i = 0; i < lines.size(); ++i)
    {
        MStringA& line = lines[i];
        if (line.size())
        {
            if (line[line.size() - 1] == '\\')
            {
                line = line.substr(0, line.size() - 1);
                lines[i] = line + lines[i + 1];
                lines.erase(lines.begin() + (i + 1));
                --i;
            }
        }
    }
}

// delete the include guard
void MMainWnd::DeleteIncludeGuard(std::vector<MStringA>& lines)
{
    size_t k0 = -1, k1 = -1;
    MStringA name0;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        MStringA& line = lines[i];
        const char *pch = mstr_skip_space(&line[0]);
        if (*pch != '#')
            continue;

        ++pch;
        pch = mstr_skip_space(pch);
        if (memcmp(pch, "ifndef", 6) == 0 && mchr_is_space(pch[6]))
        {
            // #ifndef
            pch += 6;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            MStringA name(pch0, pch);

            if (name0.empty())
            {
                k0 = i;
                name0 = name;
            }
            else
            {
                name0.clear();
                break;
            }
        }
        else if (memcmp(pch, "if", 2) == 0)
        {
            break;
        }
        else if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
        {
            if (name0.empty())
                break;

            // #define
            pch += 6;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            MStringA name(pch0, pch);
            if (name0 == name)
            {
                k1 = i;
                break;
            }
        }
        else
        {
            // otherwise
            break;
        }
    }

    if (name0.empty())
        return;

    for (size_t i = lines.size(); i > 0; )
    {
        --i;
        MStringA& line = lines[i];
        const char *pch = mstr_skip_space(&line[0]);
        if (*pch != '#')
            continue;

        ++pch;
        pch = mstr_skip_space(pch);
        if (memcmp(pch, "endif", 5) == 0)
        {
            lines.erase(lines.begin() + i);
            lines.erase(lines.begin() + k1);
            lines.erase(lines.begin() + k0);
            break;
        }
        else
        {
            break;
        }
    }
}

// add the head comments
void MMainWnd::AddHeadComment(std::vector<MStringA>& lines)
{
    if (m_szFile[0])
    {
        WCHAR title[64];
        GetFileTitleW(m_szFile, title, _countof(title));
        MStringA line = "// ";
        line += MWideToAnsi(CP_ACP, title).c_str();
        lines.insert(lines.begin(), line);
    }
    lines.insert(lines.begin(), "// Microsoft Visual C++ Compatible");
    lines.insert(lines.begin(), "//{{NO_DEPENDENCIES}}");
}

// delete the head comments
void MMainWnd::DeleteHeadComment(std::vector<MStringA>& lines)
{
    for (size_t i = 0; i < lines.size(); ++i)
    {
        MStringA& line = lines[i];
        if (line.find("//") == 0)
        {
            if (line.find("{{NO_DEPENDENCIES}}") != MStringA::npos ||
                line.find("Microsoft Visual C++") != MStringA::npos ||
                line.find(".rc") != MStringA::npos)
            {
                lines.erase(lines.begin() + i);
                --i;
            }
        }
    }
}

// delete the specific macro lines
void MMainWnd::DeleteSpecificMacroLines(std::vector<MStringA>& lines)
{
    for (size_t i = lines.size(); i > 0; )
    {
        --i;
        MStringA& line = lines[i];
        const char *pch = mstr_skip_space(&line[0]);
        if (*pch != '#')
            continue;

        ++pch;
        pch = mstr_skip_space(pch);
        if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
        {
            // #define
            pch += 6;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            MStringA name(pch0, pch);

            if (g_settings.removed_ids.find(name) != g_settings.removed_ids.end())
            {
                lines.erase(lines.begin() + i);
            }
        }
    }
}

// add additional macro lines
void MMainWnd::AddAdditionalMacroLines(std::vector<MStringA>& lines)
{
    MStringA str;
    for (auto& pair : g_settings.added_ids)
    {
        MStringA line = "#define ";
        if (pair.first == "IDC_STATIC")
        {
            line += "IDC_STATIC -1";
        }
        else
        {
            line += pair.first;
            line += " ";
            line += pair.second;
        }
        lines.push_back(line);
    }
}

// add the '#ifdef APSTUDIO_INVOKED ... #endif' block
void MMainWnd::AddApStudioBlock(std::vector<MStringA>& lines)
{
    UINT anValues[5];
    DoIDStat(anValues);

    lines.push_back("#ifdef APSTUDIO_INVOKED");
    lines.push_back("    #ifndef APSTUDIO_READONLY_SYMBOLS");

    char buf[256];
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NO_MFC                 %u", anValues[0]);
    lines.push_back(buf);
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_RESOURCE_VALUE    %u", anValues[1]);
    lines.push_back(buf);
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_COMMAND_VALUE     %u", anValues[2]);
    lines.push_back(buf);
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_CONTROL_VALUE     %u", anValues[3]);
    lines.push_back(buf);
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_SYMED_VALUE       %u", anValues[4]);
    lines.push_back(buf);
    lines.push_back("    #endif");
    lines.push_back("#endif");
}

// delete the '#ifdef APSTUDIO_INVOKED ... #endif' block
void MMainWnd::DeleteApStudioBlock(std::vector<MStringA>& lines)
{
    bool inside = false, found = false;
    size_t nest = 0, k = -1;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        MStringA& line = lines[i];
        const char *pch = mstr_skip_space(&line[0]);
        if (*pch != '#')
            continue;

        ++pch;
        pch = mstr_skip_space(pch);
        if (memcmp(pch, "ifdef", 5) == 0 && mchr_is_space(pch[5]))
        {
            // #ifdef
            pch += 5;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            MStringA name(pch0, pch);

            if (name == "APSTUDIO_INVOKED")
            {
                inside = true;
                found = false;
                k = i;
                ++nest;
            }
        }
        else if (memcmp(pch, "if", 2) == 0)
        {
            ++nest;
        }
        else if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
        {
            if (!inside)
                continue;

            // #define
            pch += 6;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            MStringA name(pch0, pch);

            if (name == "_APS_NEXT_RESOURCE_VALUE")
            {
                found = true;
            }
        }
        else if (memcmp(pch, "endif", 5) == 0)
        {
            --nest;
            if (nest == 0 && k != -1)
            {
                lines.erase(lines.begin() + k, lines.begin() + i + 1);
                break;
            }
        }
    }
}

// helper method for MMainWnd::OnUpdateResHBang
void MMainWnd::UpdateResHLines(std::vector<MStringA>& lines)
{
    JoinLinesByBackslash(lines);
    DeleteIncludeGuard(lines);
    DeleteHeadComment(lines);
    DeleteSpecificMacroLines(lines);
    AddAdditionalMacroLines(lines);
    DeleteApStudioBlock(lines);
    AddApStudioBlock(lines);
    AddHeadComment(lines);
}

// helper method for MMainWnd::OnUpdateResHBang
void MMainWnd::ReadResHLines(FILE *fp, std::vector<MStringA>& lines)
{
    // read lines
    CHAR buf[512];
    while (fgets(buf, _countof(buf), fp) != NULL)
    {
        size_t len = std::strlen(buf);
        if (len == 0)
            break;
        if (buf[len - 1] == '\n')
            buf[len - 1] = 0;
        lines.push_back(buf);
    }
}

// do save or update the resource.h file
void MMainWnd::OnUpdateResHBang(HWND hwnd)
{
    // check whether the ID list window is open or not
    BOOL bListOpen = IsWindow(m_id_list_dlg);

    // destroy the ID list window
    DestroyWindow(m_id_list_dlg);

    // // query update to the user
    // if (MsgBoxDx(IDS_UPDATERESH, MB_ICONINFORMATION | MB_YESNO) == IDNO)    // don't update
    // {
    //     ShowIDList(hwnd, bListOpen);
    //     return;
    // }

    if (1)
    {
        // build new "resource.h" file path
        WCHAR szResH[MAX_PATH];

        if (m_szResourceH[0])
        {
            StringCchCopyW(szResH, _countof(szResH), m_szResourceH);
        }
        else if (m_szFile[0])
        {
            StringCchCopyW(szResH, _countof(szResH), m_szFile);

            WCHAR *pch = wcsrchr(szResH, L'\\');
            if (pch == NULL)
                pch = wcsrchr(szResH, L'/');
            if (pch == NULL)
                return; // failure

            *pch = 0;
            StringCchCatW(szResH, _countof(szResH), L"\\resource.h");
        }
        else
        {
            StringCchCopyW(szResH, _countof(szResH), L"resource.h");
        }

        // initialize OPENFILENAME structure
        OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
        ofn.lpstrFile = szResH;
        ofn.nMaxFile = _countof(szResH);
        ofn.lpstrTitle = LoadStringDx(IDS_SAVERESH);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
                    OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"h";

        // let the user choose the path
        if (!GetSaveFileNameW(&ofn))
        {
            return;     // cancelled
        }

        // create new
        if (!DoWriteResH(szResH))
        {
            ErrorBoxDx(IDS_CANTWRITERESH);
            ShowIDList(hwnd, bListOpen);
            return;     // failure
        }

        // szResH --> m_szResourceH
        StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);
    }
    else    // update the resource.h file by modification
    {
        // do backup the resource.h file
        if (g_settings.bBackup)
            DoBackupFile(m_szResourceH);

        // open file
        FILE *fp = _wfopen(m_szResourceH, L"r");
        if (!fp)
        {
            ErrorBoxDx(IDS_CANTWRITERESH);
            ShowIDList(hwnd, bListOpen);
            return;
        }

        // read the resource.h lines
        std::vector<MStringA> lines;
        ReadResHLines(fp, lines);
        fclose(fp);     // close the files

        // modify the lines
        UpdateResHLines(lines);

        // reopen the file to write
        fp = _wfopen(m_szResourceH, L"w");
        if (!fp)
        {
            ErrorBoxDx(IDS_CANTWRITERESH);
            ShowIDList(hwnd, bListOpen);
            return;
        }

        // write now
        for (size_t i = 0; i < lines.size(); ++i)
        {
            fprintf(fp, "%s\n", lines[i].c_str());
        }

        fclose(fp);     // close the files
    }

    // clear modification of IDs
    g_settings.added_ids.clear();
    g_settings.removed_ids.clear();

    // reopen the ID list window if necessary
    ShowIDList(hwnd, bListOpen);

    DoSetFileModified(TRUE);
}

// add an icon resource
void MMainWnd::OnAddIcon(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddIconDlg dialog;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // refresh the ID list window
        DoRefreshIDList(hwnd);

        // select the entry
        SelectTV(ET_LANG, dialog, FALSE);

        DoSetFileModified(TRUE);
    }
}

// replace the icon resource
void MMainWnd::OnReplaceIcon(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected language entry
    auto entry = g_res.get_lang_entry();
    if (!entry)
        return;

    // show the dialog
    MReplaceIconDlg dialog(entry);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // select the entry
        SelectTV(ET_LANG, dialog, FALSE);

        DoSetFileModified(TRUE);
    }
}

// replace the cursor resource
void MMainWnd::OnReplaceCursor(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected language entry
    auto entry = g_res.get_lang_entry();
    if (!entry)
        return;

    // show the dialog
    MReplaceCursorDlg dialog(entry);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // select the entry
        SelectTV(ET_LANG, dialog, FALSE);

        DoSetFileModified(TRUE);
    }
}

// add a bitmap resource
void MMainWnd::OnAddBitmap(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddBitmapDlg dialog;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // refresh the ID list window
        DoRefreshIDList(hwnd);

        // select the entry
        SelectTV(ET_LANG, dialog, FALSE);

        DoSetFileModified(TRUE);
    }
}

// replace the bitmap resource
void MMainWnd::OnReplaceBitmap(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // get the selected entry
    auto entry = g_res.get_entry();
    if (!entry)
        return;

    // show the dialog
    MReplaceBitmapDlg dialog(*entry);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // select the entry
        SelectTV(ET_LANG, dialog, FALSE);

        DoSetFileModified(TRUE);
    }
}

// add a cursor
void MMainWnd::OnAddCursor(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddCursorDlg dialog;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // refresh the ID list window
        DoRefreshIDList(hwnd);

        // select the entry
        SelectTV(ET_LANG, dialog, FALSE);

        DoSetFileModified(TRUE);
    }
}

// add a resource item
void MMainWnd::OnAddRes(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// add a menu
void MMainWnd::OnAddMenu(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    dialog.m_type = RT_MENU;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// add a string table
void MMainWnd::OnAddStringTable(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    dialog.m_type = RT_STRING;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// add a message table resource
void MMainWnd::OnAddMessageTable(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    dialog.m_type = RT_MESSAGETABLE;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// add an HTML resource
void MMainWnd::OnAddHtml(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    dialog.m_type = RT_HTML;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// add an RT_ACCELERATOR resource
void MMainWnd::OnAddAccel(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    dialog.m_type = RT_ACCELERATOR;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// add a version info resource
void MMainWnd::OnAddVerInfo(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    dialog.m_type = RT_VERSION;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// add a manifest resource
void MMainWnd::OnAddManifest(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    dialog.m_type = RT_MANIFEST;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// add a resource item
void MMainWnd::DoAddRes(HWND hwnd, MAddResDlg& dialog)
{
    if (dialog.m_strTemplate.empty())   // already added
    {
        // refresh the ID list window
        DoRefreshIDList(hwnd);

        // select it
        SelectTV(ET_LANG, dialog, FALSE);

        // clear the modification flag
        Edit_SetModify(m_hSrcEdit, FALSE);
    }
    else        // use dialog.m_strTemplate
    {
        // dialog.m_strTemplate --> m_hSrcEdit
        SetWindowTextW(m_hSrcEdit, dialog.m_strTemplate.c_str());

        // compile dialog.m_strTemplate
        MStringA strOutput;
        if (CompileParts(strOutput, dialog.m_type, dialog.m_name, dialog.m_lang, dialog.m_strTemplate, FALSE))
        {
            // success. clear the modification flag
            Edit_SetModify(m_hSrcEdit, FALSE);
        }
        else
        {
            // failure
            UpdateOurToolBarButtons(2);

            // set the error message
            SetErrorMessage(strOutput, TRUE);

            // set the modification flag
            Edit_SetModify(m_hSrcEdit, TRUE);

            // make it non-read-only
            Edit_SetReadOnly(m_hSrcEdit, FALSE);
        }

        // select the added entry
        if (dialog.m_type == RT_STRING)
            SelectTV(ET_STRING, dialog.m_type, (WORD)0, BAD_LANG, FALSE);
        else if (dialog.m_type == RT_MESSAGETABLE)
            SelectTV(ET_MESSAGE, dialog.m_type, (WORD)0, BAD_LANG, FALSE);
        else
            SelectTV(ET_LANG, dialog, FALSE);
   }
}

// add a dialog template
void MMainWnd::OnAddDialog(HWND hwnd)
{
    // compile if necessary
    if (!CompileIfNecessary(FALSE))
        return;

    // show the dialog
    MAddResDlg dialog;
    dialog.m_type = RT_DIALOG;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        // add a resource item
        DoAddRes(hwnd, dialog);

        DoSetFileModified(TRUE);
    }
}

// set the file-related info
BOOL MMainWnd::UpdateFileInfo(FileType ft, LPCWSTR pszFile, BOOL bCompressed)
{
    m_file_type = ft;
    m_bUpxCompressed = bCompressed;

    if (pszFile == NULL || pszFile[0] == 0)
    {
        // clear the file infp
        m_szFile[0] = 0;
        SetWindowTextW(m_hwnd, LoadStringDx(IDS_APPNAME));
        return TRUE;
    }

    WCHAR szPath[MAX_PATH], *pch;

    // pszFile --> szPath --> m_szFile (full path)
    GetFullPathNameW(pszFile, _countof(szPath), szPath, &pch);
    StringCchCopyW(m_szFile, _countof(m_szFile), szPath);

    // find the last '\\' or '/'
    pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(szPath, L'/');
    if (pch == NULL)
        pch = szPath;
    else
        ++pch;

    // set the file title to the title bar
    SetWindowTextW(m_hwnd, LoadStringPrintfDx(IDS_TITLEWITHFILE, pch));

    // add to the recently used files
    g_settings.AddFile(m_szFile);

    // update the menu
    UpdateMenu();

    return TRUE;
}

// set the default settings
void MMainWnd::SetDefaultSettings(HWND hwnd)
{
    g_settings.bShowBinEdit = TRUE;
    g_settings.bAlwaysControl = FALSE;
    g_settings.bShowStatusBar = TRUE;
    g_settings.nTreeViewWidth = TV_WIDTH;
    g_settings.nBmpViewWidth = BV_WIDTH;
    g_settings.nBinEditHeight = BE_HEIGHT;
    g_settings.bGuiByDblClick = TRUE;
    g_settings.bResumeWindowPos = TRUE;
    g_settings.bAutoLoadNearbyResH = TRUE;
    g_settings.bAutoShowIDList = TRUE;
    g_settings.bHideID = FALSE;
    g_settings.bUseIDC_STATIC = FALSE;
    g_settings.bShowDotsOnDialog = TRUE;
    g_settings.nComboHeight = 300;
    g_settings.vecRecentlyUsed.clear();
    g_settings.nWindowLeft = CW_USEDEFAULT;
    g_settings.nWindowTop = CW_USEDEFAULT;
    g_settings.nWindowWidth = 760;
    g_settings.nWindowHeight = 480;
    g_settings.bMaximized = FALSE;
    g_settings.nIDListLeft = CW_USEDEFAULT;
    g_settings.nIDListTop = CW_USEDEFAULT;
    g_settings.nIDListWidth = 366;
    g_settings.nIDListHeight = 490;
    g_settings.nRadLeft = CW_USEDEFAULT;
    g_settings.nRadTop = CW_USEDEFAULT;
    g_settings.bAskUpdateResH = FALSE;
    g_settings.bCompressByUPX = FALSE;
    g_settings.bUseBeginEnd = FALSE;

    HFONT hFont;
    LOGFONTW lf, lfBin, lfSrc;

    // get GUI font
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);

    ZeroMemory(&lfBin, sizeof(lfBin));
    lfBin.lfHeight = 10;
    lfBin.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lfBin.lfCharSet = lf.lfCharSet;
    hFont = CreateFontIndirectW(&lfBin);
    GetObject(hFont, sizeof(lfBin), &lfBin);
    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        SelectObject(hDC, hFont);
        GetTextFace(hDC, LF_FACESIZE, lfBin.lfFaceName);
        DeleteDC(hDC);
    }
    DeleteObject(hFont);

    ZeroMemory(&lfSrc, sizeof(lfSrc));
    lfSrc.lfHeight = 13;
    lfSrc.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lfSrc.lfCharSet = lf.lfCharSet;
    hFont = CreateFontIndirectW(&lfSrc);
    GetObject(hFont, sizeof(lfSrc), &lfSrc);
    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        SelectObject(hDC, hFont);
        GetTextFace(hDC, LF_FACESIZE, lfSrc.lfFaceName);
        DeleteDC(hDC);
    }
    DeleteObject(hFont);

    g_settings.strSrcFont = lfSrc.lfFaceName;
    g_settings.strBinFont = lfBin.lfFaceName;

    g_settings.nSrcFontSize = 12;
    g_settings.nBinFontSize = 9;

    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        if (lfBin.lfHeight < 0)
        {
            g_settings.nBinFontSize = -MulDiv(lfBin.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
        }
        else
        {
            g_settings.nBinFontSize = MulDiv(lfBin.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
        }

        if (lfSrc.lfHeight < 0)
        {
            g_settings.nSrcFontSize = -MulDiv(lfSrc.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
        }
        else
        {
            g_settings.nSrcFontSize = MulDiv(lfSrc.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
        }

        DeleteDC(hDC);
    }

    auto table1 = g_db.GetTable(L"RESOURCE.ID.TYPE");
    auto table2 = g_db.GetTable(L"RESOURCE.ID.PREFIX");
    assert(table1.size() == table2.size());

    g_settings.assoc_map.clear();
    if (table1.size() && table1.size() == table2.size())
    {
        for (size_t i = 0; i < table1.size(); ++i)
        {
            g_settings.assoc_map.insert(std::make_pair(table1[i].name, table2[i].name));
        }
    }
    else
    {
        g_settings.ResetAssoc();
    }

    g_settings.id_map.clear();
    g_settings.added_ids.clear();
    g_settings.removed_ids.clear();

    g_settings.ResetMacros();

    g_settings.includes.clear();

    g_settings.strWindResExe.clear();
    g_settings.strCppExe.clear();

    // cpp.exe
    StringCchCopyW(m_szMCppExe, _countof(m_szMCppExe), m_szDataFolder);
    StringCchCatW(m_szMCppExe, _countof(m_szMCppExe), L"\\bin\\mcpp.exe");

    // windres.exe
    StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
    StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");

    g_settings.strPrevVersion.clear();

    g_settings.bSepFilesByLang = FALSE;
    g_settings.bStoreToResFolder = TRUE;
    g_settings.bSelectableByMacro = FALSE;

    g_settings.captions.clear();

    g_settings.bShowToolBar = TRUE;
    g_settings.strAtlAxWin = L"AtlAxWin110";
    g_settings.nSaveFilterIndex = 1;
    g_settings.bWordWrap = FALSE;

    g_settings.bBackup = TRUE;
    g_settings.strBackupSuffix = L"-old";

    g_settings.bRedundantComments = TRUE;
    g_settings.bWrapManifest = TRUE;

    g_settings.bRCFileUTF16 = FALSE;

    g_settings.ResetEncoding();
}

// update the prefix data
void MMainWnd::UpdatePrefixDB(HWND hwnd)
{
    // update "RESOURCE.ID.PREFIX" table
    auto& table = g_db.m_map[L"RESOURCE.ID.PREFIX"];
    for (size_t i = 0; i < table.size(); ++i)
    {
        for (auto& pair : g_settings.assoc_map)
        {
            if (table[i].name == pair.first)    // matched
            {
                // update the value
                table[i].value = mstr_parse_int(pair.second.c_str());
                break;
            }
        }
    }
}

// load the settings
BOOL MMainWnd::LoadSettings(HWND hwnd)
{
    SetDefaultSettings(hwnd);

    // open the "HKEY_CURRENT_USER\Software" key
    MRegKey key(HKCU, TEXT("Software"));
    if (!key)
        return FALSE;

    // open the "HKEY_CURRENT_USER\Software\Katayama Hirofumi MZ" key
    MRegKey keySoftware(key, TEXT("Katayama Hirofumi MZ"));
    if (!keySoftware)
        return FALSE;

    // open the "HKEY_CURRENT_USER\Software\Katayama Hirofumi MZ\RisohEditor" key
    MRegKey keyRisoh(keySoftware, TEXT("RisohEditor"));
    if (!keyRisoh)
        return FALSE;

    keyRisoh.QueryDword(TEXT("HIDE.ID"), (DWORD&)g_settings.bHideID);
    keyRisoh.QueryDword(TEXT("bUseIDC_STATIC"), (DWORD&)g_settings.bUseIDC_STATIC);
    keyRisoh.QueryDword(TEXT("ShowStatusBar"), (DWORD&)g_settings.bShowStatusBar);
    keyRisoh.QueryDword(TEXT("ShowBinEdit"), (DWORD&)g_settings.bShowBinEdit);
    keyRisoh.QueryDword(TEXT("AlwaysControl"), (DWORD&)g_settings.bAlwaysControl);
    keyRisoh.QueryDword(TEXT("TreeViewWidth"), (DWORD&)g_settings.nTreeViewWidth);
    keyRisoh.QueryDword(TEXT("BmpViewWidth"), (DWORD&)g_settings.nBmpViewWidth);
    keyRisoh.QueryDword(TEXT("BinEditHeight"), (DWORD&)g_settings.nBinEditHeight);
    keyRisoh.QueryDword(TEXT("bGuiByDblClick"), (DWORD&)g_settings.bGuiByDblClick);
    keyRisoh.QueryDword(TEXT("bResumeWindowPos"), (DWORD&)g_settings.bResumeWindowPos);
    keyRisoh.QueryDword(TEXT("bAutoLoadNearbyResH"), (DWORD&)g_settings.bAutoLoadNearbyResH);
    keyRisoh.QueryDword(TEXT("bAutoShowIDList"), (DWORD&)g_settings.bAutoShowIDList);
    keyRisoh.QueryDword(TEXT("bShowDotsOnDialog"), (DWORD&)g_settings.bShowDotsOnDialog);
    keyRisoh.QueryDword(TEXT("nComboHeight"), (DWORD&)g_settings.nComboHeight);
    keyRisoh.QueryDword(TEXT("nWindowLeft"), (DWORD&)g_settings.nWindowLeft);
    keyRisoh.QueryDword(TEXT("nWindowTop"), (DWORD&)g_settings.nWindowTop);
    keyRisoh.QueryDword(TEXT("nWindowWidth"), (DWORD&)g_settings.nWindowWidth);
    keyRisoh.QueryDword(TEXT("nWindowHeight"), (DWORD&)g_settings.nWindowHeight);
    keyRisoh.QueryDword(TEXT("bMaximized"), (DWORD&)g_settings.bMaximized);
    keyRisoh.QueryDword(TEXT("nIDListLeft"), (DWORD&)g_settings.nIDListLeft);
    keyRisoh.QueryDword(TEXT("nIDListTop"), (DWORD&)g_settings.nIDListTop);
    keyRisoh.QueryDword(TEXT("nIDListWidth"), (DWORD&)g_settings.nIDListWidth);
    keyRisoh.QueryDword(TEXT("nIDListHeight"), (DWORD&)g_settings.nIDListHeight);
    keyRisoh.QueryDword(TEXT("nRadLeft"), (DWORD&)g_settings.nRadLeft);
    keyRisoh.QueryDword(TEXT("nRadTop"), (DWORD&)g_settings.nRadTop);
    keyRisoh.QueryDword(TEXT("bAskUpdateResH"), (DWORD&)g_settings.bAskUpdateResH);
    keyRisoh.QueryDword(TEXT("bCompressByUPX"), (DWORD&)g_settings.bCompressByUPX);
    keyRisoh.QueryDword(TEXT("bUseBeginEnd"), (DWORD&)g_settings.bUseBeginEnd);

    TCHAR szText[128];
    TCHAR szValueName[128];

    // load the macros
    DWORD dwMacroCount = 0;
    if (keyRisoh.QueryDword(TEXT("dwMacroCount"), dwMacroCount) == ERROR_SUCCESS)
    {
        g_settings.macros.clear();

        for (DWORD i = 0; i < dwMacroCount; ++i)
        {
            MString key, value;

            StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroName%lu"), i);
            if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
                key = szText;

            StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroValue%lu"), i);
            if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
                value = szText;

            if (!key.empty())
                g_settings.macros.insert(std::make_pair(key, value));
        }
    }

    // load the includes
    DWORD dwNumIncludes = 0;
    if (keyRisoh.QueryDword(TEXT("dwNumIncludes"), dwNumIncludes) == ERROR_SUCCESS)
    {
        g_settings.includes.clear();

        for (DWORD i = 0; i < dwNumIncludes; ++i)
        {
            MString value;

            StringCchPrintf(szValueName, _countof(szValueName), TEXT("Include%lu"), i);
            if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
                value = szText;

            if (!value.empty())
                g_settings.includes.push_back(value);
        }
    }

    if (keyRisoh.QuerySz(TEXT("strSrcFont"), szText, _countof(szText)) == ERROR_SUCCESS)
        g_settings.strSrcFont = szText;
    keyRisoh.QueryDword(TEXT("nSrcFontSize"), (DWORD&)g_settings.nSrcFontSize);

    if (keyRisoh.QuerySz(TEXT("strBinFont"), szText, _countof(szText)) == ERROR_SUCCESS)
        g_settings.strBinFont = szText;
    keyRisoh.QueryDword(TEXT("nBinFontSize"), (DWORD&)g_settings.nBinFontSize);

    INT xVirtualScreen = GetSystemMetrics(SM_XVIRTUALSCREEN);
    INT yVirtualScreen = GetSystemMetrics(SM_YVIRTUALSCREEN);
    INT cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    INT cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    INT cxMin = 200, cyMin = 180;

    if (g_settings.nWindowLeft < xVirtualScreen)
        g_settings.nWindowLeft = CW_USEDEFAULT;
    if (g_settings.nWindowTop < yVirtualScreen)
        g_settings.nWindowTop = CW_USEDEFAULT;
    if (g_settings.nWindowWidth <= cxMin)
        g_settings.nWindowWidth = 760;
    if (g_settings.nWindowHeight <= cyMin)
        g_settings.nWindowHeight = 480;
    if (g_settings.nWindowLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
        g_settings.nWindowLeft = CW_USEDEFAULT;
    if (g_settings.nWindowTop + cyMin >= yVirtualScreen + cyVirtualScreen)
        g_settings.nWindowTop = CW_USEDEFAULT;
    if (g_settings.nWindowTop == CW_USEDEFAULT)
        g_settings.nWindowLeft = CW_USEDEFAULT;

    if (g_settings.nIDListLeft < xVirtualScreen)
        g_settings.nIDListLeft = CW_USEDEFAULT;
    if (g_settings.nIDListTop < yVirtualScreen)
        g_settings.nIDListTop = CW_USEDEFAULT;
    if (g_settings.nIDListWidth <= cxMin)
        g_settings.nIDListWidth = 366;
    if (g_settings.nIDListHeight <= cyMin)
        g_settings.nIDListHeight = 490;
    if (g_settings.nIDListLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
        g_settings.nIDListLeft = CW_USEDEFAULT;
    if (g_settings.nIDListTop + cyMin >= yVirtualScreen + cyVirtualScreen)
        g_settings.nIDListTop = CW_USEDEFAULT;
    if (g_settings.nIDListTop == CW_USEDEFAULT)
        g_settings.nIDListLeft = CW_USEDEFAULT;

    if (g_settings.nRadLeft < xVirtualScreen)
        g_settings.nRadLeft = CW_USEDEFAULT;
    if (g_settings.nRadTop < yVirtualScreen)
        g_settings.nRadTop = CW_USEDEFAULT;
    if (g_settings.nRadLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
        g_settings.nRadLeft = CW_USEDEFAULT;
    if (g_settings.nRadTop + cyMin >= yVirtualScreen + cyVirtualScreen)
        g_settings.nRadTop = CW_USEDEFAULT;
    if (g_settings.nRadTop == CW_USEDEFAULT)
        g_settings.nRadLeft = CW_USEDEFAULT;

    DWORD i, dwCount;

    TCHAR szFormat[32], szFile[MAX_PATH];

    // load the recently used files
    keyRisoh.QueryDword(TEXT("FileCount"), dwCount);
    if (dwCount > MAX_MRU)
        dwCount = MAX_MRU;
    for (i = 0; i < dwCount; ++i)
    {
        StringCchPrintf(szFormat, _countof(szFormat), TEXT("File%lu"), i);
        if (keyRisoh.QuerySz(szFormat, szFile, _countof(szFile)) == ERROR_SUCCESS)
        {
            if (PathFileExistsW(szFile))
            {
                g_settings.vecRecentlyUsed.push_back(szFile);
            }
        }
    }

    if (keyRisoh.QuerySz(TEXT("strWindResExe"), szText, _countof(szText)) == ERROR_SUCCESS)
    {
        if (PathFileExistsW(szText))
            g_settings.strWindResExe = szText;
    }

    if (keyRisoh.QuerySz(TEXT("strCppExe"), szText, _countof(szText)) == ERROR_SUCCESS)
    {
        if (PathFileExistsW(szText))
            g_settings.strCppExe = szText;
    }

    if (keyRisoh.QuerySz(TEXT("strPrevVersion"), szText, _countof(szText)) == ERROR_SUCCESS)
    {
        g_settings.strPrevVersion = szText;
    }

    // update association if version > 3.8
    if (!g_settings.strPrevVersion.empty() && g_settings.strPrevVersion > L"3.8")
    {
        TCHAR szName[MAX_PATH];
        for (auto& pair : g_settings.assoc_map)
        {
            if (keyRisoh.QuerySz(pair.first.c_str(), szName, _countof(szName)) == ERROR_SUCCESS)
            {
                pair.second = szName;
            }
        }
        UpdatePrefixDB(hwnd);
    }

    keyRisoh.QueryDword(TEXT("bSepFilesByLang"), (DWORD&)g_settings.bSepFilesByLang);
    keyRisoh.QueryDword(TEXT("bStoreToResFolder"), (DWORD&)g_settings.bStoreToResFolder);
    keyRisoh.QueryDword(TEXT("bSelectableByMacro"), (DWORD&)g_settings.bSelectableByMacro);

    // load the captions
    DWORD dwNumCaptions = 0;
    keyRisoh.QueryDword(TEXT("dwNumCaptions"), (DWORD&)dwNumCaptions);
    if (dwNumCaptions > s_nMaxCaptions)
        dwNumCaptions = s_nMaxCaptions;
    captions_type captions;
    for (DWORD i = 0; i < dwNumCaptions; ++i)
    {
        StringCchPrintf(szValueName, _countof(szValueName), TEXT("Caption%lu"), i);
        if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
        {
            g_settings.captions.push_back(szText);
        }
    }

    keyRisoh.QueryDword(TEXT("bShowToolBar"), (DWORD&)g_settings.bShowToolBar);

    if (keyRisoh.QuerySz(L"strAtlAxWin", szText, _countof(szText)) == ERROR_SUCCESS)
    {
        g_settings.strAtlAxWin = szText;
    }

    keyRisoh.QueryDword(TEXT("nSaveFilterIndex"), (DWORD&)g_settings.nSaveFilterIndex);
    keyRisoh.QueryDword(TEXT("bWordWrap"), (DWORD&)g_settings.bWordWrap);
    keyRisoh.QueryDword(TEXT("RCFileUTF16"), (DWORD&)g_settings.bRCFileUTF16);

    keyRisoh.QueryDword(TEXT("bBackup"), (DWORD&)g_settings.bBackup);

    if (keyRisoh.QuerySz(L"strBackupSuffix", szText, _countof(szText)) == ERROR_SUCCESS)
    {
        g_settings.strBackupSuffix = szText;
    }

    keyRisoh.QueryDword(TEXT("bRedundantComments"), (DWORD&)g_settings.bRedundantComments);
    keyRisoh.QueryDword(TEXT("bWrapManifest"), (DWORD&)g_settings.bWrapManifest);

    DWORD encoding_count = 0;
    if (keyRisoh.QueryDword(TEXT("encoding_count"), (DWORD&)encoding_count) == ERROR_SUCCESS)
    {
        g_settings.encoding_map.clear();
        for (DWORD i = 0; i < encoding_count; ++i)
        {
            WCHAR szName[32];
            StringCchPrintfW(szName, _countof(szName), L"encoding%lu", i);
            if (keyRisoh.QuerySz(szName, szText, _countof(szText)) == ERROR_SUCCESS)
            {
                if (LPWSTR pch = wcschr(szText, L':'))
                {
                    *pch = 0;
                    ++pch;
                    ::CharLowerW(pch);
                    if (lstrcmpW(pch, L"ansi") == 0 ||
                        lstrcmpW(pch, L"wide") == 0 ||
                        lstrcmpW(pch, L"utf8") == 0 ||
                        lstrcmpW(pch, L"utf8n") == 0 ||
                        lstrcmpW(pch, L"sjis") == 0)
                    {
                        g_settings.encoding_map[szText] = pch;
                    }
                }
            }
        }
    }

    return TRUE;
}

// save the settings
BOOL MMainWnd::SaveSettings(HWND hwnd)
{
    // open HKEY_CURRENT_USER\Software
    MRegKey key(HKCU, TEXT("Software"), TRUE);
    if (!key)
        return FALSE;

    // create HKEY_CURRENT_USER\Software\Katayama Hirofumi MZ
    MRegKey keySoftware(key, TEXT("Katayama Hirofumi MZ"), TRUE);
    if (!keySoftware)
        return FALSE;

    // create HKEY_CURRENT_USER\Software\Katayama Hirofumi MZ\RisohEditor
    MRegKey keyRisoh(keySoftware, TEXT("RisohEditor"), TRUE);
    if (!keyRisoh)
        return FALSE;

    // update pane extent settings
    if (m_splitter3.GetPaneCount() >= 2)
        g_settings.nBmpViewWidth = m_splitter3.GetPaneExtent(1);
    if (m_splitter2.GetPaneCount() >= 2)
        g_settings.nBinEditHeight = m_splitter2.GetPaneExtent(1);
    if (m_splitter1.GetPaneCount() >= 1)
        g_settings.nTreeViewWidth = m_splitter1.GetPaneExtent(0);

    keyRisoh.SetDword(TEXT("HIDE.ID"), g_settings.bHideID);
    keyRisoh.SetDword(TEXT("bUseIDC_STATIC"), g_settings.bUseIDC_STATIC);
    keyRisoh.SetDword(TEXT("ShowStatusBar"), g_settings.bShowStatusBar);
    keyRisoh.SetDword(TEXT("ShowBinEdit"), g_settings.bShowBinEdit);
    keyRisoh.SetDword(TEXT("AlwaysControl"), g_settings.bAlwaysControl);
    keyRisoh.SetDword(TEXT("TreeViewWidth"), g_settings.nTreeViewWidth);
    keyRisoh.SetDword(TEXT("BmpViewWidth"), g_settings.nBmpViewWidth);
    keyRisoh.SetDword(TEXT("BinEditHeight"), g_settings.nBinEditHeight);
    keyRisoh.SetDword(TEXT("bGuiByDblClick"), g_settings.bGuiByDblClick);
    keyRisoh.SetDword(TEXT("bResumeWindowPos"), g_settings.bResumeWindowPos);
    keyRisoh.SetDword(TEXT("bAutoLoadNearbyResH"), g_settings.bAutoLoadNearbyResH);
    keyRisoh.SetDword(TEXT("bAutoShowIDList"), g_settings.bAutoShowIDList);
    keyRisoh.SetDword(TEXT("bShowDotsOnDialog"), g_settings.bShowDotsOnDialog);
    keyRisoh.SetDword(TEXT("nComboHeight"), g_settings.nComboHeight);
    keyRisoh.SetDword(TEXT("nWindowLeft"), g_settings.nWindowLeft);
    keyRisoh.SetDword(TEXT("nWindowTop"), g_settings.nWindowTop);
    keyRisoh.SetDword(TEXT("nWindowWidth"), g_settings.nWindowWidth);
    keyRisoh.SetDword(TEXT("nWindowHeight"), g_settings.nWindowHeight);
    keyRisoh.SetDword(TEXT("bMaximized"), g_settings.bMaximized);
    keyRisoh.SetDword(TEXT("nIDListLeft"), g_settings.nIDListLeft);
    keyRisoh.SetDword(TEXT("nIDListTop"), g_settings.nIDListTop);
    keyRisoh.SetDword(TEXT("nIDListWidth"), g_settings.nIDListWidth);
    keyRisoh.SetDword(TEXT("nIDListHeight"), g_settings.nIDListHeight);
    keyRisoh.SetDword(TEXT("nRadLeft"), g_settings.nRadLeft);
    keyRisoh.SetDword(TEXT("nRadTop"), g_settings.nRadTop);
    keyRisoh.SetDword(TEXT("bAskUpdateResH"), g_settings.bAskUpdateResH);
    keyRisoh.SetDword(TEXT("bCompressByUPX"), g_settings.bCompressByUPX);
    keyRisoh.SetDword(TEXT("bUseBeginEnd"), g_settings.bUseBeginEnd);
    keyRisoh.SetSz(TEXT("strSrcFont"), g_settings.strSrcFont.c_str());
    keyRisoh.SetDword(TEXT("nSrcFontSize"), g_settings.nSrcFontSize);
    keyRisoh.SetSz(TEXT("strBinFont"), g_settings.strBinFont.c_str());
    keyRisoh.SetDword(TEXT("nBinFontSize"), g_settings.nBinFontSize);

    DWORD i, dwCount;

    // save the recently used files
    dwCount = (DWORD)g_settings.vecRecentlyUsed.size();
    if (dwCount > MAX_MRU)
        dwCount = MAX_MRU;
    keyRisoh.SetDword(TEXT("FileCount"), dwCount);
    TCHAR szValueName[128];
    for (i = 0; i < dwCount; ++i)
    {
        StringCchPrintf(szValueName, _countof(szValueName), TEXT("File%lu"), i);
        keyRisoh.SetSz(szValueName, g_settings.vecRecentlyUsed[i].c_str());
    }

    // save the ID association mapping
    for (auto& pair : g_settings.assoc_map)
    {
        keyRisoh.SetSz(pair.first.c_str(), pair.second.c_str());
        //MessageBoxW(NULL, pair.first.c_str(), pair.second.c_str(), 0);
    }

    // save the macros
    DWORD dwMacroCount = DWORD(g_settings.macros.size());
    keyRisoh.SetDword(TEXT("dwMacroCount"), dwMacroCount);
    i = 0;
    for (auto& macro : g_settings.macros)
    {
        StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroName%lu"), i);
        keyRisoh.SetSz(szValueName, macro.first.c_str());

        StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroValue%lu"), i);
        keyRisoh.SetSz(szValueName, macro.second.c_str());
        ++i;
    }

    // save the includes
    DWORD dwNumIncludes = DWORD(g_settings.includes.size());
    keyRisoh.SetDword(TEXT("dwNumIncludes"), dwNumIncludes);
    i = 0;
    for (auto& strInclude : g_settings.includes)
    {
        StringCchPrintf(szValueName, _countof(szValueName), TEXT("Include%lu"), i);
        keyRisoh.SetSz(szValueName, strInclude.c_str());
        ++i;
    }

    keyRisoh.SetSz(TEXT("strWindResExe"), g_settings.strWindResExe.c_str());
    keyRisoh.SetSz(TEXT("strCppExe"), g_settings.strCppExe.c_str());

    // always use old style
    keyRisoh.SetDword(TEXT("bOldStyle"), TRUE);

    keyRisoh.SetSz(TEXT("strPrevVersion"), TEXT(RE_VERSION));

    keyRisoh.SetDword(TEXT("bSepFilesByLang"), g_settings.bSepFilesByLang);
    keyRisoh.SetDword(TEXT("bStoreToResFolder"), g_settings.bStoreToResFolder);
    keyRisoh.SetDword(TEXT("bSelectableByMacro"), g_settings.bSelectableByMacro);

    // save the captions
    DWORD dwNumCaptions = DWORD(g_settings.captions.size());
    if (dwNumCaptions > s_nMaxCaptions)
        dwNumCaptions = s_nMaxCaptions;
    keyRisoh.SetDword(TEXT("dwNumCaptions"), dwNumCaptions);
    for (DWORD i = 0; i < dwNumCaptions; ++i)
    {
        StringCchPrintf(szValueName, _countof(szValueName), TEXT("Caption%lu"), i);
        keyRisoh.SetSz(szValueName, g_settings.captions[i].c_str());
    }

    keyRisoh.SetDword(TEXT("bShowToolBar"), g_settings.bShowToolBar);
    keyRisoh.SetSz(L"strAtlAxWin", g_settings.strAtlAxWin.c_str());
    keyRisoh.SetDword(TEXT("nSaveFilterIndex"), g_settings.nSaveFilterIndex);
    keyRisoh.SetDword(TEXT("bWordWrap"), g_settings.bWordWrap);
    keyRisoh.SetDword(TEXT("RCFileUTF16"), g_settings.bRCFileUTF16);

    keyRisoh.SetDword(TEXT("bBackup"), g_settings.bBackup);

    keyRisoh.SetSz(TEXT("strBackupSuffix"), TEXT(RE_VERSION));
    keyRisoh.SetSz(L"strBackupSuffix", g_settings.strBackupSuffix.c_str());

    keyRisoh.SetDword(TEXT("bRedundantComments"), g_settings.bRedundantComments);
    keyRisoh.SetDword(TEXT("bWrapManifest"), g_settings.bWrapManifest);

    DWORD encoding_count = DWORD(g_settings.encoding_map.size());
    keyRisoh.SetDword(TEXT("encoding_count"), encoding_count);
    {
        DWORD i = 0;
        for (auto pair : g_settings.encoding_map)
        {
            WCHAR szName[32];
            StringCchPrintfW(szName, _countof(szName), L"encoding%lu", i);

            WCHAR szText[64];
            StringCchPrintfW(szText, _countof(szText), L"%s:%s", pair.first.c_str(), pair.second.c_str());

            keyRisoh.SetSz(szName, szText);

            ++i;
        }
    }

    return TRUE;
}

BOOL MMainWnd::ReCreateSrcEdit(HWND hwnd)
{
    BOOL bModify = Edit_GetModify(m_hSrcEdit);

    if (IsWindow(m_hSrcEdit))
        DestroyWindow(m_hSrcEdit);

    DWORD style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
                  ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
                  ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
    if (!g_settings.bWordWrap)
    {
        style |= WS_HSCROLL | ES_AUTOHSCROLL;
    }

    DWORD exstyle = WS_EX_CLIENTEDGE;

    HWND hSrcEdit = ::CreateWindowEx(exstyle, L"EDIT", NULL, style,
        0, 0, 1, 1, m_splitter3, (HMENU)(INT_PTR)2, GetModuleHandle(NULL), NULL);
    if (hSrcEdit)
    {
        m_hSrcEdit.SubclassDx(hSrcEdit);

        Edit_SetModify(m_hSrcEdit, bModify);
        return TRUE;
    }
    return FALSE;
}

// WM_CREATE: the main window is to be created
BOOL MMainWnd::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    MWaitCursor wait;

    m_id_list_dlg.m_hMainWnd = hwnd;    // set the main window to the ID list window

    s_hMainWnd = hwnd;

    DoLoadLangInfo();   // load the language information

    // check the data
    INT nRet = CheckData();
    if (nRet)
        return FALSE;   // failure

    // load the RisohEditor settings
    LoadSettings(hwnd);

    if (g_settings.bResumeWindowPos)
    {
        // resume the main window pos
        if (g_settings.nWindowLeft != CW_USEDEFAULT)
        {
            POINT pt = { g_settings.nWindowLeft, g_settings.nWindowTop };
            SetWindowPosDx(&pt);
        }
        if (g_settings.nWindowWidth != CW_USEDEFAULT)
        {
            SIZE siz = { g_settings.nWindowWidth, g_settings.nWindowHeight };
            SetWindowPosDx(NULL, &siz);
        }
    }

    // create the image list for treeview
    m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 1);
    if (m_hImageList == NULL)
        return FALSE;

    // load some icons
    m_hFileIcon = LoadSmallIconDx(IDI_FILE);        // load a file icon
    m_hFolderIcon = LoadSmallIconDx(IDI_FOLDER);    // load a folder icon

    // add these icons
    ImageList_AddIcon(m_hImageList, m_hFileIcon);
    ImageList_AddIcon(m_hImageList, m_hFolderIcon);

    // create the tool image list for toolbar
    m_himlTools = (HIMAGELIST)ImageList_LoadBitmap(m_hInst, MAKEINTRESOURCE(IDB_TOOLBAR),
                                                   32, 8, RGB(255, 0, 255));
    if (m_himlTools == NULL)
    {
        DWORD dwError = GetLastError();
        DebugPrintDx(L"GetLastError(): %ld\n", dwError);
        return FALSE;
    }

    // create the toolbar
    if (!CreateOurToolBar(hwnd, m_himlTools))
    {
        return FALSE;
    }

    DWORD style, exstyle;

    // create the splitter windows
    style = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_LEFTALIGN;
    if (!m_splitter1.CreateDx(hwnd, 2, style))
        return FALSE;
    style = WS_CHILD | WS_VISIBLE | SWS_VERT | SWS_BOTTOMALIGN;
    if (!m_splitter2.CreateDx(m_splitter1, 2, style))
        return FALSE;
    style = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_RIGHTALIGN;
    if (!m_splitter3.CreateDx(m_splitter2, 1, style))
        return FALSE;

    // create a treeview (tree control) window
    style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
        TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_LINESATROOT |
        TVS_SHOWSELALWAYS | TVS_EDITLABELS | TVS_FULLROWSELECT | TVS_INFOTIP;
    m_hwndTV = CreateWindowExW(WS_EX_CLIENTEDGE,
        WC_TREEVIEWW, NULL, style, 0, 0, 0, 0, m_splitter1,
        (HMENU)1, m_hInst, NULL);
    if (m_hwndTV == NULL)
        return FALSE;

    // store the treeview handl to g_res (important!)
    g_res.m_hwndTV = m_hwndTV;

    if (s_pSetWindowTheme)
    {
        // apply Explorer's visual style
        (*s_pSetWindowTheme)(m_hwndTV, L"Explorer", NULL);
    }

    // set the imagelists to treeview
    TreeView_SetImageList(m_hwndTV, m_hImageList, TVSIL_NORMAL);

    // create the binary EDIT control
    style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
        ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
        ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
    exstyle = WS_EX_CLIENTEDGE;
    m_hBinEdit.CreateAsChildDx(m_splitter2, NULL, style, exstyle, 3);

    // create source EDIT control
    if (!ReCreateSrcEdit(hwnd))
        return FALSE;

    // create MBmpView
    if (!m_hBmpView.CreateDx(m_splitter3, 4))
        return FALSE;

    // create status bar
    style = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_BOTTOM;
    m_hStatusBar = CreateStatusWindow(style, LoadStringDx(IDS_STARTING), hwnd, 8);
    if (m_hStatusBar == NULL)
        return FALSE;

    // set the status text
    ChangeStatusText(IDS_STARTING);

    // setup the status bar
    INT anWidths[] = { -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 1, (LPARAM)anWidths);

    // show the status bar or not
    if (g_settings.bShowStatusBar)
        ShowWindow(m_hStatusBar, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hStatusBar, SW_HIDE);

    // set the pane contents of splitters
    m_splitter1.SetPane(0, m_hwndTV);
    m_splitter1.SetPane(1, m_splitter2);
    m_splitter1.SetPaneExtent(0, g_settings.nTreeViewWidth);
    m_splitter2.SetPane(0, m_splitter3);
    m_splitter2.SetPane(1, m_hBinEdit);
    m_splitter2.SetPaneExtent(1, BE_HEIGHT);
    m_splitter3.SetPane(0, m_hSrcEdit);
    //m_splitter3.SetPane(1, m_hBmpView);

    // create the fonts
    ReCreateFonts(hwnd);

    if (m_argc >= 2)
    {
        // load the file now
        DoLoadFile(hwnd, m_targv[1]);
    }

    // enable file dropping
    DragAcceptFiles(hwnd, TRUE);

    // set focus to treeview
    SetFocus(m_hwndTV);

    // update the menu
    UpdateMenu();

    // store the file paths from settings
    if (g_settings.strWindResExe.size())
    {
        StringCchCopy(m_szWindresExe, _countof(m_szWindresExe), g_settings.strWindResExe.c_str());
    }
    if (g_settings.strCppExe.size())
    {
        StringCchCopy(m_szMCppExe, _countof(m_szMCppExe), g_settings.strCppExe.c_str());
    }

    // OK, ready
    PostMessageDx(WM_COMMAND, ID_READY);

    return TRUE;    // success
}

// the window procedure of the main window
/*virtual*/ LRESULT CALLBACK
MMainWnd::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT s_uFindMsg = RegisterWindowMessage(FINDMSGSTRING);
    switch (uMsg)
    {
        DO_MSG(WM_CREATE, OnCreate);
        DO_MSG(WM_COMMAND, OnCommand);
        DO_MSG(WM_CLOSE, OnClose);
        DO_MSG(WM_DESTROY, OnDestroy);
        DO_MSG(WM_DROPFILES, OnDropFiles);
        DO_MSG(WM_MOVE, OnMove);
        DO_MSG(WM_SIZE, OnSize);
        DO_MSG(WM_NOTIFY, OnNotify);
        DO_MSG(WM_CONTEXTMENU, OnContextMenu);
        DO_MSG(WM_INITMENU, OnInitMenu);
        DO_MSG(WM_ACTIVATE, OnActivate);
        DO_MSG(WM_SYSCOLORCHANGE, OnSysColorChange);
        DO_MESSAGE(MYWM_CLEARSTATUS, OnClearStatus);
        DO_MESSAGE(MYWM_MOVESIZEREPORT, OnMoveSizeReport);
        DO_MESSAGE(MYWM_COMPILECHECK, OnCompileCheck);
        DO_MESSAGE(MYWM_REOPENRAD, OnReopenRad);
        DO_MESSAGE(MYWM_POSTSEARCH, OnPostSearch);
        DO_MESSAGE(MYWM_IDJUMPBANG, OnIDJumpBang);
        DO_MESSAGE(MYWM_SELCHANGE, OnRadSelChange);
        DO_MESSAGE(MYWM_UPDATEDLGRES, OnUpdateDlgRes);
        DO_MESSAGE(MYWM_GETDLGHEADLINES, OnGetHeadLines);
        DO_MESSAGE(MYWM_DELPHI_DFM_B2T, OnDelphiDFMB2T);

    default:
        if (uMsg == s_uFindMsg)
        {
            return OnFindMsg(hwnd, wParam, lParam);
        }
        return DefaultProcDx();
    }
}

// select the string entry in the tree control
void MMainWnd::SelectString(void)
{
    // find the string entry
    if (auto entry = g_res.find(ET_STRING, RT_STRING))
    {
        // select the entry
        SelectTV(entry, FALSE);
    }
}

// select the message entry in the tree control
void MMainWnd::SelectMessage()
{
    // find the message table entry
    if (auto entry = g_res.find(ET_MESSAGE, RT_MESSAGETABLE))
    {
        // select the entry
        SelectTV(entry, FALSE);
    }
}

// do ID jump now!
void MMainWnd::OnIDJumpBang2(HWND hwnd, const MString& name, MString& strType)
{
    if (strType == L"Unknown.ID")
        return;     // ignore Unknown.ID jump

    // revert the resource type string
    Res_ReplaceResTypeString(strType, true);

    // get the prefix
    MString prefix = name.substr(0, name.find(L'_') + 1);

    // get the IDTYPE_'s from the prefix
    auto indexes = GetPrefixIndexes(prefix);
    for (size_t i = 0; i < indexes.size(); ++i)
    {
        INT nIDTYPE_ = indexes[i];
        if (nIDTYPE_ == IDTYPE_STRING || nIDTYPE_ == IDTYPE_PROMPT)
        {
            // select the string entry
            SelectString();
            return;     // done
        }
        if (nIDTYPE_ == IDTYPE_MESSAGE)
        {
            // select the message entry
            SelectMessage();
            return;     // done
        }
    }

    // get the type value
    MIdOrString type = WORD(g_db.GetValue(L"RESOURCE", strType));
    if (type.empty())
        type.m_str = strType;

    // name --> name_or_id
    MIdOrString name_or_id;
    if (name[0] == L'\"')
    {
        // non-numeric name
        MString name_clone = name;
        mstr_unquote(name_clone);   // unquote
        name_or_id = name_clone.c_str();
    }
    else
    {
        // numeric name
        name_or_id = WORD(g_db.GetResIDValue(name));
    }

    if (name_or_id.empty())  // name_or_id was empty
    {
        // name --> strA (ANSI)
        MStringA strA = MTextToAnsi(CP_ACP, name).c_str();

        // find strA from g_settings.id_map
        auto it = g_settings.id_map.find(strA);
        if (it != g_settings.id_map.end())  // found
        {
            MStringA strA = it->second;
            if (strA[0] == 'L')
                strA = strA.substr(1);

            // unquote
            mstr_unquote(strA);

            // resource name
            name_or_id.m_str = MAnsiToWide(CP_ACP, strA).c_str();
        }
    }

    // find the entry
    if (auto entry = g_res.find(ET_LANG, type, name_or_id))
    {
        // select the entry
        SelectTV(entry, FALSE);

        // set focus to the main window
        SetForegroundWindow(m_hwnd);
        BringWindowToTop(m_hwnd);
        SetFocus(m_hwnd);
    }
}

LRESULT MMainWnd::OnDelphiDFMB2T(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    auto& str = *(MString *)wParam;
    auto& entry = *(const EntryBase *)lParam;

    auto ansi = dfm_text_from_binary(m_szDFMSC, entry.ptr(), entry.size());
    MAnsiToWide a2w(CP_ACP, ansi.c_str());
    str = a2w.c_str();
    return 0;
}

LRESULT MMainWnd::OnGetHeadLines(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // get the selected entry
    auto entry = g_res.get_lang_entry();
    if (!entry)
        return -1;

    if (entry->m_type == RT_DIALOG)
    {
        DialogRes dialog_res;
        MByteStreamEx stream(entry->m_data);
        dialog_res.LoadFromStream(stream);
        return dialog_res.GetHeadLines();;
    }
    return -1;
}

LRESULT MMainWnd::OnUpdateDlgRes(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DoSetFileModified(TRUE);

    // get the selected language entry
    auto entry = g_res.get_lang_entry();
    if (!entry || entry->m_type != RT_DIALOG)
    {
        return 0;
    }

    auto& dialog_res = m_rad_window.m_dialog_res;

    // dialog_res --> entry->m_data
    MByteStreamEx stream;
    dialog_res.SaveToStream(stream);
    entry->m_data = stream.data();

    // entry->m_lang + dialog_res --> str --> m_hSrcEdit (text)
    MString str = GetLanguageStatement(entry->m_lang);
    str += dialog_res.Dump(entry->m_name);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    // entry->m_data --> m_hBinEdit (binary)
    str = DumpBinaryAsText(entry->m_data);
    SetWindowTextW(m_hBinEdit, str.c_str());

    return 0;
}

LRESULT MMainWnd::OnRadSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (!IsWindow(m_rad_window))
    {
        m_hSrcEdit.ClearIndeces();
        return 0;
    }

    auto indeces = MRadCtrl::GetTargetIndeces();
    m_hSrcEdit.SetIndeces(indeces);
    return 0;
}

// do ID jump now!
LRESULT MMainWnd::OnIDJumpBang(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // the item index
    INT iItem = (INT)wParam;
    if (iItem == -1)
        return 0;

    // get the 1st and 2nd subitem texts
    TCHAR szText[128];
    ListView_GetItemText(m_id_list_dlg.m_hLst1, iItem, 0, szText, _countof(szText));
    MString name = szText;      // 1st is name
    ListView_GetItemText(m_id_list_dlg.m_hLst1, iItem, 1, szText, _countof(szText));
    MString strTypes = szText;  // 2nd is types

    // split the text to the types by slash
    std::vector<MString> vecTypes;
    mstr_split(vecTypes, strTypes, L"/");

    // ignore if no type
    if (vecTypes.empty() || vecTypes.size() <= size_t(lParam))
        return 0;

    // do ID jump
    OnIDJumpBang2(hwnd, name, vecTypes[lParam]);

    return 0;
}

// do something after find
LRESULT MMainWnd::OnPostSearch(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // reset the direction
    m_fr.Flags = FR_DOWN;

    if (m_search.bInternalText)
    {
        // m_search.strText --> m_szFindWhat
        StringCchCopy(m_szFindWhat, _countof(m_szFindWhat), m_search.strText.c_str());

        if (!m_search.bIgnoreCases)
        {
            m_fr.Flags |= FR_MATCHCASE;
        }

        // do search the text from source
        OnFindNext(hwnd);
    }

    return 0;
}

// start up the program
BOOL MMainWnd::StartDx()
{
    // get cursors for MSplitterWnd
    MSplitterWnd::CursorNS() = LoadCursor(m_hInst, MAKEINTRESOURCE(IDC_CURSORNS));
    MSplitterWnd::CursorWE() = LoadCursor(m_hInst, MAKEINTRESOURCE(IDC_CURSORWE));

    // get the main icon
    m_hIcon = LoadIconDx(IDI_MAIN);
    m_hIconSm = LoadSmallIconDx(IDI_MAIN);

    // get the access keys
    m_hAccel = ::LoadAccelerators(m_hInst, MAKEINTRESOURCE(IDR_MAINACCEL));

    // create the main window
    if (!CreateWindowDx(NULL, MAKEINTRESOURCE(IDS_APPNAME),
        WS_OVERLAPPEDWINDOW, 0, CW_USEDEFAULT, CW_USEDEFAULT, 760, 480))
    {
        ErrorBoxDx(TEXT("failure of CreateWindow"));
        return FALSE;
    }
    assert(IsWindow(m_hwnd));

    // maximize or not
    if (g_settings.bResumeWindowPos && g_settings.bMaximized)
    {
        ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
    }
    else
    {
        ShowWindow(m_hwnd, SW_SHOWNORMAL);
    }

    UpdateWindow(m_hwnd);

    return TRUE;
}

// do one window message
void MMainWnd::DoEvents()
{
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        DoMsg(msg);
    }
}

// do the window messages
void MMainWnd::DoMsg(MSG& msg)
{
    //// EDIT control Ctrl+A
    //if (MEditCtrl::DoMsgCtrlA(&msg))
    //    return;

    // do the popup windows
    if (IsWindow(m_rad_window.m_rad_dialog))
    {
        if (::IsDialogMessage(m_rad_window.m_rad_dialog, &msg))
            return;
    }
    if (IsWindow(m_id_list_dlg))
    {
        if (::IsDialogMessage(m_id_list_dlg, &msg))
            return;
    }

    // close the find/replace dialog if any
    if (IsWindow(m_hFindReplaceDlg))
    {
        if (::IsDialogMessage(m_hFindReplaceDlg, &msg))
            return;
    }

    // do access keys
    if (m_hAccel && IsWindow(m_hwnd))
    {
        if (::TranslateAccelerator(m_hwnd, m_hAccel, &msg))
            return;
    }

    // do the item search dialogs
    if (MItemSearchDlg::Dialogs().size())
    {
        BOOL bProcessed = FALSE;
        for (auto& item : MItemSearchDlg::Dialogs())
        {
            if (IsDialogMessage(*item, &msg))
            {
                bProcessed = TRUE;
                break;
            }
        }
        if (bProcessed)
            return;
    }

    // the default processing
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}

// the main loop
INT_PTR MMainWnd::RunDx()
{
    MSG msg;

    while (BOOL bGot = ::GetMessage(&msg, NULL, 0, 0))
    {
        if (bGot < 0)   // fatal error
        {
            DebugPrintDx(TEXT("Application fatal error: %ld\n"), GetLastError());
            DebugBreak();
            return -1;
        }

        // do messaging
        DoMsg(msg);
    }
    return INT(msg.wParam);
}

//////////////////////////////////////////////////////////////////////////////

// get the LANGUAGE statement
MString GetLanguageStatement(WORD langid, BOOL bOldStyle)
{
    MString strPrim, strSub;

#define SWITCH_SUBLANG() switch (SUBLANGID(langid))

    // try to get the primary language name and the sub-language name
    switch (PRIMARYLANGID(langid))
    {
    case LANG_NEUTRAL: strPrim = TEXT("LANG_NEUTRAL");
        break;
    case LANG_INVARIANT: strPrim = TEXT("LANG_INVARIANT");
        break;
    case LANG_AFRIKAANS: strPrim = TEXT("LANG_AFRIKAANS");
        SWITCH_SUBLANG()
        {
        case SUBLANG_AFRIKAANS_SOUTH_AFRICA: strSub = TEXT("SUBLANG_AFRIKAANS_SOUTH_AFRICA"); break;
        }
        break;
    case LANG_ALBANIAN: strPrim = TEXT("LANG_ALBANIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ALBANIAN_ALBANIA: strSub = TEXT("SUBLANG_ALBANIAN_ALBANIA"); break;
        }
        break;
    case LANG_ALSATIAN: strPrim = TEXT("LANG_ALSATIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ALSATIAN_FRANCE: strSub = TEXT("SUBLANG_ALSATIAN_FRANCE"); break;
        }
        break;
    case LANG_AMHARIC: strPrim = TEXT("LANG_AMHARIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_AMHARIC_ETHIOPIA: strSub = TEXT("SUBLANG_AMHARIC_ETHIOPIA"); break;
        }
        break;
    case LANG_ARABIC: strPrim = TEXT("LANG_ARABIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ARABIC_SAUDI_ARABIA: strSub = TEXT("SUBLANG_ARABIC_SAUDI_ARABIA"); break;
        case SUBLANG_ARABIC_IRAQ: strSub = TEXT("SUBLANG_ARABIC_IRAQ"); break;
        case SUBLANG_ARABIC_EGYPT: strSub = TEXT("SUBLANG_ARABIC_EGYPT"); break;
        case SUBLANG_ARABIC_LIBYA: strSub = TEXT("SUBLANG_ARABIC_LIBYA"); break;
        case SUBLANG_ARABIC_ALGERIA: strSub = TEXT("SUBLANG_ARABIC_ALGERIA"); break;
        case SUBLANG_ARABIC_MOROCCO: strSub = TEXT("SUBLANG_ARABIC_MOROCCO"); break;
        case SUBLANG_ARABIC_TUNISIA: strSub = TEXT("SUBLANG_ARABIC_TUNISIA"); break;
        case SUBLANG_ARABIC_OMAN: strSub = TEXT("SUBLANG_ARABIC_OMAN"); break;
        case SUBLANG_ARABIC_YEMEN: strSub = TEXT("SUBLANG_ARABIC_YEMEN"); break;
        case SUBLANG_ARABIC_SYRIA: strSub = TEXT("SUBLANG_ARABIC_SYRIA"); break;
        case SUBLANG_ARABIC_JORDAN: strSub = TEXT("SUBLANG_ARABIC_JORDAN"); break;
        case SUBLANG_ARABIC_LEBANON: strSub = TEXT("SUBLANG_ARABIC_LEBANON"); break;
        case SUBLANG_ARABIC_KUWAIT: strSub = TEXT("SUBLANG_ARABIC_KUWAIT"); break;
        case SUBLANG_ARABIC_UAE: strSub = TEXT("SUBLANG_ARABIC_UAE"); break;
        case SUBLANG_ARABIC_BAHRAIN: strSub = TEXT("SUBLANG_ARABIC_BAHRAIN"); break;
        case SUBLANG_ARABIC_QATAR: strSub = TEXT("SUBLANG_ARABIC_QATAR"); break;
        }
        break;
    case LANG_ARMENIAN: strPrim = TEXT("LANG_ARMENIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ARMENIAN_ARMENIA: strSub = TEXT("SUBLANG_ARMENIAN_ARMENIA"); break;
        }
        break;
    case LANG_ASSAMESE: strPrim = TEXT("LANG_ASSAMESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ASSAMESE_INDIA: strSub = TEXT("SUBLANG_ASSAMESE_INDIA"); break;
        }
        break;
    case LANG_AZERI: strPrim = TEXT("LANG_AZERI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_AZERI_LATIN: strSub = TEXT("SUBLANG_AZERI_LATIN"); break;
        case SUBLANG_AZERI_CYRILLIC: strSub = TEXT("SUBLANG_AZERI_CYRILLIC"); break;
        }
        break;
    //case LANG_AZERBAIJANI: strPrim = TEXT("LANG_AZERBAIJANI"); // same as LANG_AZERI
    //    SWITCH_SUBLANG()
    //    {
    //        case SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN: strSub = TEXT("SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN"); break;
    //        case SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC: strSub = TEXT("SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC"); break;
    //    }
    //    break;
    //case LANG_BANGLA: strPrim = TEXT("LANG_BANGLA"); // same as LANG_BENGALI
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_BANGLA_INDIA: strSub = TEXT("SUBLANG_BANGLA_INDIA"); break;
    //    case SUBLANG_BANGLA_BANGLADESH: strSub = TEXT("SUBLANG_BANGLA_BANGLADESH"); break;
    //    }
    //    break;
    case LANG_BASHKIR: strPrim = TEXT("LANG_BASHKIR");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BASHKIR_RUSSIA: strSub = TEXT("SUBLANG_BASHKIR_RUSSIA"); break;
        }
        break;
    case LANG_BASQUE: strPrim = TEXT("LANG_BASQUE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BASQUE_BASQUE: strSub = TEXT("SUBLANG_BASQUE_BASQUE"); break;
        }
        break;
    case LANG_BELARUSIAN: strPrim = TEXT("LANG_BELARUSIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BELARUSIAN_BELARUS: strSub = TEXT("SUBLANG_BELARUSIAN_BELARUS"); break;
        }
        break;
    case LANG_BENGALI: strPrim = TEXT("LANG_BENGALI");
        SWITCH_SUBLANG()
        {
            case SUBLANG_BENGALI_INDIA: strSub = TEXT("SUBLANG_BENGALI_INDIA"); break;
            case SUBLANG_BENGALI_BANGLADESH: strSub = TEXT("SUBLANG_BENGALI_BANGLADESH"); break;
        }
        break;
    case LANG_BRETON: strPrim = TEXT("LANG_BRETON");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BRETON_FRANCE: strSub = TEXT("SUBLANG_BRETON_FRANCE"); break;
        }
        break;
    case LANG_BOSNIAN: strPrim = TEXT("LANG_BOSNIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
        case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
        }
        break;
    case LANG_BOSNIAN_NEUTRAL: strPrim = TEXT("LANG_BOSNIAN_NEUTRAL");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
        case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
        }
        break;
    case LANG_BULGARIAN: strPrim = TEXT("LANG_BULGARIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BULGARIAN_BULGARIA: strSub = TEXT("SUBLANG_BULGARIAN_BULGARIA"); break;
        }
        break;
    case LANG_CATALAN: strPrim = TEXT("LANG_CATALAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CATALAN_CATALAN: strSub = TEXT("SUBLANG_CATALAN_CATALAN"); break;
        }
        break;
#ifdef ENABLE_NEW_LANGS
    case LANG_CENTRAL_KURDISH: strPrim = TEXT("LANG_CENTRAL_KURDISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CENTRAL_KURDISH_IRAQ: strSub = TEXT("SUBLANG_CENTRAL_KURDISH_IRAQ"); break;
        }
        break;
#endif
#ifdef ENABLE_NEW_LANGS
    case LANG_CHEROKEE: strPrim = TEXT("LANG_CHEROKEE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CHEROKEE_CHEROKEE: strSub = TEXT("SUBLANG_CHEROKEE_CHEROKEE"); break;
        }
        break;
#endif
    case LANG_CHINESE: strPrim = TEXT("LANG_CHINESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
        case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
        case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
        case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
        case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
        }
        break;
    //case LANG_CHINESE_SIMPLIFIED: strPrim = TEXT("LANG_CHINESE_SIMPLIFIED"); // same as LANG_CHINESE
    //    SWITCH_SUBLANG()
    //    {
    //        case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
    //        case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
    //        case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
    //        case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
    //        case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
    //    }
    //    break;
    case LANG_CHINESE_TRADITIONAL: strPrim = TEXT("LANG_CHINESE_TRADITIONAL");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
        case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
        case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
        case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
        case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
        }
        break;
    case LANG_CORSICAN: strPrim = TEXT("LANG_CORSICAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CORSICAN_FRANCE: strSub = TEXT("SUBLANG_CORSICAN_FRANCE"); break;
        }
        break;
    //case LANG_CROATIAN: strPrim = TEXT("LANG_CROATIAN"); // same as LANG_BOSNIAN
    //    SWITCH_SUBLANG()
    //    {
    //        case SUBLANG_CROATIAN_CROATIA: strSub = TEXT("SUBLANG_CROATIAN_CROATIA"); break;
    //        case SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
    //    }
    //    break;
    case LANG_CZECH: strPrim = TEXT("LANG_CZECH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CZECH_CZECH_REPUBLIC: strSub = TEXT("SUBLANG_CZECH_CZECH_REPUBLIC"); break;
        }
        break;
    case LANG_DANISH: strPrim = TEXT("LANG_DANISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_DANISH_DENMARK: strSub = TEXT("SUBLANG_DANISH_DENMARK"); break;
        }
        break;
    case LANG_DARI: strPrim = TEXT("LANG_DARI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_DARI_AFGHANISTAN: strSub = TEXT("SUBLANG_DARI_AFGHANISTAN"); break;
        }
        break;
    case LANG_DIVEHI: strPrim = TEXT("LANG_DIVEHI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_DIVEHI_MALDIVES: strSub = TEXT("SUBLANG_DIVEHI_MALDIVES"); break;
        }
        break;
    case LANG_DUTCH: strPrim = TEXT("LANG_DUTCH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_DUTCH: strSub = TEXT("SUBLANG_DUTCH"); break;
        case SUBLANG_DUTCH_BELGIAN: strSub = TEXT("SUBLANG_DUTCH_BELGIAN"); break;
        }
        break;
    case LANG_ENGLISH: strPrim = TEXT("LANG_ENGLISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ENGLISH_US: strSub = TEXT("SUBLANG_ENGLISH_US"); break;
        case SUBLANG_ENGLISH_UK: strSub = TEXT("SUBLANG_ENGLISH_UK"); break;
        case SUBLANG_ENGLISH_AUS: strSub = TEXT("SUBLANG_ENGLISH_AUS"); break;
        case SUBLANG_ENGLISH_CAN: strSub = TEXT("SUBLANG_ENGLISH_CAN"); break;
        case SUBLANG_ENGLISH_NZ: strSub = TEXT("SUBLANG_ENGLISH_NZ"); break;
        //case SUBLANG_ENGLISH_IRELAND: strSub = TEXT("SUBLANG_ENGLISH_IRELAND"); break; // same as SUBLANG_ENGLISH_EIRE
        case SUBLANG_ENGLISH_EIRE: strSub = TEXT("SUBLANG_ENGLISH_EIRE"); break;
        case SUBLANG_ENGLISH_SOUTH_AFRICA: strSub = TEXT("SUBLANG_ENGLISH_SOUTH_AFRICA"); break;
        case SUBLANG_ENGLISH_JAMAICA: strSub = TEXT("SUBLANG_ENGLISH_JAMAICA"); break;
        case SUBLANG_ENGLISH_CARIBBEAN: strSub = TEXT("SUBLANG_ENGLISH_CARIBBEAN"); break;
        case SUBLANG_ENGLISH_BELIZE: strSub = TEXT("SUBLANG_ENGLISH_BELIZE"); break;
        case SUBLANG_ENGLISH_TRINIDAD: strSub = TEXT("SUBLANG_ENGLISH_TRINIDAD"); break;
        case SUBLANG_ENGLISH_ZIMBABWE: strSub = TEXT("SUBLANG_ENGLISH_ZIMBABWE"); break;
        case SUBLANG_ENGLISH_PHILIPPINES: strSub = TEXT("SUBLANG_ENGLISH_PHILIPPINES"); break;
        case SUBLANG_ENGLISH_INDIA: strSub = TEXT("SUBLANG_ENGLISH_INDIA"); break;
        case SUBLANG_ENGLISH_MALAYSIA: strSub = TEXT("SUBLANG_ENGLISH_MALAYSIA"); break;
        case SUBLANG_ENGLISH_SINGAPORE: strSub = TEXT("SUBLANG_ENGLISH_SINGAPORE"); break;
        }
        break;
    case LANG_ESTONIAN: strPrim = TEXT("LANG_ESTONIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ESTONIAN_ESTONIA: strSub = TEXT("SUBLANG_ESTONIAN_ESTONIA"); break;
        }
        break;
    case LANG_FAEROESE: strPrim = TEXT("LANG_FAEROESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FAEROESE_FAROE_ISLANDS: strSub = TEXT("SUBLANG_FAEROESE_FAROE_ISLANDS"); break;
        }
        break;
    //case LANG_FARSI: strPrim = TEXT("LANG_FARSI"); // same as LANG_PERSIAN
    //    break;
    case LANG_FILIPINO: strPrim = TEXT("LANG_FILIPINO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FILIPINO_PHILIPPINES: strSub = TEXT("SUBLANG_FILIPINO_PHILIPPINES"); break;
        }
        break;
    case LANG_FINNISH: strPrim = TEXT("LANG_FINNISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FINNISH_FINLAND: strSub = TEXT("SUBLANG_FINNISH_FINLAND"); break;
        }
        break;
    case LANG_FRENCH: strPrim = TEXT("LANG_FRENCH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FRENCH: strSub = TEXT("SUBLANG_FRENCH"); break;
        case SUBLANG_FRENCH_BELGIAN: strSub = TEXT("SUBLANG_FRENCH_BELGIAN"); break;
        case SUBLANG_FRENCH_CANADIAN: strSub = TEXT("SUBLANG_FRENCH_CANADIAN"); break;
        case SUBLANG_FRENCH_SWISS: strSub = TEXT("SUBLANG_FRENCH_SWISS"); break;
        case SUBLANG_FRENCH_LUXEMBOURG: strSub = TEXT("SUBLANG_FRENCH_LUXEMBOURG"); break;
        case SUBLANG_FRENCH_MONACO: strSub = TEXT("SUBLANG_FRENCH_MONACO"); break;
        }
        break;
    case LANG_FRISIAN: strPrim = TEXT("LANG_FRISIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FRISIAN_NETHERLANDS: strSub = TEXT("SUBLANG_FRISIAN_NETHERLANDS"); break;
        }
        break;
    //case LANG_FULAH: strPrim = TEXT("LANG_FULAH"); // same as LANG_PULAR
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_FULAH_SENEGAL: strSub = TEXT("SUBLANG_FULAH_SENEGAL"); break;
    //    }
    //    break;
    case LANG_GALICIAN: strPrim = TEXT("LANG_GALICIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GALICIAN_GALICIAN: strSub = TEXT("SUBLANG_GALICIAN_GALICIAN"); break;
        }
        break;
    case LANG_GEORGIAN: strPrim = TEXT("LANG_GEORGIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GEORGIAN_GEORGIA: strSub = TEXT("SUBLANG_GEORGIAN_GEORGIA"); break;
        }
        break;
    case LANG_GERMAN: strPrim = TEXT("LANG_GERMAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GERMAN: strSub = TEXT("SUBLANG_GERMAN"); break;
        case SUBLANG_GERMAN_SWISS: strSub = TEXT("SUBLANG_GERMAN_SWISS"); break;
        case SUBLANG_GERMAN_AUSTRIAN: strSub = TEXT("SUBLANG_GERMAN_AUSTRIAN"); break;
        case SUBLANG_GERMAN_LUXEMBOURG: strSub = TEXT("SUBLANG_GERMAN_LUXEMBOURG"); break;
        case SUBLANG_GERMAN_LIECHTENSTEIN: strSub = TEXT("SUBLANG_GERMAN_LIECHTENSTEIN"); break;
        }
        break;
    case LANG_GREEK: strPrim = TEXT("LANG_GREEK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GREEK_GREECE: strSub = TEXT("SUBLANG_GREEK_GREECE"); break;
        }
        break;
    case LANG_GREENLANDIC: strPrim = TEXT("LANG_GREENLANDIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GREENLANDIC_GREENLAND: strSub = TEXT("SUBLANG_GREENLANDIC_GREENLAND"); break;
        }
        break;
    case LANG_GUJARATI: strPrim = TEXT("LANG_GUJARATI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GUJARATI_INDIA: strSub = TEXT("SUBLANG_GUJARATI_INDIA"); break;
        }
        break;
    case LANG_HAUSA: strPrim = TEXT("LANG_HAUSA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_HAUSA_NIGERIA_LATIN: strSub = TEXT("SUBLANG_HAUSA_NIGERIA_LATIN"); break;
        }
        break;
    case LANG_HEBREW: strPrim = TEXT("LANG_HEBREW");
        SWITCH_SUBLANG()
        {
        case SUBLANG_HEBREW_ISRAEL: strSub = TEXT("SUBLANG_HEBREW_ISRAEL"); break;
        }
        break;
    case LANG_HINDI: strPrim = TEXT("LANG_HINDI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_HINDI_INDIA: strSub = TEXT("SUBLANG_HINDI_INDIA"); break;
        }
        break;
    case LANG_HUNGARIAN: strPrim = TEXT("LANG_HUNGARIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_HUNGARIAN_HUNGARY: strSub = TEXT("SUBLANG_HUNGARIAN_HUNGARY"); break;
        }
        break;
    case LANG_ICELANDIC: strPrim = TEXT("LANG_ICELANDIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ICELANDIC_ICELAND: strSub = TEXT("SUBLANG_ICELANDIC_ICELAND"); break;
        }
        break;
    case LANG_IGBO: strPrim = TEXT("LANG_IGBO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_IGBO_NIGERIA: strSub = TEXT("SUBLANG_IGBO_NIGERIA"); break;
        }
        break;
    case LANG_INDONESIAN: strPrim = TEXT("LANG_INDONESIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_INDONESIAN_INDONESIA: strSub = TEXT("SUBLANG_INDONESIAN_INDONESIA"); break;
        }
        break;
    case LANG_INUKTITUT: strPrim = TEXT("LANG_INUKTITUT");
        SWITCH_SUBLANG()
        {
        case SUBLANG_INUKTITUT_CANADA: strSub = TEXT("SUBLANG_INUKTITUT_CANADA"); break;
        case SUBLANG_INUKTITUT_CANADA_LATIN: strSub = TEXT("SUBLANG_INUKTITUT_CANADA_LATIN"); break;
        }
        break;
    case LANG_IRISH: strPrim = TEXT("LANG_IRISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_IRISH_IRELAND: strSub = TEXT("SUBLANG_IRISH_IRELAND"); break;
        }
        break;
    case LANG_ITALIAN: strPrim = TEXT("LANG_ITALIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ITALIAN: strSub = TEXT("SUBLANG_ITALIAN"); break;
        case SUBLANG_ITALIAN_SWISS: strSub = TEXT("SUBLANG_ITALIAN_SWISS"); break;
        }
        break;
    case LANG_JAPANESE: strPrim = TEXT("LANG_JAPANESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_JAPANESE_JAPAN: strSub = TEXT("SUBLANG_JAPANESE_JAPAN"); break;
        }
        break;
    case LANG_KANNADA: strPrim = TEXT("LANG_KANNADA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KANNADA_INDIA: strSub = TEXT("SUBLANG_KANNADA_INDIA"); break;
        }
        break;
    case LANG_KASHMIRI: strPrim = TEXT("LANG_KASHMIRI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KASHMIRI_INDIA: strSub = TEXT("SUBLANG_KASHMIRI_INDIA"); break;
        //case SUBLANG_KASHMIRI_SASIA: strSub = TEXT("SUBLANG_KASHMIRI_SASIA"); break; // same as SUBLANG_KASHMIRI_INDIA
        }
        break;
    case LANG_KAZAK: strPrim = TEXT("LANG_KAZAK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KAZAK_KAZAKHSTAN: strSub = TEXT("SUBLANG_KAZAK_KAZAKHSTAN"); break;
        }
        break;
    case LANG_KHMER: strPrim = TEXT("LANG_KHMER");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KHMER_CAMBODIA: strSub = TEXT("SUBLANG_KHMER_CAMBODIA"); break;
        }
        break;
    case LANG_KICHE: strPrim = TEXT("LANG_KICHE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KICHE_GUATEMALA: strSub = TEXT("SUBLANG_KICHE_GUATEMALA"); break;
        }
        break;
    case LANG_KINYARWANDA: strPrim = TEXT("LANG_KINYARWANDA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KINYARWANDA_RWANDA: strSub = TEXT("SUBLANG_KINYARWANDA_RWANDA"); break;
        }
        break;
    case LANG_KONKANI: strPrim = TEXT("LANG_KONKANI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KONKANI_INDIA: strSub = TEXT("SUBLANG_KONKANI_INDIA"); break;
        }
        break;
    case LANG_KOREAN: strPrim = TEXT("LANG_KOREAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KOREAN: strSub = TEXT("SUBLANG_KOREAN"); break;
        }
        break;
    case LANG_KYRGYZ: strPrim = TEXT("LANG_KYRGYZ");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KYRGYZ_KYRGYZSTAN: strSub = TEXT("SUBLANG_KYRGYZ_KYRGYZSTAN"); break;
        }
        break;
    case LANG_LAO: strPrim = TEXT("LANG_LAO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_LAO_LAO: strSub = TEXT("SUBLANG_LAO_LAO"); break;
        }
        break;
    case LANG_LATVIAN: strPrim = TEXT("LANG_LATVIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_LATVIAN_LATVIA: strSub = TEXT("SUBLANG_LATVIAN_LATVIA"); break;
        }
        break;
    case LANG_LITHUANIAN: strPrim = TEXT("LANG_LITHUANIAN");
#if (WINVER >= 0x0600) && defined(ENABLE_NEW_LANGS)
        SWITCH_SUBLANG()
        {
        case SUBLANG_LITHUANIAN_LITHUANIA: strSub = TEXT("SUBLANG_LITHUANIAN_LITHUANIA"); break;
        }
#endif
        break;
    //case LANG_LOWER_SORBIAN: strPrim = TEXT("LANG_LOWER_SORBIAN"); // same as LANG_UPPER_SORBIAN
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_LOWER_SORBIAN_GERMANY: strSub = TEXT("SUBLANG_LOWER_SORBIAN_GERMANY"); break;
    //    }
    //    break;
    case LANG_LUXEMBOURGISH: strPrim = TEXT("LANG_LUXEMBOURGISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_LUXEMBOURGISH_LUXEMBOURG: strSub = TEXT("SUBLANG_LUXEMBOURGISH_LUXEMBOURG"); break;
        }
        break;
    case LANG_MACEDONIAN: strPrim = TEXT("LANG_MACEDONIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MACEDONIAN_MACEDONIA: strSub = TEXT("SUBLANG_MACEDONIAN_MACEDONIA"); break;
        }
        break;
    case LANG_MALAY: strPrim = TEXT("LANG_MALAY");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MALAY_MALAYSIA: strSub = TEXT("SUBLANG_MALAY_MALAYSIA"); break;
        case SUBLANG_MALAY_BRUNEI_DARUSSALAM: strSub = TEXT("SUBLANG_MALAY_BRUNEI_DARUSSALAM"); break;
        }
        break;
    case LANG_MALAYALAM: strPrim = TEXT("LANG_MALAYALAM");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MALAYALAM_INDIA: strSub = TEXT("SUBLANG_MALAYALAM_INDIA"); break;
        }
        break;
    case LANG_MALTESE: strPrim = TEXT("LANG_MALTESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MALTESE_MALTA: strSub = TEXT("SUBLANG_MALTESE_MALTA"); break;
        }
        break;
    case LANG_MANIPURI: strPrim = TEXT("LANG_MANIPURI");
        break;
    case LANG_MAORI: strPrim = TEXT("LANG_MAORI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MAORI_NEW_ZEALAND: strSub = TEXT("SUBLANG_MAORI_NEW_ZEALAND"); break;
        }
        break;
    case LANG_MAPUDUNGUN: strPrim = TEXT("LANG_MAPUDUNGUN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MAPUDUNGUN_CHILE: strSub = TEXT("SUBLANG_MAPUDUNGUN_CHILE"); break;
        }
        break;
    case LANG_MARATHI: strPrim = TEXT("LANG_MARATHI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MARATHI_INDIA: strSub = TEXT("SUBLANG_MARATHI_INDIA"); break;
        }
        break;
    case LANG_MOHAWK: strPrim = TEXT("LANG_MOHAWK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MOHAWK_MOHAWK: strSub = TEXT("SUBLANG_MOHAWK_MOHAWK"); break;
        }
        break;
    case LANG_MONGOLIAN: strPrim = TEXT("LANG_MONGOLIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA: strSub = TEXT("SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA"); break;
        case SUBLANG_MONGOLIAN_PRC: strSub = TEXT("SUBLANG_MONGOLIAN_PRC"); break;
        }
        break;
    case LANG_NEPALI: strPrim = TEXT("LANG_NEPALI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_NEPALI_NEPAL: strSub = TEXT("SUBLANG_NEPALI_NEPAL"); break;
        case SUBLANG_NEPALI_INDIA: strSub = TEXT("SUBLANG_NEPALI_INDIA"); break;
        }
        break;
    case LANG_NORWEGIAN: strPrim = TEXT("LANG_NORWEGIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_NORWEGIAN_BOKMAL: strSub = TEXT("SUBLANG_NORWEGIAN_BOKMAL"); break;
        case SUBLANG_NORWEGIAN_NYNORSK: strSub = TEXT("SUBLANG_NORWEGIAN_NYNORSK"); break;
        }
        break;
    case LANG_OCCITAN: strPrim = TEXT("LANG_OCCITAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_OCCITAN_FRANCE: strSub = TEXT("SUBLANG_OCCITAN_FRANCE"); break;
        }
        break;
    //case LANG_ODIA: strPrim = TEXT("LANG_ODIA");  // same as LANG_ORIYA
    //    break;
    case LANG_ORIYA: strPrim = TEXT("LANG_ORIYA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ORIYA_INDIA: strSub = TEXT("SUBLANG_ORIYA_INDIA"); break;
        }
        break;
    case LANG_PASHTO: strPrim = TEXT("LANG_PASHTO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_PASHTO_AFGHANISTAN: strSub = TEXT("SUBLANG_PASHTO_AFGHANISTAN"); break;
        }
        break;
    case LANG_PERSIAN: strPrim = TEXT("LANG_PERSIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_PERSIAN_IRAN: strSub = TEXT("SUBLANG_PERSIAN_IRAN"); break;
        }
        break;
    case LANG_POLISH: strPrim = TEXT("LANG_POLISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_POLISH_POLAND: strSub = TEXT("SUBLANG_POLISH_POLAND"); break;
        }
        break;
    case LANG_PORTUGUESE: strPrim = TEXT("LANG_PORTUGUESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_PORTUGUESE: strSub = TEXT("SUBLANG_PORTUGUESE"); break;
        case SUBLANG_PORTUGUESE_BRAZILIAN: strSub = TEXT("SUBLANG_PORTUGUESE_BRAZILIAN"); break;
        }
        break;
#ifdef ENABLE_NEW_LANGS
    case LANG_PULAR: strPrim = TEXT("LANG_PULAR"); // same as LANG_FULAH
        SWITCH_SUBLANG()
        {
        case SUBLANG_PULAR_SENEGAL: strSub = TEXT("SUBLANG_PULAR_SENEGAL"); break;
        default: break;
        }
#endif
        break;
    case LANG_PUNJABI: strPrim = TEXT("LANG_PUNJABI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_PUNJABI_INDIA: strSub = TEXT("SUBLANG_PUNJABI_INDIA"); break;
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_PUNJABI_PAKISTAN: strSub = TEXT("SUBLANG_PUNJABI_PAKISTAN"); break;
#endif
        default: break;
        }
        break;
    case LANG_QUECHUA: strPrim = TEXT("LANG_QUECHUA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_QUECHUA_BOLIVIA: strSub = TEXT("SUBLANG_QUECHUA_BOLIVIA"); break;
        case SUBLANG_QUECHUA_ECUADOR: strSub = TEXT("SUBLANG_QUECHUA_ECUADOR"); break;
        case SUBLANG_QUECHUA_PERU: strSub = TEXT("SUBLANG_QUECHUA_PERU"); break;
        }
        break;
    case LANG_ROMANIAN: strPrim = TEXT("LANG_ROMANIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ROMANIAN_ROMANIA: strSub = TEXT("SUBLANG_ROMANIAN_ROMANIA"); break;
        }
        break;
    case LANG_ROMANSH: strPrim = TEXT("LANG_ROMANSH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ROMANSH_SWITZERLAND: strSub = TEXT("SUBLANG_ROMANSH_SWITZERLAND"); break;
        }
        break;
    case LANG_RUSSIAN: strPrim = TEXT("LANG_RUSSIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_RUSSIAN_RUSSIA: strSub = TEXT("SUBLANG_RUSSIAN_RUSSIA"); break;
        }
        break;
#ifdef ENABLE_NEW_LANGS
    case LANG_SAKHA: strPrim = TEXT("LANG_SAKHA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SAKHA_RUSSIA: strSub = TEXT("SUBLANG_SAKHA_RUSSIA"); break;
        default: break;
        }
        break;
#endif
    case LANG_SAMI: strPrim = TEXT("LANG_SAMI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SAMI_NORTHERN_NORWAY: strSub = TEXT("SUBLANG_SAMI_NORTHERN_NORWAY"); break;
        case SUBLANG_SAMI_NORTHERN_SWEDEN: strSub = TEXT("SUBLANG_SAMI_NORTHERN_SWEDEN"); break;
        case SUBLANG_SAMI_NORTHERN_FINLAND: strSub = TEXT("SUBLANG_SAMI_NORTHERN_FINLAND"); break;
        case SUBLANG_SAMI_LULE_NORWAY: strSub = TEXT("SUBLANG_SAMI_LULE_NORWAY"); break;
        case SUBLANG_SAMI_LULE_SWEDEN: strSub = TEXT("SUBLANG_SAMI_LULE_SWEDEN"); break;
        case SUBLANG_SAMI_SOUTHERN_NORWAY: strSub = TEXT("SUBLANG_SAMI_SOUTHERN_NORWAY"); break;
        case SUBLANG_SAMI_SOUTHERN_SWEDEN: strSub = TEXT("SUBLANG_SAMI_SOUTHERN_SWEDEN"); break;
        case SUBLANG_SAMI_SKOLT_FINLAND: strSub = TEXT("SUBLANG_SAMI_SKOLT_FINLAND"); break;
        case SUBLANG_SAMI_INARI_FINLAND: strSub = TEXT("SUBLANG_SAMI_INARI_FINLAND"); break;
        }
        break;
    case LANG_SANSKRIT: strPrim = TEXT("LANG_SANSKRIT");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SANSKRIT_INDIA: strSub = TEXT("SUBLANG_SANSKRIT_INDIA"); break;
        }
        break;
#ifdef ENABLE_NEW_LANGS
    case LANG_SCOTTISH_GAELIC: strPrim = TEXT("LANG_SCOTTISH_GAELIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SCOTTISH_GAELIC: strSub = TEXT("SUBLANG_SCOTTISH_GAELIC"); break;
        default: break;
        }
        break;
#endif
    //case LANG_SERBIAN: strPrim = TEXT("LANG_SERBIAN"); // same as LANG_BOSNIAN
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_SERBIAN_LATIN: strSub = TEXT("SUBLANG_SERBIAN_LATIN"); break;
    //    case SUBLANG_SERBIAN_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_CYRILLIC"); break;
    //    case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
    //    case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
    //    case SUBLANG_SERBIAN_MONTENEGRO_LATIN: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_LATIN"); break;
    //    case SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC"); break;
    //    case SUBLANG_SERBIAN_SERBIA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_LATIN"); break;
    //    case SUBLANG_SERBIAN_SERBIA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_CYRILLIC"); break;
    //    }
    //    break;
    case LANG_SERBIAN_NEUTRAL: strPrim = TEXT("LANG_SERBIAN_NEUTRAL");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SERBIAN_LATIN: strSub = TEXT("SUBLANG_SERBIAN_LATIN"); break;
        case SUBLANG_SERBIAN_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_CYRILLIC"); break;
        case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
        case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_SERBIAN_MONTENEGRO_LATIN: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_LATIN"); break;
        case SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC"); break;
        case SUBLANG_SERBIAN_SERBIA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_LATIN"); break;
        case SUBLANG_SERBIAN_SERBIA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_CYRILLIC"); break;
#endif
        }
        break;
    case LANG_SINDHI: strPrim = TEXT("LANG_SINDHI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SINDHI_INDIA: strSub = TEXT("SUBLANG_SINDHI_INDIA"); break;
        case SUBLANG_SINDHI_AFGHANISTAN: strSub = TEXT("SUBLANG_SINDHI_AFGHANISTAN"); break;
        //case SUBLANG_SINDHI_PAKISTAN: strSub = TEXT("SUBLANG_SINDHI_PAKISTAN"); break; // same as SUBLANG_SINDHI_AFGHANISTAN
        }
        break;
    case LANG_SINHALESE: strPrim = TEXT("LANG_SINHALESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SINHALESE_SRI_LANKA: strSub = TEXT("SUBLANG_SINHALESE_SRI_LANKA"); break;
        }
        break;
    case LANG_SLOVAK: strPrim = TEXT("LANG_SLOVAK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SLOVAK_SLOVAKIA: strSub = TEXT("SUBLANG_SLOVAK_SLOVAKIA"); break;
        }
        break;
    case LANG_SLOVENIAN: strPrim = TEXT("LANG_SLOVENIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SLOVENIAN_SLOVENIA: strSub = TEXT("SUBLANG_SLOVENIAN_SLOVENIA"); break;
        }
        break;
    case LANG_SOTHO: strPrim = TEXT("LANG_SOTHO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA: strSub = TEXT("SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA"); break;
        }
        break;
    case LANG_SPANISH: strPrim = TEXT("LANG_SPANISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SPANISH: strSub = TEXT("SUBLANG_SPANISH"); break;
        case SUBLANG_SPANISH_MEXICAN: strSub = TEXT("SUBLANG_SPANISH_MEXICAN"); break;
        case SUBLANG_SPANISH_MODERN: strSub = TEXT("SUBLANG_SPANISH_MODERN"); break;
        case SUBLANG_SPANISH_GUATEMALA: strSub = TEXT("SUBLANG_SPANISH_GUATEMALA"); break;
        case SUBLANG_SPANISH_COSTA_RICA: strSub = TEXT("SUBLANG_SPANISH_COSTA_RICA"); break;
        case SUBLANG_SPANISH_PANAMA: strSub = TEXT("SUBLANG_SPANISH_PANAMA"); break;
        case SUBLANG_SPANISH_DOMINICAN_REPUBLIC: strSub = TEXT("SUBLANG_SPANISH_DOMINICAN_REPUBLIC"); break;
        case SUBLANG_SPANISH_VENEZUELA: strSub = TEXT("SUBLANG_SPANISH_VENEZUELA"); break;
        case SUBLANG_SPANISH_COLOMBIA: strSub = TEXT("SUBLANG_SPANISH_COLOMBIA"); break;
        case SUBLANG_SPANISH_PERU: strSub = TEXT("SUBLANG_SPANISH_PERU"); break;
        case SUBLANG_SPANISH_ARGENTINA: strSub = TEXT("SUBLANG_SPANISH_ARGENTINA"); break;
        case SUBLANG_SPANISH_ECUADOR: strSub = TEXT("SUBLANG_SPANISH_ECUADOR"); break;
        case SUBLANG_SPANISH_CHILE: strSub = TEXT("SUBLANG_SPANISH_CHILE"); break;
        case SUBLANG_SPANISH_URUGUAY: strSub = TEXT("SUBLANG_SPANISH_URUGUAY"); break;
        case SUBLANG_SPANISH_PARAGUAY: strSub = TEXT("SUBLANG_SPANISH_PARAGUAY"); break;
        case SUBLANG_SPANISH_BOLIVIA: strSub = TEXT("SUBLANG_SPANISH_BOLIVIA"); break;
        case SUBLANG_SPANISH_EL_SALVADOR: strSub = TEXT("SUBLANG_SPANISH_EL_SALVADOR"); break;
        case SUBLANG_SPANISH_HONDURAS: strSub = TEXT("SUBLANG_SPANISH_HONDURAS"); break;
        case SUBLANG_SPANISH_NICARAGUA: strSub = TEXT("SUBLANG_SPANISH_NICARAGUA"); break;
        case SUBLANG_SPANISH_PUERTO_RICO: strSub = TEXT("SUBLANG_SPANISH_PUERTO_RICO"); break;
        case SUBLANG_SPANISH_US: strSub = TEXT("SUBLANG_SPANISH_US"); break;
        }
        break;
    case LANG_SWAHILI: strPrim = TEXT("LANG_SWAHILI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SWAHILI_KENYA: strSub = TEXT("SUBLANG_SWAHILI_KENYA"); break;
        }
        break;
    case LANG_SWEDISH: strPrim = TEXT("LANG_SWEDISH");
        SWITCH_SUBLANG()
        {
        //case SUBLANG_SWEDISH_SWEDEN: strSub = TEXT("SUBLANG_SWEDISH_SWEDEN"); break; // same as SUBLANG_SWEDISH
        case SUBLANG_SWEDISH: strSub = TEXT("SUBLANG_SWEDISH"); break;
        case SUBLANG_SWEDISH_FINLAND: strSub = TEXT("SUBLANG_SWEDISH_FINLAND"); break;
        }
        break;
    case LANG_SYRIAC: strPrim = TEXT("LANG_SYRIAC");
#if defined(ENABLE_NEW_LANGS)
        SWITCH_SUBLANG()
        {
        case SUBLANG_SYRIAC: strSub = TEXT("SUBLANG_SYRIAC"); break;
        }
#endif
        break;
    case LANG_TAJIK: strPrim = TEXT("LANG_TAJIK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TAJIK_TAJIKISTAN: strSub = TEXT("SUBLANG_TAJIK_TAJIKISTAN"); break;
        }
        break;
    case LANG_TAMAZIGHT: strPrim = TEXT("LANG_TAMAZIGHT");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TAMAZIGHT_ALGERIA_LATIN   : strSub = TEXT("SUBLANG_TAMAZIGHT_ALGERIA_LATIN   "); break;
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_TAMAZIGHT_MOROCCO_TIFINAGH: strSub = TEXT("SUBLANG_TAMAZIGHT_MOROCCO_TIFINAGH"); break;
#endif
        }
        break;
    case LANG_TAMIL: strPrim = TEXT("LANG_TAMIL");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TAMIL_INDIA: strSub = TEXT("SUBLANG_TAMIL_INDIA"); break;
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_TAMIL_SRI_LANKA: strSub = TEXT("SUBLANG_TAMIL_SRI_LANKA"); break;
#endif
        default: break;
        }
        break;
    case LANG_TATAR: strPrim = TEXT("LANG_TATAR");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TATAR_RUSSIA: strSub = TEXT("SUBLANG_TATAR_RUSSIA"); break;
        }
        break;
    case LANG_TELUGU: strPrim = TEXT("LANG_TELUGU");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TELUGU_INDIA: strSub = TEXT("SUBLANG_TELUGU_INDIA"); break;
        }
        break;
    case LANG_THAI: strPrim = TEXT("LANG_THAI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_THAI_THAILAND: strSub = TEXT("SUBLANG_THAI_THAILAND"); break;
        }
        break;
    case LANG_TIBETAN: strPrim = TEXT("LANG_TIBETAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TIBETAN_PRC: strSub = TEXT("SUBLANG_TIBETAN_PRC"); break;
#if defined(ENABLE_NEW_LANGS)
        case SUBLANG_TIBETAN_BHUTAN: strSub = TEXT("SUBLANG_TIBETAN_BHUTAN"); break;
#endif
        }
        break;
    case LANG_TIGRIGNA: strPrim = TEXT("LANG_TIGRIGNA"); // same as LANG_TIGRINYA
        SWITCH_SUBLANG()
        {
        case SUBLANG_TIGRIGNA_ERITREA: strSub = TEXT("SUBLANG_TIGRIGNA_ERITREA"); break;
        default: break;
        }
        break;
    //case LANG_TIGRINYA: strPrim = TEXT("LANG_TIGRINYA"); // same as LANG_TIGRIGNA
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_TIGRINYA_ERITREA: strSub = TEXT("SUBLANG_TIGRINYA_ERITREA"); break;
    //    case SUBLANG_TIGRINYA_ETHIOPIA: strSub = TEXT("SUBLANG_TIGRINYA_ETHIOPIA"); break;
    //    }
    //    break;
    case LANG_TSWANA: strPrim = TEXT("LANG_TSWANA");
        SWITCH_SUBLANG()
        {
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_TSWANA_BOTSWANA: strSub = TEXT("SUBLANG_TSWANA_BOTSWANA"); break;
#endif
        case SUBLANG_TSWANA_SOUTH_AFRICA: strSub = TEXT("SUBLANG_TSWANA_SOUTH_AFRICA"); break;
        }
        break;
    case LANG_TURKISH: strPrim = TEXT("LANG_TURKISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TURKISH_TURKEY: strSub = TEXT("SUBLANG_TURKISH_TURKEY"); break;
        }
        break;
    case LANG_TURKMEN: strPrim = TEXT("LANG_TURKMEN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TURKMEN_TURKMENISTAN: strSub = TEXT("SUBLANG_TURKMEN_TURKMENISTAN"); break;
        }
        break;
    case LANG_UIGHUR: strPrim = TEXT("LANG_UIGHUR");
        SWITCH_SUBLANG()
        {
        case SUBLANG_UIGHUR_PRC: strSub = TEXT("SUBLANG_UIGHUR_PRC"); break;
        }
        break;
    case LANG_UKRAINIAN: strPrim = TEXT("LANG_UKRAINIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_UKRAINIAN_UKRAINE: strSub = TEXT("SUBLANG_UKRAINIAN_UKRAINE"); break;
        }
        break;
    case LANG_UPPER_SORBIAN: strPrim = TEXT("LANG_UPPER_SORBIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_UPPER_SORBIAN_GERMANY: strSub = TEXT("SUBLANG_UPPER_SORBIAN_GERMANY"); break;
        }
        break;
    case LANG_URDU: strPrim = TEXT("LANG_URDU");
        SWITCH_SUBLANG()
        {
        case SUBLANG_URDU_PAKISTAN: strSub = TEXT("SUBLANG_URDU_PAKISTAN"); break;
        case SUBLANG_URDU_INDIA: strSub = TEXT("SUBLANG_URDU_INDIA"); break;
        }
        break;
    case LANG_UZBEK: strPrim = TEXT("LANG_UZBEK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_UZBEK_LATIN: strSub = TEXT("SUBLANG_UZBEK_LATIN"); break;
        case SUBLANG_UZBEK_CYRILLIC: strSub = TEXT("SUBLANG_UZBEK_CYRILLIC"); break;
        }
        break;
    //case LANG_VALENCIAN: strPrim = TEXT("LANG_VALENCIAN"); // same as LANG_CATALAN
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_VALENCIAN_VALENCIA: strSub = TEXT("SUBLANG_VALENCIAN_VALENCIA"); break;
    //    }
    //    break;
    case LANG_VIETNAMESE: strPrim = TEXT("LANG_VIETNAMESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_VIETNAMESE_VIETNAM: strSub = TEXT("SUBLANG_VIETNAMESE_VIETNAM"); break;
        }
        break;
    case LANG_WELSH: strPrim = TEXT("LANG_WELSH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_WELSH_UNITED_KINGDOM: strSub = TEXT("SUBLANG_WELSH_UNITED_KINGDOM"); break;
        }
        break;
    case LANG_WOLOF: strPrim = TEXT("LANG_WOLOF");
        SWITCH_SUBLANG()
        {
        case SUBLANG_WOLOF_SENEGAL: strSub = TEXT("SUBLANG_WOLOF_SENEGAL"); break;
        }
        break;
    case LANG_XHOSA: strPrim = TEXT("LANG_XHOSA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_XHOSA_SOUTH_AFRICA: strSub = TEXT("SUBLANG_XHOSA_SOUTH_AFRICA"); break;
        }
        break;
    //case LANG_YAKUT: strPrim = TEXT("LANG_YAKUT"); // same as LANG_SAKHA
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_YAKUT_RUSSIA: strSub = TEXT("SUBLANG_YAKUT_RUSSIA"); break;
    //    }
    //    break;
    case LANG_YI: strPrim = TEXT("LANG_YI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_YI_PRC: strSub = TEXT("SUBLANG_YI_PRC"); break;
        }
        break;
    case LANG_YORUBA: strPrim = TEXT("LANG_YORUBA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_YORUBA_NIGERIA: strSub = TEXT("SUBLANG_YORUBA_NIGERIA"); break;
        }
        break;
    case LANG_ZULU: strPrim = TEXT("LANG_ZULU");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ZULU_SOUTH_AFRICA: strSub = TEXT("SUBLANG_ZULU_SOUTH_AFRICA"); break;
        }
        break;
    default:
        break;
    }

    TCHAR szText[32];
    if (strPrim.empty())
    {
        StringCchPrintf(szText, _countof(szText), TEXT("0x%04X"), PRIMARYLANGID(langid));
        strPrim = szText;
    }

    if (bOldStyle)
        strSub.clear();

    // sub-language
    if (strSub.empty())
    {
        switch (SUBLANGID(langid))
        {
        case SUBLANG_NEUTRAL: strSub = TEXT("SUBLANG_NEUTRAL"); break;
        case SUBLANG_DEFAULT: strSub = TEXT("SUBLANG_DEFAULT"); break;
        case SUBLANG_SYS_DEFAULT: strSub = TEXT("SUBLANG_SYS_DEFAULT"); break;
        case SUBLANG_CUSTOM_DEFAULT: strSub = TEXT("SUBLANG_CUSTOM_DEFAULT"); break;
        case SUBLANG_CUSTOM_UNSPECIFIED: strSub = TEXT("SUBLANG_CUSTOM_UNSPECIFIED"); break;
        case SUBLANG_UI_CUSTOM_DEFAULT: strSub = TEXT("SUBLANG_UI_CUSTOM_DEFAULT"); break;
        default:
            break;
        }
    }

    // sub-language
    if (strSub.empty())
    {
        StringCchPrintf(szText, _countof(szText), TEXT("0x%04X"), SUBLANGID(langid));
        strSub = szText;
    }
#undef SWITCH_SUBLANG

    // output the LANGUAGE statement
    MString str = TEXT("LANGUAGE ");
    str += strPrim;
    str += TEXT(", ");
    str += strSub;
    str += TEXT("\r\n");

    return str;
}

// get the RISOHTEMPLATE text
MStringW GetRisohTemplate(const MIdOrString& type, WORD wLang)
{
    // get this application module
    HINSTANCE hInst = GetModuleHandle(NULL);

    if (type.empty())
    {
        return L"";    // failure
    }

    // try to find the RISOHTEMPLATE resource
    WORD LangID = PRIMARYLANGID(wLang);
    HRSRC hRsrc = NULL;
    if (hRsrc == NULL)
        hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", type.ptr(), wLang);
    if (hRsrc == NULL)
        hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", type.ptr(), MAKELANGID(LangID, SUBLANG_NEUTRAL));
    if (hRsrc == NULL)
        hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", type.ptr(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
    if (hRsrc == NULL)
        hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", type.ptr(), MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
    if (hRsrc == NULL)
        hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", type.ptr(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    if (hRsrc == NULL)
    {
        return L"";
    }

    // get the pointer and byte size
    HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
    DWORD cb = SizeofResource(hInst, hRsrc);
    const BYTE *pb = (const BYTE *)LockResource(hGlobal);

    // ignore the BOM if any
    if (memcmp(pb, "\xEF\xBB\xBF", 3) == 0)
    {
        pb += 3;
        cb -= 3;
    }

    // convert the UTF-8 text to the UTF-16 text (wide)
    MStringA utf8((LPCSTR)(pb), (LPCSTR)(pb) + cb);
    MAnsiToWide wide(CP_UTF8, utf8);

    return wide.c_str();    // return the wide
}

////////////////////////////////////////////////////////////////////////////

static MMainWnd *s_pMainWnd = NULL;

EGA::arg_t EGA_FN EGA_RES_search(const EGA::args_t& args)
{
    return s_pMainWnd->DoEgaResSearch(args);
}

EGA::arg_t EGA_FN EGA_RES_delete(const EGA::args_t& args)
{
    return s_pMainWnd->DoEgaResDelete(args);
}

EGA::arg_t EGA_FN EGA_RES_clone_by_name(const EGA::args_t& args)
{
    return s_pMainWnd->DoEgaResCloneByName(args);
}

EGA::arg_t EGA_FN EGA_RES_clone_by_lang(const EGA::args_t& args)
{
    return s_pMainWnd->DoEgaResCloneByLang(args);
}

EGA::arg_t EGA_FN EGA_RES_unload_resh(const EGA::args_t& args)
{
    return s_pMainWnd->DoEgaResUnloadResH(args);
}

MIdOrString EGA_get_id_or_str(const arg_t& arg0)
{
    MIdOrString ret;

    if (arg0->get_type() == AST_INT)
    {
        ret = (WORD)EGA_get_int(arg0);
    }
    else
    {
        std::string str = EGA_get_str(arg0);
        MAnsiToWide wide(CP_UTF8, str.c_str());
        ret = wide.c_str();
    }

    return ret;
}

EGA::arg_t EGA_set_id_or_str(const MIdOrString& id)
{
    if (id.is_str())
    {
        MWideToAnsi ansi(CP_UTF8, id.m_str.c_str());
        return EGA::make_arg<AstStr>(ansi.c_str());
    }
    else
    {
        return EGA::make_arg<AstInt>(id.m_id);
    }
}

EGA::arg_t MMainWnd::DoEgaResSearch(const EGA::args_t& args)
{
    using namespace EGA;
    arg_t arg0, arg1, arg2;

    if (args.size() >= 1)
        arg0 = EGA_eval_arg(args[0], false);
    if (args.size() >= 2)
        arg1 = EGA_eval_arg(args[1], false);
    if (args.size() >= 3)
        arg2 = EGA_eval_arg(args[2], false);

    MIdOrString type, name;
    WORD lang = BAD_LANG;

    if (arg0)
        type = EGA_get_id_or_str(arg0);
    if (arg1)
        name = EGA_get_id_or_str(arg1);
    if (arg2)
        lang = (WORD)EGA_get_int(arg2);

    EntrySetBase found;
    g_res.search(found, ET_LANG, type, name, lang);

    auto array = make_arg<AstContainer>(AST_ARRAY, 0, "RES_LIST");
    for (auto& item : found)
    {
        auto child = make_arg<AstContainer>(AST_ARRAY, 0, "RES");
        child->add(EGA_set_id_or_str(item->m_type));
        child->add(EGA_set_id_or_str(item->m_name));
        child->add(EGA::make_arg<AstInt>(item->m_lang));
        array->add(child);
    }
    return array;
}

EGA::arg_t MMainWnd::DoEgaResDelete(const EGA::args_t& args)
{
    using namespace EGA;
    arg_t arg0, arg1, arg2;

    if (args.size() >= 1)
        arg0 = EGA_eval_arg(args[0], false);
    if (args.size() >= 2)
        arg1 = EGA_eval_arg(args[1], false);
    if (args.size() >= 3)
        arg2 = EGA_eval_arg(args[2], false);

    MIdOrString type, name;
    WORD lang = BAD_LANG;

    if (arg0)
        type = EGA_get_id_or_str(arg0);
    if (arg1)
        name = EGA_get_id_or_str(arg1);
    if (arg2)
        lang = (WORD)EGA_get_int(arg2);

    bool ret = g_res.search_and_delete(ET_ANY, type, name, lang);
    g_res.delete_invalid();

    if (ret)
        DoSetFileModified(TRUE);

    PostMessageW(s_hMainWnd, WM_COMMAND, ID_REFRESHALL, 0);

    return make_arg<AstInt>(ret);
}

EGA::arg_t MMainWnd::DoEgaResCloneByName(const EGA::args_t& args)
{
    using namespace EGA;
    arg_t arg0, arg1, arg2;

    if (args.size() >= 1)
        arg0 = EGA_eval_arg(args[0], false);
    if (args.size() >= 2)
        arg1 = EGA_eval_arg(args[1], false);
    if (args.size() >= 3)
        arg2 = EGA_eval_arg(args[2], false);

    MIdOrString type, src_name, dest_name;
    WORD lang = BAD_LANG;

    if (arg0)
        type = EGA_get_id_or_str(arg0);
    if (arg1)
        src_name = EGA_get_id_or_str(arg1);
    if (arg2)
        dest_name = EGA_get_id_or_str(arg2);

    EntrySetBase found;
    g_res.search(found, ET_LANG, type, src_name, lang);

    if (type == RT_GROUP_ICON)     // group icon
    {
        for (auto e : found)
        {
            g_res.copy_group_icon(e, dest_name, e->m_lang);
        }
    }
    else if (type == RT_GROUP_CURSOR)  // group cursor
    {
        for (auto e : found)
        {
            g_res.copy_group_cursor(e, dest_name, e->m_lang);
        }
    }
    else    // otherwise
    {
        for (auto e : found)
        {
            g_res.add_lang_entry(e->m_type, dest_name, e->m_lang, e->m_data);
        }
    }

    g_res.delete_invalid();

    if (!found.empty())
        DoSetFileModified(TRUE);

    PostMessageW(s_hMainWnd, WM_COMMAND, ID_REFRESHALL, 0);

    return make_arg<AstInt>(!found.empty());
}

EGA::arg_t MMainWnd::DoEgaResCloneByLang(const EGA::args_t& args)
{
    using namespace EGA;
    arg_t arg0, arg1, arg2, arg3;

    if (args.size() >= 1)
        arg0 = EGA_eval_arg(args[0], false);
    if (args.size() >= 2)
        arg1 = EGA_eval_arg(args[1], false);
    if (args.size() >= 3)
        arg2 = EGA_eval_arg(args[2], false);
    if (args.size() >= 4)
        arg3 = EGA_eval_arg(args[3], false);

    MIdOrString type, name;
    WORD src_lang = BAD_LANG, dest_lang = BAD_LANG;

    if (arg0)
        type = EGA_get_id_or_str(arg0);
    if (arg1)
        name = EGA_get_id_or_str(arg1);
    if (arg2)
        src_lang = EGA_get_int(arg2);
    if (arg3)
        dest_lang = EGA_get_int(arg3);

    EntrySetBase found2;
    g_res.search(found2, ET_LANG, type, name, src_lang);

    for (auto& entry : found2)
    {
        if (entry->m_type == RT_GROUP_ICON)     // group icon
        {
            // search the group icons
            EntrySetBase found;
            g_res.search(found, ET_LANG, RT_GROUP_ICON, name, src_lang);

            // copy them
            for (auto e : found)
            {
                g_res.copy_group_icon(e, e->m_name, dest_lang);
            }
        }
        else if (entry->m_type == RT_GROUP_CURSOR)
        {
            // search the group cursors
            EntrySetBase found;
            g_res.search(found, ET_LANG, RT_GROUP_CURSOR, name, src_lang);

            // copy them
            for (auto e : found)
            {
                g_res.copy_group_cursor(e, e->m_name, dest_lang);
            }
        }
        else if (entry->m_et == ET_STRING)
        {
            // search the strings
            EntrySetBase found;
            g_res.search(found, ET_LANG, RT_STRING, WORD(0), src_lang);

            // copy them
            for (auto e : found)
            {
                g_res.add_lang_entry(e->m_type, e->m_name, dest_lang, e->m_data);
            }
        }
        else if (entry->m_et == ET_MESSAGE)
        {
            // search the messagetables
            EntrySetBase found;
            g_res.search(found, ET_LANG, RT_MESSAGETABLE, WORD(0), entry->m_lang);

            // copy them
            for (auto e : found)
            {
                g_res.add_lang_entry(e->m_type, e->m_name, dest_lang, e->m_data);
            }
        }
        else
        {
            // search the entries
            EntrySetBase found;
            g_res.search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);

            // copy them
            for (auto e : found)
            {
                g_res.add_lang_entry(e->m_type, e->m_name, dest_lang, e->m_data);
            }
        }
    }

    g_res.delete_invalid();

    if (!found2.empty())
        DoSetFileModified(TRUE);

    PostMessageW(s_hMainWnd, WM_COMMAND, ID_REFRESHALL, 0);

    return make_arg<AstInt>(!found2.empty());
}

EGA::arg_t MMainWnd::DoEgaResUnloadResH(const EGA::args_t& args)
{
    using namespace EGA;

    UnloadResourceH(m_hwnd);

    DoSetFileModified(TRUE);
    PostMessageW(s_hMainWnd, WM_COMMAND, ID_REFRESHALL, 0);

    return make_arg<AstInt>(1);
}

void EGA_extension(void)
{
    EGA_add_fn("RES_search", 0, 3, EGA_RES_search, "RES_search([type[, name[, lang]]])");
    EGA_add_fn("RES_delete", 0, 3, EGA_RES_delete, "RES_delete([type[, name[, lang]]])");
    EGA_add_fn("RES_clone_by_name", 3, 3, EGA_RES_clone_by_name, "RES_clone_by_name(type, src_name, dest_name)");
    EGA_add_fn("RES_clone_by_lang", 4, 4, EGA_RES_clone_by_lang, "RES_clone_by_lang(type, name, src_lang, dest_lang)");
    EGA_add_fn("RES_unload_resh", 0, 0, EGA_RES_unload_resh, "EGA_RES_unload_resh()");
}

////////////////////////////////////////////////////////////////////////////

// the manifest information
#pragma comment(linker, "/manifestdependency:\"type='win32' \
  name='Microsoft.Windows.Common-Controls' \
  version='6.0.0.0' \
  processorArchitecture='*' \
  publicKeyToken='6595b64144ccf1df' \
  language='*'\"")

// We will dynamically create the MOleCtrl instances
IMPLEMENT_DYNAMIC(MOleCtrl)

// the main function of the windows application
extern "C"
INT WINAPI
wWinMain(HINSTANCE   hInstance,
         HINSTANCE   hPrevInstance,
         LPWSTR       lpCmdLine,
         INT         nCmdShow)
{
    SetEnvironmentVariableW(L"LANG", L"en_US");

    {
        WCHAR szPath[MAX_PATH];
        GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath));
        if (wcschr(szPath, L' ') != NULL)
        {
            MessageBoxW(NULL,
                        LoadStringDx(IDS_PATHSPACEERROR),
                        NULL,
                        MB_ICONERROR);
        }
    }

    // initialize the libraries
    OleInitialize(NULL);

    // register MOleCtrl window class
    MOleCtrl::RegisterDx();

    // initialize common controls
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_WIN95_CLASSES |
                 ICC_DATE_CLASSES |
                 ICC_USEREX_CLASSES |
                 ICC_COOL_CLASSES |
                 ICC_INTERNET_CLASSES |
                 ICC_PAGESCROLLER_CLASS |
                 ICC_NATIVEFNTCTL_CLASS |
                 ICC_STANDARD_CLASSES |
                 ICC_LINK_CLASS;
    InitCommonControlsEx(&iccx);

    // load RichEdit
    HINSTANCE hinstRichEdit = LoadLibrary(TEXT("RICHED32.DLL"));

    HINSTANCE hinstUXTheme = LoadLibrary(TEXT("UXTHEME.DLL"));
    s_pSetWindowTheme = (SETWINDOWTHEME)GetProcAddress(hinstUXTheme, "SetWindowTheme");

    // load GDI+
    Gdiplus::GdiplusStartupInput gp_startup_input;
    ULONG_PTR gp_token;
    Gdiplus::GdiplusStartup(&gp_token, &gp_startup_input, NULL);

    // main process
    int ret;
    MEditCtrl::SetCtrlAHookDx(TRUE);
    {
        MMainWnd app(__argc, __targv, hInstance);
        s_pMainWnd = &app;

        if (app.StartDx())
        {
            // main loop
            ret = INT(app.RunDx());
        }
        else
        {
            ret = 2;
        }
    }
    MEditCtrl::SetCtrlAHookDx(FALSE);

    // free GDI+
    Gdiplus::GdiplusShutdown(gp_token);

    // free the libraries
    FreeLibrary(hinstRichEdit);
    FreeLibrary(hinstUXTheme);
    OleUninitialize();
    FreeWCLib();

    // check object counts
    assert(MacroParser::BaseAst::alive_count() == 0);

#if (WINVER >= 0x0500)
    HANDLE hProcess = GetCurrentProcess();
    DebugPrintDx(TEXT("Count of GDI objects: %ld\n"),
                 GetGuiResources(hProcess, GR_GDIOBJECTS));
    DebugPrintDx(TEXT("Count of USER objects: %ld\n"),
                 GetGuiResources(hProcess, GR_USEROBJECTS));
#endif

#if defined(_MSC_VER) && !defined(NDEBUG)
    // for detecting memory leak (MSVC only)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    return ret;
}

//////////////////////////////////////////////////////////////////////////////
