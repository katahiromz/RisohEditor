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
WCHAR       g_szTitle[MAX_PATH] = L"RisohEditor by katahiromz";

WCHAR       g_szFile[MAX_PATH] = L"";
ConstantsDB g_ConstantsDB;
ResEntries  g_Entries;

HWND        g_hTreeView = NULL;
BOOL        g_bInEdit = FALSE;

HBITMAP     g_hBitmap = NULL;

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
        if (bCursor)
        {
            // mirror
            StretchBlt(hDC, 0, height, width, -height,
                       hDC, 0, 0, width, height, SRCCOPY);
        }
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

HBITMAP CreateBitmapFromIconsDx(HWND hwnd, const ResEntry& Entry)
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
        INT k = Res_Find(g_Entries, RT_ICON, pEntries[i].nID, Entry.lang);
        if (k == -1)
            k = Res_Find(g_Entries, RT_ICON, pEntries[i].nID, 0xFFFF);
        if (k == -1)
        {
            assert(0);
            return NULL;
        }
        ResEntry& IconEntry = g_Entries[k];

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
        INT k = Res_Find(g_Entries, RT_ICON, pEntries[i].nID, Entry.lang);
        if (k == -1)
            k = Res_Find(g_Entries, RT_ICON, pEntries[i].nID, 0xFFFF);
        if (k == -1)
        {
            assert(0);
            DeleteObject(hbm);
            return NULL;
        }
        ResEntry& IconEntry = g_Entries[k];
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

HBITMAP CreateBitmapFromCursorsDx(HWND hwnd, const ResEntry& Entry)
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
        INT k = Res_Find(g_Entries, RT_CURSOR, pEntries[i].nID, Entry.lang);
        if (k == -1)
            k = Res_Find(g_Entries, RT_CURSOR, pEntries[i].nID, 0xFFFF);
        if (k == -1)
        {
            assert(0);
            return NULL;
        }
        ResEntry& CursorEntry = g_Entries[k];

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
            INT k = Res_Find(g_Entries, RT_CURSOR, pEntries[i].nID, Entry.lang);
            if (k == -1)
                k = Res_Find(g_Entries, RT_CURSOR, pEntries[i].nID, 0xFFFF);
            if (k == -1)
            {
                assert(0);
                DeleteObject(hbm);
                return NULL;
            }
            ResEntry& CursorEntry = g_Entries[k];

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
    { -1, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_COMPILE },
    { -1, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_CANCELEDIT },
};

TBBUTTON g_buttons1[] =
{
    { -1, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_COMPILE },
    { -1, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_CANCELEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_GUIEDIT },
};

TBBUTTON g_buttons2[] =
{
    { -1, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_COMPILE },
    { -1, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_CANCELEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_GUIEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEST },
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
        WS_CHILD | /*WS_VISIBLE | */ CCS_TOP | TBSTYLE_WRAPABLE | TBSTYLE_LIST,
        0, 0, 0, 0, hwndParent, (HMENU)1, g_hInstance, NULL);
    if (hwndTB == NULL)
        return hwndTB;

    SendMessageW(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(0, 0));

    ToolBar_StoreStrings(hwndTB, _countof(g_buttons0), g_buttons0);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons1), g_buttons1);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons2), g_buttons2);

    ToolBar_Update(hwndTB, 0);
    return hwndTB;
}

//////////////////////////////////////////////////////////////////////////////

void Cmb3_InsertLangItemsAndSelectLang(HWND hCmb3, LANGID langid)
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

BOOL Cmb1_CheckType(HWND hCmb1, ID_OR_STRING& Type)
{
    WCHAR szType[MAX_PATH];
    GetWindowTextW(hCmb1, szType, _countof(szType));
    std::wstring Str = szType;
    str_trim(Str);
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

BOOL Cmb2_CheckName(HWND hCmb2, ID_OR_STRING& Name)
{
    WCHAR szName[MAX_PATH];
    GetWindowTextW(hCmb2, szName, _countof(szName));
    std::wstring Str = szName;
    str_trim(Str);
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

BOOL Cmb3_CheckLang(HWND hCmb3, WORD& Lang)
{
    WCHAR szLang[MAX_PATH];
    GetWindowTextW(hCmb3, szLang, _countof(szLang));
    std::wstring Str = szLang;
    str_trim(Str);
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
    std::wstring Str = szFile;
    str_trim(Str);
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
// actions

BOOL DoImport(HWND hwnd, LPCWSTR ResFile, ResEntries& entries)
{
    ByteStream stream;
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

BOOL DoSetFile(HWND hwnd, LPCWSTR FileName)
{
    if (FileName == 0 || FileName[0] == UNICODE_NULL)
    {
        SetWindowTextW(hwnd, g_szTitle);
        return TRUE;
    }

    WCHAR Path[MAX_PATH], *pch;
    GetFullPathNameW(FileName, _countof(Path), Path, &pch);
    lstrcpynW(g_szFile, Path, _countof(g_szFile));

    WCHAR sz[MAX_PATH];
    pch = wcsrchr(Path, L'\\');
    if (pch)
    {
        wsprintfW(sz, LoadStringDx(IDS_TITLEWITHFILE), pch + 1);
        SetWindowTextW(hwnd, sz);
    }
    else
    {
        SetWindowTextW(hwnd, g_szTitle);
    }
    return TRUE;
}

BOOL DoLoad(HWND hwnd, LPCWSTR FileName)
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

        g_Entries = entries;
        TV_RefreshInfo(g_hTreeView, g_Entries);
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

    g_Entries.clear();
    Res_GetListFromRes(hMod, (LPARAM)&g_Entries);
    FreeLibrary(hMod);

    TV_RefreshInfo(g_hTreeView, g_Entries);
    DoSetFile(hwnd, Path);
    g_bInEdit = FALSE;

    return TRUE;
}

BOOL DoExtractBin(LPCWSTR FileName, const ResEntry& Entry)
{
    ByteStream bs(Entry.data);
    return bs.SaveToFile(FileName);
}

BOOL DoExtractRes(HWND hwnd, LPCWSTR FileName, const ResEntries& Entries)
{
    ByteStream bs;
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
    if (DoExtractRes(hwnd, ExeFile, g_Entries))
    {
        Res_Optimize(g_Entries);
        DoSetFile(hwnd, ExeFile);
        return TRUE;
    }
    return FALSE;
}

BOOL DoSaveExeAs(HWND hwnd, LPCWSTR ExeFile)
{
    LPWSTR TempFile = GetTempFileNameDx(L"ERE");

    BOOL b1 = ::CopyFileW(g_szFile, TempFile, FALSE);
    BOOL b2 = b1 && Res_UpdateExe(hwnd, TempFile, g_Entries);
    BOOL b3 = b2 && ::CopyFileW(TempFile, ExeFile, FALSE);
    if (b3)
    {
        DeleteFileW(TempFile);
        Res_Optimize(g_Entries);
        DoSetFile(hwnd, ExeFile);

        return TRUE;
    }

    DeleteFileW(TempFile);
    return FALSE;
}

BOOL DoSaveAs(HWND hwnd, LPCWSTR ExeFile)
{
    if (g_bInEdit)
        return TRUE;

    DWORD dwBinType;
    LPCWSTR pch = wcsrchr(ExeFile, L'.');
    if ((pch && lstrcmpiW(pch, L".res") == 0) ||
        !GetBinaryType(g_szFile, &dwBinType))
    {
        return DoSaveResAs(hwnd, ExeFile);
    }

    return DoSaveExeAs(hwnd, ExeFile);
}

BOOL DoExtractIcon(LPCWSTR FileName, const ResEntry& Entry)
{
    if (Entry.type == RT_GROUP_ICON)
        return Res_ExtractGroupIcon(g_Entries, Entry, FileName);
    else if (Entry.type == RT_ICON)
        return Res_ExtractIcon(g_Entries, Entry, FileName);
    else
        return FALSE;
}

BOOL DoExtractCursor(LPCWSTR FileName, const ResEntry& Entry)
{
    if (Entry.type == RT_GROUP_CURSOR)
        return Res_ExtractGroupCursor(g_Entries, Entry, FileName);
    else if (Entry.type == RT_CURSOR)
        return Res_ExtractCursor(g_Entries, Entry, FileName);
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

    ByteStream bs;
    if (!bs.WriteRaw(FileHeader) || !bs.WriteData(&Entry[0], Entry.size()))
        return FALSE;

    return bs.SaveToFile(FileName);
}

BOOL DoAddBin(HWND hwnd,
              const ID_OR_STRING& Type,
              const ID_OR_STRING& Name,
              WORD Lang,
              const std::wstring& File)
{
    ByteStream bs;
    if (!bs.LoadFromFile(File.c_str()))
        return FALSE;

    Res_AddEntry(g_Entries, Type, Name, Lang, bs.data(), FALSE);
    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
    return TRUE;
}

BOOL DoReplaceBin(HWND hwnd,
                  const ID_OR_STRING& Type,
                  const ID_OR_STRING& Name,
                  WORD Lang,
                  const std::wstring& File)
{
    ByteStream bs;
    if (!bs.LoadFromFile(File.c_str()))
        return FALSE;

    Res_AddEntry(g_Entries, Type, Name, Lang, bs.data(), TRUE);
    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
    return TRUE;
}

BOOL DoAddIcon(HWND hwnd,
               const ID_OR_STRING& Name,
               WORD Lang,
               const std::wstring& IconFile)
{
    if (!Res_AddGroupIcon(g_Entries, Name, Lang, IconFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
    return TRUE;
}

BOOL DoReplaceIcon(HWND hwnd,
                   const ID_OR_STRING& Name,
                   WORD Lang,
                   const std::wstring& IconFile)
{
    if (!Res_AddGroupIcon(g_Entries, Name, Lang, IconFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
    return TRUE;
}

BOOL DoAddCursor(HWND hwnd,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& CurFile)
{
    if (!Res_AddGroupCursor(g_Entries, Name, Lang, CurFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
    return TRUE;
}

BOOL DoReplaceCursor(HWND hwnd,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& CurFile)
{
    if (!Res_AddGroupCursor(g_Entries, Name, Lang, CurFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
    return TRUE;
}

BOOL DoAddBitmap(HWND hwnd,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& BitmapFile)
{
    if (!Res_AddBitmap(g_Entries, Name, Lang, BitmapFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
    return TRUE;
}

BOOL DoReplaceBitmap(HWND hwnd,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& BitmapFile)
{
    if (!Res_AddBitmap(g_Entries, Name, Lang, BitmapFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// ReplaceBinDlg

struct ReplaceBinDlg : DialogBase
{
    ResEntry& m_Entry;

    ReplaceBinDlg(ResEntry& Entry) : m_Entry(Entry)
    {
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Types
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = g_ConstantsDB.GetTable(L"RESOURCE");
        for (size_t i = 0; i < Table.size(); ++i)
        {
            WCHAR sz[MAX_PATH];
            wsprintfW(sz, L"%s (%lu)", Table[i].name.c_str(), Table[i].value);
            INT k = ComboBox_AddString(hCmb1, sz);
            if (m_Entry.type == WORD(Table[i].value))
            {
                ComboBox_SetCurSel(hCmb1, k);
            }
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (m_Entry.name.is_str())
        {
            SetWindowTextW(hCmb2, m_Entry.name.m_Str.c_str());
        }
        else
        {
            SetDlgItemInt(hwnd, cmb2, m_Entry.name.m_ID, FALSE);
        }

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, m_Entry.lang);

        return TRUE;
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_REPLACERES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bin";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
        }
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = g_ConstantsDB.GetTable(L"RESOURCE");
        INT iType = ComboBox_GetCurSel(hCmb1);
        if (iType != CB_ERR && iType < INT(Table.size()))
        {
            Type = WORD(Table[iType].value);
        }
        else
        {
            if (!Cmb1_CheckType(hCmb1, Type))
                return;
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        ID_OR_STRING Name;
        if (!Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        if (!DoReplaceBin(hwnd, Type, Name, Lang, File))
        {
            ErrorBoxDx(IDS_CANNOTREPLACE);
            return;
        }

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);
    }
};

//////////////////////////////////////////////////////////////////////////////
// TestDialog

struct TestDialog : DialogBase
{
    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DWORD dwStyle;
        switch (uMsg)
        {
        case WM_LBUTTONDOWN:
            dwStyle = GetWindowStyle(hwnd);
            if (!(dwStyle & WS_SYSMENU))
            {
                EndDialog(hwnd, IDOK);
            }
            break;
        case WM_LBUTTONDBLCLK:
            EndDialog(hwnd, IDOK);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK: case IDCANCEL:
                EndDialog(hwnd, LOWORD(wParam));
                break;
            }
            break;
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////
// TestMenuDlg

struct TestMenuDlg : DialogBase
{
    HMENU m_hMenu;

    TestMenuDlg(HMENU hMenu) : m_hMenu(hMenu)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetMenu(hwnd, m_hMenu);
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK: case IDCANCEL:
            EndDialog(hwnd, id);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////
// AddIconDlg

struct AddIconDlg : DialogBase
{
    LPCWSTR File;
    HICON   m_hIcon;

    AddIconDlg() : File(NULL), m_hIcon(NULL)
    {
    }

    ~AddIconDlg()
    {
        DestroyIcon(m_hIcon);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, edt1, File);
        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);

        DragAcceptFiles(hwnd, TRUE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

        return TRUE;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_ICON;

        ID_OR_STRING Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        BOOL Overwrite = FALSE;
        INT iEntry = Res_Find(g_Entries, RT_GROUP_ICON, Name, Lang);
        if (iEntry != -1)
        {
            INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, g_szTitle,
                              MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                Overwrite = TRUE;
                break;
            case IDNO:
            case IDCANCEL:
                return;
            }
        }

        if (Overwrite)
        {
            if (!DoReplaceIcon(hwnd, Name, Lang, File))
            {
                ErrorBoxDx(IDS_CANTREPLACEICO);
                return;
            }
        }
        else
        {
            if (!DoAddIcon(hwnd, Name, Lang, File))
            {
                ErrorBoxDx(IDS_CANNOTADDICON);
                return;
            }
        }

        EndDialog(hwnd, IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_ADDICON);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"ico";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
            if (m_hIcon)
                DestroyIcon(m_hIcon);
            m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
            Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);

        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
    }
};

//////////////////////////////////////////////////////////////////////////////
// ReplaceIconDlg

struct ReplaceIconDlg : DialogBase
{
    ResEntry& m_Entry;
    HICON   m_hIcon;

    ReplaceIconDlg(ResEntry& Entry) : m_Entry(Entry)
    {
        m_hIcon = NULL;
    }

    ~ReplaceIconDlg()
    {
        DestroyIcon(m_hIcon);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Name
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (m_Entry.name.is_str())
        {
            SetWindowTextW(hCmb2, m_Entry.name.m_Str.c_str());
        }
        else
        {
            SetDlgItemInt(hwnd, cmb2, m_Entry.name.m_ID, FALSE);
        }
        EnableWindow(hCmb2, FALSE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_ICON;

        ID_OR_STRING Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        if (!DoReplaceIcon(hwnd, Name, Lang, File))
        {
            ErrorBoxDx(IDS_CANTREPLACEICO);
            return;
        }

        EndDialog(hwnd, IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_REPLACEICO);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"ico";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
            if (m_hIcon)
                DestroyIcon(m_hIcon);
            m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
            Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);

        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////
// ReplaceCursorDlg

struct ReplaceCursorDlg : DialogBase
{
    ResEntry& m_Entry;
    HCURSOR   m_hCursor;

    ReplaceCursorDlg(ResEntry& Entry) : m_Entry(Entry)
    {
        m_hCursor = NULL;
    }

    ~ReplaceCursorDlg()
    {
        DestroyCursor(m_hCursor);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Name
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (m_Entry.name.is_str())
        {
            SetWindowTextW(hCmb2, m_Entry.name.m_Str.c_str());
        }
        else
        {
            SetDlgItemInt(hwnd, cmb2, m_Entry.name.m_ID, FALSE);
        }
        EnableWindow(hCmb2, FALSE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_CURSOR;

        ID_OR_STRING Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        if (!DoReplaceCursor(hwnd, Name, Lang, File))
        {
            ErrorBoxDx(IDS_CANTREPLACECUR);
            return;
        }

        EndDialog(hwnd, IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_REPLACECUR);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"cur";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
            if (m_hCursor)
                DestroyCursor(m_hCursor);
            m_hCursor = LoadCursorFromFile(File);
            SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(m_hCursor));
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);

        if (m_hCursor)
            DestroyCursor(m_hCursor);
        m_hCursor = LoadCursorFromFile(File);
        SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(m_hCursor));
        DragFinish(hdrop);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////
// AddBitmapDlg

struct AddBitmapDlg : DialogBase
{
    LPCWSTR File;
    AddBitmapDlg() : File(NULL)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, edt1, File);

        DragAcceptFiles(hwnd, TRUE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

        return TRUE;
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_BMPFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_ADDBMP);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bmp";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
        }
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_ICON;

        ID_OR_STRING Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        BOOL Overwrite = FALSE;
        INT iEntry = Res_Find(g_Entries, RT_BITMAP, Name, Lang);
        if (iEntry != -1)
        {
            INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, g_szTitle,
                              MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                Overwrite = TRUE;
                break;
            case IDNO:
            case IDCANCEL:
                return;
            }
        }

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        if (Overwrite)
        {
            if (!DoReplaceBitmap(hwnd, Name, Lang, File))
            {
                ErrorBoxDx(IDS_CANTREPLACEBMP);
                return;
            }
        }
        else
        {
            if (!DoAddBitmap(hwnd, Name, Lang, File))
            {
                ErrorBoxDx(IDS_CANTADDBMP);
                return;
            }
        }

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////
// ReplaceBitmapDlg

struct ReplaceBitmapDlg : DialogBase
{
    ResEntry& m_Entry;

    ReplaceBitmapDlg(ResEntry& Entry) : m_Entry(Entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Name
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (m_Entry.name.is_str())
        {
            SetWindowTextW(hCmb2, m_Entry.name.m_Str.c_str());
        }
        else
        {
            SetDlgItemInt(hwnd, cmb2, m_Entry.name.m_ID, FALSE);
        }
        EnableWindow(hCmb2, FALSE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_ICON;

        ID_OR_STRING Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        if (!DoReplaceBitmap(hwnd, Name, Lang, File))
        {
            ErrorBoxDx(IDS_CANTREPLACEBMP);
            return;
        }

        EndDialog(hwnd, IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_BMPFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_REPLACEBMP);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bmp";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////
// AddCursorDlg

struct AddCursorDlg : DialogBase
{
    LPCWSTR File;
    HCURSOR   m_hCursor;

    AddCursorDlg() : File(NULL)
    {
        m_hCursor = NULL;
    }

    ~AddCursorDlg()
    {
        DestroyCursor(m_hCursor);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, edt1, File);
        if (m_hCursor)
            DestroyCursor(m_hCursor);
        m_hCursor = LoadCursorFromFile(File);
        SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(m_hCursor));

        DragAcceptFiles(hwnd, TRUE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_CURSOR;

        ID_OR_STRING Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        BOOL Overwrite = FALSE;
        INT iEntry = Res_Find(g_Entries, RT_GROUP_ICON, Name, Lang);
        if (iEntry != -1)
        {
            INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, g_szTitle,
                              MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                Overwrite = TRUE;
                break;
            case IDNO:
            case IDCANCEL:
                return;
            }
        }

        if (Overwrite)
        {
            if (!DoReplaceCursor(hwnd, Name, Lang, File))
            {
                ErrorBoxDx(IDS_CANTREPLACECUR);
                return;
            }
        }
        else
        {
            if (!DoAddCursor(hwnd, Name, Lang, File))
            {
                ErrorBoxDx(IDS_CANNOTADDCUR);
                return;
            }
        }

        EndDialog(hwnd, IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_ADDCUR);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"cur";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
            if (m_hCursor)
                DestroyCursor(m_hCursor);
            m_hCursor = LoadCursorFromFile(File);
            SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(m_hCursor));
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);

        if (m_hCursor)
            DestroyCursor(m_hCursor);
        m_hCursor = LoadCursorFromFile(File);
        SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(m_hCursor));
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////
// AddResDlg

struct AddResDlg : DialogBase
{
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Types
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = g_ConstantsDB.GetTable(L"RESOURCE");
        for (size_t i = 0; i < Table.size(); ++i)
        {
            WCHAR sz[MAX_PATH];
            wsprintfW(sz, L"%s (%lu)", Table[i].name.c_str(), Table[i].value);
            ComboBox_AddString(hCmb1, sz);
        }

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = g_ConstantsDB.GetTable(L"RESOURCE");
        INT iType = ComboBox_GetCurSel(hCmb1);
        if (iType != CB_ERR && iType < INT(Table.size()))
        {
            Type = WORD(Table[iType].value);
        }
        else
        {
            if (!Cmb1_CheckType(hCmb1, Type))
                return;
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        ID_OR_STRING Name;
        if (!Res_HasNoName(Type) && !Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Res_HasSample(Type) && !Edt1_CheckFile(hEdt1, File))
            return;

        BOOL Overwrite = FALSE;
        INT iEntry = Res_Find(g_Entries, Type, Name, Lang);
        if (iEntry != -1)
        {
            if (File.empty() && Res_HasSample(Type))
            {
                ErrorBoxDx(IDS_ALREADYEXISTS);
                return;
            }

            INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, g_szTitle,
                              MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                Overwrite = TRUE;
                break;
            case IDNO:
            case IDCANCEL:
                return;
            }
        }

        BOOL bOK = TRUE;
        if (File.empty() && Res_HasSample(Type))
        {
            if (Res_HasNoName(Type))
            {
                Res_DeleteNames(g_Entries, Type, Lang);
            }

            ByteStream stream;
            if (Type == RT_ACCELERATOR)
            {
                DWORD Size;
                const BYTE *pb = GetAccelSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_DIALOG)
            {
                DWORD Size;
                const BYTE *pb = GetDialogSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_MENU)
            {
                DWORD Size;
                const BYTE *pb = GetMenuSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_STRING)
            {
                DWORD Size;
                const BYTE *pb = GetStringSample(Size);
                stream.assign(pb, Size);
                Name = 1;
            }
            else if (Type == RT_VERSION)
            {
                DWORD Size;
                const BYTE *pb = GetVersionSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_HTML)
            {
                DWORD Size;
                const BYTE *pb = GetHtmlSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_MANIFEST)
            {
                DWORD Size;
                const BYTE *pb = GetManifestSample(Size);
                stream.assign(pb, Size);
            }
            else
            {
                bOK = FALSE;
            }

            if (bOK)
            {
                Res_AddEntry(g_Entries, Type, Name, Lang, stream.data(), FALSE);

                TV_RefreshInfo(g_hTreeView, g_Entries, FALSE, FALSE);

                ResEntry entry(Type, Name, Lang);
                TV_SelectEntry(g_hTreeView, g_Entries, entry);
            }
        }

        if (!bOK)
        {
            if (Overwrite)
            {
                if (!DoReplaceBin(hwnd, Type, Name, Lang, File))
                {
                    ErrorBoxDx(IDS_CANNOTREPLACE);
                    return;
                }
            }
            else
            {
                if (!DoAddBin(hwnd, Type, Name, Lang, File))
                {
                    ErrorBoxDx(IDS_CANNOTADDRES);
                    return;
                }
            }
        }

        EndDialog(hwnd, IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_ADDRES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bin";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        case cmb1:
            if (codeNotify == CBN_SELCHANGE)
            {
                
            }
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

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

    wsprintfW(sz, L"ImageCount: %u\r\n", dir.idCount);
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

        wsprintfW(sz, L"Image #%u: Width %u, Height %u, BitCount %u, ID %u\r\n",
                      i, Width, Height, pEntries[i].wBitCount, nID);
        ret += sz;
    }

    return ret;
}

std::wstring DumpGroupCursorInfo(const std::vector<BYTE>& data)
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

    wsprintfW(sz, L"ImageCount: %u\r\n", dir.idCount);
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

        INT k = Res_Find(g_Entries, RT_CURSOR, nID, 0xFFFF);
        if (k != -1)
        {
            const ResEntry& CursorEntry = g_Entries[k];
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

        wsprintfW(sz,
                      L"Image #%u: Width %u, Height %u, BitCount %u, xHotSpot %u, yHotSpot %u, ID %u\r\n",
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
    return str_dec(wId);
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

struct ACCEL_ENTRY
{
    WCHAR sz0[128];
    WCHAR sz1[16];
    WCHAR sz2[128];
};

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
            str = str_dec(i);
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
            str = str_quote(str2);
        }
        else
        {
            str = str_dec(i);
        }
    }

    return TRUE;
}

struct AddKeyDlg : DialogBase
{
    ACCEL_ENTRY m_entry;

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        CheckDlgButton(hwnd, chx1, BST_CHECKED);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        Cmb1_InitVirtualKeys(hCmb1);

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        GetWindowTextW(hCmb1, m_entry.sz0, _countof(m_entry.sz0));

        std::wstring str = m_entry.sz0;
        BOOL bVirtKey = IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED;
        if (!Cmb1_CheckKey(hwnd, hCmb1, bVirtKey, str))
        {
            ErrorBoxDx(IDS_INVALIDKEY);
            return;
        }
        lstrcpynW(m_entry.sz0, str.c_str(), _countof(m_entry.sz0));

        WORD wFlags = 0;
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            wFlags |= FVIRTKEY;
        if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
            wFlags |= FNOINVERT;
        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            wFlags |= FCONTROL;
        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            wFlags |= FSHIFT;
        if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
            wFlags |= FALT;

        str = GetKeyFlags(wFlags);
        lstrcpynW(m_entry.sz1, str.c_str(), _countof(m_entry.sz1));

        GetDlgItemTextW(hwnd, cmb2, m_entry.sz2, _countof(m_entry.sz2));

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

struct ModifyKeyDlg : DialogBase
{
    ACCEL_ENTRY m_entry;

    ModifyKeyDlg(ACCEL_ENTRY& entry) : m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, cmb1, m_entry.sz0);
        SetDlgItemTextW(hwnd, cmb2, m_entry.sz2);

        WORD Flags;
        SetKeyFlags(Flags, m_entry.sz1);
        if (Flags & FVIRTKEY)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (Flags & FNOINVERT)
            CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (Flags & FCONTROL)
            CheckDlgButton(hwnd, chx3, BST_CHECKED);
        if (Flags & FSHIFT)
            CheckDlgButton(hwnd, chx4, BST_CHECKED);
        if (Flags & FALT)
            CheckDlgButton(hwnd, chx5, BST_CHECKED);

        if (Flags & FVIRTKEY)
        {
            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
            Cmb1_InitVirtualKeys(hCmb1);

            INT i = ComboBox_FindStringExact(hCmb1, -1, m_entry.sz0);
            if (i != CB_ERR)
            {
                ComboBox_SetCurSel(hCmb1, i);
            }
        }

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        GetWindowTextW(hCmb1, m_entry.sz0, _countof(m_entry.sz0));

        std::wstring str = m_entry.sz0;
        BOOL bVirtKey = IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED;
        if (!Cmb1_CheckKey(hwnd, hCmb1, bVirtKey, str))
        {
            ErrorBoxDx(IDS_INVALIDKEY);
            return;
        }
        lstrcpynW(m_entry.sz0, str.c_str(), _countof(m_entry.sz0));

        WORD wFlags = 0;
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            wFlags |= FVIRTKEY;
        if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
            wFlags |= FNOINVERT;
        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            wFlags |= FCONTROL;
        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            wFlags |= FSHIFT;
        if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
            wFlags |= FALT;

        str = GetKeyFlags(wFlags);
        lstrcpynW(m_entry.sz1, str.c_str(), _countof(m_entry.sz1));

        GetDlgItemTextW(hwnd, cmb2, m_entry.sz2, _countof(m_entry.sz2));

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case chx1:
            if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            {
                Cmb1_InitVirtualKeys(GetDlgItem(hwnd, cmb1));
            }
            else
            {
                SetDlgItemTextW(hwnd, cmb1, NULL);
            }
            break;
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

struct EditAccelDlg : DialogBase
{
    AccelRes m_accel_res;

    void OnUp(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem == 0)
            return;

        ACCEL_ENTRY ae0, ae1;
        ListView_GetItemText(hCtl1, iItem - 1, 0, ae0.sz0, _countof(ae0.sz0));
        ListView_GetItemText(hCtl1, iItem - 1, 1, ae0.sz1, _countof(ae0.sz1));
        ListView_GetItemText(hCtl1, iItem - 1, 2, ae0.sz2, _countof(ae0.sz2));
        ListView_GetItemText(hCtl1, iItem, 0, ae1.sz0, _countof(ae1.sz0));
        ListView_GetItemText(hCtl1, iItem, 1, ae1.sz1, _countof(ae1.sz1));
        ListView_GetItemText(hCtl1, iItem, 2, ae1.sz2, _countof(ae1.sz2));

        ListView_SetItemText(hCtl1, iItem - 1, 0, ae1.sz0);
        ListView_SetItemText(hCtl1, iItem - 1, 1, ae1.sz1);
        ListView_SetItemText(hCtl1, iItem - 1, 2, ae1.sz2);
        ListView_SetItemText(hCtl1, iItem, 0, ae0.sz0);
        ListView_SetItemText(hCtl1, iItem, 1, ae0.sz1);
        ListView_SetItemText(hCtl1, iItem, 2, ae0.sz2);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem - 1, state, state);
    }

    void OnDown(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem + 1 == ListView_GetItemCount(hCtl1))
            return;

        ACCEL_ENTRY ae0, ae1;
        ListView_GetItemText(hCtl1, iItem, 0, ae0.sz0, _countof(ae0.sz0));
        ListView_GetItemText(hCtl1, iItem, 1, ae0.sz1, _countof(ae0.sz1));
        ListView_GetItemText(hCtl1, iItem, 2, ae0.sz2, _countof(ae0.sz2));
        ListView_GetItemText(hCtl1, iItem + 1, 0, ae1.sz0, _countof(ae1.sz0));
        ListView_GetItemText(hCtl1, iItem + 1, 1, ae1.sz1, _countof(ae1.sz1));
        ListView_GetItemText(hCtl1, iItem + 1, 2, ae1.sz2, _countof(ae1.sz2));

        ListView_SetItemText(hCtl1, iItem, 0, ae1.sz0);
        ListView_SetItemText(hCtl1, iItem, 1, ae1.sz1);
        ListView_SetItemText(hCtl1, iItem, 2, ae1.sz2);
        ListView_SetItemText(hCtl1, iItem + 1, 0, ae0.sz0);
        ListView_SetItemText(hCtl1, iItem + 1, 1, ae0.sz1);
        ListView_SetItemText(hCtl1, iItem + 1, 2, ae0.sz2);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem + 1, state, state);
    }

    void OnDelete(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem >= 0)
        {
            ListView_DeleteItem(hCtl1, iItem);
        }
    }

    void OnAdd(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        AddKeyDlg dialog;
        if (IDOK != dialog.DialogBoxDx(hwnd, IDD_ADDKEY))
        {
            return;
        }

        INT iItem = ListView_GetItemCount(hCtl1);

        LV_ITEM item;

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = dialog.m_entry.sz0;
        ListView_InsertItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = dialog.m_entry.sz1;
        ListView_SetItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 2;
        item.pszText = dialog.m_entry.sz2;
        ListView_SetItem(hCtl1, &item);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem, state, state);
    }

    void OnModify(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        ACCEL_ENTRY a_entry;
        ListView_GetItemText(hCtl1, iItem, 0, a_entry.sz0, _countof(a_entry.sz0));
        ListView_GetItemText(hCtl1, iItem, 1, a_entry.sz1, _countof(a_entry.sz1));
        ListView_GetItemText(hCtl1, iItem, 2, a_entry.sz2, _countof(a_entry.sz2));

        ModifyKeyDlg dialog(a_entry);
        if (IDOK == dialog.DialogBoxDx(hwnd, IDD_MODIFYKEY))
        {
            ListView_SetItemText(hCtl1, iItem, 0, a_entry.sz0);
            ListView_SetItemText(hCtl1, iItem, 1, a_entry.sz1);
            ListView_SetItemText(hCtl1, iItem, 2, a_entry.sz2);
        }
    }

    void OnOK(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT i, Count = ListView_GetItemCount(hCtl1);

        if (Count == 0)
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return;
        }

        m_accel_res.Entries().clear();
        for (i = 0; i < Count; ++i)
        {
            ACCEL_ENTRY a_entry;
            ListView_GetItemText(hCtl1, i, 0, a_entry.sz0, _countof(a_entry.sz0));
            ListView_GetItemText(hCtl1, i, 1, a_entry.sz1, _countof(a_entry.sz1));
            ListView_GetItemText(hCtl1, i, 2, a_entry.sz2, _countof(a_entry.sz2));

            WORD Flags;
            SetKeyFlags(Flags, a_entry.sz1);

            AccelTableEntry entry;
            entry.fFlags = Flags;
            if (Flags & FVIRTKEY)
            {
                entry.wAscii = 
                    (WORD)g_ConstantsDB.GetValue(L"VIRTUALKEYS", a_entry.sz0);
            }
            else
            {
                std::wstring str, str2 = a_entry.sz0;
                LPCWSTR pch = str2.c_str();
                if (guts_quote(str, pch))
                {
                    entry.wAscii = str[0];
                }
                else
                {
                    entry.wAscii = _wtoi(a_entry.sz0);
                }
            }
            entry.wId = _wtoi(a_entry.sz2);

            m_accel_res.Entries().push_back(entry);
        }

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1:
            OnAdd(hwnd);
            break;
        case psh2:
            OnModify(hwnd);
            break;
        case psh3:
            OnDelete(hwnd);
            break;
        case psh4:
            OnUp(hwnd);
            break;
        case psh5:
            OnDown(hwnd);
            break;
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
    {
        if (idFrom == ctl1)
        {
            if (pnmhdr->code == LVN_KEYDOWN)
            {
                LV_KEYDOWN *KeyDown = (LV_KEYDOWN *)pnmhdr;
                if (KeyDown->wVKey == VK_DELETE)
                {
                    OnDelete(hwnd);
                    return 0;
                }
            }
            if (pnmhdr->code == NM_DBLCLK)
            {
                OnModify(hwnd);
                return 0;
            }
        }
        return 0;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        }
        return DefaultProcDx();
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);
        ListView_SetExtendedListViewStyle(hCtl1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 105;
        column.pszText = LoadStringDx(IDS_KEY);
        column.iSubItem = 0;
        ListView_InsertColumn(hCtl1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 75;
        column.pszText = LoadStringDx(IDS_FLAGS);
        column.iSubItem = 1;
        ListView_InsertColumn(hCtl1, 1, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 185;
        column.pszText = LoadStringDx(IDS_COMMANDID);
        column.iSubItem = 2;
        ListView_InsertColumn(hCtl1, 2, &column);

        typedef AccelRes::entries_type entries_type;
        const entries_type& entries = m_accel_res.Entries();

        INT i = 0;
        entries_type::const_iterator it, end = entries.end();
        for (it = entries.begin(); it != end; ++it, ++i)
        {
            std::wstring str;
            if (it->fFlags & FVIRTKEY)
            {
                str = g_ConstantsDB.GetName(L"VIRTUALKEYS", it->wAscii);
            }
            else
            {
                str += (WCHAR)it->wAscii;
                str = str_quote(str);
            }

            LV_ITEM item;
            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = &str[0];
            ListView_InsertItem(hCtl1, &item);

            str = GetKeyFlags(it->fFlags);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = &str[0];
            ListView_SetItem(hCtl1, &item);

            str = GetKeyID(it->wId);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 2;
            item.pszText = &str[0];
            ListView_SetItem(hCtl1, &item);
        }

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, 0, state, state);
        SetFocus(hCtl1);

        return TRUE;
    }
};

struct STRING_ENTRY
{
    WCHAR StringID[128];
    WCHAR StringValue[512];
};

void StrDlg_GetEntry(HWND hwnd, STRING_ENTRY& entry)
{
    WCHAR Buffer[512];

    GetDlgItemTextW(hwnd, cmb1, Buffer, _countof(Buffer));
    str_trim(Buffer);
    lstrcpynW(entry.StringID, Buffer, _countof(entry.StringID));

    GetDlgItemTextW(hwnd, edt1, Buffer, _countof(Buffer));
    str_trim(Buffer);
    if (Buffer[0] == L'"')
    {
        str_unquote(Buffer);
    }
    lstrcpynW(entry.StringValue, Buffer, _countof(entry.StringValue));
}

void StrDlg_SetEntry(HWND hwnd, STRING_ENTRY& entry)
{
    SetDlgItemTextW(hwnd, cmb1, entry.StringID);

    std::wstring str = entry.StringValue;
    str = str_quote(str);

    SetDlgItemTextW(hwnd, edt1, str.c_str());
}

struct AddStrDlg : DialogBase
{
    STRING_ENTRY& m_entry;

    AddStrDlg(STRING_ENTRY& entry) : m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            StrDlg_GetEntry(hwnd, m_entry);
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return 0;
    }
};

struct ModifyStrDlg : DialogBase
{
    STRING_ENTRY& m_entry;
    ModifyStrDlg(STRING_ENTRY& entry) : m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        StrDlg_SetEntry(hwnd, m_entry);
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            StrDlg_GetEntry(hwnd, m_entry);
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

struct StringsDlg : DialogBase
{
    StringRes& m_str_res;

    StringsDlg(StringRes& str_res) : m_str_res(str_res)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);
        ListView_SetExtendedListViewStyle(hCtl1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 140;
        column.pszText = LoadStringDx(IDS_STRINGID);
        column.iSubItem = 0;
        ListView_InsertColumn(hCtl1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 500;
        column.pszText = LoadStringDx(IDS_STRINGVALUE);
        column.iSubItem = 1;
        ListView_InsertColumn(hCtl1, 1, &column);

        typedef StringRes::map_type map_type;
        const map_type& map = m_str_res.map();

        INT i = 0;
        map_type::const_iterator it, end = map.end();
        for (it = map.begin(); it != end; ++it)
        {
            if (it->second.empty())
                continue;

            std::wstring str;
            str = str_dec(it->first);

            LV_ITEM item;
            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = &str[0];
            ListView_InsertItem(hCtl1, &item);

            str = str_quote(it->second);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = &str[0];
            ListView_SetItem(hCtl1, &item);

            ++i;
        }

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, 0, state, state);
        SetFocus(hCtl1);

        return TRUE;
    }

    void OnAdd(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        STRING_ENTRY s_entry;
        ZeroMemory(&s_entry, sizeof(s_entry));
        AddStrDlg dialog(s_entry);
        INT nID = dialog.DialogBoxDx(hwnd, IDD_ADDSTR);
        if (IDOK != nID)
        {
            return;
        }

        INT iItem = ListView_GetItemCount(hCtl1);

        LV_ITEM item;

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = s_entry.StringID;
        ListView_InsertItem(hCtl1, &item);

        std::wstring str = s_entry.StringValue;
        str = str_quote(str);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = &str[0];
        ListView_SetItem(hCtl1, &item);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem, state, state);
    }

    void OnDelete(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem >= 0)
        {
            ListView_DeleteItem(hCtl1, iItem);
        }
    }

    void GetEntry(HWND hwnd, HWND hCtl1, INT iItem, STRING_ENTRY& entry)
    {
        ListView_GetItemText(hCtl1, iItem, 0, entry.StringID, _countof(entry.StringID));
        str_trim(entry.StringID);

        ListView_GetItemText(hCtl1, iItem, 1, entry.StringValue, _countof(entry.StringValue));
        str_trim(entry.StringValue);
        if (entry.StringValue[0] == L'"')
        {
            str_unquote(entry.StringValue);
        }
    }

    void OnModify(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        STRING_ENTRY s_entry;
        GetEntry(hwnd, hCtl1, iItem, s_entry);

        ModifyStrDlg dialog(s_entry);
        INT nID = dialog.DialogBoxDx(hwnd, IDD_MODIFYSTR);
        if (IDOK == nID)
        {
            ListView_SetItemText(hCtl1, iItem, 0, s_entry.StringID);

            std::wstring str = str_quote(s_entry.StringValue);
            ListView_SetItemText(hCtl1, iItem, 1, &str[0]);
        }
    }

    void OnOK(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem, Count = ListView_GetItemCount(hCtl1);
        if (Count == 0)
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return;
        }

        m_str_res.map().clear();

        STRING_ENTRY s_entry;
        for (iItem = 0; iItem < Count; ++iItem)
        {
            GetEntry(hwnd, hCtl1, iItem, s_entry);

            WORD wID = (WORD)_wtoi(s_entry.StringID);
            std::wstring str = s_entry.StringValue;

            m_str_res.map().insert(std::make_pair(wID, str));
        }

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1:
            OnAdd(hwnd);
            break;
        case psh2:
            OnModify(hwnd);
            break;
        case psh3:
            OnDelete(hwnd);
            break;
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
    {
        if (idFrom == ctl1)
        {
            if (pnmhdr->code == LVN_KEYDOWN)
            {
                LV_KEYDOWN *KeyDown = (LV_KEYDOWN *)pnmhdr;
                if (KeyDown->wVKey == VK_DELETE)
                {
                    OnDelete(hwnd);
                    return 0;
                }
            }
            if (pnmhdr->code == NM_DBLCLK)
            {
                OnModify(hwnd);
                return 0;
            }
        }
        return 0;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        }
        return DefaultProcDx();
    }
};

struct MENU_ENTRY
{
    WCHAR Caption[128];
    WCHAR Flags[64];
    WCHAR CommandID[64];
    WCHAR HelpID[64];
    WORD wDepth;
};

struct AddMItemDlg : DialogBase
{
    MENU_ENTRY& m_entry;

    AddMItemDlg(MENU_ENTRY& entry) : m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemInt(hwnd, cmb2, 0, TRUE);
        SetDlgItemInt(hwnd, edt1, 0, TRUE);
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        GetDlgItemTextW(hwnd, cmb1, m_entry.Caption, _countof(m_entry.Caption));
        str_trim(m_entry.Caption);
        if (m_entry.Caption[0] == L'"')
        {
            str_unquote(m_entry.Caption);
        }

        GetDlgItemTextW(hwnd, cmb2, m_entry.CommandID, _countof(m_entry.CommandID));
        str_trim(m_entry.CommandID);

        DWORD dwType = 0, dwState = 0;
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            dwState |= MFS_GRAYED;
        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            dwType |= MFT_BITMAP;
        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            dwType |= MFT_OWNERDRAW;
        if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
            dwState |= MFS_CHECKED;
        if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED)
            dwType |= MFT_SEPARATOR;
        if (IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED)
            dwType |= MFT_MENUBARBREAK;
        if (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED)
            dwType |= MFT_MENUBREAK;
        if (IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED)
            dwState |= MFS_DEFAULT;
        if (IsDlgButtonChecked(hwnd, chx10) == BST_CHECKED)
            dwState |= MFS_HILITE;
        if (IsDlgButtonChecked(hwnd, chx11) == BST_CHECKED)
            dwType |= MFT_RADIOCHECK;
        if (IsDlgButtonChecked(hwnd, chx12) == BST_CHECKED)
            dwType |= MFT_RIGHTORDER;
        if (IsDlgButtonChecked(hwnd, chx13) == BST_CHECKED)
            dwType |= MFT_RIGHTJUSTIFY;

        std::wstring str = GetMenuTypeAndState(dwType, dwState);
        if (m_entry.Caption[0] == 0 ||
            lstrcmpiW(m_entry.Caption, LoadStringDx(IDS_SEPARATOR)) == 0)
        {
            m_entry.Caption[0] = 0;
            dwType |= MFT_SEPARATOR;
            str = GetMenuTypeAndState(dwType, dwState);
        }
        lstrcpynW(m_entry.Flags, str.c_str(), _countof(m_entry.Flags));

        ::GetDlgItemTextW(hwnd, edt1, m_entry.HelpID, _countof(m_entry.HelpID));

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case chx6:
            if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED)
            {
                SetDlgItemTextW(hwnd, cmb1, NULL);
                SetDlgItemInt(hwnd, cmb2, 0, FALSE);
                SetDlgItemInt(hwnd, edt1, 0, FALSE);
            }
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

struct ModifyMItemDlg : DialogBase
{
    MENU_ENTRY& m_entry;

    ModifyMItemDlg(MENU_ENTRY& entry) : m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, cmb1, str_quote(m_entry.Caption).c_str());
        SetDlgItemTextW(hwnd, cmb2, m_entry.CommandID);
        SetDlgItemTextW(hwnd, edt1, m_entry.HelpID);

        DWORD dwType, dwState;
        dwType = dwState = 0;
        SetMenuTypeAndState(dwType, dwState, m_entry.Flags);

        if (lstrcmpiW(m_entry.Caption, LoadStringDx(IDS_SEPARATOR)) == 0 ||
            m_entry.Caption[0] == 0 || (dwType & MFT_SEPARATOR))
        {
            dwType |= MFT_SEPARATOR;
            SetDlgItemTextW(hwnd, cmb1, NULL);
        }

        if ((dwState & MFS_GRAYED) == MFS_GRAYED)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if ((dwType & MFT_BITMAP) == MFT_BITMAP)
            CheckDlgButton(hwnd, chx3, BST_CHECKED);
        if ((dwType & MFT_OWNERDRAW) == MFT_OWNERDRAW)
            CheckDlgButton(hwnd, chx4, BST_CHECKED);
        if ((dwState & MFS_CHECKED) == MFS_CHECKED)
            CheckDlgButton(hwnd, chx5, BST_CHECKED);
        if ((dwType & MFT_SEPARATOR) == MFT_SEPARATOR)
            CheckDlgButton(hwnd, chx6, BST_CHECKED);
        if ((dwType & MFT_MENUBARBREAK) == MFT_MENUBARBREAK)
            CheckDlgButton(hwnd, chx7, BST_CHECKED);
        if ((dwType & MFT_MENUBREAK) == MFT_MENUBREAK)
            CheckDlgButton(hwnd, chx8, BST_CHECKED);
        if ((dwState & MFS_DEFAULT) == MFS_DEFAULT)
            CheckDlgButton(hwnd, chx9, BST_CHECKED);
        if ((dwState & MFS_HILITE) == MFS_HILITE)
            CheckDlgButton(hwnd, chx10, BST_CHECKED);
        if ((dwType & MFT_RADIOCHECK) == MFT_RADIOCHECK)
            CheckDlgButton(hwnd, chx11, BST_CHECKED);
        if ((dwType & MFT_RIGHTORDER) == MFT_RIGHTORDER)
            CheckDlgButton(hwnd, chx12, BST_CHECKED);
        if ((dwType & MFT_RIGHTJUSTIFY) == MFT_RIGHTJUSTIFY)
            CheckDlgButton(hwnd, chx13, BST_CHECKED);

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        GetDlgItemTextW(hwnd, cmb1, m_entry.Caption, _countof(m_entry.Caption));
        str_trim(m_entry.Caption);
        if (m_entry.Caption[0] == L'"')
        {
            str_unquote(m_entry.Caption);
        }

        GetDlgItemTextW(hwnd, cmb2, m_entry.CommandID, _countof(m_entry.CommandID));
        str_trim(m_entry.CommandID);

        DWORD dwType = 0, dwState = 0;
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            dwState |= MFS_GRAYED;
        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            dwType |= MFT_BITMAP;
        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            dwType |= MFT_OWNERDRAW;
        if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
            dwState |= MFS_CHECKED;
        if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED)
            dwType |= MFT_SEPARATOR;
        if (IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED)
            dwType |= MFT_MENUBARBREAK;
        if (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED)
            dwType |= MFT_MENUBREAK;
        if (IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED)
            dwState |= MFS_DEFAULT;
        if (IsDlgButtonChecked(hwnd, chx10) == BST_CHECKED)
            dwState |= MFS_HILITE;
        if (IsDlgButtonChecked(hwnd, chx11) == BST_CHECKED)
            dwType |= MFT_RADIOCHECK;
        if (IsDlgButtonChecked(hwnd, chx12) == BST_CHECKED)
            dwType |= MFT_RIGHTORDER;
        if (IsDlgButtonChecked(hwnd, chx13) == BST_CHECKED)
            dwType |= MFT_RIGHTJUSTIFY;

        if (lstrcmpiW(m_entry.Caption, LoadStringDx(IDS_SEPARATOR)) == 0 ||
            m_entry.Caption[0] == 0 || (dwType & MFT_SEPARATOR))
        {
            m_entry.Caption[0] = 0;
            dwType |= MFT_SEPARATOR;
        }

        std::wstring str = GetMenuTypeAndState(dwType, dwState);
        lstrcpynW(m_entry.Flags, str.c_str(), _countof(m_entry.Flags));

        ::GetDlgItemTextW(hwnd, edt1, m_entry.HelpID, _countof(m_entry.HelpID));

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case chx6:
            if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED)
            {
                SetDlgItemTextW(hwnd, cmb1, NULL);
                SetDlgItemInt(hwnd, cmb2, 0, FALSE);
                SetDlgItemInt(hwnd, edt1, 0, FALSE);
            }
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

struct EditMenuDlg : DialogBase
{
    MenuRes& m_menu_res;

    EditMenuDlg(MenuRes& menu_res) : m_menu_res(menu_res)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        if (m_menu_res.IsExtended())
            CheckDlgButton(hwnd, chx1, BST_CHECKED);

        HWND hCtl1 = GetDlgItem(hwnd, ctl1);
        ListView_SetExtendedListViewStyle(hCtl1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 225;
        column.pszText = LoadStringDx(IDS_CAPTION);
        column.iSubItem = 0;
        ListView_InsertColumn(hCtl1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 95;
        column.pszText = LoadStringDx(IDS_FLAGS);
        column.iSubItem = 1;
        ListView_InsertColumn(hCtl1, 1, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 150;
        column.pszText = LoadStringDx(IDS_COMMANDID);
        column.iSubItem = 2;
        ListView_InsertColumn(hCtl1, 2, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 70;
        column.pszText = LoadStringDx(IDS_HELPID);
        column.iSubItem = 3;
        ListView_InsertColumn(hCtl1, 3, &column);

        INT i = 0;
        std::wstring str;
        LV_ITEM item;
        if (m_menu_res.IsExtended())
        {
            typedef MenuRes::ExMenuItemsType exitems_type;
            exitems_type& exitems = m_menu_res.exitems();
            exitems_type::iterator it, end = exitems.end();
            for (it = exitems.begin(); it != end; ++it, ++i)
            {
                str = str_repeat(LoadStringDx(IDS_INDENT), it->wDepth);
                if (it->text.empty() && it->menuId == 0)
                {
                    str += LoadStringDx(IDS_SEPARATOR);
                    it->dwType |= MFT_SEPARATOR;
                }
                else
                {
                    str += str_quote(it->text);
                }

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 0;
                item.pszText = &str[0];
                ListView_InsertItem(hCtl1, &item);

                str = GetMenuTypeAndState(it->dwType, it->dwState);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 1;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = str_dec(it->menuId);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 2;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = str_dec(it->dwHelpId);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 3;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);
            }
        }
        else
        {
            typedef MenuRes::MenuItemsType items_type;
            items_type& items = m_menu_res.items();
            items_type::iterator it, end = items.end();
            for (it = items.begin(); it != end; ++it, ++i)
            {
                str = str_repeat(LoadStringDx(IDS_INDENT), it->wDepth);
                if (it->text.empty() && it->wMenuID == 0)
                {
                    str += LoadStringDx(IDS_SEPARATOR);
                }
                else
                {
                    str += str_quote(it->text);
                }

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 0;
                item.pszText = &str[0];
                ListView_InsertItem(hCtl1, &item);

                str = GetMenuFlags(it->fItemFlags);
                if (it->text.empty() && it->wMenuID == 0)
                    str += L"S ";

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 1;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = str_dec(it->wMenuID);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 2;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = str_dec(0);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 3;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);
            }
        }

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, 0, state, state);
        SetFocus(hCtl1);

        return TRUE;
    }

    void OnAdd(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        MENU_ENTRY m_entry;
        ZeroMemory(&m_entry, sizeof(m_entry));
        AddMItemDlg dialog(m_entry);
        INT nID = dialog.DialogBoxDx(hwnd, IDD_ADDMITEM);
        if (IDOK != nID)
        {
            return;
        }

        INT iItem = ListView_GetItemCount(hCtl1);

        std::wstring str, strIndent = LoadStringDx(IDS_INDENT);
        str = m_entry.Caption;
        if (str.empty() || wcsstr(m_entry.Flags, L"S ") != NULL)
            str = LoadStringDx(IDS_SEPARATOR);
        str = str_repeat(strIndent, m_entry.wDepth) + str;

        LV_ITEM item;

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = &str[0];
        ListView_InsertItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = m_entry.Flags;
        ListView_SetItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 2;
        item.pszText = m_entry.CommandID;
        ListView_SetItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 3;
        item.pszText = m_entry.HelpID;
        ListView_SetItem(hCtl1, &item);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem, state, state);
    }

    BOOL GetEntry(HWND hwnd, HWND hCtl1, MENU_ENTRY& entry, INT iItem)
    {
        WCHAR Caption[256];
        ListView_GetItemText(hCtl1, iItem, 0, Caption, _countof(Caption));

        entry.wDepth = 0;
        std::wstring str = Caption, strIndent = LoadStringDx(IDS_INDENT);
        while (str.find(strIndent) == 0)
        {
            str = str.substr(strIndent.size());
            ++entry.wDepth;
        }
        str_trim(str);
        if (str[0] == L'"')
        {
            str_unquote(str);
        }
        if (str.empty() || str == LoadStringDx(IDS_SEPARATOR))
        {
            str.clear();
        }

        lstrcpynW(entry.Caption, str.c_str(), _countof(entry.Caption));

        ListView_GetItemText(hCtl1, iItem, 1, entry.Flags, _countof(entry.Flags));
        ListView_GetItemText(hCtl1, iItem, 2, entry.CommandID, _countof(entry.CommandID));
        ListView_GetItemText(hCtl1, iItem, 3, entry.HelpID, _countof(entry.HelpID));
        return TRUE;
    }

    BOOL SetEntry(HWND hwnd, HWND hCtl1, MENU_ENTRY& entry, INT iItem)
    {
        std::wstring str, strIndent = LoadStringDx(IDS_INDENT);
        str = str_repeat(strIndent, entry.wDepth);

        if (entry.Caption[0] == 0 || wcsstr(entry.Flags, L"S ") != NULL)
            str += LoadStringDx(IDS_SEPARATOR);
        else
            str += str_quote(entry.Caption);

        ListView_SetItemText(hCtl1, iItem, 0, &str[0]);
        ListView_SetItemText(hCtl1, iItem, 1, entry.Flags);
        ListView_SetItemText(hCtl1, iItem, 2, entry.CommandID);
        ListView_SetItemText(hCtl1, iItem, 3, entry.HelpID);
        return TRUE;
    }

    void OnModify(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        MENU_ENTRY m_entry;
        GetEntry(hwnd, hCtl1, m_entry, iItem);
        
        ModifyMItemDlg dialog(m_entry);
        INT nID = dialog.DialogBoxDx(hwnd, IDD_MODIFYMITEM);
        if (IDOK == nID)
        {
            SetEntry(hwnd, hCtl1, m_entry, iItem);
        }
    }

    void OnDelete(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem >= 0)
        {
            ListView_DeleteItem(hCtl1, iItem);
        }
    }

    void OnUp(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem <= 0)
            return;

        MENU_ENTRY entry0, entry1;

        GetEntry(hwnd, hCtl1, entry0, iItem - 1);
        GetEntry(hwnd, hCtl1, entry1, iItem);

        SetEntry(hwnd, hCtl1, entry1, iItem - 1);
        SetEntry(hwnd, hCtl1, entry0, iItem);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem - 1, state, state);
    }

    void OnDown(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        INT Count = ListView_GetItemCount(hCtl1);
        if (iItem + 1 >= Count)
            return;

        MENU_ENTRY entry0, entry1;

        GetEntry(hwnd, hCtl1, entry0, iItem);
        GetEntry(hwnd, hCtl1, entry1, iItem + 1);

        SetEntry(hwnd, hCtl1, entry1, iItem);
        SetEntry(hwnd, hCtl1, entry0, iItem + 1);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem + 1, state, state);
    }

    void OnLeft(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        WCHAR Caption[128];
        ListView_GetItemText(hCtl1, iItem, 0, Caption, _countof(Caption));

        std::wstring strIndent = LoadStringDx(IDS_INDENT);

        std::wstring str = Caption;
        if (str.find(strIndent) == 0)
        {
            str = str.substr(strIndent.size());
        }

        ListView_SetItemText(hCtl1, iItem, 0, &str[0]);
    }

    void OnRight(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        if (iItem == 0)
            return;

        WCHAR CaptionUp[128];
        ListView_GetItemText(hCtl1, iItem - 1, 0, CaptionUp, _countof(CaptionUp));
        WCHAR Caption[128];
        ListView_GetItemText(hCtl1, iItem, 0, Caption, _countof(Caption));

        std::wstring strIndent = LoadStringDx(IDS_INDENT);
        INT depth_up = str_repeat_count(CaptionUp, strIndent);
        INT depth = str_repeat_count(Caption, strIndent);

        if (depth_up < depth)
            return;

        std::wstring str = strIndent + Caption;
        ListView_SetItemText(hCtl1, iItem, 0, &str[0]);
    }

    void OnOK(HWND hwnd)
    {
        MENU_ENTRY entry;
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);
        INT iItem, Count = ListView_GetItemCount(hCtl1);

        if (Count == 0)
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return;
        }

        BOOL Extended = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        if (Extended)
        {
            m_menu_res.header().wVersion = 1;
            m_menu_res.header().wOffset = 4;
            m_menu_res.header().dwHelpId = 0;
            m_menu_res.exitems().clear();
            for (iItem = 0; iItem < Count; ++iItem)
            {
                GetEntry(hwnd, hCtl1, entry, iItem);

                MenuRes::ExMenuItem exitem;

                SetMenuTypeAndState(exitem.dwType, exitem.dwState, entry.Flags);
                exitem.menuId = _wtoi(entry.CommandID);
                exitem.bResInfo = 0;
                exitem.text = entry.Caption;
                exitem.dwHelpId = _wtoi(entry.HelpID);
                exitem.wDepth = entry.wDepth;

                m_menu_res.exitems().push_back(exitem);
            }
        }
        else
        {
            m_menu_res.header().wVersion = 0;
            m_menu_res.header().wOffset = 4;
            m_menu_res.header().dwHelpId = 0;
            m_menu_res.items().clear();
            for (iItem = 0; iItem < Count; ++iItem)
            {
                GetEntry(hwnd, hCtl1, entry, iItem);

                MenuRes::MenuItem item;

                SetMenuFlags(item.fItemFlags, entry.Flags);
                item.wMenuID = _wtoi(entry.CommandID);
                item.wDepth = entry.wDepth;
                item.text = entry.Caption;

                m_menu_res.items().push_back(item);
            }
        }

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1:
            OnAdd(hwnd);
            break;
        case psh2:
            OnModify(hwnd);
            break;
        case psh3:
            OnDelete(hwnd);
            break;
        case psh4:
            OnUp(hwnd);
            break;
        case psh5:
            OnDown(hwnd);
            break;
        case psh6:
            OnLeft(hwnd);
            break;
        case psh7:
            OnRight(hwnd);
            break;
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
    {
        if (idFrom == ctl1)
        {
            if (pnmhdr->code == LVN_KEYDOWN)
            {
                LV_KEYDOWN *KeyDown = (LV_KEYDOWN *)pnmhdr;
                if (KeyDown->wVKey == VK_DELETE)
                {
                    OnDelete(hwnd);
                    return 0;
                }
            }
            if (pnmhdr->code == NM_DBLCLK)
            {
                OnModify(hwnd);
                return 0;
            }
        }
        return 0;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        }
        return DefaultProcDx();
    }
};

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

struct BmpView : WindowBase
{
    BITMAP      m_bm;

    BmpView()
    {
        ZeroMemory(&m_bm, sizeof(m_bm));
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("RisohEditor BmpView Class");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hIcon = NULL;
        wcx.hCursor = LoadCursor(NULL, IDC_CROSS);
        wcx.hbrBackground = GetStockBrush(LTGRAY_BRUSH);
        wcx.lpszMenuName = NULL;
    }

    BOOL CreateDx(HWND hwndParent, INT CtrlID = 4)
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
        DWORD dwExStyle = WS_EX_CLIENTEDGE;
        return CreateWindowDx(hwndParent, NULL, dwStyle, dwExStyle,
            0, 0, 0, 0, (HMENU)CtrlID);
    }

    void OnPaint(HWND hwnd)
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hwnd, &ps);
        if (hDC == NULL)
            return;

        HDC hMemDC = CreateCompatibleDC(NULL);
        {
            SelectObject(hMemDC, g_hBitmap);
            INT dx = GetScrollPos(hwnd, SB_HORZ);
            INT dy = GetScrollPos(hwnd, SB_VERT);
            BitBlt(hDC, -dx, -dy, m_bm.bmWidth, m_bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
        }
        DeleteDC(hMemDC);
        EndPaint(hwnd, &ps);
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, GetStockBrush(COLOR_BACKGROUND));
        return TRUE;
    }

    void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        SCROLLINFO info;
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_POS | SIF_PAGE | SIF_DISABLENOSCROLL;
        info.nPage = rc.right - rc.left;
        switch (code)
        {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            info.nPos = pos;
            break;
        case SB_TOP:
            info.nPos = 0;
            break;
        case SB_BOTTOM:
            info.nPos = m_bm.bmHeight;
            break;
        case SB_ENDSCROLL:
            return;
        case SB_LINEDOWN:
            info.nPos = GetScrollPos(hwnd, SB_HORZ) + 10;
            break;
        case SB_LINEUP:
            info.nPos = GetScrollPos(hwnd, SB_HORZ) - 10;
            break;
        case SB_PAGEDOWN:
            info.nPos = GetScrollPos(hwnd, SB_HORZ) + info.nPage;
            break;
        case SB_PAGEUP:
            info.nPos = GetScrollPos(hwnd, SB_HORZ) - info.nPage;
            break;
        }
        SetScrollInfo(hwnd, SB_HORZ, &info, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);
    }

    void OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        SCROLLINFO info;
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_POS | SIF_PAGE | SIF_DISABLENOSCROLL;
        info.nPage = rc.bottom - rc.top;
        switch (code)
        {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            info.nPos = pos;
            break;
        case SB_TOP:
            info.nPos = 0;
            break;
        case SB_BOTTOM:
            info.nPos = m_bm.bmHeight;
            break;
        case SB_ENDSCROLL:
            return;
        case SB_LINEDOWN:
            info.nPos = GetScrollPos(hwnd, SB_VERT) + 10;
            break;
        case SB_LINEUP:
            info.nPos = GetScrollPos(hwnd, SB_VERT) - 10;
            break;
        case SB_PAGEDOWN:
            info.nPos = GetScrollPos(hwnd, SB_VERT) + info.nPage;
            break;
        case SB_PAGEUP:
            info.nPos = GetScrollPos(hwnd, SB_VERT) - info.nPage;
            break;
        }
        SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);
    }

    void UpdateScrollInfo(HWND hwnd)
    {
        if (!GetObjectW(g_hBitmap, sizeof(m_bm), &m_bm))
            return;

        RECT rc;
        GetClientRect(hwnd, &rc);

        SCROLLINFO info;

        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        info.nMin = 0;
        info.nMax = m_bm.bmWidth;
        info.nPage = rc.right - rc.left;
        info.nPos = 0;
        SetScrollInfo(hwnd, SB_HORZ, &info, TRUE);

        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        info.nMin = 0;
        info.nMax = m_bm.bmHeight;
        info.nPage = rc.bottom - rc.top;
        info.nPos = 0;
        SetScrollInfo(hwnd, SB_VERT, &info, TRUE);

        InvalidateRect(hwnd, NULL, TRUE);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        if (id != 999)
            return;

        UpdateScrollInfo(hwnd);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        UpdateScrollInfo(hwnd);
        FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProcW);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
            HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
            HANDLE_MSG(hwnd, WM_HSCROLL, OnHScroll);
            HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        default:
            return DefaultProcDx();
        }
    }
};

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

struct MainWnd : WindowBase
{
    INT         m_argc;         // number of command line parameters
    TCHAR **    m_targv;        // command line parameters

    HINSTANCE   m_hInst;        // the instance handle
    HICON       m_hIcon;        // the icon handle
    HACCEL      m_hAccel;       // the accelerator handle
    HMENU       m_hMenu;
    HIMAGELIST  m_hImageList;
    HICON       m_hFileIcon;
    HICON       m_hFolderIcon;
    HWND        m_hToolBar;
    HWND        m_hBinEdit;
    HWND        m_hSrcEdit;
    BmpView     m_bmp_view;

    HFONT       m_hNormalFont;
    HFONT       m_hLargeFont;
    HFONT       m_hSmallFont;
    WCHAR       m_szDataFolder[MAX_PATH];
    WCHAR       m_szConstantsFile[MAX_PATH];
    WCHAR       m_szCppExe[MAX_PATH];
    WCHAR       m_szWindresExe[MAX_PATH];

    MainWnd(int argc, TCHAR **targv, HINSTANCE hInst) :
        m_argc(argc),
        m_targv(targv),
        m_hInst(hInst),
        m_hIcon(NULL),
        m_hAccel(NULL)
    {
        m_hMenu = NULL;
        m_hImageList = NULL;
        m_hFileIcon = NULL;
        m_hFolderIcon = NULL;
        m_hToolBar = NULL;
        m_hBinEdit = NULL;
        m_hSrcEdit = NULL;

        m_hNormalFont = NULL;
        m_hLargeFont = NULL;
        m_hSmallFont = NULL;
        m_szDataFolder[0] = 0;
        m_szConstantsFile[0] = 0;
        m_szCppExe[0] = 0;
        m_szWindresExe[0] = 0;
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        WindowBase::ModifyWndClassDx(wcx);
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
        m_hIcon = ::LoadIcon(m_hInst, MAKEINTRESOURCE(1));
        m_hAccel = ::LoadAccelerators(m_hInst, MAKEINTRESOURCE(1));

        if (!CreateWindowDx(NULL, g_szTitle, WS_OVERLAPPEDWINDOW, 0,
            CW_USEDEFAULT, CW_USEDEFAULT, 760, 480))
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
            if (!::TranslateAccelerator(m_hwnd, m_hAccel, &msg))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
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

        LoadStringW(g_hInstance, IDS_TITLE, g_szTitle, _countof(g_szTitle));
        LoadLangInfo();

        INT nRet = CheckData();
        if (nRet)
        {
            return FALSE;
        }

        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
            TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES |
            TVS_LINESATROOT | TVS_SHOWSELALWAYS;
        g_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
            WC_TREEVIEWW, NULL, dwStyle, 0, 0, 0, 0, hwnd,
            (HMENU)1, g_hInstance, NULL);
        if (g_hTreeView == NULL)
            return FALSE;

        m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 1);

        m_hFileIcon = LoadSmallIconDx(100);
        ImageList_AddIcon(m_hImageList, m_hFileIcon);
        m_hFolderIcon = LoadSmallIconDx(101);
        ImageList_AddIcon(m_hImageList, m_hFolderIcon);

        TreeView_SetImageList(g_hTreeView, m_hImageList, TVSIL_NORMAL);

        dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
            ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
        m_hBinEdit = CreateWindowExW(WS_EX_CLIENTEDGE,
            L"EDIT", NULL, dwStyle, 0, 0, 0, 0, hwnd,
            (HMENU)2, g_hInstance, NULL);
        if (m_hBinEdit == NULL)
            return FALSE;

        m_hToolBar = ToolBar_Create(hwnd);
        if (m_hToolBar == NULL)
            return FALSE;

        m_hSrcEdit = CreateWindowExW(WS_EX_CLIENTEDGE,
            L"EDIT", NULL, dwStyle, 0, 0, 0, 0, hwnd,
            (HMENU)3, g_hInstance, NULL);
        ShowWindow(m_hSrcEdit, FALSE);

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

        m_bmp_view.CreateDx(hwnd, 4);

        if (m_argc >= 2)
        {
            DoLoad(hwnd, m_targv[1]);
        }

        m_hMenu = GetMenu(hwnd);

        DragAcceptFiles(hwnd, TRUE);
        SetFocus(g_hTreeView);
        return TRUE;
    }

    VOID HidePreview(HWND hwnd)
    {
        if (g_hBitmap)
        {
            DeleteObject(g_hBitmap);
            g_hBitmap = NULL;
        }

        SetWindowTextW(m_hBinEdit, NULL);
        ShowWindow(m_hBinEdit, SW_HIDE);
        Edit_SetModify(m_hBinEdit, FALSE);

        SetWindowTextW(m_hSrcEdit, NULL);
        ShowWindow(m_hSrcEdit, SW_HIDE);
        Edit_SetModify(m_hSrcEdit, FALSE);

        ShowWindow(m_bmp_view, SW_HIDE);
        ShowWindow(m_hToolBar, SW_HIDE);

        PostMessageW(hwnd, WM_SIZE, 0, 0);

        g_bInEdit = FALSE;
    }

    void OnDeleteRes(HWND hwnd)
    {
        if (g_bInEdit)
            return;

        HTREEITEM hItem = TreeView_GetSelection(g_hTreeView);
        if (hItem == NULL)
            return;

        TV_Delete(g_hTreeView, hItem, g_Entries);
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
        ofn.lpstrTitle = LoadStringDx2(IDS_EXTRACTRES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"res";
        if (GetSaveFileNameW(&ofn))
        {
            if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
            {
                ResEntries selection;
                INT count = TV_GetSelection(g_hTreeView, selection, g_Entries);
                if (count && !DoExtractRes(hwnd, ofn.lpstrFile, selection))
                {
                    ErrorBoxDx(IDS_CANNOTSAVE);
                }
            }
            else
            {
                if (!DoExtractBin(ofn.lpstrFile, g_Entries[i]))
                {
                    ErrorBoxDx(IDS_CANNOTSAVE);
                }
            }
        }
    }

    void OnExtractIcon(HWND hwnd)
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
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = LoadStringDx2(IDS_EXTRACTICO);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"ico";
        if (GetSaveFileNameW(&ofn))
        {
            if (!DoExtractIcon(ofn.lpstrFile, g_Entries[i]))
            {
                ErrorBoxDx(IDS_CANTEXTRACTICO);
            }
        }
    }

    void OnExtractCursor(HWND hwnd)
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
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = LoadStringDx2(IDS_EXTRACTCUR);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"cur";
        if (GetSaveFileNameW(&ofn))
        {
            if (!DoExtractCursor(ofn.lpstrFile, g_Entries[i]))
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
        ofn.lpstrTitle = LoadStringDx2(IDS_EXTRACTBMP);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"bmp";
        if (GetSaveFileNameW(&ofn))
        {
            BOOL PNG;
            PNG = (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"png") == 0);
            if (!DoExtractBitmap(ofn.lpstrFile, g_Entries[i], PNG))
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
        ReplaceBinDlg dialog(g_Entries[i]);
        dialog.DialogBoxDx(hwnd, IDD_REPLACERES);
    }

    void OnSaveAs(HWND hwnd)
    {
        if (g_bInEdit)
            return;

        WCHAR File[MAX_PATH];

        lstrcpynW(File, g_szFile, _countof(File));
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        DWORD dwBinType;
        if (g_szFile[0] == UNICODE_NULL || !GetBinaryType(g_szFile, &dwBinType))
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
        ofn.lpstrTitle = LoadStringDx2(IDS_SAVEAS);
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
        const ResEntry& Entry = g_Entries[i];
        if (Entry.type == RT_DIALOG)
        {
            TestDialog dialog;
            dialog.DialogBoxIndirectDx(hwnd, Entry.ptr());
        }
        else if (Entry.type == RT_MENU)
        {
            HMENU hMenu = LoadMenuIndirect(&Entry[0]);
            if (hMenu)
            {
                TestMenuDlg dialog(hMenu);
                dialog.DialogBoxDx(hwnd, IDD_MENUTEST);
                DestroyMenu(hMenu);
            }
        }
    }

    void OnAddIcon(HWND hwnd)
    {
        AddIconDlg dialog;
        dialog.DialogBoxDx(hwnd, IDD_ADDICON);
    }

    void OnReplaceIcon(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_NAME)
            return;

        UINT i = LOWORD(lParam);
        ReplaceIconDlg dialog(g_Entries[i]);
        dialog.DialogBoxDx(hwnd, IDD_REPLACEICON);
    }

    void OnReplaceCursor(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_NAME)
            return;

        UINT i = LOWORD(lParam);
        ReplaceCursorDlg dialog(g_Entries[i]);
        dialog.DialogBoxDx(hwnd, IDD_REPLACECUR);
    }

    void OnOpen(HWND hwnd)
    {
        if (g_bInEdit)
            return;

        WCHAR File[MAX_PATH];
        lstrcpynW(File, g_szFile, _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXEFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_OPEN);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"exe";
        if (GetOpenFileNameW(&ofn))
        {
            DoLoad(hwnd, File);
        }
    }

    void OnAddBitmap(HWND hwnd)
    {
        AddBitmapDlg dialog;
        dialog.DialogBoxDx(hwnd, IDD_ADDBITMAP);
    }

    void OnReplaceBitmap(HWND hwnd)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        if (HIWORD(lParam) != I_NAME)
            return;

        UINT i = LOWORD(lParam);
        ReplaceBitmapDlg dialog(g_Entries[i]);
        dialog.DialogBoxDx(hwnd, IDD_REPLACEBMP);
    }

    void OnAddCursor(HWND hwnd)
    {
        AddCursorDlg dialog;
        dialog.DialogBoxDx(hwnd, IDD_ADDCURSOR);
    }

    void OnAddRes(HWND hwnd)
    {
        AddResDlg dialog;
        dialog.DialogBoxDx(hwnd, IDD_ADDRES);
    }

    void OnAbout(HWND hwnd)
    {
        MSGBOXPARAMSW Params;
        ZeroMemory(&Params, sizeof(Params));
        Params.cbSize = sizeof(Params);
        Params.hwndOwner = hwnd;
        Params.hInstance = g_hInstance;
        Params.lpszText = LoadStringDx(IDS_VERSIONINFO);
        Params.lpszCaption = g_szTitle;
        Params.dwStyle = MB_OK | MB_USERICON;
        Params.lpszIcon = MAKEINTRESOURCEW(1);
        Params.dwLanguageId = LANG_USER_DEFAULT;
        MessageBoxIndirectW(&Params);
    }

    void OnImport(HWND hwnd)
    {
        if (g_bInEdit)
            return;

        WCHAR File[MAX_PATH] = TEXT("");

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_IMPORTRES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"res";
        if (GetOpenFileNameW(&ofn))
        {
            ResEntries entries;
            if (DoImport(hwnd, File, entries))
            {
                BOOL Overwrite = TRUE;
                if (Res_Intersect(g_Entries, entries))
                {
                    INT nID = MessageBoxW(hwnd, LoadStringDx(IDS_EXISTSOVERWRITE),
                                          g_szTitle,
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
                    Res_AddEntry(g_Entries, entries[i], Overwrite);
                }

                TV_RefreshInfo(g_hTreeView, g_Entries);
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

        SelectTV(hwnd, lParam, TRUE);
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
    {
        if (pnmhdr->code == NM_DBLCLK)
        {
            OnEdit(hwnd);
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
        if (!g_bInEdit)
            return;

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

        INT cchText = GetWindowTextLengthW(m_hSrcEdit);
        std::wstring WideText;
        WideText.resize(cchText);
        GetWindowTextW(m_hSrcEdit, &WideText[0], cchText + 1);

        if (DoCompileParts(hwnd, WideText))
        {
            TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
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
        ResEntry& Entry = g_Entries[i];
        if (!Res_CanGuiEdit(Entry.type))
        {
            return;
        }

        if (!CompileIfNecessary(hwnd))
        {
            return;
        }

        const ResEntry::DataType& data = Entry.data;
        ByteStream stream(data);
        if (Entry.type == RT_ACCELERATOR)
        {
            EditAccelDlg dialog;
            if (dialog.m_accel_res.LoadFromStream(stream))
            {
                INT nID = dialog.DialogBoxDx(hwnd, IDD_EDITACCEL);
                if (nID == IDOK)
                {
                    dialog.m_accel_res.Update();
                    Entry.data = dialog.m_accel_res.data();
                    SelectTV(hwnd, lParam, FALSE);
                    return;
                }
            }
        }
        else if (Entry.type == RT_MENU)
        {
            MenuRes menu_res;
            if (menu_res.LoadFromStream(stream))
            {
                EditMenuDlg dialog(menu_res);
                INT nID = dialog.DialogBoxDx(hwnd, IDD_EDITMENU);
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
            // FIXME
        }
        else if (Entry.type == RT_STRING && HIWORD(lParam) == I_STRING)
        {
            ResEntries found;
            Res_Search(found, g_Entries, RT_STRING, (WORD)0, Entry.lang);

            StringRes str_res;
            ResEntries::iterator it, end = found.end();
            for (it = found.begin(); it != end; ++it)
            {
                ByteStream stream(it->data);
                if (!str_res.LoadFromStream(stream, it->name.m_ID))
                    return;
            }

            StringsDlg dialog(str_res);
            INT nID = dialog.DialogBoxDx(hwnd, IDD_STRINGS);
            if (nID == IDOK)
            {
                std::wstring WideText = str_res.Dump();
                SetWindowTextW(m_hSrcEdit, WideText.c_str());

                if (DoCompileParts(hwnd, WideText))
                {
                    TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
                    SelectTV(hwnd, lParam, FALSE);
                }
            }
        }
    }

    void OnNew(HWND hwnd)
    {
        if (g_bInEdit)
            return;

        DoSetFile(hwnd, NULL);
        g_Entries.clear();
        TV_RefreshInfo(g_hTreeView, g_Entries);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
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
        }
    }

    void OnDestroy(HWND hwnd)
    {
        DeleteObject(g_hBitmap);
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
                AddIconDlg dialog;
                dialog.File = File;
                dialog.DialogBoxDx(hwnd, IDD_ADDICON);
                return;
            }
            else if (lstrcmpiW(pch, L".cur") == 0)
            {
                AddCursorDlg dialog;
                dialog.File = File;
                dialog.DialogBoxDx(hwnd, IDD_ADDCURSOR);
                return;
            }
            else if (lstrcmpiW(pch, L".bmp") == 0 ||
                     lstrcmpiW(pch, L".png") == 0)
            {
                AddBitmapDlg dialog;
                dialog.File = File;
                dialog.DialogBoxDx(hwnd, IDD_ADDBITMAP);
                return;
            }
            else if (lstrcmpiW(pch, L".res") == 0)
            {
                DoLoad(hwnd, File);
                return;
            }
        }

        DoLoad(hwnd, File);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        SendMessageW(m_hToolBar, TB_AUTOSIZE, 0, 0);

        RECT ToolRect, ClientRect;

        GetClientRect(hwnd, &ClientRect);
        cx = ClientRect.right - ClientRect.left;
        cy = ClientRect.bottom - ClientRect.top ;

        INT x = 0, y = 0;
        if (g_bInEdit && ::IsWindowVisible(m_hToolBar))
        {
            GetWindowRect(m_hToolBar, &ToolRect);
            y += ToolRect.bottom - ToolRect.top;
            cy -= ToolRect.bottom - ToolRect.top;
        }

        if (::IsWindowVisible(g_hTreeView))
        {
            MoveWindow(g_hTreeView, x, y, TV_WIDTH, cy, TRUE);
            x += TV_WIDTH;
            cx -= TV_WIDTH;
        }

        if (IsWindowVisible(m_hSrcEdit))
        {
            if (::IsWindowVisible(m_hToolBar))
            {
                if (::IsWindowVisible(m_hBinEdit))
                {
                    MoveWindow(m_hSrcEdit, x, y, cx, cy - BE_HEIGHT, TRUE);
                    MoveWindow(m_hBinEdit, x, y + cy - BE_HEIGHT, cx, BE_HEIGHT, TRUE);
                }
                else
                {
                    MoveWindow(m_hSrcEdit, x, y, cx, cy, TRUE);
                }
            }
            else if (IsWindowVisible(m_bmp_view))
            {
                if (::IsWindowVisible(m_hBinEdit))
                {
                    MoveWindow(m_hSrcEdit, x, y, SE_WIDTH, cy - BE_HEIGHT, TRUE);
                    MoveWindow(m_bmp_view, x + SE_WIDTH, y, cx - SE_WIDTH, cy - BE_HEIGHT, TRUE);
                    MoveWindow(m_hBinEdit, x, y + cy - BE_HEIGHT, cx, BE_HEIGHT, TRUE);
                }
                else
                {
                    MoveWindow(m_hSrcEdit, x, y, SE_WIDTH, cy, TRUE);
                    MoveWindow(m_bmp_view, x + SE_WIDTH, y, cx - SE_WIDTH, cy, TRUE);
                }
            }
            else
            {
                if (::IsWindowVisible(m_hBinEdit))
                {
                    MoveWindow(m_hSrcEdit, x, y, cx, cy - BE_HEIGHT, TRUE);
                    MoveWindow(m_hBinEdit, x, y + cy - BE_HEIGHT, cx, BE_HEIGHT, TRUE);
                }
                else
                {
                    MoveWindow(m_hSrcEdit, x, y, cx, cy, TRUE);
                }
            }
        }
        else
        {
            if (::IsWindowVisible(m_hBinEdit))
            {
                MoveWindow(m_hBinEdit, x, y, cx, cy, TRUE);
            }
        }
    }

    void OnInitMenu(HWND hwnd, HMENU hMenu)
    {
        HTREEITEM hItem = TreeView_GetSelection(g_hTreeView);
        if (hItem == NULL || g_bInEdit)
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
        const ResEntry& Entry = g_Entries[i];

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
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hMenu == NULL || hSubMenu == NULL)
            return;

        ClientToScreen(hwndContext, &pt);

        SetForegroundWindow(hwndContext);
        INT id;
        UINT Flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
        id = TrackPopupMenu(hSubMenu, Flags, pt.x, pt.y, 0,
                            hwndContext, NULL);
        PostMessageW(hwndContext, WM_NULL, 0, 0);
        DestroyMenu(hMenu);

        if (id)
        {
            SendMessageW(hwnd, WM_COMMAND, id, 0);
        }
    }

    void PreviewIcon(HWND hwnd, const ResEntry& Entry)
    {
        BITMAP bm;
        g_hBitmap = CreateBitmapFromIconOrPngDx(hwnd, Entry, bm);

        std::wstring str = DumpBitmapInfo(g_hBitmap);
        SetWindowTextW(m_hSrcEdit, str.c_str());
        ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

        SendMessageW(m_bmp_view, WM_COMMAND, 999, 0);
        ShowWindow(m_bmp_view, SW_SHOWNOACTIVATE);
    }

    void PreviewCursor(HWND hwnd, const ResEntry& Entry)
    {
        BITMAP bm;
        HCURSOR hCursor = PackedDIB_CreateIcon(&Entry[0], Entry.size(), bm, FALSE);
        g_hBitmap = CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE);
        std::wstring str = DumpCursorInfo(bm);
        DestroyCursor(hCursor);

        SetWindowTextW(m_hSrcEdit, str.c_str());
        ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

        SendMessageW(m_bmp_view, WM_COMMAND, 999, 0);
        ShowWindow(m_bmp_view, SW_SHOWNOACTIVATE);
    }

    void PreviewGroupIcon(HWND hwnd, const ResEntry& Entry)
    {
        g_hBitmap = CreateBitmapFromIconsDx(hwnd, Entry);

        std::wstring str = DumpGroupIconInfo(Entry.data);
        SetWindowTextW(m_hSrcEdit, str.c_str());
        ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

        SendMessageW(m_bmp_view, WM_COMMAND, 999, 0);
        ShowWindow(m_bmp_view, SW_SHOWNOACTIVATE);
    }

    void PreviewGroupCursor(HWND hwnd, const ResEntry& Entry)
    {
        g_hBitmap = CreateBitmapFromCursorsDx(hwnd, Entry);
        assert(g_hBitmap);

        std::wstring str = DumpGroupCursorInfo(Entry.data);
        assert(str.size());
        SetWindowTextW(m_hSrcEdit, str.c_str());
        ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

        SendMessageW(m_bmp_view, WM_COMMAND, 999, 0);
        ShowWindow(m_bmp_view, SW_SHOWNOACTIVATE);
    }

    void PreviewBitmap(HWND hwnd, const ResEntry& Entry)
    {
        g_hBitmap = PackedDIB_CreateBitmap(&Entry[0], Entry.size());

        std::wstring str = DumpBitmapInfo(g_hBitmap);
        SetWindowTextW(m_hSrcEdit, str.c_str());
        ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

        SendMessageW(m_bmp_view, WM_COMMAND, 999, 0);
        ShowWindow(m_bmp_view, SW_SHOWNOACTIVATE);
    }

    void PreviewPNG(HWND hwnd, const ResEntry& Entry)
    {
        HBITMAP hbm = ii_png_load_mem(&Entry[0], Entry.size());
        if (hbm)
        {
            BITMAP bm;
            GetObject(hbm, sizeof(bm), &bm);
            g_hBitmap = Create24BppBitmapDx(bm.bmWidth, bm.bmHeight);
            if (g_hBitmap)
            {
                ii_fill(g_hBitmap, GetStockBrush(LTGRAY_BRUSH));
                ii_draw(g_hBitmap, hbm, 0, 0);
            }
            DeleteObject(hbm);
        }

        std::wstring str = DumpBitmapInfo(g_hBitmap);
        SetWindowTextW(m_hSrcEdit, str.c_str());
        ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

        SendMessageW(m_bmp_view, WM_COMMAND, 999, 0);
        ShowWindow(m_bmp_view, SW_SHOWNOACTIVATE);
    }


    void PreviewAccel(HWND hwnd, const ResEntry& Entry)
    {
        ByteStream stream(Entry.data);
        AccelRes accel;
        if (accel.LoadFromStream(stream))
        {
            std::wstring str = accel.Dump(Entry.name);
            SetWindowTextW(m_hSrcEdit, str.c_str());
            ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
        }
    }

    void PreviewMessage(HWND hwnd, const ResEntry& Entry)
    {
        ByteStream stream(Entry.data);
        MessageRes mes;
        if (mes.LoadFromStream(stream))
        {
            std::wstring str = mes.Dump();
            SetWindowTextW(m_hSrcEdit, str.c_str());
            ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
        }
    }

    void PreviewString(HWND hwnd, const ResEntry& Entry)
    {
        ByteStream stream(Entry.data);
        StringRes str_res;
        WORD nTableID = Entry.name.m_ID;
        if (str_res.LoadFromStream(stream, nTableID))
        {
            std::wstring str = str_res.Dump(nTableID);
            SetWindowTextW(m_hSrcEdit, str.c_str());
            ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
        }
    }

    void PreviewHtml(HWND hwnd, const ResEntry& Entry)
    {
        std::wstring str = BinaryToText(Entry.data);
        SetWindowTextW(m_hSrcEdit, str.c_str());
        ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
    }

    void PreviewMenu(HWND hwnd, const ResEntry& Entry)
    {
        ByteStream stream(Entry.data);
        MenuRes menu_res;
        if (menu_res.LoadFromStream(stream))
        {
            std::wstring str = menu_res.Dump(Entry.name, g_ConstantsDB);
            SetWindowTextW(m_hSrcEdit, str.c_str());
            ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
        }
    }

    void PreviewVersion(HWND hwnd, const ResEntry& Entry)
    {
        VersionRes ver_res;
        if (ver_res.LoadFromData(Entry.data))
        {
            std::wstring str = ver_res.Dump(Entry.name);
            SetWindowTextW(m_hSrcEdit, str.c_str());
            ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
        }
    }

    void PreviewDialog(HWND hwnd, const ResEntry& Entry)
    {
        ByteStream stream(Entry.data);
        DialogRes dialog_res;
        if (dialog_res.LoadFromStream(stream))
        {
            std::wstring str = dialog_res.Dump(Entry.name, g_ConstantsDB);
            SetWindowTextW(m_hSrcEdit, str.c_str());
            ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
        }
    }

    void PreviewStringTable(HWND hwnd, const ResEntry& Entry)
    {
        ResEntries found;
        Res_Search(found, g_Entries, RT_STRING, (WORD)0, Entry.lang);

        StringRes str_res;
        ResEntries::iterator it, end = found.end();
        for (it = found.begin(); it != end; ++it)
        {
            ByteStream stream(it->data);
            if (!str_res.LoadFromStream(stream, it->name.m_ID))
                return;
        }

        std::wstring str = str_res.Dump();
        SetWindowTextW(m_hSrcEdit, str.c_str());
        ShowWindow(m_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
    }

    void PreviewMessageTable(HWND hwnd, const ResEntry& Entry)
    {
    }

    void Preview(HWND hwnd, const ResEntry& Entry)
    {
        HidePreview(hwnd);

        std::wstring str = DumpDataAsString(Entry.data);
        SetWindowTextW(m_hBinEdit, str.c_str());
        if (str.empty())
            ShowWindow(m_hBinEdit, SW_HIDE);
        else
            ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);

        if (Entry.type == RT_ICON)
        {
            PreviewIcon(hwnd, Entry);
        }
        else if (Entry.type == RT_CURSOR)
        {
            PreviewCursor(hwnd, Entry);
        }
        else if (Entry.type == RT_GROUP_ICON)
        {
            PreviewGroupIcon(hwnd, Entry);
        }
        else if (Entry.type == RT_GROUP_CURSOR)
        {
            PreviewGroupCursor(hwnd, Entry);
        }
        else if (Entry.type == RT_BITMAP)
        {
            PreviewBitmap(hwnd, Entry);
        }
        else if (Entry.type == RT_ACCELERATOR)
        {
            PreviewAccel(hwnd, Entry);
        }
        else if (Entry.type == RT_STRING)
        {
            PreviewString(hwnd, Entry);
        }
        else if (Entry.type == RT_MENU)
        {
            PreviewMenu(hwnd, Entry);
        }
        else if (Entry.type == RT_DIALOG)
        {
            PreviewDialog(hwnd, Entry);
        }
        else if (Entry.type == RT_MESSAGETABLE)
        {
            PreviewMessage(hwnd, Entry);
        }
        else if (Entry.type == RT_MANIFEST || Entry.type == RT_HTML)
        {
            PreviewHtml(hwnd, Entry);
        }
        else if (Entry.type == RT_VERSION)
        {
            PreviewVersion(hwnd, Entry);
        }
        else if (Entry.type == L"PNG")
        {
            PreviewPNG(hwnd, Entry);
        }

        PostMessageW(hwnd, WM_SIZE, 0, 0);
    }

    void SelectTV(HWND hwnd, LPARAM lParam, BOOL DoubleClick)
    {
        HidePreview(hwnd);

        WORD i = LOWORD(lParam);
        ResEntry& Entry = g_Entries[i];

        if (HIWORD(lParam) == I_LANG)
        {
            Preview(hwnd, Entry);
        }
        else if (HIWORD(lParam) == I_STRING)
        {
            SetWindowTextW(m_hBinEdit, NULL);
            ShowWindow(m_hBinEdit, SW_HIDE);
            PreviewStringTable(hwnd, Entry);
        }
        else if (HIWORD(lParam) == I_MESSAGE)
        {
            SetWindowTextW(m_hBinEdit, NULL);
            ShowWindow(m_hBinEdit, SW_HIDE);
            PreviewMessageTable(hwnd, Entry);
        }

        if (DoubleClick)
        {
            SetWindowFont(m_hSrcEdit, m_hLargeFont, TRUE);
            Edit_SetReadOnly(m_hSrcEdit, FALSE);
            SetFocus(m_hSrcEdit);

            if (Res_CanGuiEdit(Entry.type))
            {
                if (Res_IsTestable(Entry.type))
                {
                    ToolBar_Update(m_hToolBar, 2);
                }
                else
                {
                    ToolBar_Update(m_hToolBar, 1);
                }
            }
            else
            {
                ToolBar_Update(m_hToolBar, 0);
            }
            ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);

            ShowWindow(m_hSrcEdit, SW_SHOWNOACTIVATE);
            ShowWindow(g_hTreeView, SW_HIDE);
            ShowWindow(m_hBinEdit, SW_HIDE);
            SetMenu(hwnd, NULL);

            g_bInEdit = TRUE;
        }
        else
        {
            SetWindowFont(m_hSrcEdit, m_hNormalFont, TRUE);
            Edit_SetReadOnly(m_hSrcEdit, TRUE);
            SetFocus(g_hTreeView);

            ShowWindow(m_hToolBar, SW_HIDE);
            ShowWindow(m_hSrcEdit, SW_SHOWNOACTIVATE);
            ShowWindow(g_hTreeView, SW_SHOWNOACTIVATE);
            SetMenu(hwnd, m_hMenu);

            g_bInEdit = FALSE;
        }

        PostMessageW(hwnd, WM_SIZE, 0, 0);
    }

    BOOL IsEditableEntry(HWND hwnd, LPARAM lParam)
    {
        const WORD i = LOWORD(lParam);
        const ResEntry& Entry = g_Entries[i];
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

    BOOL DoWindresResult(HWND hwnd, ResEntries& entries, std::string& msg)
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);

        if (HIWORD(lParam) == I_LANG)
        {
            WORD i = LOWORD(lParam);
            ResEntry& entry = g_Entries[i];

            if (entries.size() != 1 ||
                entries[0].name != entry.name ||
                entries[0].lang != entry.lang)
            {
                msg += WideToAnsi(LoadStringDx(IDS_RESMISMATCH));
                return FALSE;
            }
            entry = entries[0];
            return TRUE;
        }
        else if (HIWORD(lParam) == I_STRING)
        {
            WORD i = LOWORD(lParam);
            ResEntry& entry = g_Entries[i];

            Res_DeleteNames(g_Entries, RT_STRING, entry.lang);

            for (size_t m = 0; m < entries.size(); ++m)
            {
                if (!Res_AddEntry(g_Entries, entries[m], TRUE))
                {
                    msg += WideToAnsi(LoadStringDx(IDS_CANNOTADDRES));
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
        ResEntry& entry = g_Entries[i];

        std::string TextUtf8 = WideToUtf8(WideText);
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
                    std::string TextAnsi = WideToAnsi(WideText);
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
        std::string msg = WideToAnsi(LoadStringDx(IDS_CANNOTSTARTUP));
        output.assign((LPBYTE)msg.c_str(), (LPBYTE)msg.c_str() + msg.size());

        BOOL Success = FALSE;
        ByteStream stream;

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
                    std::string msg;
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
                SetWindowTextW(m_hBinEdit, LoadStringDx(IDS_COMPILEERROR));
                ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);
            }
            else
            {
                output.insert(output.end(), 0);
                SetWindowTextA(m_hBinEdit, (char *)&output[0]);
                ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);
            }
        }

        PostMessageW(hwnd, WM_SIZE, 0, 0);

        return Success;
    }

    BOOL CompileIfNecessary(HWND hwnd)
    {
        if (Edit_GetModify(m_hSrcEdit))
        {
            INT id = MessageBox(hwnd, LoadStringDx(IDS_COMPILENOW), g_szTitle,
                                MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                {
                    INT cchText = GetWindowTextLengthW(m_hSrcEdit);
                    std::wstring WideText;
                    WideText.resize(cchText);
                    GetWindowTextW(m_hSrcEdit, &WideText[0], cchText + 1);

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
        MainWnd app(argc, targv, hInstance);

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
