// RisouEditor.cpp --- RisouEditor
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.hpp"

#pragma comment(lib, "msimg32.lib")

//////////////////////////////////////////////////////////////////////////////
// constants

#ifndef INVALID_FILE_ATTRIBUTES
    #define INVALID_FILE_ATTRIBUTES     ((DWORD)-1)
#endif

#define TV_WIDTH 250
#define SE_WIDTH 256
#define BE_HEIGHT 100

#ifndef RT_MANIFEST
    #define RT_MANIFEST 24
#endif

//////////////////////////////////////////////////////////////////////////////
// global variables

HINSTANCE   g_hInstance = NULL;

ConstantsDB g_ConstantsDB;

HWND        g_hTreeView = NULL;

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

void InitClassComboBox(HWND hCmb)
{
    ComboBox_ResetContent(hCmb);

    ConstantsDB::TableType table;
    table = g_ConstantsDB.GetTable(TEXT("CONTROL.CLASSES"));

    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        ComboBox_AddString(hCmb, it->name.c_str());
    }
}

void InitCtrlIDComboBox(HWND hCmb)
{
    ConstantsDB::TableType table;
    table = g_ConstantsDB.GetTable(TEXT("CTRLID"));

    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        ComboBox_AddString(hCmb, it->name.c_str());
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
            *pch = UNICODE_NULL;
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

    pszPath[0] = UNICODE_NULL;
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
                if (SUCCEEDED(hRes) && UNICODE_NULL != pszPath[0])
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

std::wstring str_vkey(WORD w)
{
    return g_ConstantsDB.GetName(L"VIRTUALKEYS", w);
}

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

BOOL DoAddCursor(HWND hwnd,
                 ResEntries& Entries,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& CurFile)
{
    if (!Res_AddGroupCursor(Entries, Name, Lang, CurFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, Entries, FALSE);
    return TRUE;
}

BOOL DoReplaceCursor(HWND hwnd,
                     ResEntries& Entries,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& CurFile)
{
    if (!Res_AddGroupCursor(Entries, Name, Lang, CurFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, Entries, FALSE);
    return TRUE;
}

BOOL DoAddIcon(HWND hwnd,
               ResEntries& Entries,
               const ID_OR_STRING& Name,
               WORD Lang,
               const std::wstring& IconFile)
{
    if (!Res_AddGroupIcon(Entries, Name, Lang, IconFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, Entries, FALSE);
    return TRUE;
}

BOOL DoReplaceIcon(HWND hwnd,
                   ResEntries& Entries,
                   const ID_OR_STRING& Name,
                   WORD Lang,
                   const std::wstring& IconFile)
{
    if (!Res_AddGroupIcon(Entries, Name, Lang, IconFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, Entries, FALSE);
    return TRUE;
}

BOOL DoAddBin(HWND hwnd,
              ResEntries& Entries,
              const ID_OR_STRING& Type,
              const ID_OR_STRING& Name,
              WORD Lang,
              const std::wstring& File)
{
    MByteStreamEx bs;
    if (!bs.LoadFromFile(File.c_str()))
        return FALSE;

    Res_AddEntry(Entries, Type, Name, Lang, bs.data(), FALSE);
    TV_RefreshInfo(g_hTreeView, Entries, FALSE);
    return TRUE;
}

BOOL DoReplaceBin(HWND hwnd,
                  ResEntries& Entries,
                  const ID_OR_STRING& Type,
                  const ID_OR_STRING& Name,
                  WORD Lang,
                  const std::wstring& File)
{
    MByteStreamEx bs;
    if (!bs.LoadFromFile(File.c_str()))
        return FALSE;

    Res_AddEntry(Entries, Type, Name, Lang, bs.data(), TRUE);
    TV_RefreshInfo(g_hTreeView, Entries, FALSE);
    return TRUE;
}

BOOL DoAddBitmap(HWND hwnd,
                 ResEntries& Entries,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& BitmapFile)
{
    if (!Res_AddBitmap(Entries, Name, Lang, BitmapFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, Entries, FALSE);
    return TRUE;
}

BOOL DoReplaceBitmap(HWND hwnd,
                     ResEntries& Entries,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& BitmapFile)
{
    if (!Res_AddBitmap(Entries, Name, Lang, BitmapFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, Entries, FALSE);
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// specialized tool bar

TBBUTTON g_buttons0[] =
{
    { -1, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_GUIEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEST },
};

TBBUTTON g_buttons1[] =
{
    { -1, ID_TEXTEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEXTEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEST },
};

TBBUTTON g_buttons2[] =
{
    { -1, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_COMPILE },
    { -1, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_CANCELEDIT },
};

TBBUTTON g_buttons3[] =
{
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
};

TBBUTTON g_buttons4[] =
{
    { -1, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_GUIEDIT },
};

TBBUTTON g_buttons5[] =
{
    { -1, ID_TEXTEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEXTEDIT },
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
        0, 0, 0, 0, hwndParent, (HMENU)1, g_hInstance, NULL);
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

BOOL CheckTypeComboBox(HWND hCmb1, ID_OR_STRING& Type)
{
    WCHAR szType[MAX_PATH];
    GetWindowTextW(hCmb1, szType, _countof(szType));
    MStringW Str = szType;
    mstr_trim(Str);
    lstrcpynW(szType, Str.c_str(), _countof(szType));

    if (szType[0] == UNICODE_NULL)
    {
        ComboBox_SetEditSel(hCmb1, 0, -1);
        SetFocus(hCmb1);
        MessageBoxW(GetParent(hCmb1), LoadStringDx(IDS_ENTERTYPE),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (iswdigit(szType[0]))
    {
        Type = WORD(wcstoul(szType, NULL, 0));
    }
    else
    {
        Type = szType;
    }

    return TRUE;
}

BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name)
{
    WCHAR szName[MAX_PATH];
    GetWindowTextW(hCmb2, szName, _countof(szName));
    MStringW Str = szName;
    mstr_trim(Str);
    lstrcpynW(szName, Str.c_str(), _countof(szName));
    if (szName[0] == UNICODE_NULL)
    {
        ComboBox_SetEditSel(hCmb2, 0, -1);
        SetFocus(hCmb2);
        MessageBoxW(GetParent(hCmb2), LoadStringDx(IDS_ENTERNAME),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (iswdigit(szName[0]))
    {
        Name = WORD(wcstoul(szName, NULL, 0));
    }
    else
    {
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

    if (szLang[0] == UNICODE_NULL)
    {
        ComboBox_SetEditSel(hCmb3, 0, -1);
        SetFocus(hCmb3);
        MessageBoxW(GetParent(hCmb3), LoadStringDx(IDS_ENTERLANG),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (iswdigit(szLang[0]))
    {
        Lang = WORD(wcstoul(szLang, NULL, 0));
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
    wsprintfW(sz, L"Width %u, Height %u, BitsPixel %u\r\n",
              bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
    ret = sz;
    return ret;
}

std::wstring DumpCursorInfo(const BITMAP& bm)
{
    std::wstring ret;

    using namespace std;
    WCHAR sz[128];
    wsprintfW(sz, L"Width %u, Height %u, BitsPixel %u\r\n",
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

std::wstring GetKeyID(UINT wId)
{
    return mstr_dec(wId);
}

void Cmb1_InitVirtualKeys(HWND hCmb1)
{
    ComboBox_ResetContent(hCmb1);

    typedef ConstantsDB::TableType TableType;
    TableType table;
    table = g_ConstantsDB.GetTable(L"VIRTUALKEYS");

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

void StrDlg_GetEntry(HWND hwnd, STRING_ENTRY& entry)
{
    MString str = MWindowBase::GetDlgItemText(hwnd, cmb1);
    mstr_trim(str);
    lstrcpynW(entry.StringID, str.c_str(), _countof(entry.StringID));

    str = MWindowBase::GetDlgItemText(hwnd, edt1);
    mstr_trim(str);
    if (str[0] == L'"')
    {
        mstr_unquote(str);
    }
    lstrcpynW(entry.StringValue, str.c_str(), _countof(entry.StringValue));
}

void StrDlg_SetEntry(HWND hwnd, STRING_ENTRY& entry)
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
    HWND        m_hToolBar;
    MEditCtrl       m_hBinEdit;
    MEditCtrl       m_hSrcEdit;
    MBmpView        m_hBmpView;
    MRadWindow      m_rad_window;
    MSplitterWnd    m_splitter1;
    MSplitterWnd    m_splitter2;
    MSplitterWnd    m_splitter3;

    WCHAR       m_szDataFolder[MAX_PATH];
    WCHAR       m_szConstantsFile[MAX_PATH];
    WCHAR       m_szCppExe[MAX_PATH];
    WCHAR       m_szWindresExe[MAX_PATH];
    WCHAR       m_szFile[MAX_PATH];
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
        m_hToolBar(NULL)
    {
        m_szDataFolder[0] = 0;
        m_szConstantsFile[0] = 0;
        m_szCppExe[0] = 0;
        m_szWindresExe[0] = 0;
        m_szFile[0] = 0;
    }

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

            if (::TranslateAccelerator(m_hwnd, m_hAccel, &msg))
                continue;

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        return INT(msg.wParam);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
            HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
            HANDLE_MSG(hwnd, WM_INITMENU, OnInitMenu);
        default:
            return DefaultProcDx();
        }
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        g_hInstance = m_hInst;
        InitCommonControls();

        LoadLangInfo();

        INT nRet = CheckData();
        if (nRet)
            return FALSE;

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

        style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
            TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES |
            TVS_LINESATROOT | TVS_SHOWSELALWAYS;
        g_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
            WC_TREEVIEWW, NULL, style, 0, 0, 0, 0, m_splitter1,
            (HMENU)1, g_hInstance, NULL);
        if (g_hTreeView == NULL)
            return FALSE;

        TreeView_SetImageList(g_hTreeView, m_hImageList, TVSIL_NORMAL);

        m_splitter1.SetPane(0, g_hTreeView);
        m_splitter1.SetPane(1, m_splitter2);
        m_splitter1.SetPaneExtent(0, TV_WIDTH);

        style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
            ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
            ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
        exstyle = WS_EX_CLIENTEDGE;
        m_hBinEdit.CreateAsChildDx(m_splitter2, NULL, style, exstyle, 3);
        m_splitter2.SetPane(0, m_splitter3);
        m_splitter2.SetPane(1, m_hBinEdit);
        m_splitter2.SetPaneExtent(1, BE_HEIGHT);

        style = WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
            ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
        exstyle = WS_EX_CLIENTEDGE;
        m_hSrcEdit.CreateAsChildDx(m_splitter3, NULL, style, exstyle, 2);
        if (!m_hBmpView.CreateDx(m_splitter3, 4))
            return FALSE;

        m_splitter3.SetPane(0, m_hSrcEdit);
        //m_splitter3.SetPane(1, m_hBmpView);

        LOGFONTW lf;
        GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
        lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
        lf.lfFaceName[0] = UNICODE_NULL;

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
        SetFocus(g_hTreeView);
        return TRUE;
    }

    VOID HidePreview(HWND hwnd)
    {
        m_hBmpView.DestroyBmp();

        ::SetWindowTextW(m_hBinEdit, NULL);
        Edit_SetModify(m_hBinEdit, FALSE);

        ::SetWindowTextW(m_hSrcEdit, NULL);
        Edit_SetModify(m_hSrcEdit, FALSE);

        ShowBmpView(FALSE);

        PostMessageDx(WM_SIZE);
    }

    BOOL DoSetFile(HWND hwnd, LPCWSTR FileName)
    {
        if (FileName == 0 || FileName[0] == UNICODE_NULL)
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
        return TRUE;
    }

    void OnDeleteRes(HWND hwnd)
    {
        if (!CompileIfNecessary(hwnd))
            return;

        HTREEITEM hItem = TreeView_GetSelection(g_hTreeView);
        if (hItem == NULL)
            return;

        TV_Delete(g_hTreeView, hItem, m_Entries);
        HidePreview(hwnd);
    }

    void OnExtractBin(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
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
                INT count = TV_GetSelection(g_hTreeView, selection, m_Entries);
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

    void OnExtractIcon(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);

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
        ofn.lpstrDefExt = L"ico";
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
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);

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
        ofn.lpstrDefExt = L"cur";
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
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_NAME)
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
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_NAME)
            return;

        UINT i = LOWORD(lParam);
        MReplaceBinDlg dialog(m_Entries, m_Entries[i], g_ConstantsDB);
        dialog.DialogBoxDx(hwnd);
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
        if (m_szFile[0] == UNICODE_NULL || !GetBinaryType(m_szFile, &dwBinType))
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
        HTREEITEM hItem = TreeView_GetSelection(g_hTreeView);
        if (hItem == NULL)
            return;

        TV_ITEM Item;
        ZeroMemory(&Item, sizeof(Item));
        Item.mask = TVIF_PARAM;
        Item.hItem = hItem;
        TreeView_GetItem(g_hTreeView, &Item);

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
        MAddIconDlg dialog(m_Entries);
        dialog.DialogBoxDx(hwnd);
    }

    void OnReplaceIcon(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_NAME)
            return;

        UINT i = LOWORD(lParam);
        MReplaceIconDlg dialog(m_Entries, m_Entries[i]);
        dialog.DialogBoxDx(hwnd);
    }

    void OnReplaceCursor(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_NAME)
            return;

        UINT i = LOWORD(lParam);
        MReplaceCursorDlg dialog(m_Entries, m_Entries[i]);
        dialog.DialogBoxDx(hwnd);
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
        MAddBitmapDlg dialog(m_Entries);
        dialog.DialogBoxDx(hwnd, IDD_ADDBITMAP);
    }

    void OnReplaceBitmap(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_NAME)
            return;

        UINT i = LOWORD(lParam);
        MReplaceBitmapDlg dialog(m_Entries, m_Entries[i]);
        dialog.DialogBoxDx(hwnd);
    }

    void OnAddCursor(HWND hwnd)
    {
        MAddCursorDlg dialog(m_Entries);
        dialog.DialogBoxDx(hwnd);
    }

    void OnAddRes(HWND hwnd)
    {
        MAddResDlg dialog(m_Entries, g_ConstantsDB, g_hTreeView);
        dialog.DialogBoxDx(hwnd);
    }

    void OnAbout(HWND hwnd)
    {
        MSGBOXPARAMSW Params;
        ZeroMemory(&Params, sizeof(Params));
        Params.cbSize = sizeof(Params);
        Params.hwndOwner = hwnd;
        Params.hInstance = g_hInstance;
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

                TV_RefreshInfo(g_hTreeView, m_Entries);
            }
            else
            {
                ErrorBoxDx(IDS_CANNOTIMPORT);
            }
        }
    }

    void OnEdit(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (!IsEditableEntry(hwnd, lParam))
            return;

        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        SelectTV(hwnd, lParam, TRUE);
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
    {
        if (pnmhdr->code == NM_DBLCLK)
        {
            if (pnmhdr->hwndFrom == g_hTreeView)
            {
                OnEdit(hwnd);
            }
        }
        else if (pnmhdr->code == TVN_SELCHANGING)
        {
            return !CompileIfNecessary(hwnd);
        }
        else if (pnmhdr->code == TVN_SELCHANGED)
        {
            NM_TREEVIEWW *pTV = (NM_TREEVIEWW *)pnmhdr;
            LPARAM lParam = pTV->itemNew.lParam;
            SelectTV(hwnd, lParam, FALSE);
        }
        else if (pnmhdr->code == TVN_KEYDOWN)
        {
            TV_KEYDOWN *pTVKD = (TV_KEYDOWN *)pnmhdr;
            switch (pTVKD->wVKey)
            {
            case VK_RETURN:
                OnEdit(hwnd);
                break;
            case VK_DELETE:
                PostMessageW(hwnd, WM_COMMAND, ID_DELETERES, 0);
                break;
            }
        }
        return 0;
    }

    void OnCancelEdit(HWND hwnd)
    {
        Edit_SetModify(m_hSrcEdit, FALSE);

        LPARAM lParam = TV_GetParam(g_hTreeView);
        SelectTV(hwnd, lParam, FALSE);
    }

    void OnCompile(HWND hwnd)
    {
        if (!Edit_GetModify(m_hSrcEdit))
        {
            LPARAM lParam = TV_GetParam(g_hTreeView);
            SelectTV(hwnd, lParam, FALSE);
            return;
        }

        INT cchText = ::GetWindowTextLengthW(m_hSrcEdit);
        std::wstring WideText;
        WideText.resize(cchText);
        ::GetWindowTextW(m_hSrcEdit, &WideText[0], cchText + 1);

        Edit_SetModify(m_hSrcEdit, FALSE);
        if (DoCompileParts(hwnd, WideText))
        {
            TV_RefreshInfo(g_hTreeView, m_Entries, FALSE);

            LPARAM lParam = TV_GetParam(g_hTreeView);
            SelectTV(hwnd, lParam, FALSE);
        }
    }

    void OnGuiEdit(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (!IsEditableEntry(hwnd, lParam))
            return;

        WORD i = LOWORD(lParam);
        ResEntry& Entry = m_Entries[i];
        if (!Res_CanGuiEdit(Entry.type))
        {
            return;
        }

        if (!CompileIfNecessary(hwnd))
        {
            return;
        }

        Edit_SetReadOnly(m_hSrcEdit, TRUE);

        const ResEntry::DataType& data = Entry.data;
        MByteStreamEx stream(data);
        if (Entry.type == RT_ACCELERATOR)
        {
            AccelRes accel_res;
            MEditAccelDlg dialog(accel_res, g_ConstantsDB);
            if (accel_res.LoadFromStream(stream))
            {
                INT nID = dialog.DialogBoxDx(hwnd);
                if (nID == IDOK)
                {
                    accel_res.Update();
                    Entry.data = accel_res.data();
                    SelectTV(hwnd, lParam, FALSE);
                }
            }
        }
        else if (Entry.type == RT_MENU)
        {
            MenuRes menu_res;
            if (menu_res.LoadFromStream(stream))
            {
                MEditMenuDlg dialog(menu_res);
                INT nID = dialog.DialogBoxDx(hwnd);
                if (nID == IDOK)
                {
                    menu_res.Update();
                    Entry.data = menu_res.data();
                    SelectTV(hwnd, lParam, FALSE);
                    return;
                }
            }
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

            DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
            if (m_rad_window.CreateWindowDx(m_hwnd, MAKEINTRESOURCE(IDS_RADWINDOW),
                                            style))
            {
                ShowWindow(m_rad_window, SW_SHOWNOACTIVATE);
                UpdateWindow(m_rad_window);
            }
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
                    return;
            }

            MStringsDlg dialog(str_res);
            INT nID = dialog.DialogBoxDx(hwnd);
            if (nID == IDOK)
            {
                std::wstring WideText = str_res.Dump();
                ::SetWindowTextW(m_hSrcEdit, WideText.c_str());

                if (DoCompileParts(hwnd, WideText))
                {
                    TV_RefreshInfo(g_hTreeView, m_Entries, FALSE);
                    SelectTV(hwnd, lParam, FALSE);
                }
            }
        }

        Edit_SetReadOnly(m_hSrcEdit, FALSE);
    }

    void OnNew(HWND hwnd)
    {
        DoSetFile(hwnd, NULL);
        m_Entries.clear();
        TV_RefreshInfo(g_hTreeView, m_Entries);
    }

    void OnUpdateRes(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_LANG)
            return;

        UINT i = LOWORD(lParam);
        ResEntry& Entry = m_Entries[i];

        DialogRes& dialog_res = m_rad_window.m_dialog_res;

        MByteStreamEx stream;
        dialog_res.SaveToStream(stream);
        Entry.data = stream.data();

        std::wstring str = dialog_res.Dump(Entry.name, g_ConstantsDB);
        ::SetWindowTextW(m_hSrcEdit, str.c_str());
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        if (codeNotify == EN_CHANGE && m_hSrcEdit == hwndCtl)
        {
            ToolBar_Update(m_hToolBar, 2);
            return;
        }

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
        case ID_UPDATERES:
            OnUpdateRes(hwnd);
            break;
        }
    }

    void OnDestroy(HWND hwnd)
    {
        m_hBmpView.DestroyBmp();
        DeleteObject(m_hNormalFont);
        DeleteObject(m_hSmallFont);
        ImageList_Destroy(m_hImageList);
        DestroyIcon(m_hFileIcon);
        DestroyIcon(m_hFolderIcon);
        PostQuitMessage(0);
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH], *pch;

        DragQueryFileW(hdrop, 0, File, _countof(File));
        DragFinish(hdrop);

        pch = wcsrchr(File, L'.');
        if (pch)
        {
            if (lstrcmpiW(pch, L".ico") == 0)
            {
                MAddIconDlg dialog(m_Entries);
                dialog.File = File;
                dialog.DialogBoxDx(hwnd);
                return;
            }
            else if (lstrcmpiW(pch, L".cur") == 0)
            {
                MAddCursorDlg dialog(m_Entries);
                dialog.File = File;
                dialog.DialogBoxDx(hwnd);
                return;
            }
            else if (lstrcmpiW(pch, L".bmp") == 0 ||
                     lstrcmpiW(pch, L".png") == 0)
            {
                MAddBitmapDlg dialog(m_Entries);
                dialog.File = File;
                dialog.DialogBoxDx(hwnd);
                return;
            }
            else if (lstrcmpiW(pch, L".res") == 0)
            {
                DoLoad(hwnd, m_Entries, File);
                return;
            }
        }

        DoLoad(hwnd, m_Entries, File);
    }

    void ShowBmpView(BOOL bShow = TRUE)
    {
        if (bShow)
        {
            ShowWindow(m_hBmpView, SW_SHOWNOACTIVATE);
            m_splitter3.SetPaneCount(2);
            m_splitter3.SetPane(0, m_hSrcEdit);
            m_splitter3.SetPane(1, m_hBmpView);
            m_splitter3.SetPaneExtent(1, 180);
        }
        else
        {
            ShowWindow(m_hBmpView, SW_HIDE);
            m_splitter3.SetPaneCount(1);
            m_splitter3.SetPane(0, m_hSrcEdit);
        }
        ::SendMessageW(m_hBmpView, WM_COMMAND, 999, 0);
    }

    void ShowBinEdit(BOOL bShow = TRUE)
    {
        if (bShow)
        {
            ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);
            m_splitter2.SetPaneCount(2);
            m_splitter2.SetPane(0, m_splitter3);
            m_splitter2.SetPane(1, m_hBinEdit);
            m_splitter2.SetPaneExtent(1, BE_HEIGHT);
        }
        else
        {
            ShowWindow(m_hBinEdit, SW_HIDE);
            m_splitter2.SetPaneCount(1);
            m_splitter2.SetPane(0, m_splitter3);
        }
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        SendMessageW(m_hToolBar, TB_AUTOSIZE, 0, 0);

        RECT ToolRect, ClientRect;

        GetClientRect(hwnd, &ClientRect);
        SIZE sizClient = SizeFromRectDx(&ClientRect);

        INT x = 0, y = 0;
        if (::IsWindowVisible(m_hToolBar))
        {
            GetWindowRect(m_hToolBar, &ToolRect);
            y += ToolRect.bottom - ToolRect.top;
            sizClient.cy -= ToolRect.bottom - ToolRect.top;
        }

        MoveWindow(m_splitter1, x, y, sizClient.cx, sizClient.cy, TRUE);
    }

    void OnInitMenu(HWND hwnd, HMENU hMenu)
    {
        HTREEITEM hItem = TreeView_GetSelection(g_hTreeView);
        if (hItem == NULL)
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
            return;
        }

        TV_ITEM Item;
        ZeroMemory(&Item, sizeof(Item));
        Item.mask = TVIF_PARAM;
        Item.hItem = hItem;
        TreeView_GetItem(g_hTreeView, &Item);

        UINT i = LOWORD(Item.lParam);
        const ResEntry& Entry = m_Entries[i];

        LPARAM lParam = TV_GetParam(g_hTreeView);
        BOOL bEditable = IsEditableEntry(hwnd, lParam);
        if (bEditable)
        {
            EnableMenuItem(hMenu, ID_EDIT, MF_ENABLED);
            if (Res_CanGuiEdit(Entry.type))
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

        switch (HIWORD(Item.lParam))
        {
        case I_TYPE:
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
            break;
        case I_NAME:
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
            break;
        case I_LANG:
            if (Entry.type == RT_GROUP_ICON || Entry.type == RT_ICON)
            {
                EnableMenuItem(hMenu, ID_EXTRACTICON, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
            }
            if (Entry.type == RT_GROUP_ICON)
            {
                EnableMenuItem(hMenu, ID_REPLACEICON, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
            }

            if (Entry.type == RT_BITMAP)
            {
                EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_ENABLED);
                EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
                EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
            }

            if (Entry.type == RT_GROUP_CURSOR || Entry.type == RT_CURSOR)
            {
                EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
            }
            if (Entry.type == RT_GROUP_CURSOR)
            {
                EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
            }

            if (Entry.type == RT_DIALOG || Entry.type == RT_MENU)
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
            break;
        case I_STRING: case I_MESSAGE:
            EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
            EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
            EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
            EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
            EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
            EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
            EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
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
            break;
        }
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext != g_hTreeView)
            return;

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

        HMENU hMenu = LoadMenuW(g_hInstance, MAKEINTRESOURCEW(2));
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
        m_hBmpView = PackedDIB_CreateBitmap(&Entry[0], Entry.size());

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

    void PreviewAccel(HWND hwnd, const ResEntry& Entry)
    {
        MByteStreamEx stream(Entry.data);
        AccelRes accel;
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
            std::wstring str = str_res.Dump(nTableID);
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
            std::wstring str = menu_res.Dump(Entry.name, g_ConstantsDB);
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
            std::wstring str = dialog_res.Dump(Entry.name, g_ConstantsDB);
            ::SetWindowTextW(m_hSrcEdit, str.c_str());
        }
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

        std::wstring str = str_res.Dump();
        ::SetWindowTextW(m_hSrcEdit, str.c_str());
    }

    void PreviewMessageTable(HWND hwnd, const ResEntry& Entry)
    {
    }

    BOOL Preview(HWND hwnd, const ResEntry& Entry)
    {
        HidePreview(hwnd);

        std::wstring str = DumpDataAsString(Entry.data);
        ::SetWindowTextW(m_hBinEdit, str.c_str());

        BOOL bEditable;
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

        PostMessageW(hwnd, WM_SIZE, 0, 0);
        return bEditable;
    }

    void SelectTV(HWND hwnd, LPARAM lParam, BOOL DoubleClick)
    {
        HidePreview(hwnd);

        WORD i = LOWORD(lParam);
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
                ::SetFocus(g_hTreeView);
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
            ::SetFocus(g_hTreeView);

            ToolBar_Update(m_hToolBar, 3);
        }
        ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);

        PostMessageDx(WM_SIZE);
    }

    BOOL IsEditableEntry(HWND hwnd, LPARAM lParam)
    {
        const WORD i = LOWORD(lParam);
        const ResEntry& Entry = m_Entries[i];
        const ID_OR_STRING& type = Entry.type;
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
        LPARAM lParam = TV_GetParam(g_hTreeView);

        if (HIWORD(lParam) == I_LANG)
        {
            WORD i = LOWORD(lParam);
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
            WORD i = LOWORD(lParam);
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
        LPARAM lParam = TV_GetParam(g_hTreeView);
        WORD i = LOWORD(lParam);
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

        WCHAR szTempPath[MAX_PATH];
        ::GetTempPathW(_countof(szTempPath), szTempPath);

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
        r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n",
                        PRIMARYLANGID(entry.lang), SUBLANGID(entry.lang));
        r1.WriteFormatA("#pragma code_page(65001)\r\n");
        r1.WriteFormatA("#include \"%S\"\r\n", szPath2);
        r1.CloseHandle();

        DWORD cbWritten;
        r2.WriteFile(TextUtf8.c_str(), TextUtf8.size() * sizeof(char), &cbWritten);
        r2.CloseHandle();

        WCHAR CmdLine[512];
    #if 1
        wsprintfW(CmdLine,
            L"\"%s\" -DRC_INVOKED -o \"%s\" -J rc -O res "
            L"-F pe-i386 --preprocessor=\"%s\" --preprocessor-arg=\"\" \"%s\"",
            m_szWindresExe, szPath3, m_szCppExe, szPath1);
    #else
        wsprintfW(CmdLine,
            L"\"%s\" -DRC_INVOKED -o \"%s\" -J rc -O res "
            L"-F pe-i386 --preprocessor=\"%s\" --preprocessor-arg=\"-v\" \"%s\"",
            m_szWindresExe, szPath3, m_szCppExe, szPath1);
    #endif

        // MessageBoxW(hwnd, CmdLine, NULL, 0);

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
            pmaker.CreateProcess(NULL, CmdLine))
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
                else  if (cbAvail == 0)
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
                        return FALSE;

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
        if (!g_ConstantsDB.LoadFromFile(m_szConstantsFile))
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
            TV_RefreshInfo(g_hTreeView, Entries);
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

        TV_RefreshInfo(g_hTreeView, Entries);
        DoSetFile(hwnd, Path);

        return TRUE;
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
            return Res_ExtractGroupIcon(m_Entries, Entry, FileName);
        else if (Entry.type == RT_ICON)
            return Res_ExtractIcon(m_Entries, Entry, FileName);
        else
            return FALSE;
    }

    BOOL DoExtractCursor(LPCWSTR FileName, const ResEntry& Entry)
    {
        if (Entry.type == RT_GROUP_CURSOR)
            return Res_ExtractGroupCursor(m_Entries, Entry, FileName);
        else if (Entry.type == RT_CURSOR)
            return Res_ExtractCursor(m_Entries, Entry, FileName);
        else
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
    CoUninitialize();

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
