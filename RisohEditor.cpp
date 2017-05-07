#include "stdafx.hpp"

#pragma comment(lib, "msimg32.lib")

HINSTANCE   g_hInstance = NULL;
HWND        g_hMainWnd = NULL;
WCHAR       g_szTitle[MAX_PATH] = L"Resource Editor by katahiromz";
WCHAR       g_szBmpView[] = L"RisohEditor BmpView Class";
INT         g_argc = 0;
LPWSTR *    g_wargv = NULL;
WCHAR       g_szFile[MAX_PATH] = L"";
ConstantsDB g_ConstantsDB;
ResEntries  g_Entries;

HWND        g_hTreeView = NULL;
HWND        g_hBinEdit = NULL;
HWND        g_hSrcEdit = NULL;
HWND        g_hBmpView = NULL;
HWND        g_hToolBar = NULL;
BOOL        g_bInEdit = FALSE;

HIMAGELIST  g_hImageList = NULL;
HICON       g_hFileIcon = NULL;
HICON       g_hFolderIcon = NULL;


BITMAP      g_bm = { 0 };
HBITMAP     g_hBitmap = NULL;
HICON       g_hIcon = NULL;
HCURSOR     g_hCursor = NULL;

HFONT       g_hNormalFont = NULL;
HFONT       g_hSmallFont = NULL;

HACCEL      g_hAccel = NULL;

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

static LPCWSTR s_pszClassName = L"katahiromz's RisohEditor";

LPWSTR LoadStringDx(UINT id)
{
    static WCHAR s_sz[MAX_PATH];
    ZeroMemory(s_sz, sizeof(s_sz));
    LoadStringW(g_hInstance, id, s_sz, _countof(s_sz));
    return s_sz;
}

LPWSTR LoadStringDx2(UINT id)
{
    static WCHAR s_sz[MAX_PATH];
    ZeroMemory(s_sz, sizeof(s_sz));
    LoadStringW(g_hInstance, id, s_sz, _countof(s_sz));
    return s_sz;
}

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

void DoIt(HWND hwnd)
{
    #if 0
        DialogTemplate Template;

        DWORD dwStyle = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
        DWORD dwExStyle = 0;
        POINT pt = {0, 0};
        SIZE siz = {100, 100};
        Template.WriteHeader(dwStyle, dwExStyle, 0, pt, siz);
        Template.DoModal(hwnd);
    #endif
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

    return TRUE;
}

LPWSTR GetTempFileNameDx(LPCWSTR pszPrefix3Chars)
{
    static WCHAR TempFile[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    ::GetTempPathW(_countof(szPath), szPath);
    ::GetTempFileNameW(szPath, L"ERE", 0, TempFile);
    return TempFile;
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
    TV_RefreshInfo(g_hTreeView, g_Entries);
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
    TV_RefreshInfo(g_hTreeView, g_Entries);
    return TRUE;
}

BOOL DoAddIcon(HWND hwnd,
               const ID_OR_STRING& Name,
               WORD Lang,
               const std::wstring& IconFile)
{
    if (!Res_AddGroupIcon(g_Entries, Name, Lang, IconFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries);
    return TRUE;
}

BOOL DoReplaceIcon(HWND hwnd,
                   const ID_OR_STRING& Name,
                   WORD Lang,
                   const std::wstring& IconFile)
{
    if (!Res_AddGroupIcon(g_Entries, Name, Lang, IconFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries);
    return TRUE;
}

BOOL DoAddCursor(HWND hwnd,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& CurFile)
{
    if (!Res_AddGroupCursor(g_Entries, Name, Lang, CurFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries);
    return TRUE;
}

BOOL DoReplaceCursor(HWND hwnd,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& CurFile)
{
    if (!Res_AddGroupCursor(g_Entries, Name, Lang, CurFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries);
    return TRUE;
}

BOOL DoAddBitmap(HWND hwnd,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& BitmapFile)
{
    if (!Res_AddBitmap(g_Entries, Name, Lang, BitmapFile, FALSE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries);
    return TRUE;
}

BOOL DoReplaceBitmap(HWND hwnd,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& BitmapFile)
{
    if (!Res_AddBitmap(g_Entries, Name, Lang, BitmapFile, TRUE))
        return FALSE;
    TV_RefreshInfo(g_hTreeView, g_Entries);
    return TRUE;
}

HICON LoadSmallIconDx(UINT id)
{
    return HICON(LoadImageW(g_hInstance, MAKEINTRESOURCEW(id),
                            IMAGE_ICON, 16, 16, 0));
}

TBBUTTON g_buttons0[] =
{
    { -1, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_DELETE },
    { -1, ID_REPLACERES, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_REPLACE },
};

TBBUTTON g_buttons1[] =
{
    { -1, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_DELETE },
    { -1, ID_EDITRES, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_EDIT },
};

TBBUTTON g_buttons2[] =
{
    { -1, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_CANCELEDIT },
    { -1, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_COMPILE },
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
        if (pButtons[i].idCommand == 0)
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
        WS_CHILD | WS_VISIBLE | WS_BORDER | CCS_TOP | TBSTYLE_WRAPABLE | TBSTYLE_LIST,
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

BOOL MainWnd_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
        TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES |
        TVS_LINESATROOT | TVS_SHOWSELALWAYS;
    g_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
        WC_TREEVIEWW, NULL, dwStyle, 0, 0, 0, 0, hwnd,
        (HMENU)1, g_hInstance, NULL);
    if (g_hTreeView == NULL)
        return FALSE;

    g_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 1);

    g_hFileIcon = LoadSmallIconDx(100);
    ImageList_AddIcon(g_hImageList, g_hFileIcon);
    g_hFolderIcon = LoadSmallIconDx(101);
    ImageList_AddIcon(g_hImageList, g_hFolderIcon);

    TreeView_SetImageList(g_hTreeView, g_hImageList, TVSIL_NORMAL);

    dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
        ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
    g_hBinEdit = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", NULL, dwStyle, 0, 0, 0, 0, hwnd,
        (HMENU)2, g_hInstance, NULL);
    if (g_hBinEdit == NULL)
        return FALSE;

    g_hToolBar = ToolBar_Create(hwnd);
    if (g_hToolBar == NULL)
        return FALSE;

    g_hNormalFont = GetStockFont(DEFAULT_GUI_FONT);
    g_hSrcEdit = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", NULL, dwStyle, 0, 0, 0, 0, hwnd,
        (HMENU)3, g_hInstance, NULL);
    SetWindowFont(g_hSrcEdit, g_hNormalFont, TRUE);
    ShowWindow(g_hSrcEdit, FALSE);

    LOGFONTW lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = 10;
    lf.lfWeight = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lf.lfFaceName[0] = UNICODE_NULL;
    g_hSmallFont = CreateFontIndirectW(&lf);
    assert(g_hSmallFont);
    SetWindowFont(g_hBinEdit, g_hSmallFont, TRUE);

    dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
    g_hBmpView = CreateWindowExW(WS_EX_CLIENTEDGE,
        g_szBmpView, NULL, dwStyle, 0, 0, 0, 0, hwnd,
        (HMENU)4, g_hInstance, NULL);
    
    ShowWindow(g_hBmpView, FALSE);

    if (g_argc >= 2)
    {
        DoLoad(hwnd, g_wargv[1]);
    }

    DragAcceptFiles(hwnd, TRUE);
    SetFocus(g_hTreeView);
    return TRUE;
}

void MainDlg_OnDeleteRes(HWND hwnd)
{
    HTREEITEM hItem = TreeView_GetSelection(g_hTreeView);
    if (hItem == NULL)
        return;

    TV_Delete(g_hTreeView, hItem, g_Entries);
}

void MainDlg_OnExtractBin(HWND hwnd)
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
                MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTSAVE),
                            g_szTitle, MB_ICONERROR);
            }
        }
        else
        {
            if (!DoExtractBin(ofn.lpstrFile, g_Entries[i]))
            {
                MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTSAVE),
                            g_szTitle, MB_ICONERROR);
            }
        }
    }
}

void MainDlg_OnExtractIcon(HWND hwnd)
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
            MessageBoxW(hwnd, LoadStringDx(IDS_CANTEXTRACTICO),
                        g_szTitle, MB_ICONERROR);
        }
    }
}

void MainDlg_OnExtractCursor(HWND hwnd)
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
            MessageBoxW(hwnd, LoadStringDx(IDS_CANTEXTRACTCUR),
                        g_szTitle, MB_ICONERROR);
        }
    }
}

void MainDlg_OnExtractBitmap(HWND hwnd)
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
            MessageBoxW(hwnd, LoadStringDx(IDS_CANTEXTRACTBMP),
                        g_szTitle, MB_ICONERROR);
        }
    }
}

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

BOOL ReplaceBinDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    DragAcceptFiles(hwnd, TRUE);

    ResEntry& Entry = *(ResEntry *)lParam;

    // for Types
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    const ConstantsDB::TableType& Table = g_ConstantsDB.GetTable(L"RESOURCE");
    for (size_t i = 0; i < Table.size(); ++i)
    {
        WCHAR sz[MAX_PATH];
        wsprintfW(sz, L"%s (%lu)", Table[i].name.c_str(), Table[i].value);
        INT k = ComboBox_AddString(hCmb1, sz);
        if (Entry.type == WORD(Table[i].value))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
    }

    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    if (Entry.name.is_str())
    {
        SetWindowTextW(hCmb2, Entry.name.m_Str.c_str());
    }
    else
    {
        SetDlgItemInt(hwnd, cmb2, Entry.name.m_ID, FALSE);
    }

    // for Langs
    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    Cmb3_InsertLangItemsAndSelectLang(hCmb3, Entry.lang);

    return TRUE;
}

void ReplaceBinDlg_OnPsh1(HWND hwnd)
{
    WCHAR File[MAX_PATH];
    GetDlgItemText(hwnd, edt1, File, _countof(File));

    std::wstring strFile = File;
    trim(strFile);
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

BOOL Cmb1_CheckType(HWND hCmb1, ID_OR_STRING& Type)
{
    WCHAR szType[MAX_PATH];
    GetWindowTextW(hCmb1, szType, _countof(szType));
    std::wstring Str = szType;
    trim(Str);
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
    trim(Str);
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
    trim(Str);
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
    trim(Str);
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

void ReplaceBinDlg_OnOK(HWND hwnd)
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
        MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTREPLACE),
                    NULL, MB_ICONERROR);
        return;
    }

    EndDialog(hwnd, IDOK);
}

void ReplaceBinDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        ReplaceBinDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        ReplaceBinDlg_OnPsh1(hwnd);
        break;
    }
}

void ReplaceBinDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);
}

INT_PTR CALLBACK
ReplaceBinDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, ReplaceBinDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, ReplaceBinDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, ReplaceBinDlg_OnCommand);
    }
    return 0;
}

void MainDlg_OnReplaceBin(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(g_hTreeView);
    if (HIWORD(lParam) != I_NAME)
        return;

    UINT i = LOWORD(lParam);
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_REPLACERES), hwnd,
                    ReplaceBinDlgProc, (LPARAM)&g_Entries[i]);
}

void MainWnd_OnSaveAs(HWND hwnd)
{
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
                MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTSAVE), NULL, MB_ICONERROR);
            }
        }
        else
        {
            if (!DoSaveAs(hwnd, File))
            {
                MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTSAVE), NULL, MB_ICONERROR);
            }
        }
    }
}

INT_PTR CALLBACK
TestDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
    return 0;
}

INT_PTR CALLBACK
TestMenuDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HMENU s_hMenu = NULL;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        s_hMenu = (HMENU)lParam;
        SetMenu(hwnd, s_hMenu);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK: case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            break;
        }
        break;
    }
    return FALSE;
}

void MainDlg_OnTest(HWND hwnd)
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
        DialogBoxIndirectW(g_hInstance,
                           (LPDLGTEMPLATE)Entry.ptr(),
                           hwnd,
                           TestDialogProc);
    }
    else if (Entry.type == RT_MENU)
    {
        HMENU hMenu = LoadMenuIndirect(&Entry[0]);
        if (hMenu)
        {
            DialogBoxParamW(g_hInstance,
                            MAKEINTRESOURCEW(IDD_MENUTEST),
                            hwnd,
                            TestMenuDlgProc,
                            (LPARAM)hMenu);
            DestroyMenu(hMenu);
        }
    }
}

BOOL AddIconDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    if (lParam)
    {
        LPCWSTR File = (LPCWSTR)lParam;
        SetDlgItemTextW(hwnd, edt1, File);
        if (g_hIcon)
            DestroyIcon(g_hIcon);
        g_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), g_hIcon);
    }

    DragAcceptFiles(hwnd, TRUE);

    // for Langs
    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

    return TRUE;
}

void AddIconDlg_OnOK(HWND hwnd)
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
        INT id = MessageBoxW(hwnd, LoadStringDx(IDS_ALREADYEXISTS), g_szTitle,
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
            MessageBoxW(hwnd, LoadStringDx(IDS_CANTREPLACEICO),
                        NULL, MB_ICONERROR);
            return;
        }
    }
    else
    {
        if (!DoAddIcon(hwnd, Name, Lang, File))
        {
            MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTADDICON),
                        NULL, MB_ICONERROR);
            return;
        }
    }

    EndDialog(hwnd, IDOK);
}

void AddIconDlg_OnPsh1(HWND hwnd)
{
    WCHAR File[MAX_PATH];
    GetDlgItemText(hwnd, edt1, File, _countof(File));

    std::wstring strFile = File;
    trim(strFile);
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
        if (g_hIcon)
            DestroyIcon(g_hIcon);
        g_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), g_hIcon);
    }
}

void AddIconDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        AddIconDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        AddIconDlg_OnPsh1(hwnd);
        break;
    }
}

void AddIconDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);

    if (g_hIcon)
        DestroyIcon(g_hIcon);
    g_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
    Static_SetIcon(GetDlgItem(hwnd, ico1), g_hIcon);
}

INT_PTR CALLBACK
AddIconDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddIconDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, AddIconDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, AddIconDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnAddIcon(HWND hwnd)
{
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDICON), hwnd,
                    AddIconDialogProc, 0);
}

BOOL ReplaceIconDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    DragAcceptFiles(hwnd, TRUE);

    ResEntry& Entry = *(ResEntry *)lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);

    // for Name
    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    if (Entry.name.is_str())
    {
        SetWindowTextW(hCmb2, Entry.name.m_Str.c_str());
    }
    else
    {
        SetDlgItemInt(hwnd, cmb2, Entry.name.m_ID, FALSE);
    }
    EnableWindow(hCmb2, FALSE);

    // for Langs
    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

    return TRUE;
}

void ReplaceIconDlg_OnOK(HWND hwnd)
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
        MessageBoxW(hwnd, LoadStringDx(IDS_CANTREPLACEICO), NULL, MB_ICONERROR);
        return;
    }

    EndDialog(hwnd, IDOK);
}

void ReplaceIconDlg_OnPsh1(HWND hwnd)
{
    WCHAR File[MAX_PATH];
    GetDlgItemText(hwnd, edt1, File, _countof(File));

    std::wstring strFile = File;
    trim(strFile);
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
        if (g_hIcon)
            DestroyIcon(g_hIcon);
        g_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), g_hIcon);
    }
}

void ReplaceIconDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        ReplaceIconDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        ReplaceIconDlg_OnPsh1(hwnd);
        break;
    }
}

void ReplaceIconDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);

    if (g_hIcon)
        DestroyIcon(g_hIcon);
    g_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
    Static_SetIcon(GetDlgItem(hwnd, ico1), g_hIcon);
}

INT_PTR CALLBACK
ReplaceIconDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, ReplaceIconDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, ReplaceIconDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, ReplaceIconDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnReplaceIcon(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(g_hTreeView);
    if (HIWORD(lParam) != I_NAME)
        return;

    UINT i = LOWORD(lParam);
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_REPLACEICON), hwnd,
                    ReplaceIconDialogProc, (LPARAM)&g_Entries[i]);
}

BOOL ReplaceCursorDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    DragAcceptFiles(hwnd, TRUE);

    ResEntry& Entry = *(ResEntry *)lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);

    // for Name
    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    if (Entry.name.is_str())
    {
        SetWindowTextW(hCmb2, Entry.name.m_Str.c_str());
    }
    else
    {
        SetDlgItemInt(hwnd, cmb2, Entry.name.m_ID, FALSE);
    }
    EnableWindow(hCmb2, FALSE);

    // for Langs
    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

    return TRUE;
}

void ReplaceCursorDlg_OnOK(HWND hwnd)
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
        MessageBoxW(hwnd, LoadStringDx(IDS_CANTREPLACECUR),
                    NULL, MB_ICONERROR);
        return;
    }

    EndDialog(hwnd, IDOK);
}

void ReplaceCursorDlg_OnPsh1(HWND hwnd)
{
    WCHAR File[MAX_PATH];
    GetDlgItemText(hwnd, edt1, File, _countof(File));

    std::wstring strFile = File;
    trim(strFile);
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
        if (g_hCursor)
            DestroyCursor(g_hCursor);
        g_hCursor = LoadCursorFromFile(File);
        SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(g_hCursor));
    }
}

void ReplaceCursorDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        ReplaceCursorDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        ReplaceCursorDlg_OnPsh1(hwnd);
        break;
    }
}

void ReplaceCursorDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);

    if (g_hCursor)
        DestroyCursor(g_hCursor);
    g_hCursor = LoadCursorFromFile(File);
    SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(g_hCursor));
}

INT_PTR CALLBACK
ReplaceCursorDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, ReplaceCursorDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, ReplaceCursorDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, ReplaceCursorDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnReplaceCursor(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(g_hTreeView);
    if (HIWORD(lParam) != I_NAME)
        return;

    UINT i = LOWORD(lParam);
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_REPLACECUR), hwnd,
                    ReplaceCursorDialogProc, (LPARAM)&g_Entries[i]);
}

void MainWnd_OnOpen(HWND hwnd)
{
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

BOOL AddBitmapDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    if (lParam)
    {
        LPCWSTR File = (LPCWSTR)lParam;
        SetDlgItemTextW(hwnd, edt1, File);
    }

    DragAcceptFiles(hwnd, TRUE);

    // for Langs
    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

    return TRUE;
}

void AddBitmapDlg_OnPsh1(HWND hwnd)
{
    WCHAR File[MAX_PATH];
    GetDlgItemText(hwnd, edt1, File, _countof(File));

    std::wstring strFile = File;
    trim(strFile);
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

void AddBitmapDlg_OnOK(HWND hwnd)
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
        INT id = MessageBoxW(hwnd, LoadStringDx(IDS_ALREADYEXISTS), g_szTitle,
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
            MessageBoxW(hwnd, LoadStringDx(IDS_CANTREPLACEBMP),
                        NULL, MB_ICONERROR);
            return;
        }
    }
    else
    {
        if (!DoAddBitmap(hwnd, Name, Lang, File))
        {
            MessageBoxW(hwnd, LoadStringDx(IDS_CANTADDBMP),
                        NULL, MB_ICONERROR);
            return;
        }
    }

    EndDialog(hwnd, IDOK);
}

void AddBitmapDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        AddBitmapDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        AddBitmapDlg_OnPsh1(hwnd);
        break;
    }
}

void AddBitmapDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);
}

INT_PTR CALLBACK
AddBitmapDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddBitmapDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, AddBitmapDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, AddBitmapDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnAddBitmap(HWND hwnd)
{
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDBITMAP), hwnd,
                    AddBitmapDialogProc, 0);
}

BOOL ReplaceBitmapDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    DragAcceptFiles(hwnd, TRUE);

    ResEntry& Entry = *(ResEntry *)lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);

    // for Name
    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    if (Entry.name.is_str())
    {
        SetWindowTextW(hCmb2, Entry.name.m_Str.c_str());
    }
    else
    {
        SetDlgItemInt(hwnd, cmb2, Entry.name.m_ID, FALSE);
    }
    EnableWindow(hCmb2, FALSE);

    // for Langs
    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

    return TRUE;
}

void ReplaceBitmapDlg_OnOK(HWND hwnd)
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
        MessageBoxW(hwnd, LoadStringDx(IDS_CANTREPLACEBMP),
                    NULL, MB_ICONERROR);
        return;
    }

    EndDialog(hwnd, IDOK);
}

void ReplaceBitmapDlg_OnPsh1(HWND hwnd)
{
    WCHAR File[MAX_PATH];
    GetDlgItemText(hwnd, edt1, File, _countof(File));

    std::wstring strFile = File;
    trim(strFile);
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

void ReplaceBitmapDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        ReplaceBitmapDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        ReplaceBitmapDlg_OnPsh1(hwnd);
        break;
    }
}

void ReplaceBitmapDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);
}

INT_PTR CALLBACK
ReplaceBitmapDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, ReplaceBitmapDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, ReplaceBitmapDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, ReplaceBitmapDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnReplaceBitmap(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(g_hTreeView);
    if (HIWORD(lParam) != I_NAME)
        return;

    UINT i = LOWORD(lParam);
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_REPLACEBMP), hwnd,
                    ReplaceBitmapDialogProc, (LPARAM)&g_Entries[i]);
}

BOOL AddCursorDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    if (lParam)
    {
        LPCWSTR File = (LPCWSTR)lParam;
        SetDlgItemTextW(hwnd, edt1, File);
        if (g_hCursor)
            DestroyCursor(g_hCursor);
        g_hCursor = LoadCursorFromFile(File);
        SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(g_hCursor));
    }

    DragAcceptFiles(hwnd, TRUE);

    // for Langs
    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

    return TRUE;
}

void AddCursorDlg_OnOK(HWND hwnd)
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
        INT id = MessageBoxW(hwnd, LoadStringDx(IDS_ALREADYEXISTS), g_szTitle,
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
            MessageBoxW(hwnd, LoadStringDx(IDS_CANTREPLACECUR),
                        NULL, MB_ICONERROR);
            return;
        }
    }
    else
    {
        if (!DoAddCursor(hwnd, Name, Lang, File))
        {
            MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTADDCUR), NULL,
                        MB_ICONERROR);
            return;
        }
    }

    EndDialog(hwnd, IDOK);
}

void AddCursorDlg_OnPsh1(HWND hwnd)
{
    WCHAR File[MAX_PATH];
    GetDlgItemText(hwnd, edt1, File, _countof(File));

    std::wstring strFile = File;
    trim(strFile);
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
        if (g_hCursor)
            DestroyCursor(g_hCursor);
        g_hCursor = LoadCursorFromFile(File);
        SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(g_hCursor));
    }
}

void AddCursorDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        AddCursorDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        AddCursorDlg_OnPsh1(hwnd);
        break;
    }
}

void AddCursorDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);

    if (g_hCursor)
        DestroyCursor(g_hCursor);
    g_hCursor = LoadCursorFromFile(File);
    SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(g_hCursor));
}

INT_PTR CALLBACK
AddCursorDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddCursorDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, AddCursorDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, AddCursorDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnAddCursor(HWND hwnd)
{
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDCURSOR), hwnd,
                    AddCursorDialogProc, 0);
}

BOOL AddBinDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
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

void AddBinDlg_OnOK(HWND hwnd)
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

    BOOL Overwrite = FALSE;
    INT iEntry = Res_Find(g_Entries, Type, Name, Lang);
    if (iEntry != -1)
    {
        INT id = MessageBoxW(hwnd, LoadStringDx(IDS_ALREADYEXISTS), g_szTitle,
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
        if (!DoReplaceBin(hwnd, Type, Name, Lang, File))
        {
            MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTREPLACE),
                        NULL, MB_ICONERROR);
            return;
        }
    }
    else
    {
        if (!DoAddBin(hwnd, Type, Name, Lang, File))
        {
            MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTADDRES), NULL,
                        MB_ICONERROR);
            return;
        }
    }

    EndDialog(hwnd, IDOK);
}

void AddBinDlg_OnPsh1(HWND hwnd)
{
    WCHAR File[MAX_PATH];
    GetDlgItemText(hwnd, edt1, File, _countof(File));

    std::wstring strFile = File;
    trim(strFile);
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

void AddBinDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        AddBinDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        AddBinDlg_OnPsh1(hwnd);
        break;
    }
}

void AddBinDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);
}

INT_PTR CALLBACK
AddBinDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddBinDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, AddBinDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, AddBinDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnAddBin(HWND hwnd)
{
    DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDRES), hwnd,
               AddBinDlgProc);
}

void MainWnd_OnAddDialog(HWND hwnd)
{
}

void MainDlg_OnAbout(HWND hwnd)
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

void MainWnd_OnImport(HWND hwnd)
{
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
                INT nID = MessageBoxW(hwnd, LoadStringDx(IDS_ALREADYEXISTS),
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
            MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTIMPORT), NULL,
                        MB_ICONERROR);
        }
    }
}

void MainWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_NEW:
        DoSetFile(hwnd, NULL);
        g_Entries.clear();
        TV_RefreshInfo(g_hTreeView, g_Entries);
        break;
    case ID_OPEN:
        MainWnd_OnOpen(hwnd);
        break;
    case ID_SAVEAS:
        MainWnd_OnSaveAs(hwnd);
        break;
    case ID_IMPORT:
        MainWnd_OnImport(hwnd);
        break;
    case ID_EXIT:
        DestroyWindow(hwnd);
        break;
    case ID_ADDICON:
        MainWnd_OnAddIcon(hwnd);
        break;
    case ID_ADDCURSOR:
        MainWnd_OnAddCursor(hwnd);
        break;
    case ID_ADDBITMAP:
        MainWnd_OnAddBitmap(hwnd);
        break;
    case ID_ADDDIALOG:
        MainWnd_OnAddDialog(hwnd);
        break;
    case ID_ADDBIN:
        MainWnd_OnAddBin(hwnd);
        break;
    case ID_REPLACEICON:
        MainWnd_OnReplaceIcon(hwnd);
        break;
    case ID_REPLACECURSOR:
        break;
    case ID_REPLACEBITMAP:
        MainWnd_OnReplaceBitmap(hwnd);
        break;
    case ID_REPLACEBIN:
        MainDlg_OnReplaceBin(hwnd);
        break;
    case ID_DELETERES:
        MainDlg_OnDeleteRes(hwnd);
        break;
    case ID_EDITDIALOG:
        break;
    case ID_EXTRACTICON:
        MainDlg_OnExtractIcon(hwnd);
        break;
    case ID_EXTRACTCURSOR:
        MainDlg_OnExtractCursor(hwnd);
        break;
    case ID_EXTRACTBITMAP:
        MainDlg_OnExtractBitmap(hwnd);
        break;
    case ID_EXTRACTBIN:
        MainDlg_OnExtractBin(hwnd);
        break;
    case ID_ABOUT:
        MainDlg_OnAbout(hwnd);
        break;
    case ID_TEST:
        MainDlg_OnTest(hwnd);
        break;
    }
}

void MainWnd_OnDestroy(HWND hwnd)
{
    DeleteObject(g_hBitmap);
    DeleteObject(g_hNormalFont);
    DeleteObject(g_hSmallFont);
    ImageList_Destroy(g_hImageList);
    DestroyIcon(g_hFileIcon);
    DestroyIcon(g_hFolderIcon);
    PostQuitMessage(0);
}

void MainWnd_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH], *pch;

    DragQueryFileW(hdrop, 0, File, _countof(File));
    DragFinish(hdrop);

    pch = wcsrchr(File, L'.');
    if (pch)
    {
        if (lstrcmpiW(pch, L".ico") == 0)
        {
            DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDICON), hwnd,
                            AddIconDialogProc, (LPARAM)File);
            return;
        }
        else if (lstrcmpiW(pch, L".cur") == 0)
        {
            DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDCURSOR), hwnd,
                            AddCursorDialogProc, (LPARAM)File);
            return;
        }
        else if (lstrcmpiW(pch, L".bmp") == 0)
        {
            DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDBITMAP), hwnd,
                            AddBitmapDialogProc, (LPARAM)File);
            return;
        }
        else if (lstrcmpiW(pch, L".png") == 0)
        {
            DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDBITMAP), hwnd,
                            AddBitmapDialogProc, (LPARAM)File);
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

void MainWnd_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    SendMessageW(g_hToolBar, TB_AUTOSIZE, 0, 0);

    RECT ToolRect, ClientRect;

    GetWindowRect(g_hToolBar, &ToolRect);

    GetClientRect(hwnd, &ClientRect);
    cx = ClientRect.right - ClientRect.left;
    cy = ClientRect.bottom - ClientRect.top - (ToolRect.bottom - ToolRect.top);

#define TV_WIDTH 250
    MoveWindow(g_hTreeView, 0, 0, TV_WIDTH, cy, TRUE);

    if (IsWindowVisible(g_hSrcEdit))
    {
#define SE_WIDTH 256
#define BE_HEIGHT 100
        if (IsWindowVisible(g_hBmpView))
        {
            MoveWindow(g_hSrcEdit, TV_WIDTH, ToolRect.top, SE_WIDTH, cy - BE_HEIGHT, TRUE);
            MoveWindow(g_hBmpView, TV_WIDTH + SE_WIDTH, ToolRect.top, cx - (TV_WIDTH + SE_WIDTH), cy - BE_HEIGHT, TRUE);
            MoveWindow(g_hBinEdit, TV_WIDTH, ToolRect.top + cy - BE_HEIGHT, cx - TV_WIDTH, BE_HEIGHT, TRUE);
        }
        else
        {
            MoveWindow(g_hSrcEdit, TV_WIDTH, ToolRect.top, cx - TV_WIDTH, cy - BE_HEIGHT, TRUE);
            MoveWindow(g_hBinEdit, TV_WIDTH, ToolRect.top + cy - BE_HEIGHT, cx - TV_WIDTH, BE_HEIGHT, TRUE);
        }
    }
    else
    {
        MoveWindow(g_hBinEdit, TV_WIDTH, ToolRect.top, cx - TV_WIDTH, cy, TRUE);
    }
}

std::wstring
DumpDataAsString(const std::vector<BYTE>& data)
{
    std::wstring ret;
    WCHAR sz[64];
    DWORD addr, size = DWORD(data.size());

    if (data.empty())
    {
        ret = L"(Empty)";
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
CreateBitmapFromIconOrPng(HWND hwnd, const ResEntry& Entry, BITMAP& bm)
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
CreateBitmapFromCursor(HWND hwnd, const ResEntry& Entry, BITMAP& bm)
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
        HBITMAP hbmIcon = CreateBitmapFromIconOrPng(hwnd, IconEntry, bm);

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
        HBITMAP hbmIcon = CreateBitmapFromIconOrPng(hwnd, IconEntry, bm);

        ii_draw(hbm, hbmIcon, 0, y);
        y += bm.bmHeight;
    }

    return hbm;
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
        HBITMAP hbmCursor = CreateBitmapFromCursor(hwnd, CursorEntry, bm);
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
            HBITMAP hbmCursor = CreateBitmapFromCursor(hwnd, CursorEntry, bm);
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

VOID MainWnd_HidePreview(HWND hwnd)
{
    if (g_hBitmap)
    {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
    }

    SetWindowTextW(g_hBinEdit, NULL);
    Edit_SetModify(g_hBinEdit, FALSE);

    ShowWindow(g_hSrcEdit, SW_HIDE);
    Edit_SetModify(g_hSrcEdit, FALSE);
    
    ShowWindow(g_hBmpView, SW_HIDE);
    PostMessage(hwnd, WM_SIZE, 0, 0);
}

void MainWnd_PreviewIcon(HWND hwnd, const ResEntry& Entry)
{
    BITMAP bm;
    g_hBitmap = CreateBitmapFromIconOrPng(hwnd, Entry, bm);

    std::wstring str = DumpBitmapInfo(g_hBitmap);
    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

    SendMessageW(g_hBmpView, WM_COMMAND, 999, 0);
    ShowWindow(g_hBmpView, SW_SHOWNOACTIVATE);
}

void MainWnd_PreviewCursor(HWND hwnd, const ResEntry& Entry)
{
    BITMAP bm;
    HCURSOR hCursor = PackedDIB_CreateIcon(&Entry[0], Entry.size(), bm, FALSE);
    g_hBitmap = CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE);
    std::wstring str = DumpCursorInfo(bm);
    DestroyCursor(hCursor);

    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

    SendMessageW(g_hBmpView, WM_COMMAND, 999, 0);
    ShowWindow(g_hBmpView, SW_SHOWNOACTIVATE);
}

void MainWnd_PreviewGroupIcon(HWND hwnd, const ResEntry& Entry)
{
    g_hBitmap = CreateBitmapFromIconsDx(hwnd, Entry);

    std::wstring str = DumpGroupIconInfo(Entry.data);
    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

    SendMessageW(g_hBmpView, WM_COMMAND, 999, 0);
    ShowWindow(g_hBmpView, SW_SHOWNOACTIVATE);
}

void MainWnd_PreviewGroupCursor(HWND hwnd, const ResEntry& Entry)
{
    g_hBitmap = CreateBitmapFromCursorsDx(hwnd, Entry);
    assert(g_hBitmap);

    std::wstring str = DumpGroupCursorInfo(Entry.data);
    assert(str.size());
    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

    SendMessageW(g_hBmpView, WM_COMMAND, 999, 0);
    ShowWindow(g_hBmpView, SW_SHOWNOACTIVATE);
}

void MainWnd_PreviewBitmap(HWND hwnd, const ResEntry& Entry)
{
    g_hBitmap = PackedDIB_CreateBitmap(&Entry[0], Entry.size());

    std::wstring str = DumpBitmapInfo(g_hBitmap);
    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

    SendMessageW(g_hBmpView, WM_COMMAND, 999, 0);
    ShowWindow(g_hBmpView, SW_SHOWNOACTIVATE);
}

void MainWnd_PreviewPNG(HWND hwnd, const ResEntry& Entry)
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
    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

    SendMessageW(g_hBmpView, WM_COMMAND, 999, 0);
    ShowWindow(g_hBmpView, SW_SHOWNOACTIVATE);
}


void MainWnd_PreviewAccel(HWND hwnd, const ResEntry& Entry)
{
    ByteStream stream(Entry.data);
    AccelRes accel;
    if (accel.LoadFromStream(stream))
    {
        std::wstring str = accel.Dump(Entry.name);
        SetWindowTextW(g_hSrcEdit, str.c_str());
        ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
    }
}

void MainWnd_PreviewMessage(HWND hwnd, const ResEntry& Entry)
{
    ByteStream stream(Entry.data);
    MessageRes mes;
    if (mes.LoadFromStream(stream))
    {
        std::wstring str = mes.Dump();
        SetWindowTextW(g_hSrcEdit, str.c_str());
        ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
    }
}

void MainWnd_PreviewString(HWND hwnd, const ResEntry& Entry)
{
    ByteStream stream(Entry.data);
    StringRes str_res;
    WORD nTableID = Entry.name.m_ID;
    if (str_res.LoadFromStream(stream, nTableID))
    {
        std::wstring str = str_res.Dump(nTableID);
        SetWindowTextW(g_hSrcEdit, str.c_str());
        ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
    }
}

void MainWnd_PreviewHtml(HWND hwnd, const ResEntry& Entry)
{
    std::wstring str = BinaryToText(Entry.data);
    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
}

void MainWnd_PreviewMenu(HWND hwnd, const ResEntry& Entry)
{
    ByteStream stream(Entry.data);
    MenuRes menu_res;
    if (menu_res.LoadFromStream(stream))
    {
        std::wstring str = menu_res.Dump(Entry.name, g_ConstantsDB);
        SetWindowTextW(g_hSrcEdit, str.c_str());
        ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
    }
}

void MainWnd_PreviewVersion(HWND hwnd, const ResEntry& Entry)
{
    VersionRes ver_res;
    if (ver_res.LoadFromData(Entry.data))
    {
        std::wstring str = ver_res.Dump(Entry.name);
        SetWindowTextW(g_hSrcEdit, str.c_str());
        ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
    }
}

void MainWnd_PreviewDialog(HWND hwnd, const ResEntry& Entry)
{
    ByteStream stream(Entry.data);
    DialogRes dialog_res;
    if (dialog_res.LoadFromStream(stream))
    {
        std::wstring str = dialog_res.Dump(Entry.name, g_ConstantsDB);
        SetWindowTextW(g_hSrcEdit, str.c_str());
        ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
    }
}

void MainWnd_PreviewStringTable(HWND hwnd, const ResEntry& Entry)
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
    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));
}

void MainWnd_PreviewMessageTable(HWND hwnd, const ResEntry& Entry)
{
}

void MainWnd_Preview(HWND hwnd, const ResEntry& Entry)
{
    MainWnd_HidePreview(hwnd);

    std::wstring str = DumpDataAsString(Entry.data);
    SetWindowTextW(g_hBinEdit, str.c_str());

    if (Entry.type == RT_ICON)
    {
        MainWnd_PreviewIcon(hwnd, Entry);
    }
    else if (Entry.type == RT_CURSOR)
    {
        MainWnd_PreviewCursor(hwnd, Entry);
    }
    else if (Entry.type == RT_GROUP_ICON)
    {
        MainWnd_PreviewGroupIcon(hwnd, Entry);
    }
    else if (Entry.type == RT_GROUP_CURSOR)
    {
        MainWnd_PreviewGroupCursor(hwnd, Entry);
    }
    else if (Entry.type == RT_BITMAP)
    {
        MainWnd_PreviewBitmap(hwnd, Entry);
    }
    else if (Entry.type == RT_ACCELERATOR)
    {
        MainWnd_PreviewAccel(hwnd, Entry);
    }
    else if (Entry.type == RT_STRING)
    {
        MainWnd_PreviewString(hwnd, Entry);
    }
    else if (Entry.type == RT_MENU)
    {
        MainWnd_PreviewMenu(hwnd, Entry);
    }
    else if (Entry.type == RT_DIALOG)
    {
        MainWnd_PreviewDialog(hwnd, Entry);
    }
    else if (Entry.type == RT_MESSAGETABLE)
    {
        MainWnd_PreviewMessage(hwnd, Entry);
    }
#ifndef RT_MANIFEST
    #define RT_MANIFEST 24
#endif
    else if (Entry.type == RT_MANIFEST || Entry.type == RT_HTML)
    {
        MainWnd_PreviewHtml(hwnd, Entry);
    }
    else if (Entry.type == RT_VERSION)
    {
        MainWnd_PreviewVersion(hwnd, Entry);
    }
    else if (Entry.type == L"PNG")
    {
        MainWnd_PreviewPNG(hwnd, Entry);
    }

    PostMessage(hwnd, WM_SIZE, 0, 0);
}

LRESULT MainWnd_OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    if (pnmhdr->code == TVN_SELCHANGED)
    {
        NM_TREEVIEWW *pTV = (NM_TREEVIEWW *)pnmhdr;
        LPARAM lParam = pTV->itemNew.lParam;

        MainWnd_HidePreview(hwnd);

        if (HIWORD(lParam) == I_LANG)
        {
            WORD i = LOWORD(lParam);
            MainWnd_Preview(hwnd, g_Entries[i]);
        }
        else if (HIWORD(lParam) == I_STRING)
        {
            WORD i = LOWORD(lParam);
            MainWnd_PreviewStringTable(hwnd, g_Entries[i]);
        }
        else if (HIWORD(lParam) == I_MESSAGE)
        {
            WORD i = LOWORD(lParam);
            MainWnd_PreviewMessageTable(hwnd, g_Entries[i]);
        }
        PostMessage(hwnd, WM_SIZE, 0, 0);
    }
    if (pnmhdr->code == TVN_KEYDOWN)
    {
        TV_KEYDOWN *pTVKD = (TV_KEYDOWN *)pnmhdr;
        if (pTVKD->wVKey == VK_DELETE)
        {
            PostMessage(hwnd, WM_COMMAND, ID_DELETERES, 0);
        }
    }
    return 0;
}

void MainWnd_OnInitMenu(HWND hwnd, HMENU hMenu)
{
    HTREEITEM hItem = TreeView_GetSelection(g_hTreeView);
    if (hItem == NULL || g_bInEdit)
    {
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EDITDIALOG, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        return;
    }

    TV_ITEM Item;
    ZeroMemory(&Item, sizeof(Item));
    Item.mask = TVIF_PARAM;
    Item.hItem = hItem;
    TreeView_GetItem(g_hTreeView, &Item);

    UINT i = LOWORD(Item.lParam);
    const ResEntry& Entry = g_Entries[i];

    switch (HIWORD(Item.lParam))
    {
    case I_TYPE:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EDITDIALOG, MF_GRAYED);
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
        EnableMenuItem(hMenu, ID_EDITDIALOG, MF_GRAYED);
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

        if (Entry.type == RT_DIALOG)
        {
            EnableMenuItem(hMenu, ID_EDITDIALOG, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_EDITDIALOG, MF_GRAYED);
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
        EnableMenuItem(hMenu, ID_EDITDIALOG, MF_GRAYED);
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
        EnableMenuItem(hMenu, ID_EDITDIALOG, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
        break;
    }
}

void MainWnd_OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
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
    MainWnd_OnInitMenu(hwnd, hMenu);
    HMENU hSubMenu = GetSubMenu(hMenu, 0);
    if (hMenu == NULL || hSubMenu == NULL)
        return;

    ClientToScreen(hwndContext, &pt);

    SetForegroundWindow(hwndContext);
    INT id;
    UINT Flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
    id = TrackPopupMenu(hSubMenu, Flags, pt.x, pt.y, 0,
                        hwndContext, NULL);
    PostMessage(hwndContext, WM_NULL, 0, 0);
    DestroyMenu(hMenu);

    if (id)
    {
        SendMessageW(hwnd, WM_COMMAND, id, 0);
    }
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, MainWnd_OnCreate);
        HANDLE_MSG(hwnd, WM_COMMAND, MainWnd_OnCommand);
        HANDLE_MSG(hwnd, WM_DESTROY, MainWnd_OnDestroy);
        HANDLE_MSG(hwnd, WM_DROPFILES, MainWnd_OnDropFiles);
        HANDLE_MSG(hwnd, WM_SIZE, MainWnd_OnSize);
        HANDLE_MSG(hwnd, WM_NOTIFY, MainWnd_OnNotify);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU, MainWnd_OnContextMenu);
        HANDLE_MSG(hwnd, WM_INITMENU, MainWnd_OnInitMenu);

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
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

void BmpView_OnPaint(HWND hwnd)
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
        BitBlt(hDC, -dx, -dy, g_bm.bmWidth, g_bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
    }
    DeleteDC(hMemDC);
    EndPaint(hwnd, &ps);
}

BOOL BmpView_OnEraseBkgnd(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, GetStockBrush(COLOR_BACKGROUND));
    return TRUE;
}

void BmpView_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
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
        info.nPos = g_bm.bmHeight;
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

void BmpView_OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
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
        info.nPos = g_bm.bmHeight;
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

void BmpView_UpdateScrollInfo(HWND hwnd)
{
    if (!GetObjectW(g_hBitmap, sizeof(g_bm), &g_bm))
        return;

    RECT rc;
    GetClientRect(hwnd, &rc);

    SCROLLINFO info;

    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    info.nMin = 0;
    info.nMax = g_bm.bmWidth;
    info.nPage = rc.right - rc.left;
    info.nPos = 0;
    SetScrollInfo(hwnd, SB_HORZ, &info, TRUE);

    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    info.nMin = 0;
    info.nMax = g_bm.bmHeight;
    info.nPage = rc.bottom - rc.top;
    info.nPos = 0;
    SetScrollInfo(hwnd, SB_VERT, &info, TRUE);

    InvalidateRect(hwnd, NULL, TRUE);
}

void BmpView_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (id != 999)
        return;

    BmpView_UpdateScrollInfo(hwnd);
}

void BmpView_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    BmpView_UpdateScrollInfo(hwnd);
    FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProcW);
}

LRESULT CALLBACK
BmpViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_ERASEBKGND, BmpView_OnEraseBkgnd);
        HANDLE_MSG(hwnd, WM_PAINT, BmpView_OnPaint);
        HANDLE_MSG(hwnd, WM_HSCROLL, BmpView_OnHScroll);
        HANDLE_MSG(hwnd, WM_VSCROLL, BmpView_OnVScroll);
        HANDLE_MSG(hwnd, WM_COMMAND, BmpView_OnCommand);
        HANDLE_MSG(hwnd, WM_SIZE, BmpView_OnSize);
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
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

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    g_hInstance = hInstance;
    InitCommonControls();
    g_wargv = CommandLineToArgvW(GetCommandLineW(), &g_argc);

    LoadStringW(hInstance, IDS_TITLE, g_szTitle, _countof(g_szTitle));
    g_hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(1));

    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // load constants
    WCHAR szPath[MAX_PATH], *pch;
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    pch = wcsrchr(szPath, L'\\');
    lstrcpyW(pch, L"\\Constants.txt");
    if (!g_ConstantsDB.LoadFromFile(szPath))
    {
        lstrcpyW(pch, L"\\..\\Constants.txt");
        if (!g_ConstantsDB.LoadFromFile(szPath))
        {
            lstrcpyW(pch, L"\\..\\..\\Constants.txt");
            g_ConstantsDB.LoadFromFile(szPath);
        }
    }

    // get languages
    EnumSystemLocalesW(EnumLocalesProc, LCID_SUPPORTED);
    {
        LangEntry entry;
        entry.LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        entry.Str = LoadStringDx(IDS_NEUTRAL);
        g_Langs.push_back(entry);
    }
    std::sort(g_Langs.begin(), g_Langs.end());

    WNDCLASSW wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = HBRUSH(COLOR_3DFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(1);
    wc.lpszClassName = s_pszClassName;
    if (!RegisterClassW(&wc))
    {
        MessageBoxA(NULL, "RegisterClass failed", NULL, MB_ICONERROR);
        return 2;
    }
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = BmpViewWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wc.hbrBackground = GetStockBrush(LTGRAY_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szBmpView;
    if (!RegisterClassW(&wc))
    {
        MessageBoxA(NULL, "RegisterClass failed", NULL, MB_ICONERROR);
        return 3;
    }

    g_hMainWnd = CreateWindowW(s_pszClassName, g_szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 480,
        NULL, NULL, hInstance, NULL);
    if (g_hMainWnd == NULL)
    {
        MessageBoxA(NULL, "CreateWindow failed", NULL, MB_ICONERROR);
        return 1;
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (TranslateAccelerator(g_hMainWnd, g_hAccel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (SUCCEEDED(hRes))
    {
        CoUninitialize();
    }

    return INT(msg.wParam);
}
