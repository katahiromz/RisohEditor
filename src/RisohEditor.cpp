#include "stdafx.hpp"

#pragma comment(lib, "msimg32.lib")

HINSTANCE   g_hInstance = NULL;
HWND        g_hMainWnd = NULL;
HMENU       g_hMenu = NULL;
WCHAR       g_szTitle[MAX_PATH] = L"RisohEditor by katahiromz";
WCHAR       g_szBmpViewClass[]  = L"RisohEditor BmpView Class";
WCHAR       g_szRadBaseClass[]  = L"RisohEditor RAD Base Class";

INT         g_argc = 0;
LPWSTR *    g_wargv = NULL;
WCHAR       g_szFile[MAX_PATH] = L"";
ConstantsDB g_ConstantsDB;
ResEntries  g_Entries;

WCHAR       g_szDataFolder[MAX_PATH] = L"";
WCHAR       g_szConstantsFile[MAX_PATH] = L"";
WCHAR       g_szCppExe[MAX_PATH] = L"";
WCHAR       g_szWindresExe[MAX_PATH] = L"";

HWND        g_hTreeView = NULL;
HWND        g_hBinEdit = NULL;
HWND        g_hSrcEdit = NULL;
HWND        g_hBmpView = NULL;
HWND        g_hToolBar = NULL;
HWND        g_hRadBase = NULL;
HWND        g_hRadDialog = NULL;
BOOL        g_bInEdit = FALSE;

HIMAGELIST  g_hImageList = NULL;
HICON       g_hFileIcon = NULL;
HICON       g_hFolderIcon = NULL;

BITMAP      g_bm = { 0 };
HBITMAP     g_hBitmap = NULL;
HICON       g_hIcon = NULL;
HCURSOR     g_hCursor = NULL;

HFONT       g_hNormalFont = NULL;
HFONT       g_hLargeFont = NULL;
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

BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size)
{
    using namespace std;
    FILE *fp = _tfopen(filename, _T("wb"));
    int n = fwrite(pv, size, 1, fp);
    fclose(fp);
    return n == 1;
}

std::wstring str_vkey(WORD w)
{
    return g_ConstantsDB.GetName(L"VIRTUALKEYS", w);
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
    g_bInEdit = FALSE;

    return TRUE;
}

LPWSTR GetTempFileNameDx(LPCWSTR pszPrefix3Chars)
{
    static WCHAR TempFile[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    ::GetTempPathW(_countof(szPath), szPath);
    ::GetTempFileNameW(szPath, L"KRE", 0, TempFile);
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

HICON LoadSmallIconDx(UINT id)
{
    return HICON(LoadImageW(g_hInstance, MAKEINTRESOURCEW(id),
                            IMAGE_ICON, 16, 16, 0));
}

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

    g_hSrcEdit = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"EDIT", NULL, dwStyle, 0, 0, 0, 0, hwnd,
        (HMENU)3, g_hInstance, NULL);
    ShowWindow(g_hSrcEdit, FALSE);

    LOGFONTW lf;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    lf.lfFaceName[0] = UNICODE_NULL;

    lf.lfHeight = 11;
    g_hSmallFont = CreateFontIndirectW(&lf);
    assert(g_hSmallFont);

    lf.lfHeight = 13;
    g_hNormalFont = ::CreateFontIndirectW(&lf);
    assert(g_hNormalFont);

    lf.lfHeight = 15;
    g_hLargeFont = ::CreateFontIndirectW(&lf);
    assert(g_hLargeFont);

    SetWindowFont(g_hSrcEdit, g_hNormalFont, TRUE);
    SetWindowFont(g_hBinEdit, g_hSmallFont, TRUE);

    dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
    g_hBmpView = CreateWindowExW(WS_EX_CLIENTEDGE,
        g_szBmpViewClass, NULL, dwStyle, 0, 0, 0, 0, hwnd,
        (HMENU)4, g_hInstance, NULL);
    ShowWindow(g_hBmpView, FALSE);

    if (g_argc >= 2)
    {
        DoLoad(hwnd, g_wargv[1]);
    }

    g_hMenu = GetMenu(hwnd);

    DragAcceptFiles(hwnd, TRUE);
    SetFocus(g_hTreeView);
    return TRUE;
}

VOID MainWnd_HidePreview(HWND hwnd)
{
    if (g_hBitmap)
    {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
    }

    SetWindowTextW(g_hBinEdit, NULL);
    ShowWindow(g_hBinEdit, SW_HIDE);
    Edit_SetModify(g_hBinEdit, FALSE);

    SetWindowTextW(g_hSrcEdit, NULL);
    ShowWindow(g_hSrcEdit, SW_HIDE);
    Edit_SetModify(g_hSrcEdit, FALSE);

    ShowWindow(g_hBmpView, SW_HIDE);
    ShowWindow(g_hToolBar, SW_HIDE);

    PostMessageW(hwnd, WM_SIZE, 0, 0);

    g_bInEdit = FALSE;
}

void MainWnd_OnDeleteRes(HWND hwnd)
{
    if (g_bInEdit)
        return;

    HTREEITEM hItem = TreeView_GetSelection(g_hTreeView);
    if (hItem == NULL)
        return;

    TV_Delete(g_hTreeView, hItem, g_Entries);
    MainWnd_HidePreview(hwnd);
}

void MainWnd_OnExtractBin(HWND hwnd)
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

void MainWnd_OnExtractIcon(HWND hwnd)
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

void MainWnd_OnExtractCursor(HWND hwnd)
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

void MainWnd_OnExtractBitmap(HWND hwnd)
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

void MainWnd_OnReplaceBin(HWND hwnd)
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

void MainWnd_OnTest(HWND hwnd)
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
        INT id = MessageBoxW(hwnd, LoadStringDx(IDS_EXISTSOVERWRITE), g_szTitle,
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
        INT id = MessageBoxW(hwnd, LoadStringDx(IDS_EXISTSOVERWRITE), g_szTitle,
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
        INT id = MessageBoxW(hwnd, LoadStringDx(IDS_EXISTSOVERWRITE), g_szTitle,
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

BOOL AddResDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
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

void AddResDlg_OnOK(HWND hwnd)
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
            MessageBoxW(hwnd, LoadStringDx(IDS_ALREADYEXISTS), g_szTitle,
                        MB_ICONERROR);
            return;
        }

        INT id = MessageBoxW(hwnd, LoadStringDx(IDS_EXISTSOVERWRITE), g_szTitle,
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
    }

    EndDialog(hwnd, IDOK);
}

void AddResDlg_OnPsh1(HWND hwnd)
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

void AddResDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        AddResDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        AddResDlg_OnPsh1(hwnd);
        break;
    case cmb1:
        if (codeNotify == CBN_SELCHANGE)
        {
            
        }
        break;
    }
}

void AddResDlg_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR File[MAX_PATH];
    DragQueryFileW(hdrop, 0, File, _countof(File));
    SetDlgItemTextW(hwnd, edt1, File);
}

INT_PTR CALLBACK
AddResDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddResDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_DROPFILES, AddResDlg_OnDropFiles);
        HANDLE_MSG(hwnd, WM_COMMAND, AddResDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnAddRes(HWND hwnd)
{
    DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDRES), hwnd,
               AddResDlgProc);
}

void MainWnd_OnAbout(HWND hwnd)
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
            MessageBoxW(hwnd, LoadStringDx(IDS_CANNOTIMPORT), NULL,
                        MB_ICONERROR);
        }
    }
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

void MainWnd_PreviewGroupIcon(HWND hwnd, const ResEntry& Entry)
{
    g_hBitmap = CreateBitmapFromIconsDx(hwnd, Entry);

    std::wstring str = DumpGroupIconInfo(Entry.data);
    SetWindowTextW(g_hSrcEdit, str.c_str());
    ShowWindow(g_hSrcEdit, (str.empty() ? SW_HIDE : SW_SHOWNOACTIVATE));

    SendMessageW(g_hBmpView, WM_COMMAND, 999, 0);
    ShowWindow(g_hBmpView, SW_SHOWNOACTIVATE);
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

void MainWnd_Preview(HWND hwnd, const ResEntry& Entry)
{
    MainWnd_HidePreview(hwnd);

    std::wstring str = DumpDataAsString(Entry.data);
    SetWindowTextW(g_hBinEdit, str.c_str());
    if (str.empty())
        ShowWindow(g_hBinEdit, SW_HIDE);
    else
        ShowWindow(g_hBinEdit, SW_SHOWNOACTIVATE);

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

    PostMessageW(hwnd, WM_SIZE, 0, 0);
}

void MainWnd_SelectTV(HWND hwnd, LPARAM lParam, BOOL DoubleClick)
{
    MainWnd_HidePreview(hwnd);

    WORD i = LOWORD(lParam);
    ResEntry& Entry = g_Entries[i];

    if (HIWORD(lParam) == I_LANG)
    {
        MainWnd_Preview(hwnd, Entry);
    }
    else if (HIWORD(lParam) == I_STRING)
    {
        SetWindowTextW(g_hBinEdit, NULL);
        ShowWindow(g_hBinEdit, SW_HIDE);
        MainWnd_PreviewStringTable(hwnd, Entry);
    }
    else if (HIWORD(lParam) == I_MESSAGE)
    {
        SetWindowTextW(g_hBinEdit, NULL);
        ShowWindow(g_hBinEdit, SW_HIDE);
        MainWnd_PreviewMessageTable(hwnd, Entry);
    }

    if (DoubleClick)
    {
        SetWindowFont(g_hSrcEdit, g_hLargeFont, TRUE);
        Edit_SetReadOnly(g_hSrcEdit, FALSE);
        SetFocus(g_hSrcEdit);

        if (Res_CanGuiEdit(Entry.type))
        {
            if (Res_IsTestable(Entry.type))
            {
                ToolBar_Update(g_hToolBar, 2);
            }
            else
            {
                ToolBar_Update(g_hToolBar, 1);
            }
        }
        else
        {
            ToolBar_Update(g_hToolBar, 0);
        }
        ShowWindow(g_hToolBar, SW_SHOWNOACTIVATE);

        ShowWindow(g_hSrcEdit, SW_SHOWNOACTIVATE);
        ShowWindow(g_hTreeView, SW_HIDE);
        ShowWindow(g_hBinEdit, SW_HIDE);
        SetMenu(hwnd, NULL);

        g_bInEdit = TRUE;
    }
    else
    {
        SetWindowFont(g_hSrcEdit, g_hNormalFont, TRUE);
        Edit_SetReadOnly(g_hSrcEdit, TRUE);
        SetFocus(g_hTreeView);

        ShowWindow(g_hToolBar, SW_HIDE);
        ShowWindow(g_hSrcEdit, SW_SHOWNOACTIVATE);
        ShowWindow(g_hTreeView, SW_SHOWNOACTIVATE);
        SetMenu(hwnd, g_hMenu);

        g_bInEdit = FALSE;
    }

    PostMessageW(hwnd, WM_SIZE, 0, 0);
}

BOOL MainWnd_IsEditableEntry(HWND hwnd, LPARAM lParam)
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

void MainWnd_OnEdit(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(g_hTreeView);
    if (!MainWnd_IsEditableEntry(hwnd, lParam))
        return;

    MainWnd_SelectTV(hwnd, lParam, TRUE);
}

LRESULT MainWnd_OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    if (pnmhdr->code == NM_DBLCLK)
    {
        MainWnd_OnEdit(hwnd);
    }
    else if (pnmhdr->code == TVN_SELCHANGED)
    {
        NM_TREEVIEWW *pTV = (NM_TREEVIEWW *)pnmhdr;
        LPARAM lParam = pTV->itemNew.lParam;
        MainWnd_SelectTV(hwnd, lParam, FALSE);
    }
    else if (pnmhdr->code == TVN_KEYDOWN)
    {
        TV_KEYDOWN *pTVKD = (TV_KEYDOWN *)pnmhdr;
        switch (pTVKD->wVKey)
        {
        case VK_RETURN:
            MainWnd_OnEdit(hwnd);
            break;
        case VK_DELETE:
            PostMessageW(hwnd, WM_COMMAND, ID_DELETERES, 0);
            break;
        }
    }
    return 0;
}

void MainWnd_OnCancelEdit(HWND hwnd)
{
    if (!g_bInEdit)
        return;

    LPARAM lParam = TV_GetParam(g_hTreeView);
    MainWnd_SelectTV(hwnd, lParam, FALSE);
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

void ReplaceBackslash(LPWSTR szPath)
{
    for (WCHAR *pch = szPath; *pch; ++pch)
    {
        if (*pch == L'\\')
            *pch = L'/';
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
            MainWnd_SelectTV(hwnd, lParam, FALSE);

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
        g_szWindresExe, szPath3, g_szCppExe, szPath1);
#else
    wsprintfW(CmdLine,
        L"\"%s\" -DRC_INVOKED -o \"%s\" -J rc -O res "
        L"-F pe-i386 --preprocessor=\"%s\" --preprocessor-arg=\"-v\" \"%s\"",
        g_szWindresExe, szPath3, g_szCppExe, szPath1);
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
            SetWindowTextW(g_hBinEdit, LoadStringDx(IDS_COMPILEERROR));
            ShowWindow(g_hBinEdit, SW_SHOWNOACTIVATE);
        }
        else
        {
            output.insert(output.end(), 0);
            SetWindowTextA(g_hBinEdit, (char *)&output[0]);
            ShowWindow(g_hBinEdit, SW_SHOWNOACTIVATE);
        }
    }

    PostMessageW(hwnd, WM_SIZE, 0, 0);

    return Success;
}

void MainWnd_OnCompile(HWND hwnd)
{
    if (!Edit_GetModify(g_hSrcEdit))
    {
        LPARAM lParam = TV_GetParam(g_hTreeView);
        MainWnd_SelectTV(hwnd, lParam, FALSE);
        return;
    }

    INT cchText = GetWindowTextLengthW(g_hSrcEdit);
    std::wstring WideText;
    WideText.resize(cchText);
    GetWindowTextW(g_hSrcEdit, &WideText[0], cchText + 1);

    if (DoCompileParts(hwnd, WideText))
    {
        TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
        LPARAM lParam = TV_GetParam(g_hTreeView);
        MainWnd_SelectTV(hwnd, lParam, FALSE);
    }
}

std::wstring GetKeyID(UINT wId)
{
    return str_dec(wId);
}

BOOL EditAccelDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    AccelRes& accel_res = *(AccelRes *)lParam;

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
    const entries_type& entries = accel_res.Entries();

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

struct ACCEL_ENTRY
{
    WCHAR sz0[128];
    WCHAR sz1[16];
    WCHAR sz2[128];
};

void EditAccelDlg_OnUp(HWND hwnd)
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

void EditAccelDlg_OnDown(HWND hwnd)
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

void EditAccelDlg_OnDelete(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
    if (iItem >= 0)
    {
        ListView_DeleteItem(hCtl1, iItem);
    }
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

BOOL AddKeyDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    ACCEL_ENTRY& a_entry = *(ACCEL_ENTRY *)lParam;

    CheckDlgButton(hwnd, chx1, BST_CHECKED);

    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    Cmb1_InitVirtualKeys(hCmb1);

    return TRUE;
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

void AddKeyDlg_OnOK(HWND hwnd)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ACCEL_ENTRY& a_entry = *(ACCEL_ENTRY *)lParam;

    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    GetWindowTextW(hCmb1, a_entry.sz0, _countof(a_entry.sz0));

    std::wstring str = a_entry.sz0;
    BOOL bVirtKey = IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED;
    if (!Cmb1_CheckKey(hwnd, hCmb1, bVirtKey, str))
    {
        MessageBoxW(hwnd, LoadStringDx(IDS_INVALIDKEY), NULL, MB_ICONERROR);
        return;
    }
    lstrcpynW(a_entry.sz0, str.c_str(), _countof(a_entry.sz0));

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
    lstrcpynW(a_entry.sz1, str.c_str(), _countof(a_entry.sz1));

    GetDlgItemTextW(hwnd, cmb2, a_entry.sz2, _countof(a_entry.sz2));

    EndDialog(hwnd, IDOK);
}

void AddKeyDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        AddKeyDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
}

INT_PTR CALLBACK
AddKeyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddKeyDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, AddKeyDlg_OnCommand);
    }
    return 0;
}

void EditAccelDlg_OnAdd(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    ACCEL_ENTRY a_entry;
    if (IDOK != DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDKEY),
                                hwnd, AddKeyDlgProc, (LPARAM)&a_entry))
    {
        return;
    }

    INT iItem = ListView_GetItemCount(hCtl1);

    LV_ITEM item;

    ZeroMemory(&item, sizeof(item));
    item.iItem = iItem;
    item.mask = LVIF_TEXT;
    item.iSubItem = 0;
    item.pszText = a_entry.sz0;
    ListView_InsertItem(hCtl1, &item);

    ZeroMemory(&item, sizeof(item));
    item.iItem = iItem;
    item.mask = LVIF_TEXT;
    item.iSubItem = 1;
    item.pszText = a_entry.sz1;
    ListView_SetItem(hCtl1, &item);

    ZeroMemory(&item, sizeof(item));
    item.iItem = iItem;
    item.mask = LVIF_TEXT;
    item.iSubItem = 2;
    item.pszText = a_entry.sz2;
    ListView_SetItem(hCtl1, &item);

    UINT state = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItemState(hCtl1, iItem, state, state);
}

BOOL ModifyKeyDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    const ACCEL_ENTRY& a_entry = *(const ACCEL_ENTRY *)lParam;

    SetDlgItemTextW(hwnd, cmb1, a_entry.sz0);
    SetDlgItemTextW(hwnd, cmb2, a_entry.sz2);

    WORD Flags;
    SetKeyFlags(Flags, a_entry.sz1);
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

        INT i = ComboBox_FindStringExact(hCmb1, -1, a_entry.sz0);
        if (i != CB_ERR)
        {
            ComboBox_SetCurSel(hCmb1, i);
        }
    }

    return TRUE;
}

void ModifyKeyDlg_OnOK(HWND hwnd)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ACCEL_ENTRY& a_entry = *(ACCEL_ENTRY *)lParam;

    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    GetWindowTextW(hCmb1, a_entry.sz0, _countof(a_entry.sz0));

    std::wstring str = a_entry.sz0;
    BOOL bVirtKey = IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED;
    if (!Cmb1_CheckKey(hwnd, hCmb1, bVirtKey, str))
    {
        MessageBoxW(hwnd, LoadStringDx(IDS_INVALIDKEY), NULL, MB_ICONERROR);
        return;
    }
    lstrcpynW(a_entry.sz0, str.c_str(), _countof(a_entry.sz0));

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
    lstrcpynW(a_entry.sz1, str.c_str(), _countof(a_entry.sz1));

    GetDlgItemTextW(hwnd, cmb2, a_entry.sz2, _countof(a_entry.sz2));

    EndDialog(hwnd, IDOK);
}

void ModifyKeyDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
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
        ModifyKeyDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
}

INT_PTR CALLBACK
ModifyKeyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, ModifyKeyDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, ModifyKeyDlg_OnCommand);
    }
    return 0;
}

void EditAccelDlg_OnModify(HWND hwnd)
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

    if (IDOK == DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_MODIFYKEY),
                                hwnd, ModifyKeyDlgProc, (LPARAM)&a_entry))
    {
        ListView_SetItemText(hCtl1, iItem, 0, a_entry.sz0);
        ListView_SetItemText(hCtl1, iItem, 1, a_entry.sz1);
        ListView_SetItemText(hCtl1, iItem, 2, a_entry.sz2);
    }
}

void EditAccelDlg_OnOK(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT i, Count = ListView_GetItemCount(hCtl1);

    if (Count == 0)
    {
        MessageBox(hwnd, LoadStringDx(IDS_DATAISEMPTY), NULL, MB_ICONERROR);
        return;
    }

    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    AccelRes& accel_res = *(AccelRes *)lParam;
    accel_res.Entries().clear();
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

        accel_res.Entries().push_back(entry);
    }

    EndDialog(hwnd, IDOK);
}

void EditAccelDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case psh1:
        EditAccelDlg_OnAdd(hwnd);
        break;
    case psh2:
        EditAccelDlg_OnModify(hwnd);
        break;
    case psh3:
        EditAccelDlg_OnDelete(hwnd);
        break;
    case psh4:
        EditAccelDlg_OnUp(hwnd);
        break;
    case psh5:
        EditAccelDlg_OnDown(hwnd);
        break;
    case IDOK:
        EditAccelDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
}

LRESULT EditAccelDlg_OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    if (idFrom == ctl1)
    {
        if (pnmhdr->code == LVN_KEYDOWN)
        {
            LV_KEYDOWN *KeyDown = (LV_KEYDOWN *)pnmhdr;
            if (KeyDown->wVKey == VK_DELETE)
            {
                EditAccelDlg_OnDelete(hwnd);
                return 0;
            }
        }
        if (pnmhdr->code == NM_DBLCLK)
        {
            EditAccelDlg_OnModify(hwnd);
            return 0;
        }
    }
    return 0;
}

INT_PTR CALLBACK
EditAccelDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, EditAccelDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, EditAccelDlg_OnCommand);
        HANDLE_MSG(hwnd, WM_NOTIFY, EditAccelDlg_OnNotify);
    }
    return 0;
}

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

BOOL StringsDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    StringRes& str_res = *(StringRes *)lParam;

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
    const map_type& map = str_res.map();

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

BOOL AddStrDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    STRING_ENTRY& s_entry = *(STRING_ENTRY *)lParam;

    return TRUE;
}

void AddStrDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    STRING_ENTRY& s_entry = *(STRING_ENTRY *)lParam;
    switch (id)
    {
    case IDOK:
        StrDlg_GetEntry(hwnd, s_entry);
        EndDialog(hwnd, IDOK);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
}

INT_PTR CALLBACK
AddStrDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddStrDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, AddStrDlg_OnCommand);
    }
    return 0;
}

void StringsDlg_OnAdd(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    STRING_ENTRY s_entry;
    ZeroMemory(&s_entry, sizeof(s_entry));
    INT nID = DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDSTR),
                              hwnd, AddStrDlgProc, (LPARAM)&s_entry);
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

void StringsDlg_OnDelete(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
    if (iItem >= 0)
    {
        ListView_DeleteItem(hCtl1, iItem);
    }
}

BOOL ModifyStrDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    STRING_ENTRY& s_entry = *(STRING_ENTRY *)lParam;

    StrDlg_SetEntry(hwnd, s_entry);

    return TRUE;
}

void ModifyStrDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    STRING_ENTRY& s_entry = *(STRING_ENTRY *)lParam;
    switch (id)
    {
    case IDOK:
        StrDlg_GetEntry(hwnd, s_entry);
        EndDialog(hwnd, IDOK);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
}

INT_PTR CALLBACK
ModifyStrDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, ModifyStrDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, ModifyStrDlg_OnCommand);
    }
    return 0;
}

void StringsDlg_GetEntry(HWND hwnd, HWND hCtl1, INT iItem, STRING_ENTRY& entry)
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

void StringsDlg_OnModify(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
    if (iItem < 0)
    {
        return;
    }

    STRING_ENTRY s_entry;
    StringsDlg_GetEntry(hwnd, hCtl1, iItem, s_entry);

    INT nID = DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_MODIFYSTR),
                              hwnd, ModifyStrDlgProc, (LPARAM)&s_entry);
    if (IDOK == nID)
    {
        ListView_SetItemText(hCtl1, iItem, 0, s_entry.StringID);

        std::wstring str = str_quote(s_entry.StringValue);
        ListView_SetItemText(hCtl1, iItem, 1, &str[0]);
    }
}

void StringsDlg_OnOK(HWND hwnd)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    StringRes& str_res = *(StringRes *)lParam;

    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT iItem, Count = ListView_GetItemCount(hCtl1);
    if (Count == 0)
    {
        MessageBox(hwnd, LoadStringDx(IDS_DATAISEMPTY), NULL, MB_ICONERROR);
        return;
    }

    str_res.map().clear();

    STRING_ENTRY s_entry;
    for (iItem = 0; iItem < Count; ++iItem)
    {
        StringsDlg_GetEntry(hwnd, hCtl1, iItem, s_entry);

        WORD wID = (WORD)_wtoi(s_entry.StringID);
        std::wstring str = s_entry.StringValue;

        str_res.map().insert(std::make_pair(wID, str));
    }

    EndDialog(hwnd, IDOK);
}

void StringsDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case psh1:
        StringsDlg_OnAdd(hwnd);
        break;
    case psh2:
        StringsDlg_OnModify(hwnd);
        break;
    case psh3:
        StringsDlg_OnDelete(hwnd);
        break;
    case IDOK:
        StringsDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
}

LRESULT StringsDlg_OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    if (idFrom == ctl1)
    {
        if (pnmhdr->code == LVN_KEYDOWN)
        {
            LV_KEYDOWN *KeyDown = (LV_KEYDOWN *)pnmhdr;
            if (KeyDown->wVKey == VK_DELETE)
            {
                StringsDlg_OnDelete(hwnd);
                return 0;
            }
        }
        if (pnmhdr->code == NM_DBLCLK)
        {
            StringsDlg_OnModify(hwnd);
            return 0;
        }
    }
    return 0;
}

INT_PTR CALLBACK
StringsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, StringsDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, StringsDlg_OnCommand);
        HANDLE_MSG(hwnd, WM_NOTIFY, StringsDlg_OnNotify);
    }
    return 0;
}

struct MENU_ENTRY
{
    WCHAR Caption[128];
    WCHAR Flags[64];
    WCHAR CommandID[64];
    WCHAR HelpID[64];
    WORD wDepth;
};

BOOL EditMenuDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    MenuRes& menu_res = *(MenuRes *)lParam;

    if (menu_res.IsExtended())
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
    if (menu_res.IsExtended())
    {
        typedef MenuRes::ExMenuItemsType exitems_type;
        exitems_type& exitems = menu_res.exitems();
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
        items_type& items = menu_res.items();
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

BOOL AddMItemDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    MENU_ENTRY& m_entry = *(MENU_ENTRY *)lParam;

    SetDlgItemInt(hwnd, cmb2, 0, TRUE);
    SetDlgItemInt(hwnd, edt1, 0, TRUE);

    return TRUE;
}

void AddMItemDlg_OnOK(HWND hwnd)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    MENU_ENTRY& m_entry = *(MENU_ENTRY *)lParam;

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

void AddMItemDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        AddMItemDlg_OnOK(hwnd);
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

INT_PTR CALLBACK
AddMItemDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddMItemDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, AddMItemDlg_OnCommand);
    }
    return 0;
}

void EditMenuDlg_OnAdd(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    MENU_ENTRY m_entry;
    ZeroMemory(&m_entry, sizeof(m_entry));
    INT nID = DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ADDMITEM),
                              hwnd, AddMItemDlgProc, (LPARAM)&m_entry);
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

BOOL ModifyMItemDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
    MENU_ENTRY& m_entry = *(MENU_ENTRY *)lParam;

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

void ModifyMItemDlg_OnOK(HWND hwnd)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    MENU_ENTRY& m_entry = *(MENU_ENTRY *)lParam;

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

void ModifyMItemDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        ModifyMItemDlg_OnOK(hwnd);
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

INT_PTR CALLBACK
ModifyMItemDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, ModifyMItemDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, ModifyMItemDlg_OnCommand);
    }
    return 0;
}

BOOL EditMenuDlg_GetEntry(HWND hwnd, HWND hCtl1, MENU_ENTRY& entry, INT iItem)
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

BOOL EditMenuDlg_SetEntry(HWND hwnd, HWND hCtl1, MENU_ENTRY& entry, INT iItem)
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

void EditMenuDlg_OnModify(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
    if (iItem < 0)
    {
        return;
    }

    MENU_ENTRY m_entry;
    EditMenuDlg_GetEntry(hwnd, hCtl1, m_entry, iItem);

    INT nID = DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_MODIFYMITEM),
                              hwnd, ModifyMItemDlgProc, (LPARAM)&m_entry);
    if (IDOK == nID)
    {
        EditMenuDlg_SetEntry(hwnd, hCtl1, m_entry, iItem);
    }
}

void EditMenuDlg_OnDelete(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
    if (iItem >= 0)
    {
        ListView_DeleteItem(hCtl1, iItem);
    }
}

void EditMenuDlg_OnUp(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
    if (iItem <= 0)
        return;

    MENU_ENTRY entry0, entry1;

    EditMenuDlg_GetEntry(hwnd, hCtl1, entry0, iItem - 1);
    EditMenuDlg_GetEntry(hwnd, hCtl1, entry1, iItem);

    EditMenuDlg_SetEntry(hwnd, hCtl1, entry1, iItem - 1);
    EditMenuDlg_SetEntry(hwnd, hCtl1, entry0, iItem);

    UINT state = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItemState(hCtl1, iItem - 1, state, state);
}

void EditMenuDlg_OnDown(HWND hwnd)
{
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);

    INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
    if (iItem < 0)
        return;

    INT Count = ListView_GetItemCount(hCtl1);
    if (iItem + 1 >= Count)
        return;

    MENU_ENTRY entry0, entry1;

    EditMenuDlg_GetEntry(hwnd, hCtl1, entry0, iItem);
    EditMenuDlg_GetEntry(hwnd, hCtl1, entry1, iItem + 1);

    EditMenuDlg_SetEntry(hwnd, hCtl1, entry1, iItem);
    EditMenuDlg_SetEntry(hwnd, hCtl1, entry0, iItem + 1);

    UINT state = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItemState(hCtl1, iItem + 1, state, state);
}

void EditMenuDlg_OnLeft(HWND hwnd)
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

void EditMenuDlg_OnRight(HWND hwnd)
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

void EditMenuDlg_OnOK(HWND hwnd)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    MenuRes& menu_res = *(MenuRes *)lParam;

    MENU_ENTRY entry;
    HWND hCtl1 = GetDlgItem(hwnd, ctl1);
    INT iItem, Count = ListView_GetItemCount(hCtl1);

    if (Count == 0)
    {
        MessageBox(hwnd, LoadStringDx(IDS_DATAISEMPTY), NULL, MB_ICONERROR);
        return;
    }

    BOOL Extended = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
    if (Extended)
    {
        menu_res.header().wVersion = 1;
        menu_res.header().wOffset = 4;
        menu_res.header().dwHelpId = 0;
        menu_res.exitems().clear();
        for (iItem = 0; iItem < Count; ++iItem)
        {
            EditMenuDlg_GetEntry(hwnd, hCtl1, entry, iItem);

            MenuRes::ExMenuItem exitem;

            SetMenuTypeAndState(exitem.dwType, exitem.dwState, entry.Flags);
            exitem.menuId = _wtoi(entry.CommandID);
            exitem.bResInfo = 0;
            exitem.text = entry.Caption;
            exitem.dwHelpId = _wtoi(entry.HelpID);
            exitem.wDepth = entry.wDepth;

            menu_res.exitems().push_back(exitem);
        }
    }
    else
    {
        menu_res.header().wVersion = 0;
        menu_res.header().wOffset = 4;
        menu_res.header().dwHelpId = 0;
        menu_res.items().clear();
        for (iItem = 0; iItem < Count; ++iItem)
        {
            EditMenuDlg_GetEntry(hwnd, hCtl1, entry, iItem);

            MenuRes::MenuItem item;

            SetMenuFlags(item.fItemFlags, entry.Flags);
            item.wMenuID = _wtoi(entry.CommandID);
            item.wDepth = entry.wDepth;
            item.text = entry.Caption;

            menu_res.items().push_back(item);
        }
    }

    EndDialog(hwnd, IDOK);
}

void EditMenuDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case psh1:
        EditMenuDlg_OnAdd(hwnd);
        break;
    case psh2:
        EditMenuDlg_OnModify(hwnd);
        break;
    case psh3:
        EditMenuDlg_OnDelete(hwnd);
        break;
    case psh4:
        EditMenuDlg_OnUp(hwnd);
        break;
    case psh5:
        EditMenuDlg_OnDown(hwnd);
        break;
    case psh6:
        EditMenuDlg_OnLeft(hwnd);
        break;
    case psh7:
        EditMenuDlg_OnRight(hwnd);
        break;
    case IDOK:
        EditMenuDlg_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
}

LRESULT EditMenuDlg_OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    if (idFrom == ctl1)
    {
        if (pnmhdr->code == LVN_KEYDOWN)
        {
            LV_KEYDOWN *KeyDown = (LV_KEYDOWN *)pnmhdr;
            if (KeyDown->wVKey == VK_DELETE)
            {
                EditMenuDlg_OnDelete(hwnd);
                return 0;
            }
        }
        if (pnmhdr->code == NM_DBLCLK)
        {
            EditMenuDlg_OnModify(hwnd);
            return 0;
        }
    }
    return 0;
}

INT_PTR CALLBACK
EditMenuDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, EditMenuDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, EditMenuDlg_OnCommand);
        HANDLE_MSG(hwnd, WM_NOTIFY, EditMenuDlg_OnNotify);
    }
    return 0;
}

BOOL MainWnd_CompileIfNecessary(HWND hwnd)
{
    if (Edit_GetModify(g_hSrcEdit))
    {
        INT id = MessageBox(hwnd, LoadStringDx(IDS_COMPILENOW), g_szTitle,
                            MB_ICONINFORMATION | MB_YESNOCANCEL);
        switch (id)
        {
        case IDYES:
            {
                INT cchText = GetWindowTextLengthW(g_hSrcEdit);
                std::wstring WideText;
                WideText.resize(cchText);
                GetWindowTextW(g_hSrcEdit, &WideText[0], cchText + 1);

                if (!DoCompileParts(hwnd, WideText))
                    return FALSE;

                Edit_SetModify(g_hSrcEdit, FALSE);
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

struct RadHelper
{
    HWND m_hwndOwner;
    HWND m_hwnd;
    LONG m_DialogBaseUnits;
    WNDPROC m_OldWndProc;
    DialogRes m_dialog_res;

    // // PrintWindow
    // HINSTANCE hUser32;
    // typedef BOOL (WINAPI *PRINTWINDOW)(HWND, HDC, UINT);
    // PRINTWINDOW pPrintWindow;
#ifndef PW_CLIENTONLY
    #define PW_CLIENTONLY   1
#endif
    //BOOL Print(HWND hwnd, HDC hDC)
    //{
    //    UINT uFlags = PRF_CHILDREN | PRF_CLIENT | PRF_ERASEBKGND |
    //                  PRF_NONCLIENT | PRF_OWNED;
    //    PostMessage(hwnd, WM_PRINT, (WPARAM)hDC, uFlags);
    //    return TRUE;
    //}

    RadHelper()
    {
        m_hwnd = m_hwndOwner = NULL;
        m_DialogBaseUnits = 0;
        m_OldWndProc = NULL;
    }

    ~RadHelper()
    {
    }

    static LRESULT CALLBACK 
    ControlWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCXBUTTONDOWN:
        case WM_NCXBUTTONUP:
        case WM_NCXBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
            SendMouseMesssageToParent(hwnd, uMsg, wParam, lParam);
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATEANDEAT;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
            SendKeyMesssageToParent(hwnd, uMsg, wParam, lParam);
            return 0;
        case WM_SETFOCUS:
        case WM_SETCURSOR:
            return 0;
        }
        WNDPROC OldProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        return CallWindowProc(OldProc, hwnd, uMsg, wParam, lParam);
    }

    void SubclassChild(HWND hCtrl)
    {
        WNDPROC OldProc = (WNDPROC)
            SetWindowLongPtr(hCtrl, GWLP_WNDPROC,
                             (LONG_PTR)RadHelper::ControlWindowProc);
        SetWindowLongPtr(hCtrl, GWLP_USERDATA, (LONG_PTR)OldProc);

        SubclassAllChildren(hCtrl);
    }

    void SubclassAllChildren(HWND hwnd)
    {
        HWND hCtrl = GetTopWindow(hwnd);
        while (hCtrl)
        {
            SubclassChild(hCtrl);

            hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_DialogBaseUnits = GetDialogBaseUnits();

        m_OldWndProc = (WNDPROC)
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, 
                             (LONG_PTR)RadHelper::WindowProc);

        SetParent(hwnd, m_hwndOwner);

        RECT Rect;
        GetWindowRect(hwnd, &Rect);
        SIZE Size;
        Size.cx = Rect.right - Rect.left;
        Size.cy = Rect.bottom - Rect.top;
        MoveWindow(hwnd, 0, 0, Size.cx, Size.cy, TRUE);

        DWORD style = GetWindowLong(m_hwndOwner, GWL_STYLE);
        DWORD exstyle = GetWindowLong(m_hwndOwner, GWL_EXSTYLE);
        SetRect(&Rect, 0, 0, Size.cx, Size.cy);
        AdjustWindowRectEx(&Rect, style, FALSE, exstyle);
        OffsetRect(&Rect, -Rect.left, -Rect.top);
        MoveWindow(m_hwndOwner, 0, 0, Rect.right, Rect.bottom, TRUE);

        SubclassAllChildren(hwnd);

        m_hwnd = hwnd;
        return FALSE;
    }

    static void
    SendMouseMesssageToParent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        HWND Parent = GetParent(hwnd);
        if (Parent == NULL)
            Parent = GetWindow(hwnd, GW_OWNER);
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        if (uMsg != WM_MOUSEWHEEL)
        {
            MapWindowPoints(hwnd, Parent, &pt, 1);
            lParam = MAKELPARAM(pt.x, pt.y);
        }
        SendMessage(Parent, uMsg, wParam, lParam);
    }

    static void
    SendKeyMesssageToParent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        HWND Parent = GetParent(hwnd);
        if (Parent == NULL)
            Parent = GetWindow(hwnd, GW_OWNER);
        SendMessage(Parent, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK
    WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCXBUTTONDOWN:
        case WM_NCXBUTTONUP:
        case WM_NCXBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
            SendMouseMesssageToParent(hwnd, uMsg, wParam, lParam);
            break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
            SendKeyMesssageToParent(hwnd, uMsg, wParam, lParam);
            break;
        case WM_NCHITTEST:
            return HTCLIENT;
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS | DLGC_WANTMESSAGE;
        case WM_NCDESTROY:
            m_hwnd = NULL;
            return CallWindowProc(m_OldWndProc, hwnd, uMsg, wParam, lParam);
        default:
            return CallWindowProc(m_OldWndProc, hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    static LRESULT CALLBACK 
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        RadHelper *pRad;
        LPARAM nData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        pRad = (RadHelper *)nData;

        if (pRad)
        {
            return pRad->WndProc(hwnd, uMsg, wParam, lParam);
        }

        return 0;
    }

    INT_PTR CALLBACK
    DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        }
        return 0;
    }

    static INT_PTR CALLBACK 
    DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        RadHelper *pRad;
        if (uMsg == WM_INITDIALOG)
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            pRad = (RadHelper *)lParam;
        }
        else
        {
            LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            pRad = (RadHelper *)lParam;
        }

        if (pRad)
        {
            return pRad->DlgProc(hwnd, uMsg, wParam, lParam);
        }

        return 0;
    }

    void Create()
    {
        m_dialog_res.Fixup(FALSE);
        std::vector<BYTE> data = m_dialog_res.data();
        m_dialog_res.Fixup(TRUE);

        g_hRadDialog = CreateDialogIndirectParam(
            g_hInstance, (LPDLGTEMPLATE)&data[0], m_hwndOwner, 
            RadHelper::DialogProc, (LPARAM)this);
        if (g_hRadDialog == NULL)
        {
            DWORD err = GetLastError();
        }

        ShowWindow(g_hRadDialog, SW_SHOWNORMAL);
        UpdateWindow(g_hRadDialog);
    }
};

BOOL RadBase_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    RadHelper& rad = *(RadHelper *)lpCreateStruct->lpCreateParams;
    rad.m_hwndOwner = hwnd;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&rad);

    rad.Create();

    return TRUE;
}

void RadBase_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    // FIXME
}

void RadBase_OnNCDestroy(HWND hwnd)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    delete (RadHelper *)lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
}

void RadBase_OnRButtonUp(HWND hwnd, int x, int y, UINT flags)
{
    MessageBoxA(NULL, "OK", NULL, 0);
}

void RadBase_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    LPARAM lParam = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (g_hRadDialog == NULL || lParam == 0)
    {
        FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProcW);
        return;
    }

    RadHelper& rad = *(RadHelper *)lParam;
    if (rad.m_hwnd == NULL)
    {
        FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProcW);
        return;
    }

    LONG Units = rad.m_DialogBaseUnits;
    if (Units == 0)
    {
        FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProcW);
        return;
    }

    RECT Rect;
    GetClientRect(hwnd, &Rect);

    SIZE Size;
    Size.cx = Rect.right - Rect.left;
    Size.cy = Rect.bottom - Rect.top;

    cx = (Size.cx * 4) / Units;
    cy = (Size.cy * 8) / Units;

    rad.m_dialog_res.m_siz.cx = cx;
    rad.m_dialog_res.m_siz.cy = cy;

    MoveWindow(rad.m_hwnd, 0, 0, Size.cx, Size.cy, TRUE);
}

LRESULT CALLBACK
RadBaseWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, RadBase_OnCreate);
        HANDLE_MSG(hwnd, WM_COMMAND, RadBase_OnCommand);
        HANDLE_MSG(hwnd, WM_RBUTTONUP, RadBase_OnRButtonUp);
        HANDLE_MSG(hwnd, WM_NCDESTROY, RadBase_OnNCDestroy);
        HANDLE_MSG(hwnd, WM_SIZE, RadBase_OnSize);
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void MainWnd_OnGuiEdit(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(g_hTreeView);
    if (!MainWnd_IsEditableEntry(hwnd, lParam))
        return;

    WORD i = LOWORD(lParam);
    ResEntry& Entry = g_Entries[i];
    if (!Res_CanGuiEdit(Entry.type))
    {
        return;
    }

    if (!MainWnd_CompileIfNecessary(hwnd))
    {
        return;
    }

    const ResEntry::DataType& data = Entry.data;
    ByteStream stream(data);
    if (Entry.type == RT_ACCELERATOR)
    {
        AccelRes accel_res;
        if (accel_res.LoadFromStream(stream))
        {
            INT nID = DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_EDITACCEL),
                                      hwnd, EditAccelDlgProc, (LPARAM)&accel_res);
            if (nID == IDOK)
            {
                accel_res.Update();
                Entry.data = accel_res.data();
                MainWnd_SelectTV(hwnd, lParam, FALSE);
                return;
            }
        }
    }
    else if (Entry.type == RT_MENU)
    {
        MenuRes menu_res;
        if (menu_res.LoadFromStream(stream))
        {
            INT nID = DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_EDITMENU),
                                      hwnd, EditMenuDlgProc, (LPARAM)&menu_res);
            if (nID == IDOK)
            {
                menu_res.Update();
                Entry.data = menu_res.data();
                MainWnd_SelectTV(hwnd, lParam, FALSE);
                return;
            }
        }
    }
    else if (Entry.type == RT_DIALOG)
    {
        if (IsWindow(g_hRadBase))
        {
            SetForegroundWindow(g_hRadBase);
        }
        else
        {
            RadHelper *rad = new RadHelper;
            if (rad->m_dialog_res.LoadFromStream(stream))
            {
                g_hRadBase = CreateWindow(g_szRadBaseClass, 
                    LoadStringDx(IDS_RADWINDOW),
                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                    hwnd, NULL, g_hInstance, rad);
                ShowWindow(g_hRadBase, SW_SHOWNORMAL);
                UpdateWindow(g_hRadBase);
            }
        }
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

        INT nID = DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_STRINGS),
                                  hwnd, StringsDlgProc, (LPARAM)&str_res);
        if (nID == IDOK)
        {
            std::wstring WideText = str_res.Dump();
            SetWindowTextW(g_hSrcEdit, WideText.c_str());

            if (DoCompileParts(hwnd, WideText))
            {
                TV_RefreshInfo(g_hTreeView, g_Entries, FALSE);
                MainWnd_SelectTV(hwnd, lParam, FALSE);
            }
        }
    }
}

void MainWnd_OnNew(HWND hwnd)
{
    if (g_bInEdit)
        return;

    DoSetFile(hwnd, NULL);
    g_Entries.clear();
    TV_RefreshInfo(g_hTreeView, g_Entries);
}

void MainWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_NEW:
        MainWnd_OnNew(hwnd);
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
    case ID_ADDRES:
        MainWnd_OnAddRes(hwnd);
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
        MainWnd_OnReplaceBin(hwnd);
        break;
    case ID_DELETERES:
        MainWnd_OnDeleteRes(hwnd);
        break;
    case ID_EDIT:
        MainWnd_OnEdit(hwnd);
        break;
    case ID_EXTRACTICON:
        MainWnd_OnExtractIcon(hwnd);
        break;
    case ID_EXTRACTCURSOR:
        MainWnd_OnExtractCursor(hwnd);
        break;
    case ID_EXTRACTBITMAP:
        MainWnd_OnExtractBitmap(hwnd);
        break;
    case ID_EXTRACTBIN:
        MainWnd_OnExtractBin(hwnd);
        break;
    case ID_ABOUT:
        MainWnd_OnAbout(hwnd);
        break;
    case ID_TEST:
        MainWnd_OnTest(hwnd);
        break;
    case ID_CANCELEDIT:
        MainWnd_OnCancelEdit(hwnd);
        break;
    case ID_COMPILE:
        MainWnd_OnCompile(hwnd);
        break;
    case ID_GUIEDIT:
        MainWnd_OnGuiEdit(hwnd);
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

    GetClientRect(hwnd, &ClientRect);
    cx = ClientRect.right - ClientRect.left;
    cy = ClientRect.bottom - ClientRect.top ;

    INT x = 0, y = 0;
    if (g_bInEdit && ::IsWindowVisible(g_hToolBar))
    {
        GetWindowRect(g_hToolBar, &ToolRect);
        y += ToolRect.bottom - ToolRect.top;
        cy -= ToolRect.bottom - ToolRect.top;
    }

#define TV_WIDTH 250
#define SE_WIDTH 256
#define BE_HEIGHT 100

    if (::IsWindowVisible(g_hTreeView))
    {
        MoveWindow(g_hTreeView, x, y, TV_WIDTH, cy, TRUE);
        x += TV_WIDTH;
        cx -= TV_WIDTH;
    }

    if (IsWindowVisible(g_hSrcEdit))
    {
        if (::IsWindowVisible(g_hToolBar))
        {
            if (::IsWindowVisible(g_hBinEdit))
            {
                MoveWindow(g_hSrcEdit, x, y, cx, cy - BE_HEIGHT, TRUE);
                MoveWindow(g_hBinEdit, x, y + cy - BE_HEIGHT, cx, BE_HEIGHT, TRUE);
            }
            else
            {
                MoveWindow(g_hSrcEdit, x, y, cx, cy, TRUE);
            }
        }
        else if (IsWindowVisible(g_hBmpView))
        {
            if (::IsWindowVisible(g_hBinEdit))
            {
                MoveWindow(g_hSrcEdit, x, y, SE_WIDTH, cy - BE_HEIGHT, TRUE);
                MoveWindow(g_hBmpView, x + SE_WIDTH, y, cx - SE_WIDTH, cy - BE_HEIGHT, TRUE);
                MoveWindow(g_hBinEdit, x, y + cy - BE_HEIGHT, cx, BE_HEIGHT, TRUE);
            }
            else
            {
                MoveWindow(g_hSrcEdit, x, y, SE_WIDTH, cy, TRUE);
                MoveWindow(g_hBmpView, x + SE_WIDTH, y, cx - SE_WIDTH, cy, TRUE);
            }
        }
        else
        {
            if (::IsWindowVisible(g_hBinEdit))
            {
                MoveWindow(g_hSrcEdit, x, y, cx, cy - BE_HEIGHT, TRUE);
                MoveWindow(g_hBinEdit, x, y + cy - BE_HEIGHT, cx, BE_HEIGHT, TRUE);
            }
            else
            {
                MoveWindow(g_hSrcEdit, x, y, cx, cy, TRUE);
            }
        }
    }
    else
    {
        if (::IsWindowVisible(g_hBinEdit))
        {
            MoveWindow(g_hBinEdit, x, y, cx, cy, TRUE);
        }
    }
}

void MainWnd_OnInitMenu(HWND hwnd, HMENU hMenu)
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
    BOOL bEditable = MainWnd_IsEditableEntry(hwnd, lParam);
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
    PostMessageW(hwndContext, WM_NULL, 0, 0);
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

#ifndef INVALID_FILE_ATTRIBUTES
    #define INVALID_FILE_ATTRIBUTES     ((DWORD)-1)
#endif

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
    lstrcpynW(g_szDataFolder, szPath, MAX_PATH);
    return TRUE;
}

INT CheckData(VOID)
{
    if (!CheckDataFolder())
    {
        MessageBoxA(NULL, "ERROR: data folder was not found!",
                    NULL, MB_ICONERROR);
        return -1;  // failure
    }

    // Constants.txt
    lstrcpyW(g_szConstantsFile, g_szDataFolder);
    lstrcatW(g_szConstantsFile, L"\\Constants.txt");
    if (!g_ConstantsDB.LoadFromFile(g_szConstantsFile))
    {
        MessageBoxA(NULL, "ERROR: Unable to load Constants.txt file.",
                    NULL, MB_ICONERROR);
        return -2;  // failure
    }
    ReplaceBackslash(g_szConstantsFile);

    // cpp.exe
    lstrcpyW(g_szCppExe, g_szDataFolder);
    lstrcatW(g_szCppExe, L"\\bin\\cpp.exe");
    if (::GetFileAttributesW(g_szCppExe) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBoxA(NULL, "ERROR: No cpp.exe found.", NULL, MB_ICONERROR);
        return -3;  // failure
    }
    ReplaceBackslash(g_szCppExe);

    // windres.exe
    lstrcpyW(g_szWindresExe, g_szDataFolder);
    lstrcatW(g_szWindresExe, L"\\bin\\windres.exe");
    if (::GetFileAttributesW(g_szWindresExe) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBoxA(NULL, "ERROR: No windres.exe found.", NULL, MB_ICONERROR);
        return -4;  // failure
    }
    ReplaceBackslash(g_szWindresExe);

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

INT InitInstance(HINSTANCE hInstance)
{
    g_hInstance = hInstance;
    InitCommonControls();
    g_wargv = CommandLineToArgvW(GetCommandLineW(), &g_argc);

    LoadStringW(hInstance, IDS_TITLE, g_szTitle, _countof(g_szTitle));
    g_hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(1));

    LoadLangInfo();

    INT nRet = CheckData();
    if (nRet)
    {
        return nRet;    // failure
    }

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
    wc.lpszClassName = g_szBmpViewClass;
    if (!RegisterClassW(&wc))
    {
        MessageBoxA(NULL, "RegisterClass failed", NULL, MB_ICONERROR);
        return 3;
    }

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = RadBaseWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockBrush(GRAY_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szRadBaseClass;
    if (!RegisterClassW(&wc))
    {
        MessageBoxA(NULL, "RegisterClass failed", NULL, MB_ICONERROR);
        return 4;
    }

    return 0;   // success
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    INT nRet = InitInstance(hInstance);
    if (nRet)
    {
        return nRet;    // failure
    }

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    g_hMainWnd = CreateWindowW(s_pszClassName, g_szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 760, 480,
        NULL, NULL, hInstance, NULL);
    if (g_hMainWnd == NULL)
    {
        MessageBoxA(NULL, "CreateWindow failed", NULL, MB_ICONERROR);
        return 1;       // failure
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (TranslateAccelerator(g_hMainWnd, g_hAccel, &msg))
            continue;
        if (IsDialogMessage(g_hRadDialog, &msg))
            continue;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();

    return INT(msg.wParam);
}
