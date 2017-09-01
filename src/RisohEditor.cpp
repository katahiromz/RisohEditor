// RisouEditor.cpp --- RisouEditor
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.hpp"

#pragma comment(lib, "msimg32.lib")

//////////////////////////////////////////////////////////////////////////////
// constants

#ifndef INVALID_FILE_ATTRIBUTES
    #define INVALID_FILE_ATTRIBUTES     ((DWORD)-1)
#endif

#ifndef RT_MANIFEST
    #define RT_MANIFEST 24
#endif

#define TV_WIDTH        250     // default m_hTreeView width
#define BV_WIDTH        160     // default m_hBmpView width
#define BE_HEIGHT       90      // default m_hBinEdit height
#define CX_STATUS_PART  80      // status bar part width

//////////////////////////////////////////////////////////////////////////////

void GetSelection(HWND hLst, std::vector<BYTE>& sel)
{
    for (size_t i = 0; i < sel.size(); ++i)
    {
        sel[i] = (ListBox_GetSel(hLst, (DWORD)i) > 0);
    }
}

void GetSelection(std::vector<BYTE>& sel,
                  const ConstantsDB::TableType& table, DWORD dwValue)
{
    sel.resize(table.size());
    for (size_t i = 0; i < table.size(); ++i)
    {
        if ((dwValue & table[i].mask) == table[i].value)
            sel[i] = TRUE;
        else
            sel[i] = FALSE;
    }
}

DWORD AnalyseDifference(
    DWORD dwValue, ConstantsDB::TableType& table,
    std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel)
{
    assert(old_sel.size() == new_sel.size());
    for (size_t i = 0; i < old_sel.size(); ++i)
    {
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

void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table)
{
    ListBox_ResetContent(hLst);

    if (table.size())
    {
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            ListBox_AddString(hLst, it->name.c_str());
        }
    }
}

static int CALLBACK
EnumFontFamProc(
    ENUMLOGFONT *lpelf,
    NEWTEXTMETRIC *lpntm,
    int FontType,
    LPARAM lParam)
{
    HWND hCmb = HWND(lParam);
    ComboBox_AddString(hCmb, lpelf->elfLogFont.lfFaceName);
    return TRUE;
}

void InitFontComboBox(HWND hCmb)
{
    HDC hDC = CreateCompatibleDC(NULL);
    ::EnumFontFamilies(hDC, NULL, (FONTENUMPROC)EnumFontFamProc, (LPARAM)hCmb);
    DeleteDC(hDC);
}

typedef struct CharSetInfo
{
    BYTE CharSet;
    LPCTSTR Name;
} CharSetInfo;

static const CharSetInfo s_charset_entries[] =
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

void InitCharSetComboBox(HWND hCmb, BYTE CharSet)
{
    ComboBox_ResetContent(hCmb);

    for (UINT i = 0; i < _countof(s_charset_entries); ++i)
    {
        ComboBox_AddString(hCmb, s_charset_entries[i].Name);
    }
    for (UINT i = 0; i < _countof(s_charset_entries); ++i)
    {
        ComboBox_SetItemData(hCmb, i, s_charset_entries[i].CharSet);
    }
    ComboBox_SetCurSel(hCmb, 1);
    for (UINT i = 0; i < _countof(s_charset_entries); ++i)
    {
        if (s_charset_entries[i].CharSet == CharSet)
        {
            ComboBox_SetCurSel(hCmb, i);
            break;
        }
    }
}

BYTE GetCharSetFromComboBox(HWND hCmb)
{
    INT i = ComboBox_GetCurSel(hCmb);
    if (i == CB_ERR)
        return DEFAULT_CHARSET;
    if (i < _countof(s_charset_entries))
        return s_charset_entries[i].CharSet;
    return DEFAULT_CHARSET;
}

void InitClassComboBox(HWND hCmb, ConstantsDB& db, LPCTSTR pszClass)
{
    ComboBox_ResetContent(hCmb);

    ConstantsDB::TableType table;
    table = db.GetTable(TEXT("CONTROL.CLASSES"));

    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == pszClass)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

void InitWndClassComboBox(HWND hCmb, ConstantsDB& db, LPCTSTR pszWndClass)
{
    ComboBox_ResetContent(hCmb);

    ConstantsDB::TableType table;
    table = db.GetTable(TEXT("CONTROL.CLASSES"));

    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        if (it->value > 2)
            continue;

        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == pszWndClass)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

void InitCtrlIDComboBox(HWND hCmb, ConstantsDB& db)
{
    ConstantsDB::TableType table;
    table = db.GetTable(TEXT("CTRLID"));

    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        ComboBox_AddString(hCmb, it->name.c_str());
    }

    if ((BOOL)db.GetValue(L"HIDE.ID", L"HIDE.ID"))
        return;

    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_CONTROL].name;
    if (prefix.empty())
        return;

    table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
    end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        ComboBox_AddString(hCmb, it->name.c_str());
    }
}

void InitCommandComboBox(HWND hCmb, ConstantsDB& db, MString strCommand)
{
    SetWindowText(hCmb, strCommand.c_str());

    if ((BOOL)db.GetValue(L"HIDE.ID", L"HIDE.ID"))
        return;

    ConstantsDB::TableType table;
    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_COMMAND].name;
    if (prefix.empty())
        return;

    table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == strCommand)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

BOOL CheckCommand(ConstantsDB& db, MString strCommand)
{
    mstr_trim(strCommand);
    if (('0' <= strCommand[0] && strCommand[0] <= '9') ||
        strCommand[0] == '-' || strCommand[0] == '+')
    {
        return TRUE;
    }
    return db.HasResID(strCommand);
}

void InitStringComboBox(HWND hCmb, ConstantsDB& db, MString strString)
{
    SetWindowText(hCmb, strString.c_str());

    if ((BOOL)db.GetValue(L"HIDE.ID", L"HIDE.ID"))
        return;

    ConstantsDB::TableType table;
    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_STRING].name;
    if (prefix.empty())
        return;

    table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == strString)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// languages

struct LangEntry
{
    WORD LangID;
    std::wstring Str;

    bool operator<(const LangEntry& ent) const
    {
        return Str < ent.Str;
    }
};
std::vector<LangEntry> g_Langs;

//////////////////////////////////////////////////////////////////////////////
// useful global functions

LPWSTR MakeFilterDx(LPWSTR psz)
{
    for (LPWSTR pch = psz; *pch; ++pch)
    {
        if (*pch == L'|')
            *pch = 0;
    }
    return psz;
}

BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath)
{
    BOOL                bRes = FALSE;
    WIN32_FIND_DATAW    find;
    IShellLinkW*        pShellLink;
    IPersistFile*       pPersistFile;
    HRESULT             hRes;

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

HBITMAP Create24BppBitmapDx(INT width, INT height)
{
    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    HDC hDC = CreateCompatibleDC(NULL);
    LPVOID pvBits;
    HBITMAP hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS,
                                   &pvBits, NULL, 0);
    DeleteDC(hDC);
    return hbm;
}

BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size)
{
    using namespace std;
    FILE *fp = _tfopen(filename, _T("wb"));
    int n = fwrite(pv, size, 1, fp);
    fclose(fp);
    return n == 1;
}

LPWSTR GetTempFileNameDx(LPCWSTR pszPrefix3Chars)
{
    static WCHAR TempFile[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    ::GetTempPathW(_countof(szPath), szPath);
    ::GetTempFileNameW(szPath, L"KRE", 0, TempFile);
    return TempFile;
}

//////////////////////////////////////////////////////////////////////////////
// specialized global functions

HBITMAP CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor)
{
    HBITMAP hbm = Create24BppBitmapDx(width, height);
    if (hbm == NULL)
    {
        assert(0);
        return NULL;
    }
    ii_fill(hbm, GetStockBrush(LTGRAY_BRUSH));

    HDC hDC = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        HBRUSH hbr = GetStockBrush(LTGRAY_BRUSH);
        DrawIconEx(hDC, 0, 0, hIcon, width, height, 0, hbr, DI_NORMAL);
    }
    SelectObject(hDC, hbmOld);
    DeleteDC(hDC);

    return hbm;
}

HBITMAP
CreateBitmapFromIconOrPngDx(HWND hwnd, const ResEntry& Entry, BITMAP& bm)
{
    HBITMAP hbmIcon;

    if (Entry.size() >= 4 &&
        memcmp(&Entry[0], "\x89\x50\x4E\x47", 4) == 0)
    {
        hbmIcon = ii_png_load_mem(&Entry[0], Entry.size());
    }
    else
    {
        HICON hIcon;
        BITMAP bm;
        hIcon = PackedDIB_CreateIcon(&Entry[0], Entry.size(), bm, TRUE);
        assert(hIcon);
        hbmIcon = CreateBitmapFromIconDx(hIcon,
                                         bm.bmWidth, bm.bmHeight, FALSE);
        DestroyIcon(hIcon);
    }

    GetObject(hbmIcon, sizeof(bm), &bm);
    if (bm.bmBitsPixel == 32)
    {
        ii_premultiply(hbmIcon);
    }

    return hbmIcon;
}

HBITMAP
CreateBitmapFromIconsDx(HWND hwnd, ResEntries& Entries, const ResEntry& Entry)
{
    ICONDIR dir;
    if (Entry.size() < sizeof(dir))
    {
        assert(0);
        return NULL;
    }

    memcpy(&dir, &Entry[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
    {
        assert(0);
        return NULL;
    }

    const GRPICONDIRENTRY *pEntries;
    pEntries = (const GRPICONDIRENTRY *)&Entry[sizeof(dir)];

    LONG cx = 0, cy = 0;
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find(Entries, RT_ICON, pEntries[i].nID, Entry.lang);
        if (k == -1)
            k = Res_Find(Entries, RT_ICON, pEntries[i].nID, 0xFFFF);
        if (k == -1)
        {
            assert(0);
            return NULL;
        }
        ResEntry& IconEntry = Entries[k];

        BITMAP bm;
        HBITMAP hbmIcon = CreateBitmapFromIconOrPngDx(hwnd, IconEntry, bm);

        if (cx < bm.bmWidth)
            cx = bm.bmWidth;
        cy += bm.bmHeight;

        DeleteObject(hbmIcon);
    }

    HBITMAP hbm = Create24BppBitmapDx(cx, cy);
    if (hbm == NULL)
    {
        assert(0);
        return NULL;
    }
    ii_fill(hbm, GetStockBrush(LTGRAY_BRUSH));
    
    BITMAP bm;
    GetObject(hbm, sizeof(bm), &bm);

    INT y = 0;
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find(Entries, RT_ICON, pEntries[i].nID, Entry.lang);
        if (k == -1)
            k = Res_Find(Entries, RT_ICON, pEntries[i].nID, 0xFFFF);
        if (k == -1)
        {
            assert(0);
            DeleteObject(hbm);
            return NULL;
        }
        ResEntry& IconEntry = Entries[k];
        HBITMAP hbmIcon = CreateBitmapFromIconOrPngDx(hwnd, IconEntry, bm);

        ii_draw(hbm, hbmIcon, 0, y);
        y += bm.bmHeight;
    }

    return hbm;
}

HBITMAP
CreateBitmapFromCursorDx(HWND hwnd, const ResEntry& Entry, BITMAP& bm)
{
    HBITMAP hbmCursor;

    HICON hCursor;
    hCursor = PackedDIB_CreateIcon(&Entry[0], Entry.size(), bm, FALSE);
    assert(hCursor);
    hbmCursor = CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE);
    DestroyCursor(hCursor);

    GetObject(hbmCursor, sizeof(bm), &bm);
    assert(hbmCursor);
    return hbmCursor;
}

HBITMAP
CreateBitmapFromCursorsDx(HWND hwnd, ResEntries& Entries, const ResEntry& Entry)
{
    ICONDIR dir;
    if (Entry.size() < sizeof(dir))
    {
        assert(0);
        return NULL;
    }

    memcpy(&dir, &Entry[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
    {
        assert(0);
        return NULL;
    }

    const GRPCURSORDIRENTRY *pEntries;
    pEntries = (const GRPCURSORDIRENTRY *)&Entry[sizeof(dir)];

    LONG cx = 0, cy = 0;
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find(Entries, RT_CURSOR, pEntries[i].nID, Entry.lang);
        if (k == -1)
            k = Res_Find(Entries, RT_CURSOR, pEntries[i].nID, 0xFFFF);
        if (k == -1)
        {
            assert(0);
            return NULL;
        }
        ResEntry& CursorEntry = Entries[k];

        BITMAP bm;
        HBITMAP hbmCursor = CreateBitmapFromCursorDx(hwnd, CursorEntry, bm);
        assert(hbmCursor);
        assert(bm.bmWidth);
        assert(bm.bmHeight);

        if (cx < bm.bmWidth)
            cx = bm.bmWidth;
        cy += bm.bmHeight;

        DeleteObject(hbmCursor);
    }

    HBITMAP hbm = Create24BppBitmapDx(cx, cy);
    if (hbm == NULL)
    {
        assert(0);
        return NULL;
    }
    ii_fill(hbm, GetStockBrush(LTGRAY_BRUSH));

    HDC hDC = CreateCompatibleDC(NULL);
    HDC hDC2 = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        INT y = 0;
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            INT k = Res_Find(Entries, RT_CURSOR, pEntries[i].nID, Entry.lang);
            if (k == -1)
                k = Res_Find(Entries, RT_CURSOR, pEntries[i].nID, 0xFFFF);
            if (k == -1)
            {
                assert(0);
                DeleteObject(hbm);
                return NULL;
            }
            ResEntry& CursorEntry = Entries[k];

            BITMAP bm;
            HBITMAP hbmCursor = CreateBitmapFromCursorDx(hwnd, CursorEntry, bm);
            assert(hbmCursor);
            assert(bm.bmWidth);
            assert(bm.bmHeight);
            {
                HGDIOBJ hbm2Old = SelectObject(hDC2, hbmCursor);
                BitBlt(hDC, 0, y, bm.bmWidth, bm.bmHeight, hDC2, 0, 0, SRCCOPY);
                SelectObject(hDC2, hbm2Old);
            }
            DeleteObject(hbmCursor);

            y += bm.bmHeight;
        }
    }
    SelectObject(hDC, hbmOld);
    DeleteDC(hDC2);
    DeleteDC(hDC);

    return hbm;
}

//////////////////////////////////////////////////////////////////////////////
// specialized tool bar

TBBUTTON g_buttons0[] =
{
    { -1, CMDID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_GUIEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, CMDID_TEST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEST },
};

TBBUTTON g_buttons1[] =
{
    { -1, CMDID_TEXTEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEXTEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, CMDID_TEST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEST },
};

TBBUTTON g_buttons2[] =
{
    { -1, CMDID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_COMPILE },
    { -1, CMDID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_CANCELEDIT },
};

TBBUTTON g_buttons3[] =
{
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
};

TBBUTTON g_buttons4[] =
{
    { -1, CMDID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_GUIEDIT },
};

TBBUTTON g_buttons5[] =
{
    { -1, CMDID_TEXTEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEXTEDIT },
};

void ToolBar_Update(HWND hwnd, INT iType)
{
    while (SendMessageW(hwnd, TB_DELETEBUTTON, 0, 0))
        ;

    switch (iType)
    {
    case 0:
        SendMessageW(hwnd, TB_ADDBUTTONS, _countof(g_buttons0), (LPARAM)g_buttons0);
        break;
    case 1:
        SendMessageW(hwnd, TB_ADDBUTTONS, _countof(g_buttons1), (LPARAM)g_buttons1);
        break;
    case 2:
        SendMessageW(hwnd, TB_ADDBUTTONS, _countof(g_buttons2), (LPARAM)g_buttons2);
        break;
    case 3:
        //SendMessageW(hwnd, TB_ADDBUTTONS, _countof(g_buttons3), (LPARAM)g_buttons3);
        break;
    case 4:
        SendMessageW(hwnd, TB_ADDBUTTONS, _countof(g_buttons4), (LPARAM)g_buttons4);
        break;
    case 5:
        SendMessageW(hwnd, TB_ADDBUTTONS, _countof(g_buttons5), (LPARAM)g_buttons5);
        break;
    }
}

VOID ToolBar_StoreStrings(HWND hwnd, INT nCount, TBBUTTON *pButtons)
{
    for (INT i = 0; i < nCount; ++i)
    {
        if (pButtons[i].idCommand == 0 || (pButtons[i].fsStyle & BTNS_SEP))
            continue;

        INT_PTR id = pButtons[i].iString;
        LPWSTR psz = LoadStringDx(id);
        id = SendMessageW(hwnd, TB_ADDSTRING, 0, (LPARAM)psz);
        pButtons[i].iString = id;
    }
}

HWND ToolBar_Create(HWND hwndParent)
{
    HWND hwndTB;
    hwndTB = CreateWindowW(TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_WRAPABLE | TBSTYLE_LIST,
        0, 0, 0, 0, hwndParent, (HMENU)1, GetWindowInstance(hwndParent), NULL);
    if (hwndTB == NULL)
        return hwndTB;

    SendMessageW(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(0, 0));

    ToolBar_StoreStrings(hwndTB, _countof(g_buttons0), g_buttons0);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons1), g_buttons1);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons2), g_buttons2);
    //ToolBar_StoreStrings(hwndTB, _countof(g_buttons3), g_buttons3);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons4), g_buttons4);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons5), g_buttons5);

    ToolBar_Update(hwndTB, 3);
    return hwndTB;
}

//////////////////////////////////////////////////////////////////////////////

void InitLangComboBox(HWND hCmb3, LANGID langid)
{
    for (size_t i = 0; i < g_Langs.size(); ++i)
    {
        WCHAR sz[MAX_PATH];
        wsprintfW(sz, L"%s (%u)", g_Langs[i].Str.c_str(), g_Langs[i].LangID);
        INT k = ComboBox_AddString(hCmb3, sz);
        if (langid == g_Langs[i].LangID)
        {
            ComboBox_SetCurSel(hCmb3, k);
        }
    }
}

BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& Type)
{
    WCHAR szType[MAX_PATH];
    GetWindowTextW(hCmb1, szType, _countof(szType));
    MStringW Str = szType;
    mstr_trim(Str);
    lstrcpynW(szType, Str.c_str(), _countof(szType));

    if (szType[0] == 0)
    {
        ComboBox_SetEditSel(hCmb1, 0, -1);
        SetFocus(hCmb1);
        MessageBoxW(GetParent(hCmb1), LoadStringDx(IDS_ENTERTYPE),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (iswdigit(szType[0]) || szType[0] == L'-' || szType[0] == L'+')
    {
        Type = WORD(wcstol(szType, NULL, 0));
    }
    else
    {
        Type = szType;
    }

    return TRUE;
}

BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& Name)
{
    WCHAR szName[MAX_PATH];
    GetWindowTextW(hCmb2, szName, _countof(szName));
    MStringW Str = szName;
    mstr_trim(Str);
    lstrcpynW(szName, Str.c_str(), _countof(szName));
    if (szName[0] == 0)
    {
        ComboBox_SetEditSel(hCmb2, 0, -1);
        SetFocus(hCmb2);
        MessageBoxW(GetParent(hCmb2), LoadStringDx(IDS_ENTERNAME),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (iswdigit(szName[0]) || szName[0] == L'-' || szName[0] == L'+')
    {
        Name = WORD(wcstol(szName, NULL, 0));
    }
    else
    {
        if (db.HasResID(szName))
            Name = (WORD)db.GetResIDValue(szName);
        else
            Name = szName;
    }
    return TRUE;
}

BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang)
{
    WCHAR szLang[MAX_PATH];
    GetWindowTextW(hCmb3, szLang, _countof(szLang));
    MStringW Str = szLang;
    mstr_trim(Str);
    lstrcpynW(szLang, Str.c_str(), _countof(szLang));

    if (szLang[0] == 0)
    {
        ComboBox_SetEditSel(hCmb3, 0, -1);
        SetFocus(hCmb3);
        MessageBoxW(GetParent(hCmb3), LoadStringDx(IDS_ENTERLANG),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (iswdigit(szLang[0]) || szLang[0] == L'-' || szLang[0] == L'+')
    {
        Lang = WORD(wcstol(szLang, NULL, 0));
    }
    else
    {
        INT i = ComboBox_GetCurSel(hCmb3);
        if (i == CB_ERR || i >= INT(g_Langs.size()))
        {
            ComboBox_SetEditSel(hCmb3, 0, -1);
            SetFocus(hCmb3);
            MessageBoxW(GetParent(hCmb3), LoadStringDx(IDS_ENTERLANG),
                        NULL, MB_ICONERROR);
            return FALSE;
        }
        Lang = g_Langs[i].LangID;
    }

    return TRUE;
}

BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File)
{
    WCHAR szFile[MAX_PATH];
    GetWindowTextW(hEdt1, szFile, _countof(szFile));
    MStringW Str = szFile;
    mstr_trim(Str);
    lstrcpynW(szFile, Str.c_str(), _countof(szFile));
    if (::GetFileAttributesW(szFile) == 0xFFFFFFFF)
    {
        Edit_SetSel(hEdt1, 0, -1);
        SetFocus(hEdt1);
        MessageBoxW(GetParent(hEdt1), LoadStringDx(IDS_FILENOTFOUND),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    File = szFile;
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

std::wstring DumpBitmapInfo(HBITMAP hbm)
{
    std::wstring ret;
    BITMAP bm;
    if (!GetObjectW(hbm, sizeof(bm), &bm))
        return ret;

    WCHAR sz[64];
    wsprintfW(sz, LoadStringDx(IDS_IMAGEINFO),
              bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
    ret = sz;
    return ret;
}

std::wstring DumpCursorInfo(const BITMAP& bm)
{
    std::wstring ret;

    using namespace std;
    WCHAR sz[128];
    wsprintfW(sz, LoadStringDx(IDS_IMAGEINFO),
              bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
    ret = sz;

    return ret;
}

std::wstring DumpGroupIconInfo(const std::vector<BYTE>& data)
{
    std::wstring ret;
    WCHAR sz[128];

    ICONDIR dir;
    if (data.size() < sizeof(dir))
        return ret;

    memcpy(&dir, &data[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != 1 || dir.idCount == 0)
    {
        return ret;
    }

    wsprintfW(sz, LoadStringDx(IDS_IMAGECOUNT), dir.idCount);
    ret += sz;
    ret += L"-------\r\n";

    const GRPICONDIRENTRY *pEntries;
    pEntries = (const GRPICONDIRENTRY *)&data[sizeof(dir)];

    for (WORD i = 0; i < dir.idCount; ++i)
    {
        WORD Width = pEntries[i].bWidth;
        WORD Height = pEntries[i].bHeight;
        WORD nID = pEntries[i].nID;

        if (Width == 0)
            Width = 256;
        if (Height == 0)
            Height = 256;

        wsprintfW(sz, LoadStringDx(IDS_ICONINFO),
                  i, Width, Height, pEntries[i].wBitCount, nID);
        ret += sz;
    }

    return ret;
}

std::wstring DumpGroupCursorInfo(ResEntries& Entries, const std::vector<BYTE>& data)
{
    std::wstring ret;
    WCHAR sz[128];

    ICONDIR dir;
    if (data.size() < sizeof(dir))
        return ret;

    memcpy(&dir, &data[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
    {
        return ret;
    }

    wsprintfW(sz, LoadStringDx(IDS_IMAGECOUNT), dir.idCount);
    ret += sz;
    ret += L"-------\r\n";

    const GRPCURSORDIRENTRY *pEntries;
    pEntries = (const GRPCURSORDIRENTRY *)&data[sizeof(dir)];

    for (WORD i = 0; i < dir.idCount; ++i)
    {
        WORD Width = pEntries[i].wWidth;
        WORD Height = pEntries[i].wHeight / 2;
        WORD BitCount = pEntries[i].wBitCount;
        WORD nID = pEntries[i].nID;
        WORD xHotSpot = 0;
        WORD yHotSpot = 0;

        INT k = Res_Find(Entries, RT_CURSOR, nID, 0xFFFF);
        if (k != -1)
        {
            const ResEntry& CursorEntry = Entries[k];
            LOCALHEADER header;
            if (CursorEntry.size() >= sizeof(header))
            {
                memcpy(&header, &CursorEntry[0], sizeof(header));
                xHotSpot = header.xHotSpot;
                yHotSpot = header.yHotSpot;
            }
        }

        if (Width == 0)
            Width = 256;
        if (Height == 0)
            Height = 256;

        wsprintfW(sz, LoadStringDx(IDS_CURSORINFO),
                  i, Width, Height, BitCount, xHotSpot, yHotSpot, nID);
        ret += sz;
    }

    return ret;
}

std::wstring
DumpDataAsString(const std::vector<BYTE>& data)
{
    std::wstring ret;
    WCHAR sz[64];
    DWORD addr, size = DWORD(data.size());

    if (data.empty())
    {
        return ret;
    }

    ret.reserve(data.size() * 3);   // for speed

    ret +=
        L"+ADDRESS  +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F  0123456789ABCDEF\r\n"
        L"--------  -----------------------  -----------------------  ----------------\r\n";

    for (addr = 0; ; ++addr)
    {
        if ((addr & 0xF) == 0)
        {
            wsprintfW(sz, L"%08lX  ", addr);
            ret += sz;

            bool flag = false;
            for (DWORD i = 0; i < 16; ++i)
            {
                if (i == 8)
                    ret += L' ';
                DWORD offset = addr + i;
                if (offset < size)
                {
                    wsprintfW(sz, L"%02X ", data[offset]);
                    ret += sz;
                }
                else
                {
                    ret += L"   ";
                    flag = true;
                }
            }

            ret += L' ';

            for (DWORD i = 0; i < 16; ++i)
            {
                DWORD offset = addr + i;
                if (offset < size)
                {
                    if (data[offset] == 0)
                        ret += L' ';
                    else if (data[offset] < 0x20 || data[offset] > 0x7F)
                        ret += L'.';
                    else
                        ret += WCHAR(data[offset]);
                }
                else
                {
                    ret += L' ';
                    flag = true;
                }
            }

            ret += L"\r\n";

            if (flag)
                break;
        }
    }

    return ret;
}

void ReplaceBackslash(LPWSTR szPath)
{
    for (WCHAR *pch = szPath; *pch; ++pch)
    {
        if (*pch == L'\\')
            *pch = L'/';
    }
}

std::wstring GetKeyID(ConstantsDB& db, UINT wId)
{
    if ((BOOL)db.GetValue(L"HIDE.ID", L"HIDE.ID"))
        return mstr_dec(wId);

    return db.GetNameOfResID(IDTYPE_COMMAND, wId);
}

void Cmb1_InitVirtualKeys(HWND hCmb1, ConstantsDB& db)
{
    ComboBox_ResetContent(hCmb1);

    typedef ConstantsDB::TableType TableType;
    TableType table;
    table = db.GetTable(L"VIRTUALKEYS");

    TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        ComboBox_AddString(hCmb1, it->name.c_str());
    }
}

BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, std::wstring& str)
{
    if (bVirtKey)
    {
        INT i = ComboBox_FindStringExact(hCmb1, -1, str.c_str());
        if (i == CB_ERR)
        {
            BOOL bOK;
            i = GetDlgItemInt(hwnd, cmb1, &bOK, TRUE);
            if (!bOK)
            {
                return FALSE;
            }
            str = mstr_dec(i);
        }
    }
    else
    {
        BOOL bOK;
        INT i = GetDlgItemInt(hwnd, cmb1, &bOK, TRUE);
        if (!bOK)
        {
            LPCWSTR pch = str.c_str();
            std::wstring str2;
            if (!guts_quote(str2, pch) || str2.size() != 1)
            {
                return FALSE;
            }
            str = mstr_quote(str2);
        }
        else
        {
            str = mstr_dec(i);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// STRING_ENTRY

BOOL StrDlg_GetEntry(HWND hwnd, STRING_ENTRY& entry, ConstantsDB& db)
{
    MString str = MWindowBase::GetDlgItemText(hwnd, cmb1);
    mstr_trim(str);
    if (('0' <= str[0] && str[0] <= '9') || str[0] == '-' || str[0] == '+')
    {
        LONG n = wcstol(str.c_str(), NULL, 0);
        str = mstr_dec((WORD)n);
    }
    else if (!db.HasResID(str))
    {
        return FALSE;
    }
    lstrcpynW(entry.StringID, str.c_str(), _countof(entry.StringID));

    str = MWindowBase::GetDlgItemText(hwnd, edt1);
    mstr_trim(str);
    if (str[0] == L'"')
    {
        mstr_unquote(str);
    }
    lstrcpynW(entry.StringValue, str.c_str(), _countof(entry.StringValue));
    return TRUE;
}

void StrDlg_SetEntry(HWND hwnd, STRING_ENTRY& entry, ConstantsDB& db)
{
    SetDlgItemTextW(hwnd, cmb1, entry.StringID);

    std::wstring str = entry.StringValue;
    str = mstr_quote(str);

    SetDlgItemTextW(hwnd, edt1, str.c_str());
}

BOOL CALLBACK
EnumLocalesProc(LPWSTR lpLocaleString)
{
    LangEntry Entry;
    LCID lcid = wcstoul(lpLocaleString, NULL, 16);
    Entry.LangID = LANGIDFROMLCID(lcid);

    WCHAR sz[MAX_PATH] = L"";
    if (lcid == 0)
        return TRUE;
    if (!GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, sz, _countof(sz)))
        return TRUE;

    Entry.Str = sz;
    g_Langs.push_back(Entry);
    return TRUE;
}

std::wstring
Res_GetLangName(WORD Lang)
{
    WCHAR sz[64], szLoc[64];
    LCID lcid = MAKELCID(Lang, SORT_DEFAULT);
    if (lcid == 0)
    {
        wsprintfW(sz, L"%s (0)", LoadStringDx(IDS_NEUTRAL));
    }
    else
    {
        GetLocaleInfo(lcid, LOCALE_SLANGUAGE, szLoc, 64);
        wsprintfW(sz, L"%s (%u)", szLoc, Lang);
    }
    return std::wstring(sz);
}

//////////////////////////////////////////////////////////////////////////////

#define TV_WIDTH        250     // default m_hTreeView width
#define BV_WIDTH        160     // default m_hBmpView width
#define BE_HEIGHT       90      // default m_hBinEdit height
#define CX_STATUS_PART  80      // status bar part width

//////////////////////////////////////////////////////////////////////////////
// MMainWnd

class MMainWnd : public MWindowBase
{
public:
    INT         m_argc;         // number of command line parameters
    TCHAR **    m_targv;        // command line parameters

    HINSTANCE   m_hInst;        // the instance handle
    HICON       m_hIcon;        // the icon handle
    HACCEL      m_hAccel;       // the accelerator handle
    HIMAGELIST  m_hImageList;
    HICON       m_hFileIcon;
    HICON       m_hFolderIcon;
    HFONT       m_hNormalFont;
    HFONT       m_hLargeFont;
    HFONT       m_hSmallFont;
    HWND        m_hTreeView;
    HWND        m_hToolBar;
    HWND        m_hStatusBar;
    RisohSettings   m_settings;
    ConstantsDB     m_db;
    MRadWindow      m_rad_window;
    MEditCtrl       m_hBinEdit;
    MEditCtrl       m_hSrcEdit;
    MBmpView        m_hBmpView;
    MSplitterWnd    m_splitter1;
    MSplitterWnd    m_splitter2;
    MSplitterWnd    m_splitter3;
    MIDListDlg      m_id_list_dlg;

    WCHAR       m_szDataFolder[MAX_PATH];
    WCHAR       m_szConstantsFile[MAX_PATH];
    WCHAR       m_szCppExe[MAX_PATH];
    WCHAR       m_szWindresExe[MAX_PATH];
    WCHAR       m_szFile[MAX_PATH];
    WCHAR       m_szResourceH[MAX_PATH];
    ResEntries  m_Entries;

    MMainWnd(int argc, TCHAR **targv, HINSTANCE hInst) :
        m_argc(argc),
        m_targv(targv),
        m_hInst(hInst),
        m_hIcon(NULL),
        m_hAccel(NULL),
        m_hImageList(NULL),
        m_hFileIcon(NULL),
        m_hFolderIcon(NULL),
        m_hNormalFont(NULL),
        m_hLargeFont(NULL),
        m_hSmallFont(NULL),
        m_hTreeView(NULL),
        m_hToolBar(NULL),
        m_hStatusBar(NULL),
        m_rad_window(m_db, m_settings),
        m_id_list_dlg(m_db, m_settings)
    {
        m_szDataFolder[0] = 0;
        m_szConstantsFile[0] = 0;
        m_szCppExe[0] = 0;
        m_szWindresExe[0] = 0;
        m_szFile[0] = 0;
        m_szResourceH[0] = 0;
    }

    void SetDefaultSettings(HWND hwnd);
    BOOL LoadSettings(HWND hwnd);
    BOOL SaveSettings(HWND hwnd);

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        MWindowBase::ModifyWndClassDx(wcx);
        wcx.lpszMenuName = MAKEINTRESOURCE(1);
        wcx.hIcon = m_hIcon;
        wcx.hIconSm = m_hIcon;
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("katahiromz's RisohEditor");
    }

    BOOL StartDx(INT nCmdShow)
    {
        MSplitterWnd::CursorNS() = LoadCursor(m_hInst, MAKEINTRESOURCE(1));
        MSplitterWnd::CursorWE() = LoadCursor(m_hInst, MAKEINTRESOURCE(2));

        m_hIcon = ::LoadIcon(m_hInst, MAKEINTRESOURCE(1));
        m_hAccel = ::LoadAccelerators(m_hInst, MAKEINTRESOURCE(1));

        if (!CreateWindowDx(NULL, MAKEINTRESOURCE(IDS_APPNAME),
            WS_OVERLAPPEDWINDOW, 0, CW_USEDEFAULT, CW_USEDEFAULT, 760, 480))
        {
            ErrorBoxDx(TEXT("failure of CreateWindow"));
            return FALSE;
        }

        ShowWindow(m_hwnd, SW_SHOWDEFAULT);
        UpdateWindow(m_hwnd);

        return TRUE;
    }

    // message loop
    INT_PTR RunDx()
    {
        MSG msg;
        while (::GetMessage(&msg, NULL, 0, 0))
        {
            if (::IsDialogMessage(m_rad_window.m_rad_dialog, &msg))
                continue;
            if (::IsDialogMessage(m_id_list_dlg, &msg))
                continue;
            if (::TranslateAccelerator(m_hwnd, m_hAccel, &msg))
                continue;

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        return INT(msg.wParam);
    }

    void UpdateMenu()
    {
        HMENU hMenu = GetMenu(m_hwnd);
        HMENU hFileMenu = GetSubMenu(hMenu, 0);
        HMENU hMruMenu = GetSubMenu(hFileMenu, GetMenuItemCount(hFileMenu) - 3);
        assert(hMruMenu);
        while (DeleteMenu(hMruMenu, 0, MF_BYPOSITION))
            ;

        INT i = 0;
        TCHAR szText[MAX_PATH * 2];
        static const TCHAR szPrefix[] = TEXT("123456789ABCDEF0");
        mru_type::iterator it, end = m_settings.vecRecentlyUsed.end();
        for (it = m_settings.vecRecentlyUsed.begin(); it != end; ++it)
        {
#if 1
            LPCTSTR pch = _tcsrchr(it->c_str(), TEXT('\\'));
            if (pch == NULL)
                pch = _tcsrchr(it->c_str(), TEXT('/'));
            if (pch == NULL)
                pch = it->c_str();
            else
                ++pch;
            wsprintf(szText, TEXT("&%c  %s"), szPrefix[i], pch);
            InsertMenu(hMruMenu, i, MF_BYPOSITION | MF_STRING, CMDID_MRUFILE0 + i, szText);
#else
            InsertMenu(hMruMenu, i, MF_BYPOSITION | MF_STRING, CMDID_MRUFILE0 + i, it->c_str());
#endif
            ++i;
        }

        if (m_settings.vecRecentlyUsed.empty())
        {
            InsertMenu(hMruMenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, -1, LoadStringDx(IDS_NONE));
        }
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            DO_MSG(WM_CREATE, OnCreate);
            DO_MSG(WM_COMMAND, OnCommand);
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
        default:
            return DefaultProcDx();
        }
    }

    void OnSysColorChange(HWND hwnd)
    {
        m_splitter1.SendMessageDx(WM_SYSCOLORCHANGE);
        m_splitter2.SendMessageDx(WM_SYSCOLORCHANGE);
        m_splitter3.SendMessageDx(WM_SYSCOLORCHANGE);
        m_rad_window.SendMessageDx(WM_SYSCOLORCHANGE);
    }

    LRESULT OnCompileCheck(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        if (!CompileIfNecessary(hwnd))
        {
            return FALSE;
        }
        return FALSE;
    }

    LRESULT OnMoveSizeReport(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        INT x = (SHORT)LOWORD(wParam);
        INT y = (SHORT)HIWORD(wParam);
        INT cx = (SHORT)LOWORD(lParam);
        INT cy = (SHORT)HIWORD(lParam);

        WCHAR szText[64];
        wsprintfW(szText, LoadStringDx(IDS_COORD), x, y, cx, cy);
        ChangeStatusText(szText);
        return 0;
    }

    LRESULT OnClearStatus(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        ChangeStatusText(TEXT(""));
        return 0;
    }

    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
    {
        if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
        {
            SetFocus(m_hTreeView);
        }
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        m_id_list_dlg.m_hMainWnd = hwnd;

        LoadLangInfo();

        INT nRet = CheckData();
        if (nRet)
            return FALSE;

        LoadSettings(hwnd);

        if (m_settings.bResumeWindowPos)
        {
            if (m_settings.nWindowLeft != CW_USEDEFAULT)
            {
                POINT pt = { m_settings.nWindowLeft, m_settings.nWindowTop };
                SetWindowPosDx(&pt);
            }
            if (m_settings.nWindowWidth != CW_USEDEFAULT)
            {
                SIZE siz = { m_settings.nWindowWidth, m_settings.nWindowHeight };
                SetWindowPosDx(NULL, &siz);
            }
        }

        m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 1);
        if (m_hImageList == NULL)
            return FALSE;
        m_hFileIcon = LoadSmallIconDx(100);
        ImageList_AddIcon(m_hImageList, m_hFileIcon);
        m_hFolderIcon = LoadSmallIconDx(101);
        ImageList_AddIcon(m_hImageList, m_hFolderIcon);

        m_hToolBar = ToolBar_Create(hwnd);
        if (m_hToolBar == NULL)
            return FALSE;

        DWORD style, exstyle;

        style = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_LEFTALIGN;
        if (!m_splitter1.CreateDx(hwnd, 2, style))
            return FALSE;

        style = WS_CHILD | WS_VISIBLE | SWS_VERT | SWS_BOTTOMALIGN;
        if (!m_splitter2.CreateDx(m_splitter1, 2, style))
            return FALSE;

        style = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_RIGHTALIGN;
        if (!m_splitter3.CreateDx(m_splitter2, 1, style))
            return FALSE;

        style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
            TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES |
            TVS_LINESATROOT | TVS_SHOWSELALWAYS;
        m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
            WC_TREEVIEWW, NULL, style, 0, 0, 0, 0, m_splitter1,
            (HMENU)1, m_hInst, NULL);
        if (m_hTreeView == NULL)
            return FALSE;

        TreeView_SetImageList(m_hTreeView, m_hImageList, TVSIL_NORMAL);

        m_splitter1.SetPane(0, m_hTreeView);
        m_splitter1.SetPane(1, m_splitter2);
        m_splitter1.SetPaneExtent(0, m_settings.nTreeViewWidth);

        style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
            ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
            ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
        exstyle = WS_EX_CLIENTEDGE;
        m_hBinEdit.CreateAsChildDx(m_splitter2, NULL, style, exstyle, 3);
        m_splitter2.SetPane(0, m_splitter3);
        m_splitter2.SetPane(1, m_hBinEdit);
        m_splitter2.SetPaneExtent(1, BE_HEIGHT);

        style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
            ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
            ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
        exstyle = WS_EX_CLIENTEDGE;
        m_hSrcEdit.CreateAsChildDx(m_splitter3, NULL, style, exstyle, 2);
        if (!m_hBmpView.CreateDx(m_splitter3, 4))
            return FALSE;

        m_splitter3.SetPane(0, m_hSrcEdit);
        //m_splitter3.SetPane(1, m_hBmpView);

        style = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_BOTTOM;
        m_hStatusBar = CreateStatusWindow(style, LoadStringDx(IDS_STARTING), hwnd, 8);
        if (m_hStatusBar == NULL)
            return FALSE;

        INT anWidths[] = { -1 };
        SendMessage(m_hStatusBar, SB_SETPARTS, 1, (LPARAM)anWidths);
        ChangeStatusText(IDS_STARTING);

        if (m_settings.bShowStatusBar)
            ShowWindow(m_hStatusBar, SW_SHOWNOACTIVATE);
        else
            ShowWindow(m_hStatusBar, SW_HIDE);

        LOGFONTW lf;
        GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
        lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
        lf.lfFaceName[0] = 0;

        lf.lfHeight = 11;
        m_hSmallFont = CreateFontIndirectW(&lf);
        assert(m_hSmallFont);

        lf.lfHeight = 13;
        m_hNormalFont = ::CreateFontIndirectW(&lf);
        assert(m_hNormalFont);

        lf.lfHeight = 15;
        m_hLargeFont = ::CreateFontIndirectW(&lf);
        assert(m_hLargeFont);

        SetWindowFont(m_hSrcEdit, m_hNormalFont, TRUE);
        SetWindowFont(m_hBinEdit, m_hSmallFont, TRUE);

        if (m_argc >= 2)
        {
            DoLoad(hwnd, m_Entries, m_targv[1]);
        }

        DragAcceptFiles(hwnd, TRUE);
        SetFocus(m_hTreeView);
        UpdateMenu();

        PostMessageDx(WM_COMMAND, CMDID_READY);

        return TRUE;
    }

    VOID HidePreview(HWND hwnd)
    {
        m_hBmpView.DestroyView();

        ::SetWindowTextW(m_hBinEdit, NULL);
        Edit_SetModify(m_hBinEdit, FALSE);

        ::SetWindowTextW(m_hSrcEdit, NULL);
        Edit_SetModify(m_hSrcEdit, FALSE);

        ShowBmpView(FALSE);

        PostMessageDx(WM_SIZE);
    }

    BOOL DoSetFile(HWND hwnd, LPCWSTR FileName)
    {
        if (FileName == 0 || FileName[0] == 0)
        {
            ::SetWindowTextW(hwnd, LoadStringDx(IDS_APPNAME));
            return TRUE;
        }

        WCHAR Path[MAX_PATH], *pch;
        GetFullPathNameW(FileName, _countof(Path), Path, &pch);
        lstrcpynW(m_szFile, Path, _countof(m_szFile));

        WCHAR sz[MAX_PATH];
        pch = wcsrchr(Path, L'\\');
        if (pch)
        {
            wsprintfW(sz, LoadStringDx(IDS_TITLEWITHFILE), pch + 1);
            ::SetWindowTextW(hwnd, sz);
        }
        else
        {
            ::SetWindowTextW(hwnd, LoadStringDx(IDS_APPNAME));
        }

        m_settings.AddFile(m_szFile);
        UpdateMenu();
        return TRUE;
    }

    void OnDeleteRes(HWND hwnd)
    {
        if (!CompileIfNecessary(hwnd))
            return;

        HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
        if (hItem == NULL)
            return;

        TV_Delete(m_hTreeView, hItem, m_Entries);
        HidePreview(hwnd);
    }

    void OnExtractBin(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) == I_NONE)
            return;

        UINT i = LOWORD(lParam);

        WCHAR szFile[MAX_PATH] = L"";
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        if (HIWORD(lParam) == I_STRING || HIWORD(lParam) == I_MESSAGE)
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
        if (HIWORD(lParam) == I_LANG)
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESBINFILTER));
        else
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"res";
        if (GetSaveFileNameW(&ofn))
        {
            if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
            {
                ResEntries selection;
                INT count = TV_GetSelection(m_hTreeView, selection, m_Entries);
                if (count && !DoExtractRes(hwnd, ofn.lpstrFile, selection))
                {
                    ErrorBoxDx(IDS_CANNOTSAVE);
                }
            }
            else
            {
                if (!DoExtractBin(ofn.lpstrFile, m_Entries[i]))
                {
                    ErrorBoxDx(IDS_CANNOTSAVE);
                }
            }
        }
    }

    void OnPlay(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        ResEntry& entry = m_Entries[i];

        if (entry.type == L"WAVE")
        {
            PlaySound((LPCTSTR)&entry[0], NULL, SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
        }
    }

    void OnExtractIcon(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        ResEntry& entry = m_Entries[i];

        WCHAR szFile[MAX_PATH] = L"";
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTICO);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (entry.type == RT_ANIICON)
        {
            ofn.nFilterIndex = 2;
            ofn.lpstrDefExt = L"ani";
        }
        else
        {
            ofn.nFilterIndex = 1;
            ofn.lpstrDefExt = L"ico";
        }
        if (GetSaveFileNameW(&ofn))
        {
            if (!DoExtractIcon(ofn.lpstrFile, m_Entries[i]))
            {
                ErrorBoxDx(IDS_CANTEXTRACTICO);
            }
        }
    }

    void OnExtractCursor(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        ResEntry& entry = m_Entries[i];

        WCHAR szFile[MAX_PATH] = L"";
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTCUR);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (entry.type == RT_ANICURSOR)
        {
            ofn.nFilterIndex = 2;
            ofn.lpstrDefExt = L"ani";
        }
        else
        {
            ofn.nFilterIndex = 1;
            ofn.lpstrDefExt = L"cur";
        }
        if (GetSaveFileNameW(&ofn))
        {
            if (!DoExtractCursor(ofn.lpstrFile, m_Entries[i]))
            {
                ErrorBoxDx(IDS_CANTEXTRACTCUR);
            }
        }
    }

    void OnExtractBitmap(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);

        WCHAR szFile[MAX_PATH] = L"";
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_BMPFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTBMP);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"bmp";
        if (GetSaveFileNameW(&ofn))
        {
            BOOL PNG;
            PNG = (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"png") == 0);
            if (!DoExtractBitmap(ofn.lpstrFile, m_Entries[i], PNG))
            {
                ErrorBoxDx(IDS_CANTEXTRACTBMP);
            }
        }
    }

    void OnReplaceBin(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        MReplaceBinDlg dialog(m_Entries, m_Entries[i], m_db);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
        }
    }

    void OnSaveAs(HWND hwnd)
    {
        if (!CompileIfNecessary(hwnd))
            return;

        WCHAR File[MAX_PATH];

        lstrcpynW(File, m_szFile, _countof(File));
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        DWORD dwBinType;
        if (m_szFile[0] == 0 || !GetBinaryType(m_szFile, &dwBinType))
        {
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
            ofn.lpstrDefExt = L"res";
        }
        else
        {
            ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXEFILTER));
            ofn.lpstrDefExt = L"exe";
        }
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx(IDS_SAVEAS);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (GetSaveFileNameW(&ofn))
        {
            if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
            {
                if (!DoSaveResAs(hwnd, File))
                {
                    ErrorBoxDx(IDS_CANNOTSAVE);
                }
            }
            else
            {
                if (!DoSaveAs(hwnd, File))
                {
                    ErrorBoxDx(IDS_CANNOTSAVE);
                }
            }
        }
    }

    void OnTest(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
        if (hItem == NULL)
            return;

        TV_ITEM Item;
        ZeroMemory(&Item, sizeof(Item));
        Item.mask = TVIF_PARAM;
        Item.hItem = hItem;
        TreeView_GetItem(m_hTreeView, &Item);

        if (HIWORD(Item.lParam) != 3)
            return;

        UINT i = LOWORD(Item.lParam);
        const ResEntry& Entry = m_Entries[i];
        if (Entry.type == RT_DIALOG)
        {
            MTestDialog dialog;
            dialog.DialogBoxIndirectDx(hwnd, Entry.ptr());
        }
        else if (Entry.type == RT_MENU)
        {
            HMENU hMenu = LoadMenuIndirect(&Entry[0]);
            if (hMenu)
            {
                MTestMenuDlg dialog(hMenu);
                dialog.DialogBoxDx(hwnd, IDD_MENUTEST);
                DestroyMenu(hMenu);
            }
        }
    }

    void OnAddIcon(HWND hwnd)
    {
        MAddIconDlg dialog(m_db, m_Entries);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
        }
    }

    void OnReplaceIcon(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        MReplaceIconDlg dialog(m_db, m_Entries, m_Entries[i]);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
        }
    }

    void OnReplaceCursor(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        MReplaceCursorDlg dialog(m_db, m_Entries, m_Entries[i]);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
        }
    }

    void OnOpen(HWND hwnd)
    {
        if (!CompileIfNecessary(hwnd))
            return;

        WCHAR File[MAX_PATH];
        lstrcpynW(File, m_szFile, _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXEFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx(IDS_OPEN);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"exe";
        if (GetOpenFileNameW(&ofn))
        {
            DoLoad(hwnd, m_Entries, File);
        }
    }

    void OnAddBitmap(HWND hwnd)
    {
        MAddBitmapDlg dialog(m_db, m_Entries);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
        }
    }

    void OnReplaceBitmap(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        MReplaceBitmapDlg dialog(m_db, m_Entries, m_Entries[i]);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
        }
    }

    void OnAddCursor(HWND hwnd)
    {
        MAddCursorDlg dialog(m_db, m_Entries);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
        }
    }

    void OnAddRes(HWND hwnd)
    {
        MAddResDlg dialog(m_Entries, m_db);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
        }
    }

    void OnAbout(HWND hwnd)
    {
        MSGBOXPARAMSW Params;
        ZeroMemory(&Params, sizeof(Params));
        Params.cbSize = sizeof(Params);
        Params.hwndOwner = hwnd;
        Params.hInstance = m_hInst;
        Params.lpszText = LoadStringDx(IDS_VERSIONINFO);
        Params.lpszCaption = LoadStringDx(IDS_APPNAME);
        Params.dwStyle = MB_OK | MB_USERICON;
        Params.lpszIcon = MAKEINTRESOURCEW(1);
        Params.dwLanguageId = LANG_USER_DEFAULT;
        MessageBoxIndirectW(&Params);
    }

    void OnImport(HWND hwnd)
    {
        if (!CompileIfNecessary(hwnd))
            return;

        WCHAR File[MAX_PATH] = TEXT("");

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx(IDS_IMPORTRES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"res";
        if (GetOpenFileNameW(&ofn))
        {
            ResEntries entries;
            if (DoImport(hwnd, File, entries))
            {
                BOOL Overwrite = TRUE;
                if (Res_Intersect(m_Entries, entries))
                {
                    INT nID = MsgBoxDx(IDS_EXISTSOVERWRITE,
                                       MB_ICONINFORMATION | MB_YESNOCANCEL);
                    switch (nID)
                    {
                    case IDYES:
                        break;
                    case IDNO:
                        Overwrite = FALSE;
                        break;
                    case IDCANCEL:
                        return;
                    }
                }

                size_t i, count = entries.size();
                for (i = 0; i < count; ++i)
                {
                    Res_AddEntry(m_Entries, entries[i], Overwrite);
                }

                TV_RefreshInfo(m_hTreeView, m_Entries);
            }
            else
            {
                ErrorBoxDx(IDS_CANNOTIMPORT);
            }
        }
    }

    void OnEdit(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (!IsEditableEntry(hwnd, lParam))
            return;

        Edit_SetReadOnly(m_hSrcEdit, FALSE);
        SelectTV(hwnd, lParam, TRUE);
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
    {
        if (pnmhdr->code == NM_DBLCLK)
        {
            if (pnmhdr->hwndFrom == m_hTreeView)
            {
                LPARAM lParam = TV_GetParam(m_hTreeView);
                if (HIWORD(lParam) == I_LANG)
                {
                    OnEdit(hwnd);
                    if (m_settings.bGuiByDblClick)
                    {
                        OnGuiEdit(hwnd);
                    }
                    return 1;
                }
            }
        }
        else if (pnmhdr->code == TVN_SELCHANGING)
        {
            if (m_rad_window)
            {
                DestroyWindow(m_rad_window);
                return FALSE;
            }
            else
            {
                return !CompileIfNecessary(hwnd);
            }
        }
        else if (pnmhdr->code == TVN_SELCHANGED)
        {
            NM_TREEVIEWW *pTV = (NM_TREEVIEWW *)pnmhdr;
            LPARAM lParam = pTV->itemNew.lParam;
            SelectTV(hwnd, lParam, FALSE);
        }
        else if (pnmhdr->code == NM_RETURN)
        {
            if (pnmhdr->hwndFrom == m_hTreeView)
            {
                LPARAM lParam = TV_GetParam(m_hTreeView);
                if (HIWORD(lParam) == I_LANG)
                {
                    OnEdit(hwnd);
                    if (m_settings.bGuiByDblClick)
                    {
                        OnGuiEdit(hwnd);
                    }
                    return 1;
                }
            }
        }
        else if (pnmhdr->code == TVN_KEYDOWN)
        {
            TV_KEYDOWN *pTVKD = (TV_KEYDOWN *)pnmhdr;
            switch (pTVKD->wVKey)
            {
            case VK_DELETE:
                PostMessageW(hwnd, WM_COMMAND, CMDID_DELETERES, 0);
                return 1;
            }
        }
        return 0;
    }

    void OnCancelEdit(HWND hwnd)
    {
        Edit_SetModify(m_hSrcEdit, FALSE);
        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        LPARAM lParam = TV_GetParam(m_hTreeView);
        SelectTV(hwnd, lParam, FALSE);
    }

    void OnCompile(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (!Edit_GetModify(m_hSrcEdit))
        {
            SelectTV(hwnd, lParam, FALSE);
            return;
        }

        WORD i = LOWORD(lParam);
        if (i >= m_Entries.size())
            return;

        ResEntry& entry = m_Entries[i];

        ChangeStatusText(IDS_COMPILING);

        INT cchText = ::GetWindowTextLengthW(m_hSrcEdit);
        std::wstring WideText;
        WideText.resize(cchText);
        ::GetWindowTextW(m_hSrcEdit, &WideText[0], cchText + 1);

        Edit_SetModify(m_hSrcEdit, FALSE);
        if (DoCompileParts(hwnd, WideText))
        {
            TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, entry);
        }
    }

    void OnGuiEdit(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (!IsEditableEntry(hwnd, lParam))
            return;

        WORD i = LOWORD(lParam);
        if (i >= m_Entries.size())
            return;

        ResEntry& Entry = m_Entries[i];
        if (!Res_CanGuiEdit(Entry.type))
        {
            return;
        }

        if (!CompileIfNecessary(hwnd))
        {
            return;
        }

        const ResEntry::DataType& data = Entry.data;
        MByteStreamEx stream(data);
        if (Entry.type == RT_ACCELERATOR)
        {
            AccelRes accel_res(m_db);
            MEditAccelDlg dialog(accel_res, m_db);
            if (accel_res.LoadFromStream(stream))
            {
                ChangeStatusText(IDS_EDITINGBYGUI);
                INT nID = dialog.DialogBoxDx(hwnd);
                if (nID == IDOK)
                {
                    accel_res.Update();
                    Entry.data = accel_res.data();
                    SelectTV(hwnd, lParam, FALSE);
                }
            }
            Edit_SetReadOnly(m_hSrcEdit, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, Entry);
            ChangeStatusText(IDS_READY);
        }
        else if (Entry.type == RT_MENU)
        {
            MenuRes menu_res;
            if (menu_res.LoadFromStream(stream))
            {
                ChangeStatusText(IDS_EDITINGBYGUI);
                MEditMenuDlg dialog(m_db, menu_res);
                INT nID = dialog.DialogBoxDx(hwnd);
                if (nID == IDOK)
                {
                    menu_res.Update();
                    Entry.data = menu_res.data();
                    SelectTV(hwnd, lParam, FALSE);
                }
            }
            Edit_SetReadOnly(m_hSrcEdit, FALSE);
            TV_SelectEntry(m_hTreeView, m_Entries, Entry);
            ChangeStatusText(IDS_READY);
        }
        else if (Entry.type == RT_DIALOG)
        {
            if (m_rad_window)
            {
                DestroyWindow(m_rad_window);
            }

            MByteStreamEx stream(Entry.data);
            m_rad_window.m_dialog_res.LoadFromStream(stream);
            m_rad_window.m_dialog_res.m_LangID = Entry.lang;

            ChangeStatusText(IDS_EDITINGBYGUI);
            DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
            if (m_rad_window.CreateWindowDx(m_hwnd, MAKEINTRESOURCE(IDS_RADWINDOW),
                                            style))
            {
                CenterWindowDx(m_rad_window);
                ShowWindow(m_rad_window, SW_SHOWNORMAL);
                UpdateWindow(m_rad_window);
            }
            else
            {
                ErrorBoxDx(IDS_DLGFAIL);
            }
            Edit_SetReadOnly(m_hSrcEdit, FALSE);
        }
        else if (Entry.type == RT_STRING && HIWORD(lParam) == I_STRING)
        {
            ResEntries found;
            Res_Search(found, m_Entries, RT_STRING, (WORD)0, Entry.lang);

            StringRes str_res;
            ResEntries::iterator it, end = found.end();
            for (it = found.begin(); it != end; ++it)
            {
                MByteStreamEx stream(it->data);
                if (!str_res.LoadFromStream(stream, it->name.m_ID))
                {
                    ErrorBoxDx(IDS_CANNOTLOAD);
                    return;
                }
            }

            ChangeStatusText(IDS_EDITINGBYGUI);
            MStringsDlg dialog(m_db, str_res);
            INT nID = dialog.DialogBoxDx(hwnd);
            if (nID == IDOK)
            {
                std::wstring WideText = str_res.Dump(m_db);
                ::SetWindowTextW(m_hSrcEdit, WideText.c_str());

                if (DoCompileParts(hwnd, WideText))
                {
                    TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
                    TV_SelectEntry(m_hTreeView, m_Entries, Entry);
                    SelectTV(hwnd, lParam, FALSE);
                }
            }
            Edit_SetReadOnly(m_hSrcEdit, FALSE);
            ChangeStatusText(IDS_READY);
        }
    }

    void OnNew(HWND hwnd)
    {
        DoSetFile(hwnd, NULL);
        m_Entries.clear();
        TV_RefreshInfo(m_hTreeView, m_Entries);
    }

    void OnUpdateDlgRes(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        ResEntry& Entry = m_Entries[i];
        if (Entry.type != RT_DIALOG)
        {
            return;
        }

        DialogRes& dialog_res = m_rad_window.m_dialog_res;

        MByteStreamEx stream;
        dialog_res.SaveToStream(stream);
        Entry.data = stream.data();

        std::wstring str = dialog_res.Dump(Entry.name, m_db);
        ::SetWindowTextW(m_hSrcEdit, str.c_str());

        str = DumpDataAsString(Entry.data);
        ::SetWindowTextW(m_hBinEdit, str.c_str());
    }

    void ChangeStatusText(INT nID)
    {
        ChangeStatusText(LoadStringDx(nID));
    }
    void ChangeStatusText(LPCTSTR pszText)
    {
        SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)pszText);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        if (codeNotify == EN_CHANGE && m_hSrcEdit == hwndCtl)
        {
            ToolBar_Update(m_hToolBar, 2);
            ChangeStatusText(IDS_READY);
            return;
        }

        if (!::IsWindow(m_rad_window) && id >= 100)
            ChangeStatusText(IDS_EXECUTINGCMD);

        static INT s_nCount = 0;
        ++s_nCount;
        BOOL bUpdateStatus = TRUE;
        switch (id)
        {
        case CMDID_NEW:
            OnNew(hwnd);
            break;
        case CMDID_OPEN:
            OnOpen(hwnd);
            break;
        case CMDID_SAVEAS:
            OnSaveAs(hwnd);
            break;
        case CMDID_IMPORT:
            OnImport(hwnd);
            break;
        case CMDID_EXIT:
            DestroyWindow(hwnd);
            break;
        case CMDID_ADDICON:
            OnAddIcon(hwnd);
            break;
        case CMDID_ADDCURSOR:
            OnAddCursor(hwnd);
            break;
        case CMDID_ADDBITMAP:
            OnAddBitmap(hwnd);
            break;
        case CMDID_ADDRES:
            OnAddRes(hwnd);
            break;
        case CMDID_REPLACEICON:
            OnReplaceIcon(hwnd);
            break;
        case CMDID_REPLACECURSOR:
            OnReplaceCursor(hwnd);
            break;
        case CMDID_REPLACEBITMAP:
            OnReplaceBitmap(hwnd);
            break;
        case CMDID_REPLACEBIN:
            OnReplaceBin(hwnd);
            break;
        case CMDID_DELETERES:
            OnDeleteRes(hwnd);
            break;
        case CMDID_EDIT:
            OnEdit(hwnd);
            break;
        case CMDID_EXTRACTICON:
            OnExtractIcon(hwnd);
            break;
        case CMDID_EXTRACTCURSOR:
            OnExtractCursor(hwnd);
            break;
        case CMDID_EXTRACTBITMAP:
            OnExtractBitmap(hwnd);
            break;
        case CMDID_EXTRACTBIN:
            OnExtractBin(hwnd);
            break;
        case CMDID_ABOUT:
            OnAbout(hwnd);
            break;
        case CMDID_TEST:
            OnTest(hwnd);
            break;
        case CMDID_CANCELEDIT:
            OnCancelEdit(hwnd);
            break;
        case CMDID_COMPILE:
            OnCompile(hwnd);
            break;
        case CMDID_GUIEDIT:
            OnGuiEdit(hwnd);
            break;
        case CMDID_DESTROYRAD:
            OnCancelEdit(hwnd);
            break;
        case CMDID_UPDATEDLGRES:
            OnUpdateDlgRes(hwnd);
            bUpdateStatus = FALSE;
            break;
        case CMDID_DELCTRL:
            MRadCtrl::DeleteSelection();
            break;
        case CMDID_ADDCTRL:
            m_rad_window.OnAddCtrl(m_rad_window);
            break;
        case CMDID_CTRLPROP:
            m_rad_window.OnCtrlProp(m_rad_window);
            break;
        case CMDID_DLGPROP:
            m_rad_window.OnDlgProp(m_rad_window);
            break;
        case CMDID_CTRLINDEXTOP:
            m_rad_window.IndexTop(m_rad_window);
            break;
        case CMDID_CTRLINDEXBOTTOM:
            m_rad_window.IndexBottom(m_rad_window);
            break;
        case CMDID_CTRLINDEXMINUS:
            m_rad_window.IndexMinus(m_rad_window);
            break;
        case CMDID_CTRLINDEXPLUS:
            m_rad_window.IndexPlus(m_rad_window);
            break;
        case CMDID_SHOWHIDEINDEX:
            m_rad_window.OnShowHideIndex(m_rad_window);
            break;
        case CMDID_TOPALIGN:
            m_rad_window.OnTopAlign(m_rad_window);
            break;
        case CMDID_BOTTOMALIGN:
            m_rad_window.OnBottomAlign(m_rad_window);
            break;
        case CMDID_LEFTALIGN:
            m_rad_window.OnLeftAlign(m_rad_window);
            break;
        case CMDID_RIGHTALIGN:
            m_rad_window.OnRightAlign(m_rad_window);
            break;
        case CMDID_STATUSBAR:
            m_settings.bShowStatusBar = !m_settings.bShowStatusBar;
            ShowStatusBar(m_settings.bShowStatusBar);
            PostMessageDx(WM_SIZE);
            break;
        case CMDID_BINARYPANE:
            m_settings.bShowBinEdit = !m_settings.bShowBinEdit;
            ShowBinEdit(m_settings.bShowBinEdit);
            break;
        case CMDID_ALWAYSCONTROL:
            {
                m_settings.bAlwaysControl = !m_settings.bAlwaysControl;
                LPARAM lParam = TV_GetParam(m_hTreeView);
                SelectTV(hwnd, lParam, TRUE);
            }
            break;
        case CMDID_MRUFILE0:
        case CMDID_MRUFILE1:
        case CMDID_MRUFILE2:
        case CMDID_MRUFILE3:
        case CMDID_MRUFILE4:
        case CMDID_MRUFILE5:
        case CMDID_MRUFILE6:
        case CMDID_MRUFILE7:
        case CMDID_MRUFILE8:
        case CMDID_MRUFILE9:
        case CMDID_MRUFILE10:
        case CMDID_MRUFILE11:
        case CMDID_MRUFILE12:
        case CMDID_MRUFILE13:
        case CMDID_MRUFILE14:
        case CMDID_MRUFILE15:
            {
                DWORD i = id - CMDID_MRUFILE0;
                if (i < m_settings.vecRecentlyUsed.size())
                {
                    DoLoad(hwnd, m_Entries, m_settings.vecRecentlyUsed[i].c_str());
                }
            }
            break;
        case CMDID_PLAY:
            OnPlay(hwnd);
            break;
        case CMDID_READY:
            break;
        case CMDID_IDASSOC:
            OnIdAssoc(hwnd);
            break;
        case CMDID_LOADRESH:
            OnLoadResH(hwnd);
            break;
        case CMDID_IDLIST:
            OnIDList(hwnd);
            break;
        case CMDID_UNLOADRESH:
            OnUnloadResH(hwnd);
            break;
        case CMDID_HIDEIDMACROS:
            OnHideIDMacros(hwnd);
            break;
        case CMDID_CONFIG:
            OnConfig(hwnd);
            break;
        case CMDID_ADVICERESH:
            OnAdviceResH(hwnd);
            break;
        case CMDID_UPDATEID:
            OnUpdateID(hwnd);
            break;
        case CMDID_OPENREADME:
            OnOpenReadMe(hwnd);
            break;
        case CMDID_OPENREADMEJP:
            OnOpenReadMeJp(hwnd);
            break;
        default:
            bUpdateStatus = FALSE;
            break;
        }
        --s_nCount;

        if (bUpdateStatus && !::IsWindow(m_rad_window) && s_nCount == 0)
            ChangeStatusText(IDS_READY);
    }

    void OnOpenReadMe(HWND hwnd)
    {
        WCHAR szPath[MAX_PATH];
        GetModuleFileNameW(NULL, szPath, _countof(szPath));
        LPWSTR pch = wcsrchr(szPath, L'\\');
        if (pch == NULL)
            return;

        ++pch;
        lstrcpyW(pch, L"README.txt");
        if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            lstrcpyW(pch, L"../README.txt");
            if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                lstrcpyW(pch, L"../../README.txt");
                if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    lstrcpyW(pch, L"../../../README.txt");
                    if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                    {
                        return;
                    }
                }
            }
        }
        ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
    }

    void OnOpenReadMeJp(HWND hwnd)
    {
        WCHAR szPath[MAX_PATH];
        GetModuleFileNameW(NULL, szPath, _countof(szPath));
        LPWSTR pch = wcsrchr(szPath, L'\\');
        if (pch == NULL)
            return;

        ++pch;
        lstrcpyW(pch, L"READMEJP.txt");
        if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            lstrcpyW(pch, L"../READMEJP.txt");
            if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                lstrcpyW(pch, L"../../READMEJP.txt");
                if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    lstrcpyW(pch, L"../../../READMEJP.txt");
                    if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                    {
                        return;
                    }
                }
            }
        }
        ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
    }

    void OnUpdateID(HWND hwnd)
    {
        SelectTV(hwnd, 0, FALSE);
    }

    void OnAdviceResH(HWND hwnd)
    {
        MString str;

        if (m_settings.added_ids.empty() &&
            m_settings.removed_ids.empty())
        {
            str += LoadStringDx(IDS_NOCHANGE);
            str += TEXT("\r\n");
        }

        if (!m_settings.added_ids.empty())
        {
            str += LoadStringDx(IDS_ADDNEXTIDS);

            id_map_type::iterator it, end = m_settings.added_ids.end();
            for (it = m_settings.added_ids.begin(); it != end; ++it)
            {
                str += TEXT("#define ");
                str += MAnsiToText(it->first).c_str();
                str += TEXT(" ");
                str += MAnsiToText(it->second).c_str();
                str += TEXT("\r\n");
            }
            str += TEXT("\r\n");
        }

        if (!m_settings.removed_ids.empty())
        {
            str += LoadStringDx(IDS_DELETENEXTIDS);

            id_map_type::iterator it, end = m_settings.removed_ids.end();
            for (it = m_settings.removed_ids.begin(); it != end; ++it)
            {
                str += TEXT("#define ");
                str += MAnsiToText(it->first).c_str();
                str += TEXT(" ");
                str += MAnsiToText(it->second).c_str();
                str += TEXT("\r\n");
            }
            str += TEXT("\r\n");
        }

        MAdviceResHDlg dialog(m_settings, str);
        dialog.DialogBoxDx(hwnd);
    }

    void OnUnloadResH(HWND hwnd)
    {
        m_db.m_map[L"RESOURCE.ID"].clear();
        m_settings.added_ids.clear();
        m_settings.removed_ids.clear();
        m_szResourceH[0] = 0;
        ShowIDList(hwnd, FALSE);
    }

    void OnConfig(HWND hwnd)
    {
        MConfigDlg dialog(m_settings);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            SelectTV(hwnd, 0, FALSE);
        }
    }

    void OnHideIDMacros(HWND hwnd)
    {
        BOOL bHideID = (BOOL)m_db.GetValue(L"HIDE.ID", L"HIDE.ID");
        bHideID = !bHideID;
        m_settings.bHideID = bHideID;
        ConstantsDB::TableType& table = m_db.m_map[L"HIDE.ID"];
        table.clear();
        ConstantsDB::EntryType entry(L"HIDE.ID", bHideID);
        table.push_back(entry);
        SelectTV(hwnd, 0, FALSE);
    }

    void UpdateIDList(HWND hwnd)
    {
        if (!IsWindow(m_id_list_dlg))
            return;

        m_id_list_dlg.SetItems();
    }

    void ShowIDList(HWND hwnd, BOOL bShow = TRUE)
    {
        if (bShow)
        {
            DestroyWindow(m_id_list_dlg);
            m_id_list_dlg.CreateDialogDx(NULL);
            ShowWindow(m_id_list_dlg, SW_SHOWNORMAL);
            UpdateWindow(m_id_list_dlg);
            UpdateIDList(hwnd);
        }
        else
        {
            DestroyWindow(m_id_list_dlg);
        }
    }

    void OnIDList(HWND hwnd)
    {
        ShowIDList(hwnd, TRUE);
    }

    BOOL ParseMacros(HWND hwnd, LPCTSTR pszFile, std::vector<MStringA>& macros, MStringA& str)
    {
        std::vector<MStringA> lines;
        mstr_trim(str);
        mstr_split(lines, str, "\n");

        size_t len = lines.size() - 1;
        if (macros.size() < len)
            len = macros.size();

        for (size_t i = 0; i < len; ++i)
        {
            const MStringA& macro = macros[i];
            const MStringA& line = lines[i + 1];
            using namespace MacroParser;
            StringScanner scanner(line);
            TokenStream stream(scanner);
            stream.read_tokens();
            Parser parser(stream);
            if (parser.parse())
            {
                int value = 0;
                if (eval_ast(parser.ast(), value))
                {
                    char sz[32];
                    wsprintfA(sz, "%d", value);
                    m_settings.id_map[macro] = sz;
                }
#if 0
                else if (parser.ast()->m_id == ASTID_STRING)
                {
                    StringAst *str = (StringAst *)parser.ast();
                    m_settings.id_map[macro] = str->m_str;
                }
#endif
            }
        }

        ConstantsDB::TableType& table = m_db.m_map[L"RESOURCE.ID"];
        table.clear();
        id_map_type::iterator it, end = m_settings.id_map.end();
        for (it = m_settings.id_map.begin(); it != end; ++it)
        {
            MStringW str1 = MAnsiToWide(it->first).c_str();
            MStringW str2 = MAnsiToWide(it->second).c_str();
            DWORD value2 = wcstol(str2.c_str(), NULL, 0);
            ConstantsDB::EntryType entry(str1, value2);
            table.push_back(entry);
        }

        lstrcpynW(m_szResourceH, pszFile, _countof(m_szResourceH));
        if (m_settings.bAutoShowIDList)
        {
            ShowIDList(hwnd, TRUE);
        }
        return TRUE;
    }

    BOOL ParseResH(HWND hwnd, LPCTSTR pszFile, const char *psz, DWORD len)
    {
        MStringA str(psz, len);
        std::vector<MStringA> lines, macros;
        mstr_split(lines, str, "\n");

        for (size_t i = 0; i < lines.size(); ++i)
        {
            MStringA& line = lines[i];
            mstr_trim(line);
            if (line.empty())
                continue;
            if (line.find("#define _") != MStringA::npos)
                continue;
            size_t found0 = line.find("#define ");
            if (found0 == MStringA::npos)
                continue;
            line = line.substr(strlen("#define "));
            size_t found1 = line.find_first_of(" \t");
            size_t found2 = line.find('(');
            if (found1 == MStringA::npos)
                continue;
            if (found2 != MStringA::npos && found2 < found1)
                continue;
            macros.push_back(line.substr(0, found1));
        }

        if (macros.empty())
            return TRUE;

        WCHAR szTempFile1[MAX_PATH];
        lstrcpynW(szTempFile1, GetTempFileNameDx(L"R1"), MAX_PATH);
        ReplaceBackslash(szTempFile1);

        DWORD cbWritten;
        MFile file1(szTempFile1, TRUE);
        char buf[MAX_PATH + 64];
        WCHAR szFile[MAX_PATH];
        lstrcpyW(szFile, pszFile);
        ReplaceBackslash(szFile);
        wsprintfA(buf, "#include \"%s\"\n", MTextToAnsi(szFile).c_str());
        file1.WriteSzA(buf, &cbWritten);
        file1.WriteSzA("#pragma RisohEditor\n", &cbWritten);
        for (size_t i = 0; i < macros.size(); ++i)
        {
            wsprintfA(buf, "%s\n", macros[i].c_str());
            file1.WriteSzA(buf, &cbWritten);
        }
        file1.CloseHandle();

        WCHAR szCmdLine[512];
        wsprintfW(szCmdLine, L"\"%s\" -Wp,-E \"%s\"", m_szCppExe, szTempFile1);
        //MessageBoxW(hwnd, szCmdLine, NULL, 0);

        MProcessMaker pmaker;
        pmaker.SetShowWindow(SW_HIDE);
        MFile hInputWrite, hOutputRead;
        if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
            pmaker.CreateProcess(NULL, szCmdLine))
        {
            std::vector<char> data;
            DWORD cbAvail, cbRead;
            CHAR szBuf[256];
            while (hOutputRead.PeekNamedPipe(NULL, 0, NULL, &cbAvail))
            {
                if (cbAvail == 0)
                {
                    if (!pmaker.IsRunning())
                        break;

                    pmaker.WaitForSingleObject(500);
                    continue;
                }

                if (cbAvail > sizeof(szBuf))
                    cbAvail = sizeof(szBuf);
                else if (cbAvail == 0)
                    continue;

                if (hOutputRead.ReadFile(szBuf, cbAvail, &cbRead))
                {
                    if (cbRead == 0)
                        continue;

                    data.insert(data.end(), &szBuf[0], &szBuf[cbRead]);
                }
            }
            pmaker.CloseAll();

            MStringA str((char *)&data[0], data.size());
            size_t pragma_found = str.find("#pragma RisohEditor");
            if (pragma_found != MStringA::npos)
            {
                DeleteFileW(szTempFile1);
                str = str.substr(pragma_found);
                return ParseMacros(hwnd, pszFile, macros, str);
            }
        }

        DeleteFileW(szTempFile1);
        return FALSE;
    }

    BOOL DoLoadResH(HWND hwnd, LPCTSTR pszFile)
    {
        WCHAR szTempFile[MAX_PATH];
        lstrcpynW(szTempFile, GetTempFileNameDx(L"R1"), MAX_PATH);
        ReplaceBackslash(szTempFile);

        MFile file(szTempFile, TRUE);
        file.CloseHandle();

        WCHAR szCmdLine[512];
        wsprintfW(szCmdLine,
            L"-E -dM -DRC_INVOKED -o \"%s\" -x none \"%s\"", szTempFile, pszFile);
        //MessageBoxW(hwnd, szCmdLine, NULL, 0);

        SHELLEXECUTEINFOW info;
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_UNICODE | SEE_MASK_FLAG_NO_UI;
        info.hwnd = hwnd;
        info.lpFile = m_szCppExe;
        info.lpParameters = szCmdLine;
        info.nShow = SW_HIDE;
        if (ShellExecuteExW(&info))
        {
            WaitForSingleObject(info.hProcess, INFINITE);
            CloseHandle(info.hProcess);
            if (file.OpenFileForInput(szTempFile))
            {
                DWORD cbRead;
                CHAR szBuf[512];
                std::vector<char> data;
                while (file.ReadFile(szBuf, 512, &cbRead) && cbRead)
                {
                    data.insert(data.end(), &szBuf[0], &szBuf[cbRead]);
                }
                file.CloseHandle();
                DeleteFileW(szTempFile);
                data.push_back(0);
                return ParseResH(hwnd, pszFile, &data[0], (DWORD)(data.size() - 1));
            }
        }
        DeleteFileW(szTempFile);

        return FALSE;
    }

    void OnLoadResH(HWND hwnd)
    {
        if (!CompileIfNecessary(hwnd))
            return;

        WCHAR szFile[MAX_PATH];
        if (m_szResourceH[0])
            lstrcpynW(szFile, m_szResourceH, _countof(szFile));
        else
            lstrcpyW(szFile, L"resource.h");

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_LOADRESH);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"h";
        if (GetOpenFileNameW(&ofn))
        {
            DoLoadResH(hwnd, szFile);
        }
    }

    void OnDestroy(HWND hwnd)
    {
        SaveSettings(hwnd);

        m_hBmpView.DestroyView();
        DeleteObject(m_hNormalFont);
        DeleteObject(m_hSmallFont);
        ImageList_Destroy(m_hImageList);
        DestroyIcon(m_hFileIcon);
        DestroyIcon(m_hFolderIcon);
        PostQuitMessage(0);
    }

    void OnIdAssoc(HWND hwnd)
    {
        MIdAssocDlg dialog(m_settings.assoc_map);
        dialog.DialogBoxDx(hwnd);
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH], *pch;

        ChangeStatusText(IDS_EXECUTINGCMD);

        DragQueryFileW(hdrop, 0, File, _countof(File));
        DragFinish(hdrop);

        pch = wcsrchr(File, L'.');
        if (pch)
        {
            if (lstrcmpiW(pch, L".ico") == 0)
            {
                MAddIconDlg dialog(m_db, m_Entries);
                dialog.File = File;
                dialog.DialogBoxDx(hwnd);
                TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
                TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
                ChangeStatusText(IDS_READY);
                return;
            }
            else if (lstrcmpiW(pch, L".cur") == 0 || lstrcmpiW(pch, L".ani") == 0)
            {
                MAddCursorDlg dialog(m_db, m_Entries);
                dialog.m_File = File;
                dialog.DialogBoxDx(hwnd);
                TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
                TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
                ChangeStatusText(IDS_READY);
                return;
            }
            else if (lstrcmpiW(pch, L".wav") == 0)
            {
                MAddResDlg dialog(m_Entries, m_db);
                dialog.m_type = L"WAVE";
                dialog.m_file = File;
                if (dialog.DialogBoxDx(hwnd) == IDOK)
                {
                    TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
                    TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
                    ChangeStatusText(IDS_READY);
                }
                return;
            }
            else if (lstrcmpiW(pch, L".bmp") == 0 ||
                     lstrcmpiW(pch, L".png") == 0)
            {
                MAddBitmapDlg dialog(m_db, m_Entries);
                dialog.File = File;
                if (dialog.DialogBoxDx(hwnd) == IDOK)
                {
                    TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
                    TV_SelectEntry(m_hTreeView, m_Entries, dialog.m_entry);
                    ChangeStatusText(IDS_READY);
                }
                return;
            }
            else if (lstrcmpiW(pch, L".res") == 0)
            {
                DoLoad(hwnd, m_Entries, File);
                TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
                ChangeStatusText(IDS_READY);
                return;
            }
        }

        DoLoad(hwnd, m_Entries, File);
        TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);
        ChangeStatusText(IDS_READY);
    }

    void ShowBmpView(BOOL bShow = TRUE)
    {
        if (bShow)
        {
            ShowWindow(m_hBmpView, SW_SHOWNOACTIVATE);
            m_splitter3.SetPaneCount(2);
            m_splitter3.SetPane(0, m_hSrcEdit);
            m_splitter3.SetPane(1, m_hBmpView);
            m_splitter3.SetPaneExtent(1, m_settings.nBmpViewWidth);
        }
        else
        {
            if (m_splitter3.GetPaneCount() >= 2)
                m_settings.nBmpViewWidth = m_splitter3.GetPaneExtent(1);
            ShowWindow(m_hBmpView, SW_HIDE);
            m_splitter3.SetPaneCount(1);
            m_splitter3.SetPane(0, m_hSrcEdit);
        }
        ::SendMessageW(m_hBmpView, WM_COMMAND, 999, 0);
    }

    void ShowStatusBar(BOOL bShow = TRUE)
    {
        if (bShow)
            ShowWindow(m_hStatusBar, SW_SHOWNOACTIVATE);
        else
            ShowWindow(m_hStatusBar, SW_HIDE);
    }

    void ShowBinEdit(BOOL bShow = TRUE)
    {
        if (bShow && m_settings.bShowBinEdit)
        {
            ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);
            m_splitter2.SetPaneCount(2);
            m_splitter2.SetPane(0, m_splitter3);
            m_splitter2.SetPane(1, m_hBinEdit);
            m_splitter2.SetPaneExtent(1, m_settings.nBinEditHeight);
        }
        else
        {
            if (m_splitter2.GetPaneCount() >= 2)
                m_settings.nBinEditHeight = m_splitter2.GetPaneExtent(1);
            ShowWindow(m_hBinEdit, SW_HIDE);
            m_splitter2.SetPaneCount(1);
            m_splitter2.SetPane(0, m_splitter3);
        }
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        m_settings.nWindowLeft = rc.left;
        m_settings.nWindowTop = rc.top;
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        SendMessageW(m_hToolBar, TB_AUTOSIZE, 0, 0);
        SendMessageW(m_hStatusBar, WM_SIZE, 0, 0);

        RECT rc, ClientRect;

        GetWindowRect(hwnd, &rc);
        m_settings.nWindowWidth = rc.right - rc.left;
        m_settings.nWindowHeight = rc.bottom - rc.top;

        GetClientRect(hwnd, &ClientRect);
        SIZE sizClient = SizeFromRectDx(&ClientRect);

        INT x = 0, y = 0;
        if (::IsWindowVisible(m_hToolBar))
        {
            GetWindowRect(m_hToolBar, &rc);
            y += rc.bottom - rc.top;
            sizClient.cy -= rc.bottom - rc.top;
        }
        if (::IsWindowVisible(m_hStatusBar))
        {
            INT anWidths[] = { ClientRect.right - CX_STATUS_PART, -1 };
            SendMessage(m_hStatusBar, SB_SETPARTS, 2, (LPARAM)anWidths);
            GetWindowRect(m_hStatusBar, &rc);
            sizClient.cy -= rc.bottom - rc.top;
        }

        MoveWindow(m_splitter1, x, y, sizClient.cx, sizClient.cy, TRUE);
    }

    void OnInitMenu(HWND hwnd, HMENU hMenu)
    {
        if (IsWindowVisible(m_hStatusBar))
            CheckMenuItem(hMenu, CMDID_STATUSBAR, MF_CHECKED);
        else
            CheckMenuItem(hMenu, CMDID_STATUSBAR, MF_UNCHECKED);

        if (IsWindowVisible(m_hBinEdit))
            CheckMenuItem(hMenu, CMDID_BINARYPANE, MF_CHECKED);
        else
            CheckMenuItem(hMenu, CMDID_BINARYPANE, MF_UNCHECKED);

        if (m_settings.bAlwaysControl)
            CheckMenuItem(hMenu, CMDID_ALWAYSCONTROL, MF_CHECKED);
        else
            CheckMenuItem(hMenu, CMDID_ALWAYSCONTROL, MF_UNCHECKED);

        if ((BOOL)m_db.GetValue(L"HIDE.ID", L"HIDE.ID"))
            CheckMenuItem(hMenu, CMDID_HIDEIDMACROS, MF_CHECKED);
        else
            CheckMenuItem(hMenu, CMDID_HIDEIDMACROS, MF_UNCHECKED);

        HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
        if (hItem == NULL)
        {
            EnableMenuItem(hMenu, CMDID_REPLACEICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACECURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBIN, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTCURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBIN, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_DELETERES, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_TEST, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EDIT, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_GUIEDIT, MF_GRAYED);
            return;
        }

        TV_ITEM Item;
        ZeroMemory(&Item, sizeof(Item));
        Item.mask = TVIF_PARAM;
        Item.hItem = hItem;
        TreeView_GetItem(m_hTreeView, &Item);

        UINT i = LOWORD(Item.lParam);
        const ResEntry& Entry = m_Entries[i];

        LPARAM lParam = TV_GetParam(m_hTreeView);
        BOOL bEditable = IsEditableEntry(hwnd, lParam);
        if (bEditable)
        {
            EnableMenuItem(hMenu, CMDID_EDIT, MF_ENABLED);
            if (Res_CanGuiEdit(Entry.type))
            {
                EnableMenuItem(hMenu, CMDID_GUIEDIT, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, CMDID_GUIEDIT, MF_GRAYED);
            }
        }
        else
        {
            EnableMenuItem(hMenu, CMDID_EDIT, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_GUIEDIT, MF_GRAYED);
        }

        switch (HIWORD(Item.lParam))
        {
        case I_TYPE:
            EnableMenuItem(hMenu, CMDID_REPLACEICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACECURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBIN, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTCURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBIN, MF_ENABLED);
            EnableMenuItem(hMenu, CMDID_DELETERES, MF_ENABLED);
            EnableMenuItem(hMenu, CMDID_TEST, MF_GRAYED);
            break;
        case I_NAME:
            EnableMenuItem(hMenu, CMDID_REPLACEICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACECURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBIN, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTCURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBIN, MF_ENABLED);
            EnableMenuItem(hMenu, CMDID_DELETERES, MF_ENABLED);
            EnableMenuItem(hMenu, CMDID_TEST, MF_GRAYED);
            break;
        case I_LANG:
            if (Entry.type == RT_GROUP_ICON || Entry.type == RT_ICON ||
                Entry.type == RT_ANIICON)
            {
                EnableMenuItem(hMenu, CMDID_EXTRACTICON, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, CMDID_EXTRACTICON, MF_GRAYED);
            }
            if (Entry.type == RT_GROUP_ICON || Entry.type == RT_ANIICON)
            {
                EnableMenuItem(hMenu, CMDID_REPLACEICON, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, CMDID_REPLACEICON, MF_GRAYED);
            }

            if (Entry.type == RT_BITMAP)
            {
                EnableMenuItem(hMenu, CMDID_EXTRACTBITMAP, MF_ENABLED);
                EnableMenuItem(hMenu, CMDID_REPLACEBITMAP, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, CMDID_EXTRACTBITMAP, MF_GRAYED);
                EnableMenuItem(hMenu, CMDID_REPLACEBITMAP, MF_GRAYED);
            }

            if (Entry.type == RT_GROUP_CURSOR || Entry.type == RT_CURSOR ||
                Entry.type == RT_ANICURSOR)
            {
                EnableMenuItem(hMenu, CMDID_EXTRACTCURSOR, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, CMDID_EXTRACTCURSOR, MF_GRAYED);
            }
            if (Entry.type == RT_GROUP_CURSOR || Entry.type == RT_ANICURSOR)
            {
                EnableMenuItem(hMenu, CMDID_REPLACECURSOR, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, CMDID_REPLACECURSOR, MF_GRAYED);
            }

            if (Entry.type == RT_DIALOG || Entry.type == RT_MENU)
            {
                EnableMenuItem(hMenu, CMDID_TEST, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, CMDID_TEST, MF_GRAYED);
            }

            EnableMenuItem(hMenu, CMDID_REPLACEBIN, MF_ENABLED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBIN, MF_ENABLED);
            EnableMenuItem(hMenu, CMDID_DELETERES, MF_ENABLED);
            break;
        case I_STRING: case I_MESSAGE:
            EnableMenuItem(hMenu, CMDID_REPLACEICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACECURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBIN, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTCURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBIN, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_DELETERES, MF_ENABLED);
            EnableMenuItem(hMenu, CMDID_TEST, MF_GRAYED);
            break;
        default:
            EnableMenuItem(hMenu, CMDID_REPLACEICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACECURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_REPLACEBIN, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTICON, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTCURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_EXTRACTBIN, MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_DELETERES, MF_GRAYED);
            break;
        }
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext != m_hTreeView)
            return;

        if (IsWindowVisible(m_rad_window))
        {
            DestroyWindow(m_rad_window);
        }

        POINT pt = {(INT)xPos, (INT)yPos};
        HTREEITEM hItem;
        if (xPos == -1 && yPos == -1)
        {
            hItem = TreeView_GetSelection(hwndContext);

            RECT rc;
            TreeView_GetItemRect(hwndContext, hItem, &rc, FALSE);
            pt.x = (rc.left + rc.right) / 2;
            pt.y = (rc.top + rc.bottom) / 2;
        }
        else
        {
            ScreenToClient(hwndContext, &pt);

            TV_HITTESTINFO HitTest;
            ZeroMemory(&HitTest, sizeof(HitTest));
            HitTest.pt = pt;
            TreeView_HitTest(hwndContext, &HitTest);

            hItem = HitTest.hItem;
        }

        TreeView_SelectItem(hwndContext, hItem);

        HMENU hMenu = LoadMenuW(m_hInst, MAKEINTRESOURCEW(2));
        OnInitMenu(hwnd, hMenu);
        HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
        if (hMenu == NULL || hSubMenu == NULL)
            return;

        ClientToScreen(hwndContext, &pt);

        ::SetForegroundWindow(hwndContext);
        INT id;
        UINT Flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
        id = TrackPopupMenu(hSubMenu, Flags, pt.x, pt.y, 0,
                            hwndContext, NULL);
        ::PostMessageW(hwndContext, WM_NULL, 0, 0);
        ::DestroyMenu(hMenu);

        if (id)
        {
            SendMessageW(hwnd, WM_COMMAND, id, 0);
        }
    }

    void PreviewIcon(HWND hwnd, const ResEntry& Entry)
    {
        BITMAP bm;
        m_hBmpView = CreateBitmapFromIconOrPngDx(hwnd, Entry, bm);

        std::wstring str = DumpBitmapInfo(m_hBmpView.m_hBitmap);
        ::SetWindowTextW(m_hSrcEdit, str.c_str());

        ShowBmpView(TRUE);
    }

    void PreviewCursor(HWND hwnd, const ResEntry& Entry)
    {
        BITMAP bm;
        HCURSOR hCursor = PackedDIB_CreateIcon(&Entry[0], Entry.size(), bm, FALSE);
        m_hBmpView = CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE);
        std::wstring str = DumpCursorInfo(bm);
        DestroyCursor(hCursor);

        ::SetWindowTextW(m_hSrcEdit, str.c_str());

        ShowBmpView(TRUE);
    }

    void PreviewGroupIcon(HWND hwnd, const ResEntry& Entry)
    {
        m_hBmpView = CreateBitmapFromIconsDx(hwnd, m_Entries, Entry);

        std::wstring str = DumpGroupIconInfo(Entry.data);
        ::SetWindowTextW(m_hSrcEdit, str.c_str());

        ShowBmpView(TRUE);
    }

    void PreviewGroupCursor(HWND hwnd, const ResEntry& Entry)
    {
        m_hBmpView = CreateBitmapFromCursorsDx(hwnd, m_Entries, Entry);
        assert(m_hBmpView);

        std::wstring str = DumpGroupCursorInfo(m_Entries, Entry.data);
        assert(str.size());
        ::SetWindowTextW(m_hSrcEdit, str.c_str());

        ShowBmpView(TRUE);
    }

    void PreviewBitmap(HWND hwnd, const ResEntry& Entry)
    {
        HBITMAP hbm = PackedDIB_CreateBitmap(&Entry[0], Entry.size());
        if (hbm == NULL)
        {
            // Try a dirty hack for BI_RLE4, BI_RLE8, ...
            WCHAR szPath[MAX_PATH], szTempFile[MAX_PATH];
            GetTempPathW(_countof(szPath), szPath);
            GetTempFileNameW(szPath, L"reb", 0, szTempFile);

            if (DoExtractBitmap(szTempFile, Entry, FALSE))
            {
                hbm = (HBITMAP)LoadImageW(NULL, szTempFile, IMAGE_BITMAP, 0, 0,
                    LR_CREATEDIBSECTION | LR_LOADFROMFILE);
            }
            DeleteFileW(szTempFile);
        }
        m_hBmpView = hbm;

        std::wstring str = DumpBitmapInfo(m_hBmpView.m_hBitmap);
        ::SetWindowTextW(m_hSrcEdit, str.c_str());

        ShowBmpView(TRUE);
    }

    void PreviewPNG(HWND hwnd, const ResEntry& Entry)
    {
        HBITMAP hbm = ii_png_load_mem(&Entry[0], Entry.size());
        if (hbm)
        {
            BITMAP bm;
            GetObject(hbm, sizeof(bm), &bm);
            m_hBmpView = Create24BppBitmapDx(bm.bmWidth, bm.bmHeight);
            if (!!m_hBmpView)
            {
                ii_fill(m_hBmpView.m_hBitmap, GetStockBrush(LTGRAY_BRUSH));
                ii_draw(m_hBmpView.m_hBitmap, hbm, 0, 0);
            }
            DeleteObject(hbm);
        }

        std::wstring str = DumpBitmapInfo(m_hBmpView.m_hBitmap);
        ::SetWindowTextW(m_hSrcEdit, str.c_str());

        ShowBmpView(TRUE);
    }

    void PreviewWAVE(HWND hwnd, const ResEntry& Entry)
    {
        ::SetWindowTextW(m_hSrcEdit, LoadStringDx(IDS_WAVESOUND));

        m_hBmpView.SetPlay();
        ShowBmpView(TRUE);
    }

    void PreviewAccel(HWND hwnd, const ResEntry& Entry)
    {
        MByteStreamEx stream(Entry.data);
        AccelRes accel(m_db);
        if (accel.LoadFromStream(stream))
        {
            std::wstring str = accel.Dump(Entry.name);
            ::SetWindowTextW(m_hSrcEdit, str.c_str());
        }
    }

    void PreviewMessage(HWND hwnd, const ResEntry& Entry)
    {
        MByteStreamEx stream(Entry.data);
        MessageRes mes;
        if (mes.LoadFromStream(stream))
        {
            std::wstring str = mes.Dump();
            ::SetWindowTextW(m_hSrcEdit, str.c_str());
        }
    }

    void PreviewString(HWND hwnd, const ResEntry& Entry)
    {
        MByteStreamEx stream(Entry.data);
        StringRes str_res;
        WORD nTableID = Entry.name.m_ID;
        if (str_res.LoadFromStream(stream, nTableID))
        {
            std::wstring str = str_res.Dump(m_db, nTableID);
            ::SetWindowTextW(m_hSrcEdit, str.c_str());
        }
    }

    void PreviewHtml(HWND hwnd, const ResEntry& Entry)
    {
        MTextType type;
        type.nNewLine = MNEWLINE_CRLF;
        std::wstring str = mstr_from_bin(&Entry.data[0], Entry.data.size(), &type);
        ::SetWindowTextW(m_hSrcEdit, str.c_str());
    }

    void PreviewMenu(HWND hwnd, const ResEntry& Entry)
    {
        MByteStreamEx stream(Entry.data);
        MenuRes menu_res;
        if (menu_res.LoadFromStream(stream))
        {
            std::wstring str = menu_res.Dump(Entry.name, m_db);
            ::SetWindowTextW(m_hSrcEdit, str.c_str());
        }
    }

    void PreviewVersion(HWND hwnd, const ResEntry& Entry)
    {
        VersionRes ver_res;
        if (ver_res.LoadFromData(Entry.data))
        {
            std::wstring str = ver_res.Dump(Entry.name);
            ::SetWindowTextW(m_hSrcEdit, str.c_str());
        }
    }

    void PreviewDialog(HWND hwnd, const ResEntry& Entry)
    {
        MByteStreamEx stream(Entry.data);
        DialogRes dialog_res;
        if (dialog_res.LoadFromStream(stream))
        {
            std::wstring str = dialog_res.Dump(Entry.name, m_db, m_settings.bAlwaysControl);
            ::SetWindowTextW(m_hSrcEdit, str.c_str());
        }
    }

    void PreviewAniIcon(HWND hwnd, const ResEntry& Entry, BOOL bIcon)
    {
        HICON hIcon = NULL;

        {
            WCHAR szPath[MAX_PATH], szTempFile[MAX_PATH];
            GetTempPathW(_countof(szPath), szPath);
            GetTempFileNameW(szPath, L"ani", 0, szTempFile);

            MFile file;
            DWORD cbWritten = 0;
            if (file.OpenFileForOutput(szTempFile) &&
                file.WriteFile(&Entry[0], Entry.size(), &cbWritten))
            {
                file.CloseHandle();
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
            if (bIcon)
                ::SetWindowTextW(m_hSrcEdit, LoadStringDx(IDS_ANIICON));
            else
                ::SetWindowTextW(m_hSrcEdit, LoadStringDx(IDS_ANICURSOR));
        }
        else
        {
            m_hBmpView.DestroyView();
        }
        ShowBmpView(TRUE);
    }

    void PreviewStringTable(HWND hwnd, const ResEntry& Entry)
    {
        ResEntries found;
        Res_Search(found, m_Entries, RT_STRING, (WORD)0, Entry.lang);

        StringRes str_res;
        ResEntries::iterator it, end = found.end();
        for (it = found.begin(); it != end; ++it)
        {
            MByteStreamEx stream(it->data);
            if (!str_res.LoadFromStream(stream, it->name.m_ID))
                return;
        }

        std::wstring str = str_res.Dump(m_db);
        ::SetWindowTextW(m_hSrcEdit, str.c_str());
    }

    void PreviewMessageTable(HWND hwnd, const ResEntry& Entry)
    {
        assert(0);
    }

    BOOL Preview(HWND hwnd, const ResEntry& Entry)
    {
        HidePreview(hwnd);

        std::wstring str = DumpDataAsString(Entry.data);
        ::SetWindowTextW(m_hBinEdit, str.c_str());

        BOOL bEditable = FALSE;
        if (Entry.type == RT_ICON)
        {
            PreviewIcon(hwnd, Entry);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_CURSOR)
        {
            PreviewCursor(hwnd, Entry);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_GROUP_ICON)
        {
            PreviewGroupIcon(hwnd, Entry);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_GROUP_CURSOR)
        {
            PreviewGroupCursor(hwnd, Entry);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_BITMAP)
        {
            PreviewBitmap(hwnd, Entry);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_ACCELERATOR)
        {
            PreviewAccel(hwnd, Entry);
            bEditable = TRUE;
        }
        else if (Entry.type == RT_STRING)
        {
            PreviewString(hwnd, Entry);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_MENU)
        {
            PreviewMenu(hwnd, Entry);
            bEditable = TRUE;
        }
        else if (Entry.type == RT_DIALOG)
        {
            PreviewDialog(hwnd, Entry);
            bEditable = TRUE;
        }
        else if (Entry.type == RT_ANIICON)
        {
            PreviewAniIcon(hwnd, Entry, TRUE);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_ANICURSOR)
        {
            PreviewAniIcon(hwnd, Entry, FALSE);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_MESSAGETABLE)
        {
            PreviewMessage(hwnd, Entry);
            bEditable = FALSE;
        }
        else if (Entry.type == RT_MANIFEST || Entry.type == RT_HTML)
        {
            PreviewHtml(hwnd, Entry);
            bEditable = TRUE;
        }
        else if (Entry.type == RT_VERSION)
        {
            PreviewVersion(hwnd, Entry);
            bEditable = TRUE;
        }
        else if (Entry.type == L"PNG")
        {
            PreviewPNG(hwnd, Entry);
            bEditable = FALSE;
        }
        else if (Entry.type == L"WAVE")
        {
            PreviewWAVE(hwnd, Entry);
            bEditable = FALSE;
        }

        PostMessageW(hwnd, WM_SIZE, 0, 0);
        return bEditable;
    }

    void SelectTV(HWND hwnd, LPARAM lParam, BOOL DoubleClick)
    {
        HidePreview(hwnd);

        if (lParam == 0)
            lParam = TV_GetParam(m_hTreeView);

        WORD i = LOWORD(lParam);
        if (m_Entries.size() <= i)
            return;

        ResEntry& Entry = m_Entries[i];

        BOOL bEditable, bSelectNone = FALSE;
        if (HIWORD(lParam) == I_LANG)
        {
            bEditable = Preview(hwnd, Entry);
            ShowBinEdit(TRUE);
        }
        else if (HIWORD(lParam) == I_STRING)
        {
            ::SetWindowTextW(m_hBinEdit, NULL);
            PreviewStringTable(hwnd, Entry);
            ShowBinEdit(FALSE);
            bEditable = TRUE;
        }
        else if (HIWORD(lParam) == I_MESSAGE)
        {
            ::SetWindowTextW(m_hBinEdit, NULL);
            PreviewMessageTable(hwnd, Entry);
            ShowBinEdit(FALSE);
            bEditable = FALSE;
        }
        else
        {
            ShowBinEdit(FALSE);
            bEditable = FALSE;
            bSelectNone = TRUE;
        }

        if (bEditable)
        {
            Edit_SetReadOnly(m_hSrcEdit, FALSE);
            if (DoubleClick)
            {
                ::SetFocus(m_hSrcEdit);
            }
            else
            {
                ::SetFocus(m_hTreeView);
            }

            if (Edit_GetModify(m_hSrcEdit))
            {
                ToolBar_Update(m_hToolBar, 2);
            }
            else if (Res_IsTestable(Entry.type))
            {
                ToolBar_Update(m_hToolBar, 0);
            }
            else if (Res_CanGuiEdit(Entry.type))
            {
                ToolBar_Update(m_hToolBar, 4);
            }
            else
            {
                ToolBar_Update(m_hToolBar, 3);
            }
        }
        else
        {
            Edit_SetReadOnly(m_hSrcEdit, TRUE);
            ::SetFocus(m_hTreeView);

            ToolBar_Update(m_hToolBar, 3);
        }
        ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);

        PostMessageDx(WM_SIZE);
    }

    BOOL IsEditableEntry(HWND hwnd, LPARAM lParam)
    {
        const WORD i = LOWORD(lParam);
        if (m_Entries.size() <= i)
            return FALSE;

        const ResEntry& Entry = m_Entries[i];
        const MIdOrString& type = Entry.type;
        switch (HIWORD(lParam))
        {
        case I_LANG:
            if (type == RT_ACCELERATOR || type == RT_DIALOG || type == RT_HTML ||
                type == RT_MANIFEST || type == RT_MENU || type == RT_VERSION)
            {
                ;
            }
            else
            {
                return FALSE;
            }
            break;
        case I_STRING: case I_MESSAGE:
            break;
        default:
            return FALSE;
        }
        return TRUE;
    }

    BOOL DoWindresResult(HWND hwnd, ResEntries& entries, MStringA& msg)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        WORD i = LOWORD(lParam);
        if (m_Entries.size() <= i)
            return FALSE;

        if (HIWORD(lParam) == I_LANG)
        {
            ResEntry& entry = m_Entries[i];

            if (entries.size() != 1 ||
                entries[0].name != entry.name ||
                entries[0].lang != entry.lang)
            {
                msg += MWideToAnsi(LoadStringDx(IDS_RESMISMATCH));
                return FALSE;
            }
            entry = entries[0];
            return TRUE;
        }
        else if (HIWORD(lParam) == I_STRING)
        {
            ResEntry& entry = m_Entries[i];

            Res_DeleteNames(m_Entries, RT_STRING, entry.lang);

            for (size_t m = 0; m < entries.size(); ++m)
            {
                if (!Res_AddEntry(m_Entries, entries[m], TRUE))
                {
                    msg += MWideToAnsi(LoadStringDx(IDS_CANNOTADDRES));
                    return FALSE;
                }
            }

            return TRUE;
        }
        else if (HIWORD(lParam) == I_MESSAGE)
        {
            // FIXME
            return TRUE;
        }
        else
        {
            // FIXME
            return TRUE;
        }
    }

    BOOL DoCompileParts(HWND hwnd, const std::wstring& WideText)
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        WORD i = LOWORD(lParam);
        if (m_Entries.size() <= i)
            return FALSE;

        ResEntry& entry = m_Entries[i];

        MStringA TextUtf8;
        TextUtf8 = MWideToUtf8(WideText);
        if (HIWORD(lParam) == I_LANG)
        {
            if (Res_IsPlainText(entry.type))
            {
                if (WideText.find(L"\"UTF-8\"") != std::wstring::npos)
                {
                    entry.data.assign(TextUtf8.begin(), TextUtf8.end());

                    static const BYTE bom[] = {0xEF, 0xBB, 0xBF, 0};
                    entry.data.insert(entry.data.begin(), &bom[0], &bom[3]);
                }
                else
                {
                    MStringA TextAnsi;
                    TextAnsi = MWideToAnsi(WideText);
                    entry.data.assign(TextAnsi.begin(), TextAnsi.end());
                }
                SelectTV(hwnd, lParam, FALSE);

                return TRUE;    // success
            }
        }

        WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

        lstrcpynW(szPath1, GetTempFileNameDx(L"R1"), MAX_PATH);
        ReplaceBackslash(szPath1);
        MFile r1(szPath1, TRUE);

        lstrcpynW(szPath2, GetTempFileNameDx(L"R2"), MAX_PATH);
        ReplaceBackslash(szPath2);
        MFile r2(szPath2, TRUE);

        lstrcpynW(szPath3, GetTempFileNameDx(L"R3"), MAX_PATH);
        ReplaceBackslash(szPath3);
        MFile r3(szPath3, TRUE);
        r3.CloseHandle();

        r1.WriteFormatA("#include <windows.h>\r\n");
        r1.WriteFormatA("#include <commctrl.h>\r\n");
        r1.WriteFormatA("#include <prsht.h>\r\n");
        r1.WriteFormatA("#include <dlgs.h>\r\n");
        if (m_szResourceH[0])
            r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(m_szResourceH).c_str());
        r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n",
                        PRIMARYLANGID(entry.lang), SUBLANGID(entry.lang));
        r1.WriteFormatA("#pragma code_page(65001)\r\n");
        r1.WriteFormatA("#include \"%S\"\r\n", szPath2);
        r1.CloseHandle();

        DWORD cbWritten;
        r2.WriteFile(TextUtf8.c_str(), TextUtf8.size() * sizeof(char), &cbWritten);
        r2.CloseHandle();

        WCHAR szCmdLine[512];
#if 1
        wsprintfW(szCmdLine,
            L"\"%s\" -DRC_INVOKED -o \"%s\" -J rc -O res "
            L"-F pe-i386 --preprocessor=\"%s\" --preprocessor-arg=\"\" \"%s\"",
            m_szWindresExe, szPath3, m_szCppExe, szPath1);
#else
        wsprintfW(szCmdLine,
            L"\"%s\" -DRC_INVOKED -o \"%s\" -J rc -O res "
            L"-F pe-i386 --preprocessor=\"%s\" --preprocessor-arg=\"-v\" \"%s\"",
            m_szWindresExe, szPath3, m_szCppExe, szPath1);
#endif
        // MessageBoxW(hwnd, szCmdLine, NULL, 0);

        std::vector<BYTE> output;
        MStringA msg;
        msg = MWideToAnsi(LoadStringDx(IDS_CANNOTSTARTUP));
        output.assign((LPBYTE)msg.c_str(), (LPBYTE)msg.c_str() + msg.size());

        BOOL Success = FALSE;
        MByteStreamEx stream;

        MProcessMaker pmaker;
        pmaker.SetShowWindow(SW_HIDE);
        MFile hInputWrite, hOutputRead;
        if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
            pmaker.CreateProcess(NULL, szCmdLine))
        {
            DWORD cbAvail;
            while (hOutputRead.PeekNamedPipe(NULL, 0, NULL, &cbAvail))
            {
                if (cbAvail == 0)
                {
                    if (!pmaker.IsRunning())
                        break;

                    pmaker.WaitForSingleObject(500);
                    continue;
                }

                CHAR szBuf[256];
                DWORD cbRead;
                if (cbAvail > sizeof(szBuf))
                    cbAvail = sizeof(szBuf);
                else if (cbAvail == 0)
                    continue;

                if (hOutputRead.ReadFile(szBuf, cbAvail, &cbRead))
                {
                    if (cbRead == 0)
                        continue;

                    stream.WriteData(szBuf, cbRead);
                }
            }

            output = stream.data();

            if (pmaker.GetExitCode() == 0)
            {
                ResEntries entries;
                if (DoImport(hwnd, szPath3, entries))
                {
                    MStringA msg;
                    Success = DoWindresResult(hwnd, entries, msg);
                    if (msg.size())
                    {
                        output.insert(output.end(), msg.begin(), msg.end());
                    }
                }
            }
        }

        ::DeleteFileW(szPath1);
        ::DeleteFileW(szPath2);
        ::DeleteFileW(szPath3);

        if (!Success)
        {
            if (output.empty())
            {
                ::SetWindowTextW(m_hBinEdit, LoadStringDx(IDS_COMPILEERROR));
                ::ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);
            }
            else
            {
                output.insert(output.end(), 0);
                ::SetWindowTextA(m_hBinEdit, (char *)&output[0]);
                ::ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);
            }
        }

        PostMessageW(hwnd, WM_SIZE, 0, 0);

        return Success;
    }

    BOOL CompileIfNecessary(HWND hwnd)
    {
        if (Edit_GetModify(m_hSrcEdit))
        {
            INT id = MsgBoxDx(IDS_COMPILENOW, MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                {
                    INT cchText = ::GetWindowTextLengthW(m_hSrcEdit);
                    std::wstring WideText;
                    WideText.resize(cchText);
                    ::GetWindowTextW(m_hSrcEdit, &WideText[0], cchText + 1);

                    if (!DoCompileParts(hwnd, WideText))
                    {
                        return FALSE;
                    }

                    LPARAM lParam = TV_GetParam(m_hTreeView);
                    WORD i = LOWORD(lParam);
                    if (m_Entries.size() <= i)
                        return FALSE;
                    ResEntry& entry = m_Entries[i];

                    if (HIWORD(lParam) == I_LANG && IsWindow(m_rad_window) &&
                        entry.type == RT_DIALOG)
                    {
                        DestroyWindow(m_rad_window);
                        m_rad_window.Detach();

                        MByteStreamEx stream(entry.data);
                        m_rad_window.m_dialog_res.LoadFromStream(stream);

                        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
                        if (m_rad_window.CreateWindowDx(m_hwnd, MAKEINTRESOURCE(IDS_RADWINDOW),
                                                        style))
                        {
                            CenterWindowDx(m_rad_window);
                            ShowWindow(m_rad_window, SW_SHOWNORMAL);
                            UpdateWindow(m_rad_window);
                        }
                    }

                    Edit_SetModify(m_hSrcEdit, FALSE);
                }
                break;
            case IDNO:
                break;
            case IDCANCEL:
                return FALSE;
            }
        }
        return TRUE;
    }

    BOOL CheckDataFolder(VOID)
    {
        WCHAR szPath[MAX_PATH], *pch;
        GetModuleFileNameW(NULL, szPath, _countof(szPath));
        pch = wcsrchr(szPath, L'\\');
        lstrcpyW(pch, L"\\data");
        if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            lstrcpyW(pch, L"\\..\\data");
            if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                lstrcpyW(pch, L"\\..\\..\\data");
                if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    lstrcpyW(pch, L"\\..\\..\\..\\data");
                    if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                    {
                        lstrcpyW(pch, L"\\..\\..\\..\\..\\data");
                        if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                        {
                            return FALSE;
                        }
                    }
                }
            }
        }
        lstrcpynW(m_szDataFolder, szPath, MAX_PATH);
        return TRUE;
    }

    INT CheckData(VOID)
    {
        if (!CheckDataFolder())
        {
            ErrorBoxDx(TEXT("ERROR: data folder was not found!"));
            return -1;  // failure
        }

        // Constants.txt
        lstrcpyW(m_szConstantsFile, m_szDataFolder);
        lstrcatW(m_szConstantsFile, L"\\Constants.txt");
        if (!m_db.LoadFromFile(m_szConstantsFile))
        {
            ErrorBoxDx(TEXT("ERROR: Unable to load Constants.txt file."));
            return -2;  // failure
        }
        ReplaceBackslash(m_szConstantsFile);

        // cpp.exe
        lstrcpyW(m_szCppExe, m_szDataFolder);
        lstrcatW(m_szCppExe, L"\\bin\\cpp.exe");
        if (::GetFileAttributesW(m_szCppExe) == INVALID_FILE_ATTRIBUTES)
        {
            ErrorBoxDx(TEXT("ERROR: No cpp.exe found."));
            return -3;  // failure
        }
        ReplaceBackslash(m_szCppExe);

        // windres.exe
        lstrcpyW(m_szWindresExe, m_szDataFolder);
        lstrcatW(m_szWindresExe, L"\\bin\\windres.exe");
        if (::GetFileAttributesW(m_szWindresExe) == INVALID_FILE_ATTRIBUTES)
        {
            ErrorBoxDx(TEXT("ERROR: No windres.exe found."));
            return -4;  // failure
        }
        ReplaceBackslash(m_szWindresExe);

        return 0;   // success
    }

    void LoadLangInfo(VOID)
    {
        EnumSystemLocalesW(EnumLocalesProc, LCID_SUPPORTED);
        {
            LangEntry entry;
            entry.LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
            entry.Str = LoadStringDx(IDS_NEUTRAL);
            g_Langs.push_back(entry);
        }
        std::sort(g_Langs.begin(), g_Langs.end());
    }

    BOOL DoLoad(HWND hwnd, ResEntries& Entries, LPCWSTR FileName)
    {
        WCHAR Path[MAX_PATH], ResolvedPath[MAX_PATH], *pchPart;

        if (GetPathOfShortcutDx(hwnd, FileName, ResolvedPath))
        {
            GetFullPathNameW(ResolvedPath, _countof(Path), Path, &pchPart);
        }
        else
        {
            GetFullPathNameW(FileName, _countof(Path), Path, &pchPart);
        }

        LPWSTR pch = wcsrchr(Path, L'.');
        if (pch && lstrcmpiW(pch, L".res") == 0)
        {
            // .res files
            ResEntries entries;
            if (!DoImport(hwnd, Path, entries))
                return FALSE;

            Entries = entries;
            TV_RefreshInfo(m_hTreeView, Entries);
            DoSetFile(hwnd, Path);
            return TRUE;
        }

        // executable files
        HMODULE hMod = LoadLibraryExW(Path, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hMod == NULL)
        {
            MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTOPEN), NULL, MB_ICONERROR);
            return FALSE;
        }

        Entries.clear();
        Res_GetListFromRes(hMod, (LPARAM)&Entries);
        FreeLibrary(hMod);

        TV_RefreshInfo(m_hTreeView, Entries);
        DoSetFile(hwnd, Path);

        m_szResourceH[0] = 0;
        m_settings.added_ids.clear();
        m_settings.removed_ids.clear();
        if (m_settings.bAutoLoadNearbyResH)
            CheckResourceH(hwnd, Path);

        return TRUE;
    }

    BOOL CheckResourceH(HWND hwnd, LPCTSTR Path)
    {
        m_szResourceH[0] = 0;
        m_settings.added_ids.clear();
        m_settings.removed_ids.clear();

        TCHAR szPath[MAX_PATH];
        lstrcpyn(szPath, Path, _countof(szPath));
        ReplaceBackslash(szPath);

        TCHAR *pch = _tcsrchr(szPath, TEXT('/'));
        if (pch == NULL)
            return FALSE;

        ++pch;
        lstrcpy(pch, TEXT("resource.h"));
        if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            lstrcpy(pch, TEXT("../resource.h"));
            if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                lstrcpy(pch, TEXT("../../resource.h"));
                if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    lstrcpy(pch, TEXT("../../../resource.h"));
                    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                    {
                        lstrcpy(pch, TEXT("../src/resource.h"));
                        if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                        {
                            lstrcpy(pch, TEXT("../../src/resource.h"));
                            if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                            {
                                lstrcpy(pch, TEXT("../../../src/resource.h"));
                                if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                                {
                                    return FALSE;
                                }
                            }
                        }
                    }
                }
            }
        }

        return DoLoadResH(hwnd, szPath);
    }

    BOOL DoExtractBin(LPCWSTR FileName, const ResEntry& Entry)
    {
        MByteStreamEx bs(Entry.data);
        return bs.SaveToFile(FileName);
    }

    BOOL DoImport(HWND hwnd, LPCWSTR ResFile, ResEntries& entries)
    {
        MByteStreamEx stream;
        if (!stream.LoadFromFile(ResFile))
            return FALSE;

        ResourceHeader header;
        while (header.ReadFrom(stream))
        {
            if (header.DataSize == 0)
            {
                stream.ReadDwordAlignment();
                continue;
            }

            ResEntry entry;
            entry.data.resize(header.DataSize);
            if (!stream.ReadData(&entry.data[0], header.DataSize))
            {
                break;
            }

            entry.lang = header.LanguageId;
            entry.updated = TRUE;
            entry.type = header.Type;
            entry.name = header.Name;
            entries.push_back(entry);

            stream.ReadDwordAlignment();
        }
        return TRUE;
    }

    BOOL DoExtractRes(HWND hwnd, LPCWSTR FileName, const ResEntries& Entries)
    {
        MByteStreamEx bs;
        ResourceHeader header;
        if (!header.WriteTo(bs))
            return FALSE;

        ResEntries::const_iterator it, end = Entries.end();
        for (it = Entries.begin(); it != end; ++it)
        {
            const ResEntry& Entry = *it;

            header.DataSize = Entry.size();
            header.HeaderSize = header.GetHeaderSize(Entry.type, Entry.name);
            header.Type = Entry.type;
            header.Name = Entry.name;
            header.DataVersion = 0;
            header.MemoryFlags = MEMORYFLAG_DISCARDABLE | MEMORYFLAG_PURE |
                                 MEMORYFLAG_MOVEABLE;
            header.LanguageId = Entry.lang;
            header.Version = 0;
            header.Characteristics = 0;

            if (!header.WriteTo(bs))
                return FALSE;

            if (!bs.WriteData(&Entry[0], Entry.size()))
                return FALSE;

            bs.WriteDwordAlignment();
        }

        return bs.SaveToFile(FileName);
    }

    BOOL DoSaveResAs(HWND hwnd, LPCWSTR ExeFile)
    {
        if (!CompileIfNecessary(hwnd))
            return FALSE;

        if (DoExtractRes(hwnd, ExeFile, m_Entries))
        {
            Res_Optimize(m_Entries);
            DoSetFile(hwnd, ExeFile);
            return TRUE;
        }
        return FALSE;
    }

    BOOL DoSaveAs(HWND hwnd, LPCWSTR ExeFile)
    {
        if (!CompileIfNecessary(hwnd))
            return TRUE;

        DWORD dwBinType;
        LPCWSTR pch = wcsrchr(ExeFile, L'.');
        if ((pch && lstrcmpiW(pch, L".res") == 0) ||
            !GetBinaryType(m_szFile, &dwBinType))
        {
            return DoSaveResAs(hwnd, ExeFile);
        }

        return DoSaveExeAs(hwnd, ExeFile);
    }

    BOOL DoSaveExeAs(HWND hwnd, LPCWSTR ExeFile)
    {
        LPWSTR TempFile = GetTempFileNameDx(L"ERE");

        BOOL b1 = ::CopyFileW(m_szFile, TempFile, FALSE);
        BOOL b2 = b1 && Res_UpdateExe(hwnd, TempFile, m_Entries);
        BOOL b3 = b2 && ::CopyFileW(TempFile, ExeFile, FALSE);
        if (b3)
        {
            DeleteFileW(TempFile);
            Res_Optimize(m_Entries);
            DoSetFile(hwnd, ExeFile);

            return TRUE;
        }

        DeleteFileW(TempFile);
        return FALSE;
    }

    BOOL DoExtractIcon(LPCWSTR FileName, const ResEntry& Entry)
    {
        if (Entry.type == RT_GROUP_ICON)
        {
            return Res_ExtractGroupIcon(m_Entries, Entry, FileName);
        }
        else if (Entry.type == RT_ICON)
        {
            return Res_ExtractIcon(m_Entries, Entry, FileName);
        }
        else if (Entry.type == RT_ANIICON)
        {
            MFile file;
            DWORD cbWritten = 0;
            if (file.OpenFileForOutput(FileName) &&
                file.WriteFile(&Entry[0], Entry.size(), &cbWritten))
            {
                file.CloseHandle();
                return TRUE;
            }
        }
        return FALSE;
    }

    BOOL DoExtractCursor(LPCWSTR FileName, const ResEntry& Entry)
    {
        if (Entry.type == RT_GROUP_CURSOR)
        {
            return Res_ExtractGroupCursor(m_Entries, Entry, FileName);
        }
        else if (Entry.type == RT_CURSOR)
        {
            return Res_ExtractCursor(m_Entries, Entry, FileName);
        }
        else if (Entry.type == RT_ANICURSOR)
        {
            MFile file;
            DWORD cbWritten = 0;
            if (file.OpenFileForOutput(FileName) &&
                file.WriteFile(&Entry[0], Entry.size(), &cbWritten))
            {
                file.CloseHandle();
                return TRUE;
            }
        }
        return FALSE;
    }

    BOOL DoExtractBitmap(LPCWSTR FileName, const ResEntry& Entry, BOOL WritePNG)
    {
        BITMAPFILEHEADER FileHeader;

        if (WritePNG)
        {
            HBITMAP hbm = PackedDIB_CreateBitmap(&Entry[0], Entry.size());
            BOOL ret = !!ii_png_save_w(FileName, hbm, 0);
            DeleteObject(hbm);
            return ret;
        }

        FileHeader.bfType = 0x4d42;
        FileHeader.bfSize = (DWORD)(sizeof(FileHeader) + Entry.size());
        FileHeader.bfReserved1 = 0;
        FileHeader.bfReserved2 = 0;

        DWORD Offset = PackedDIB_GetBitsOffset(&Entry[0], Entry.size());
        if (Offset == 0)
            return FALSE;

        FileHeader.bfOffBits = sizeof(FileHeader) + Offset;

        MByteStreamEx bs;
        if (!bs.WriteRaw(FileHeader) || !bs.WriteData(&Entry[0], Entry.size()))
            return FALSE;

        return bs.SaveToFile(FileName);
    }
};

void MMainWnd::SetDefaultSettings(HWND hwnd)
{
    m_settings.bShowBinEdit = TRUE;
    m_settings.bAlwaysControl = FALSE;
    m_settings.bShowStatusBar = TRUE;
    m_settings.nTreeViewWidth = TV_WIDTH;
    m_settings.nBmpViewWidth = BV_WIDTH;
    m_settings.nBinEditHeight = BE_HEIGHT;
    m_settings.bGuiByDblClick = TRUE;
    m_settings.bResumeWindowPos = TRUE;
    m_settings.bAutoLoadNearbyResH = TRUE;
    m_settings.bAutoShowIDList = TRUE;
    m_settings.nComboHeight = 300;
    m_settings.vecRecentlyUsed.clear();
    m_settings.nWindowLeft = CW_USEDEFAULT;
    m_settings.nWindowTop = CW_USEDEFAULT;
    m_settings.nWindowWidth = 760;
    m_settings.nWindowHeight = 480;
    m_settings.nIDListLeft = CW_USEDEFAULT;
    m_settings.nIDListTop = CW_USEDEFAULT;
    m_settings.nIDListWidth = 366;
    m_settings.nIDListHeight = 490;
    m_settings.nRadLeft = CW_USEDEFAULT;
    m_settings.nRadTop = CW_USEDEFAULT;

    ConstantsDB::TableType table1, table2;
    table1 = m_db.GetTable(L"RESOURCE.ID.TYPE");
    table2 = m_db.GetTable(L"RESOURCE.ID.PREFIX");
    assert(table1.size() == table2.size());

    m_settings.assoc_map.clear();
    if (table1.size() && table1.size() == table2.size())
    {
        for (size_t i = 0; i < table1.size(); ++i)
        {
            m_settings.assoc_map.insert(std::make_pair(table1[i].name, table2[i].name));
        }
    }
    else
    {
        m_settings.assoc_map[L"Cursor.ID"] = L"IDC_";
        m_settings.assoc_map[L"Bitmap.ID"] = L"IDB_";
        m_settings.assoc_map[L"Menu.ID"] = L"IDM_";
        m_settings.assoc_map[L"Dialog.ID"] = L"IDD_";
        m_settings.assoc_map[L"String.ID"] = L"IDS_";
        m_settings.assoc_map[L"Accel.ID"] = L"IDA_";
        m_settings.assoc_map[L"Icon.ID"] = L"IDI_";
        m_settings.assoc_map[L"AniCursor.ID"] = L"IDAC_";
        m_settings.assoc_map[L"AniIcon.ID"] = L"IDAI_";
        m_settings.assoc_map[L"Html.ID"] = L"IDH_";
        m_settings.assoc_map[L"Help.ID"] = L"HELPID_";
        m_settings.assoc_map[L"Command.ID"] = L"CMDID_";
        m_settings.assoc_map[L"Control.ID"] = L"CID_";
        m_settings.assoc_map[L"Resource.ID"] = L"IDR_";
    }

    m_settings.id_map.clear();
    m_settings.added_ids.clear();
    m_settings.removed_ids.clear();
}

BOOL MMainWnd::LoadSettings(HWND hwnd)
{
    SetDefaultSettings(hwnd);

    MRegKey key(HKCU, TEXT("Software"));
    if (!key)
        return FALSE;

    MRegKey keySoftware(key, TEXT("Katayama Hirofumi MZ"));
    if (!keySoftware)
        return FALSE;

    MRegKey keyRisoh(keySoftware, TEXT("RisohEditor"));
    if (!keyRisoh)
        return FALSE;

    BOOL bHideID = (BOOL)m_db.GetValue(L"HIDE.ID", L"HIDE.ID");
    keyRisoh.QueryDword(TEXT("HIDE.ID"), (DWORD&)bHideID);
    {
        ConstantsDB::TableType& table = m_db.m_map[L"HIDE.ID"];
        table.clear();
        ConstantsDB::EntryType entry(L"HIDE.ID", bHideID);
        table.push_back(entry);

    }
    m_settings.bHideID = bHideID;

    keyRisoh.QueryDword(TEXT("ShowStatusBar"), (DWORD&)m_settings.bShowStatusBar);
    keyRisoh.QueryDword(TEXT("ShowBinEdit"), (DWORD&)m_settings.bShowBinEdit);
    keyRisoh.QueryDword(TEXT("AlwaysControl"), (DWORD&)m_settings.bAlwaysControl);
    keyRisoh.QueryDword(TEXT("TreeViewWidth"), (DWORD&)m_settings.nTreeViewWidth);
    keyRisoh.QueryDword(TEXT("BmpViewWidth"), (DWORD&)m_settings.nBmpViewWidth);
    keyRisoh.QueryDword(TEXT("BinEditHeight"), (DWORD&)m_settings.nBinEditHeight);
    keyRisoh.QueryDword(TEXT("bGuiByDblClick"), (DWORD&)m_settings.bGuiByDblClick);
    keyRisoh.QueryDword(TEXT("bResumeWindowPos"), (DWORD&)m_settings.bResumeWindowPos);
    keyRisoh.QueryDword(TEXT("bAutoLoadNearbyResH"), (DWORD&)m_settings.bAutoLoadNearbyResH);
    keyRisoh.QueryDword(TEXT("bAutoShowIDList"), (DWORD&)m_settings.bAutoShowIDList);
    keyRisoh.QueryDword(TEXT("nComboHeight"), (DWORD&)m_settings.nComboHeight);
    keyRisoh.QueryDword(TEXT("nWindowLeft"), (DWORD&)m_settings.nWindowLeft);
    keyRisoh.QueryDword(TEXT("nWindowTop"), (DWORD&)m_settings.nWindowTop);
    keyRisoh.QueryDword(TEXT("nWindowWidth"), (DWORD&)m_settings.nWindowWidth);
    keyRisoh.QueryDword(TEXT("nWindowHeight"), (DWORD&)m_settings.nWindowHeight);
    keyRisoh.QueryDword(TEXT("nIDListLeft"), (DWORD&)m_settings.nIDListLeft);
    keyRisoh.QueryDword(TEXT("nIDListTop"), (DWORD&)m_settings.nIDListTop);
    keyRisoh.QueryDword(TEXT("nIDListWidth"), (DWORD&)m_settings.nIDListWidth);
    keyRisoh.QueryDword(TEXT("nIDListHeight"), (DWORD&)m_settings.nIDListHeight);
    keyRisoh.QueryDword(TEXT("nRadLeft"), (DWORD&)m_settings.nRadLeft);
    keyRisoh.QueryDword(TEXT("nRadTop"), (DWORD&)m_settings.nRadTop);

    DWORD i, dwCount;
    keyRisoh.QueryDword(TEXT("FileCount"), dwCount);
    if (dwCount > MAX_MRU)
        dwCount = MAX_MRU;

    TCHAR szFormat[32], szFile[MAX_PATH];
    for (i = 0; i < dwCount; ++i)
    {
        wsprintf(szFormat, TEXT("File%lu"), i);
        keyRisoh.QuerySz(szFormat, szFile, _countof(szFile));
        m_settings.vecRecentlyUsed.push_back(szFile);
    }

    TCHAR szName[MAX_PATH];
    assoc_map_type::iterator it, end = m_settings.assoc_map.end();
    for (it = m_settings.assoc_map.begin(); it != end; ++it)
    {
        keyRisoh.QuerySz(it->first.c_str(), szName, _countof(szName));
        if (szName[0])
            it->second = szName;
    }

    return TRUE;
}

BOOL MMainWnd::SaveSettings(HWND hwnd)
{
    MRegKey key(HKCU, TEXT("Software"), TRUE);
    if (!key)
        return FALSE;

    MRegKey keySoftware(key, TEXT("Katayama Hirofumi MZ"), TRUE);
    if (!keySoftware)
        return FALSE;

    MRegKey keyRisoh(keySoftware, TEXT("RisohEditor"), TRUE);
    if (!keyRisoh)
        return FALSE;

    if (m_splitter3.GetPaneCount() >= 2)
        m_settings.nBmpViewWidth = m_splitter3.GetPaneExtent(1);
    if (m_splitter2.GetPaneCount() >= 2)
        m_settings.nBinEditHeight = m_splitter2.GetPaneExtent(1);
    if (m_splitter1.GetPaneCount() >= 1)
        m_settings.nTreeViewWidth = m_splitter1.GetPaneExtent(0);

    BOOL bHideID = (BOOL)m_db.GetValue(L"HIDE.ID", L"HIDE.ID");
    m_settings.bHideID = bHideID;
    keyRisoh.SetDword(TEXT("HIDE.ID"), m_settings.bHideID);
    keyRisoh.SetDword(TEXT("ShowStatusBar"), m_settings.bShowStatusBar);
    keyRisoh.SetDword(TEXT("ShowBinEdit"), m_settings.bShowBinEdit);
    keyRisoh.SetDword(TEXT("AlwaysControl"), m_settings.bAlwaysControl);
    keyRisoh.SetDword(TEXT("TreeViewWidth"), m_settings.nTreeViewWidth);
    keyRisoh.SetDword(TEXT("BmpViewWidth"), m_settings.nBmpViewWidth);
    keyRisoh.SetDword(TEXT("BinEditHeight"), m_settings.nBinEditHeight);
    keyRisoh.SetDword(TEXT("bGuiByDblClick"), m_settings.bGuiByDblClick);
    keyRisoh.SetDword(TEXT("bResumeWindowPos"), m_settings.bResumeWindowPos);
    keyRisoh.SetDword(TEXT("bAutoLoadNearbyResH"), m_settings.bAutoLoadNearbyResH);
    keyRisoh.SetDword(TEXT("bAutoShowIDList"), m_settings.bAutoShowIDList);
    keyRisoh.SetDword(TEXT("nComboHeight"), m_settings.nComboHeight);
    keyRisoh.SetDword(TEXT("nWindowLeft"), m_settings.nWindowLeft);
    keyRisoh.SetDword(TEXT("nWindowTop"), m_settings.nWindowTop);
    keyRisoh.SetDword(TEXT("nWindowWidth"), m_settings.nWindowWidth);
    keyRisoh.SetDword(TEXT("nWindowHeight"), m_settings.nWindowHeight);
    keyRisoh.SetDword(TEXT("nIDListLeft"), m_settings.nIDListLeft);
    keyRisoh.SetDword(TEXT("nIDListTop"), m_settings.nIDListTop);
    keyRisoh.SetDword(TEXT("nIDListWidth"), m_settings.nIDListWidth);
    keyRisoh.SetDword(TEXT("nIDListHeight"), m_settings.nIDListHeight);
    keyRisoh.SetDword(TEXT("nRadLeft"), m_settings.nRadLeft);
    keyRisoh.SetDword(TEXT("nRadTop"), m_settings.nRadTop);

    DWORD i, dwCount = (DWORD)m_settings.vecRecentlyUsed.size();
    if (dwCount > MAX_MRU)
        dwCount = MAX_MRU;
    keyRisoh.SetDword(TEXT("FileCount"), dwCount);

    TCHAR szFormat[32];
    for (i = 0; i < dwCount; ++i)
    {
        wsprintf(szFormat, TEXT("File%lu"), i);
        keyRisoh.SetSz(szFormat, m_settings.vecRecentlyUsed[i].c_str());
    }

    assoc_map_type::iterator it, end = m_settings.assoc_map.end();
    for (it = m_settings.assoc_map.begin(); it != end; ++it)
    {
        keyRisoh.SetSz(it->first.c_str(), it->second.c_str());
    }

    return TRUE;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    int ret;

    INT argc = 0;
    LPWSTR *targv = CommandLineToArgvW(GetCommandLineW(), &argc);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    InitCommonControls();
    MEditCtrl::SetCtrlAHookDx(TRUE);
    HINSTANCE hinstRichEdit = LoadLibrary(TEXT("RICHED32.DLL"));
    {
        MMainWnd app(argc, targv, hInstance);

        if (app.StartDx(nCmdShow))
        {
            ret = INT(app.RunDx());
        }
        else
        {
            ret = 2;
        }
    }
    FreeLibrary(hinstRichEdit);
    MEditCtrl::SetCtrlAHookDx(FALSE);
    CoUninitialize();

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
