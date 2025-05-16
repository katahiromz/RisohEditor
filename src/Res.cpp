// Res.cpp --- Win32 Resources
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

#include "Res.hpp"
#include "ToolbarRes.hpp"

struct AutoDeleteFileW
{
    std::wstring m_file;
    AutoDeleteFileW(const std::wstring& file) : m_file(file)
    {
    }
    ~AutoDeleteFileW()
    {
        ::DeleteFileW(m_file.c_str());
    }
};

BOOL
Res_IsEntityType(const MIdOrString& type)
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

    auto table = g_db.GetTable(L"NON.ENTITY.RESOURCE.TYPE");
    for (auto& table_entry : table)
    {
        if (table_entry.name == name)
        {
            return FALSE;
        }
    }

    return TRUE;
}

MStringW EntryBase::get_name_label() const
{
    WORD id = m_name.m_id;
    if (!id)
        return m_name.m_str;        // string name resource name

    // get an IDTYPE_ value
    IDTYPE_ nIDTYPE_ = g_db.IDTypeFromResType(m_type);

    // RT_DLGINIT uses dialog name
    if (m_type == RT_DLGINIT)
        nIDTYPE_ = IDTYPE_DIALOG;

    // RT_TOOLBAR uses bitmap name
    if (m_type == RT_TOOLBAR)
        nIDTYPE_ = IDTYPE_BITMAP;

    // get the label from an IDTYPE_ value
    MStringW label = g_db.GetNameOfResID(nIDTYPE_, id);
    if (label.empty() || m_type == RT_STRING || m_type == RT_MESSAGETABLE)
    {
        return mstr_dec_word(id);   // returns numeric text
    }

    // got the label 
    if (!mchr_is_digit(label[0]))   // first character is not digit
    {
        // add a parenthesis pair and numeric text
        label += L" (";
        label += mstr_dec_word(id);
        label += L")";
    }
    return label;
}

BOOL EntryBase::is_editable(LPCWSTR pszVCBat) const
{
    if (!this)
        return FALSE;

    const MIdOrString& type = m_type;
    switch (m_et)
    {
    case ET_LANG:
        if (type == RT_ACCELERATOR || type == RT_DIALOG || type == RT_HTML ||
            type == RT_MANIFEST || type == RT_MENU || type == RT_VERSION ||
            type == RT_DLGINIT || type == RT_TOOLBAR ||
            type == TEXT("RISOHTEMPLATE"))
        {
            return TRUE;
        }
        if (type == RT_RCDATA && is_delphi_dfm())
        {
            return TRUE;
        }
        if (type == L"TYPELIB" && PathFileExistsW(pszVCBat))
            return TRUE;
        return FALSE;
    case ET_STRING:
        return TRUE;
    case ET_MESSAGE:
        return !g_settings.bUseMSMSGTABLE;
    default:
        return FALSE;
    }
}

std::string
dfm_text_from_binary(LPCWSTR pszDFMSC, const void *binary, size_t size,
                     INT codepage, BOOL bComments)
{
    // get the temporary file path
    WCHAR szPath4[MAX_PATH], szPath5[MAX_PATH];
    StringCbCopyW(szPath4, sizeof(szPath4), GetTempFileNameDx(L"R4"));
    StringCbCopyW(szPath5, sizeof(szPath5), szPath4);
    StringCbCatW(szPath5, sizeof(szPath5), L".txt");

    // create the temporary file and wait
    DWORD cbWritten;
    MFile r4(szPath4, TRUE);
    r4.WriteFile(binary, DWORD(size), &cbWritten);
    r4.FlushFileBuffers();
    r4.CloseHandle();

    AutoDeleteFileW ad4(szPath4);
    AutoDeleteFileW ad5(szPath5);

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += pszDFMSC;
    strCmdLine += L"\" --b2t";
    if (bComments)
    {
        strCmdLine += L" --comments";
    }
    if (codepage != 0)
    {
        strCmdLine += L" --codepage ";
        strCmdLine += mstr_dec_word(codepage);
    }
    strCmdLine += L" \"";
    strCmdLine += szPath4;
    strCmdLine += L"\"";
    //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    // create an mcdx.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    if (pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        SetPriorityClass(pmaker.GetProcessHandle(), HIGH_PRIORITY_CLASS);
        pmaker.WaitForSingleObject();
        pmaker.CloseAll();

        bSuccess = PathFileExistsW(szPath5);
    }

    if (bSuccess)
    {
        MStringA text;
        MFile input(szPath5);
        if (input.ReadAll(text))
            return text;
    }

    return std::string();
}

EntryBase::data_type
dfm_binary_from_text(LPCWSTR pszDFMSC, const std::string& text,
                     INT codepage, BOOL no_unicode, INT& iLine)
{
    // get the temporary file path
    WCHAR szPath6[MAX_PATH], szPath7[MAX_PATH];
    StringCbCopyW(szPath6, sizeof(szPath6), GetTempFileNameDx(L"R6"));
    StringCbCopyW(szPath7, sizeof(szPath7), szPath6);
    StringCbCatW(szPath7, sizeof(szPath7), L".dfm");

    // create the temporary file and wait
    DWORD cbWritten;
    MFile r6(szPath6, TRUE);
    r6.WriteFile("\xEF\xBB\xBF", 3, &cbWritten);
    r6.WriteFile(text.c_str(), DWORD(text.size()), &cbWritten);
    r6.FlushFileBuffers();
    r6.CloseHandle();

    AutoDeleteFileW adf6(szPath6);
    AutoDeleteFileW adf7(szPath7);

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += pszDFMSC;
    strCmdLine += L"\" --t2b";
    if (no_unicode)
    {
        strCmdLine += L" --no-unicode";
    }
    if (codepage != 0)
    {
        strCmdLine += L" --codepage ";
        strCmdLine += mstr_dec_word(codepage);
    }
    strCmdLine += L" \"";
    strCmdLine += szPath6;
    strCmdLine += L"\"";
    //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    // create an mcdx.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile error;
    pmaker.PrepareForRedirect(NULL, &error, &error);

    if (pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        SetPriorityClass(pmaker.GetProcessHandle(), HIGH_PRIORITY_CLASS);
        pmaker.WaitForSingleObject();
        DWORD dwExitCode = pmaker.GetExitCode();
        pmaker.CloseAll();

        if (dwExitCode == 0 && PathFileExistsW(szPath7))
        {
            bSuccess = TRUE;
        }
        else
        {
            std::string strOutput;
            error.ReadAll(strOutput);
            size_t ich = strOutput.find("expected on line ");
            if (ich != strOutput.npos)
            {
                ich += 17; // "expected on line "
                iLine = INT(strtoul(&strOutput[ich], NULL, 10));
            }
        }
    }

    if (bSuccess)
    {
        MStringA text;
        MFile input(szPath7);
        if (input.ReadAll(text))
            return EntryBase::data_type(text.begin(), text.end());
    }

    return EntryBase::data_type();
}

std::string
tlb_text_from_binary(LPCWSTR pszOleBow, const void *binary, size_t size)
{
    // get the temporary file path
    WCHAR szPath4[MAX_PATH], szPath5[MAX_PATH];
    StringCbCopyW(szPath4, sizeof(szPath4), GetTempFileNameDx(L"R4"));
    StringCbCopyW(szPath5, sizeof(szPath5), szPath4);
    StringCbCatW(szPath5, sizeof(szPath5), L".idl");

    // create the temporary file and wait
    DWORD cbWritten;
    MFile r4(szPath4, TRUE);
    r4.WriteFile(binary, DWORD(size), &cbWritten);
    r4.FlushFileBuffers();
    r4.CloseHandle();

    AutoDeleteFileW ad4(szPath4);
    AutoDeleteFileW ad5(szPath5);

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += pszOleBow;
    strCmdLine += L"\" --codepage 65001 --sort \"";
    strCmdLine += szPath4;
    strCmdLine += L"\" \"";
    strCmdLine += szPath5;
    strCmdLine += L"\"";
    //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    // create an OleBow.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    if (pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        SetPriorityClass(pmaker.GetProcessHandle(), HIGH_PRIORITY_CLASS);
        pmaker.WaitForSingleObject(16 * 1000);
        pmaker.CloseAll();

        bSuccess = PathFileExistsW(szPath5);
    }

    if (bSuccess)
    {
        MStringA text;
        MFile input(szPath5);
        if (input.ReadAll(text))
            return text;
    }

    return std::string();
}

EntryBase::data_type
tlb_binary_from_text(LPCWSTR pszMidlWrap, LPCWSTR pszVCBat, MStringA& strOutput,
                     const std::string& text, bool is_64bit)
{
    EntryBase::data_type ret;

    // get the temporary file path
    WCHAR szPath4[MAX_PATH], szPath5[MAX_PATH];
    StringCbCopyW(szPath4, sizeof(szPath4), GetTempFileNameDx(L"R4"));
    StringCbCopyW(szPath5, sizeof(szPath5), szPath4);
    StringCbCatW(szPath5, sizeof(szPath5), L".tlb");

    // create the temporary file and wait
    DWORD cbWritten;
    MFile r4(szPath4, TRUE);
    r4.WriteFile(text.c_str(), DWORD(text.size()), &cbWritten);
    r4.FlushFileBuffers();
    r4.CloseHandle();

    AutoDeleteFileW ad4(szPath4);
    AutoDeleteFileW ad5(szPath5);

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L"cmd /C call \"";
    strCmdLine += pszMidlWrap;
    strCmdLine += L"\" \"";
    strCmdLine += pszVCBat;
    if (is_64bit)
        strCmdLine += L"\" amd64 \"";
    else
        strCmdLine += L"\" x86 \"";
    strCmdLine += szPath4;
    strCmdLine += L"\" \"";
    strCmdLine += szPath5;
    strCmdLine += L"\"";
    //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    // create an midlwrap.bat process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    //pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile error;
    pmaker.PrepareForRedirect(NULL, &error, &error);

    if (pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        SetPriorityClass(pmaker.GetProcessHandle(), HIGH_PRIORITY_CLASS);
        pmaker.WaitForSingleObject(16 * 1000);
        pmaker.CloseAll();

        error.ReadAll(strOutput);

        bSuccess = PathFileExistsW(szPath5);
    }

    if (bSuccess)
    {
        MStringA text;
        MFile input(szPath5);
        if (input.ReadAll(text))
        {
            ret.assign(text.begin(), text.end());
        }
    }

    return ret;
}

bool EntrySet::intersect(const EntrySet& another) const
{
    if (size() == 0 && another.size() == 0)
        return false;

    for (auto item1 : *this)
    {
        if (item1->m_et != ET_LANG)
            continue;

        for (auto item2 : another)
        {
            if (item2->m_et != ET_LANG)
                continue;

            if (*item1 == *item2 && item1->valid() && item2->valid())
                return true;    // found
        }
    }

    return false;   // not found
}

EntryBase *
EntrySet::add_lang_entry(const MIdOrString& type, const MIdOrString& name, 
                         WORD lang, const EntryBase::data_type& data)
{
    if (m_hwndTV)   // it has the treeview handle
    {
        // add the related entries
        if (type == RT_STRING)
        {
            add_string_entry(lang);
        }
        if (type == RT_MESSAGETABLE)
        {
            add_message_entry(lang);
        }
    }

    // find the entry
    auto entry = find(ET_LANG, type, name, lang, true);
    if (!entry)
    {
        // if not found, then create it
        entry = Res_NewLangEntry(type, name, lang);
    }

    // store the data
    entry->m_data = data;

    // fixup RT_TOOLBAR
    if (entry->m_type == RT_TOOLBAR)
    {
        ToolbarRes toolbar_res;
        MByteStreamEx stream(entry->m_data);
        toolbar_res.LoadFromStream(stream);
        entry->m_data = toolbar_res.data();
    }

    // finish
    return on_insert_entry(entry);
}

void EntrySet::delete_entry(EntryBase *entry)
{
    // delete the related entries
    switch (entry->m_et)
    {
    case ET_LANG:
        if (entry->m_type == RT_GROUP_CURSOR)
        {
            on_delete_group_cursor(entry);
        }
        if (entry->m_type == RT_GROUP_ICON)
        {
            on_delete_group_icon(entry);
        }
        if (entry->m_type == RT_DIALOG)
        {
            on_delete_dialog(entry);
        }
        break;

    case ET_STRING:
        on_delete_string(entry);
        break;

    case ET_MESSAGE:
        on_delete_message(entry);
        break;

    default:
        break;
    }

    // mark it as invalid. real deletion is done in delete_invalid
    entry->mark_invalid();

    // delete the parent if necessary
    do
    {
        EntryBase *parent = get_parent(entry);
        if (!parent)
            break;  // no parent

        if (get_child(parent))
            break;  // has child

        delete_entry(parent);   // delete the parent
    } while (0);
}

void EntrySet::delete_invalid()
{
    // search the invalid
    super_type found;
    search_invalid(found);

    if (found.empty())
        return;

    // for all the invalid entries
    for (auto entry : found)
    {
        if (super()->find(entry) == super()->end())
            continue;   // not owned. skip it

        if (m_hwndTV && entry->m_hItem && entry == get_entry(entry->m_hItem))
        {
            // delete from treeview
            TreeView_DeleteItem(m_hwndTV, entry->m_hItem);
        }

        // real delete
        erase(entry);
        delete entry;
    }
}

UINT EntrySet::get_last_id(const MIdOrString& type, WORD lang) const
{
    WORD wLastID = 0;
    for (auto entry : *this)
    {
        // invalid?
        if (!entry->valid())
            continue;

        // not matched?
        if (entry->m_type != type || !entry->m_name.is_int() || entry->m_name.is_zero())
            continue;

        // not matched language?
        if (entry->m_lang != lang)
            continue;

        // update wLastID if necessary
        if (wLastID < entry->m_name.m_id)
            wLastID = entry->m_name.m_id;
    }
    return wLastID;
}

BOOL EntrySet::update_exe(LPCWSTR ExeFile) const
{
    // begin the update
    HANDLE hUpdate = ::BeginUpdateResourceW(ExeFile, TRUE);
    if (hUpdate == NULL)
    {
        return FALSE;   // failure
    }

    // for all the language entries
    for (auto entry : *this)
    {
        if (entry->m_et != ET_LANG)
            continue;

        // get the pointer and size
        void *pv = NULL;
        DWORD size = 0;
        if (!(*entry).empty())
        {
            pv = const_cast<void *>((*entry).ptr());
            size = (*entry).size();
        }

        // skip the empty entries
        if (!pv || !size)
            continue;

        // do update
        if (!::UpdateResourceW(hUpdate, (*entry).m_type.ptr(), (*entry).m_name.ptr(),
                               (*entry).m_lang, pv, size))
        {
            assert(0);
            ::EndUpdateResourceW(hUpdate, TRUE);    // discard
            return FALSE;   // failure
        }
    }

    // finish
    return ::EndUpdateResourceW(hUpdate, FALSE);
}

void EntrySet::do_bitmap(MTitleToBitmap& title_to_bitmap, DialogItem& item, WORD lang)
{
    MIdOrString type = RT_BITMAP;

    // find the entry
    auto entry = find(ET_LANG, type, item.m_title, lang);
    if (!entry)
    {
        entry = find(ET_LANG, type, item.m_title, BAD_LANG);
        if (!entry)
            return;
    }

    // create the bitmap object
    HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&(*entry)[0], (*entry).size());
    if (hbm)
    {
        if (!item.m_title.empty())  // title is not empty
        {
            // delete the previous
            if (title_to_bitmap[item.m_title])
                DeleteObject(title_to_bitmap[item.m_title]);

            // update title_to_bitmap
            title_to_bitmap[item.m_title] = hbm;
        }
    }
}

void EntrySet::do_icon(MTitleToIcon& title_to_icon, DialogItem& item, WORD lang)
{
    MIdOrString type = RT_GROUP_ICON;

    // find the entry
    auto entry = find(ET_LANG, type, item.m_title, lang);
    if (!entry)
    {
        entry = find(ET_LANG, type, item.m_title, BAD_LANG);
        if (!entry)
            return;
    }

    // too small?
    if (entry->size() < sizeof(ICONDIR) + sizeof(GRPICONDIRENTRY))
        return;

    // get the entries information
    ICONDIR& dir = (ICONDIR&)(*entry)[0];
    GRPICONDIRENTRY *pGroupIcon = (GRPICONDIRENTRY *)&(*entry)[sizeof(ICONDIR)];

    // get the largest icon image
    int cx = 0, cy = 0, bits = 0, n = 0;
    for (int m = 0; m < dir.idCount; ++m)
    {
        if (cx < pGroupIcon[m].bWidth ||
            cy < pGroupIcon[m].bHeight ||
            bits < pGroupIcon[m].wBitCount)
        {
            cx = pGroupIcon[m].bWidth;
            cy = pGroupIcon[m].bHeight;
            bits = pGroupIcon[m].wBitCount;
            n = m;
        }
    }

    // find the entry of the largest icon
    type = RT_ICON;
    entry = find(ET_LANG, type, pGroupIcon[n].nID, lang);
    if (!entry)
    {
        entry = find(ET_LANG, type, pGroupIcon[n].nID, BAD_LANG);
        if (!entry)
            return;
    }

    // create an icon object
    HICON hIcon = CreateIconFromResource((PBYTE)&(*entry)[0], (*entry).size(), TRUE, 0x00030000);
    if (hIcon)
    {
        if (!item.m_title.empty())  // the title was not empty
        {
            // delete the previous
            if (title_to_icon[item.m_title])
                DestroyIcon(title_to_icon[item.m_title]);

            // update title_to_icon
            title_to_icon[item.m_title] = hIcon;
        }
    }
}

bool EntrySet::extract_cursor(const EntryBase& c_entry, const wchar_t *file_name) const
{
    // copy the header
    LOCALHEADER local;
    if (c_entry.size() < sizeof(local))
    {
        assert(0);
        return false;   // too small
    }
    memcpy(&local, &c_entry[0], sizeof(local));

    // get the remainder pointer and size
    LPBYTE pb = LPBYTE(&c_entry[0]) + sizeof(local);
    DWORD cb = c_entry.size() - sizeof(local);

    // get the BITMAP info
    BITMAP bm;
    if (!PackedDIB_GetInfo(pb, cb, bm))
    {
        assert(0);
        return false;   // unable to get
    }

    // store data to the structures
    ICONDIR dir = { 0, RES_CURSOR, 1 };
    ICONDIRENTRY entry;
    entry.bWidth = (BYTE)bm.bmWidth;
    entry.bHeight = (BYTE)(bm.bmHeight / 2);
    entry.bColorCount = 0;
    entry.bReserved = 0;
    entry.xHotSpot = local.xHotSpot;
    entry.yHotSpot = local.yHotSpot;
    entry.dwBytesInRes = c_entry.size() - sizeof(local);
    entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

    // write them to the stream
    MByteStreamEx stream;
    if (!stream.WriteRaw(dir) ||
        !stream.WriteData(&entry, sizeof(entry)) ||
        !stream.WriteData(pb, cb))
    {
        assert(0);
        return false;
    }

    // save the stream to a file
    return stream.SaveToFile(file_name);
}

bool EntrySet::extract_group_cursor(const EntryBase& group, const wchar_t *file_name) const
{
    ICONDIR dir;
    if (group.m_type != RT_GROUP_CURSOR || group.size() < sizeof(dir))
    {
        assert(0);
        return false;   // invalid
    }

    // group --> dir
    memcpy(&dir, &group[0], sizeof(dir));
    if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
    {
        assert(0);
        return false;   // invalid
    }

    // check the size
    DWORD SizeOfCursorEntries = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
    if (group.size() < sizeof(dir) + SizeOfCursorEntries)
    {
        assert(0);
        return false;   // invalid
    }

    // group --> GroupEntries
    std::vector<GRPCURSORDIRENTRY> GroupEntries(dir.idCount);
    memcpy(&GroupEntries[0], &group[sizeof(dir)], 
           SizeOfCursorEntries);

    // set the current offset
    DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;

    // store the entries to DirEntries
    std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
    for (UINT i = 0; i < dir.idCount; ++i)
    {
        // find the RT_CURSOR
        auto entry = find(ET_LANG, RT_CURSOR, GroupEntries[i].nID, group.m_lang);
        if (!entry)
        {
            entry = find(ET_LANG, RT_CURSOR, GroupEntries[i].nID, BAD_LANG);
            if (!entry)
                continue;   // not found
        }

        // get the LOCALHEADER header
        LOCALHEADER local;
        if (entry->size() >= sizeof(local))
            memcpy(&local, &(*entry)[0], sizeof(local));

        // GroupEntries[i] --> DirEntries[i]
        DirEntries[i].bWidth = (BYTE)GroupEntries[i].wWidth;
        DirEntries[i].bHeight = (BYTE)GroupEntries[i].wHeight;
        if (GroupEntries[i].wBitCount >= 8)
            DirEntries[i].bColorCount = 0;
        else
            DirEntries[i].bColorCount = 1 << GroupEntries[i].wBitCount;
        DirEntries[i].bReserved = 0;
        DirEntries[i].xHotSpot = local.xHotSpot;
        DirEntries[i].yHotSpot = local.yHotSpot;
        DirEntries[i].dwBytesInRes = (*entry).size() - sizeof(local);
        DirEntries[i].dwImageOffset = offset;

        // move the offset
        offset += DirEntries[i].dwBytesInRes;
    }

    // write the header to the stream
    MByteStreamEx stream;
    if (!stream.WriteRaw(dir))
    {
        assert(0);
        return false;   // unable to write
    }

    // write the dir entries to the stream
    DWORD SizeOfDirEntries = sizeof(ICONDIRENTRY) * dir.idCount;
    if (!stream.WriteData(&DirEntries[0], SizeOfDirEntries))
    {
        assert(0);
        return false;   // unable to write
    }

    // write the images to the stream
    for (UINT i = 0; i < dir.idCount; ++i)
    {
        // find RT_CURSOR
        auto entry = find(ET_LANG, RT_CURSOR, GroupEntries[i].nID, group.m_lang);
        if (!entry)
        {
            entry = find(ET_LANG, RT_CURSOR, GroupEntries[i].nID, BAD_LANG);
            if (!entry)
                continue;
        }

        DWORD cbLocal = sizeof(LOCALHEADER);

        // get the current pointer and size
        LPBYTE pb = LPBYTE(&(*entry)[0]) + cbLocal;
        DWORD dwSize = (*entry).size() - cbLocal;
        if (!stream.WriteData(pb, dwSize))
        {
            assert(0);
            return FALSE;   // unable to write
        }
    }

    // save the stream to a file
    return stream.SaveToFile(file_name);
}

BOOL EntrySet::extract_icon(const EntryBase& i_entry, const wchar_t *file_name) const
{
    // get the BITMAP info
    BITMAP bm;
    if (!PackedDIB_GetInfo(&i_entry[0], i_entry.size(), bm))
    {
        MBitmapDx bitmap;
        bitmap.CreateFromMemory(&i_entry[0], i_entry.size());

        LONG cx, cy;
        HBITMAP hbm = bitmap.GetHBITMAP32(cx, cy);
        GetObject(hbm, sizeof(bm), &bm);
        DeleteObject(hbm);
    }

    // store
    ICONDIR dir = { 0, RES_ICON, 1 };
    ICONDIRENTRY entry;
    entry.bWidth = (BYTE)bm.bmWidth;
    entry.bHeight = (BYTE)bm.bmHeight;
    entry.bColorCount = 0;
    entry.bReserved = 0;
    entry.wPlanes = 1;
    entry.wBitCount = bm.bmBitsPixel;
    entry.dwBytesInRes = i_entry.size();
    entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

    // write the data to the straem
    MByteStreamEx stream;
    if (!stream.WriteRaw(dir) ||
        !stream.WriteData(&entry, sizeof(entry)) ||
        !stream.WriteData(&i_entry[0], i_entry.size()))
    {
        assert(0);
        return false;
    }

    // save the stream to a file
    return stream.SaveToFile(file_name);
}

bool EntrySet::extract_group_icon(const EntryBase& group, const wchar_t *file_name) const
{
    ICONDIR dir;

    // check the format
    if (group.m_type != RT_GROUP_ICON || group.size() < sizeof(dir))
    {
        assert(0);
        return false;   // invalid
    }

    // group --> dir
    memcpy(&dir, &group[0], sizeof(dir));

    // check the dir
    if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
    {
        assert(0);
        return false;   // invalid
    }

    // check the size
    DWORD SizeOfIconEntries = sizeof(GRPICONDIRENTRY) * dir.idCount;
    if (group.size() < sizeof(dir) + SizeOfIconEntries)
    {
        assert(0);
        return false;   // invalid
    }

    // group --> GroupEntries
    std::vector<GRPICONDIRENTRY> GroupEntries(dir.idCount);
    memcpy(&GroupEntries[0], &group[sizeof(dir)], SizeOfIconEntries);

    // set the current offset
    DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;

    std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
    for (UINT i = 0; i < dir.idCount; ++i)
    {
        // find the RT_ICON entry
        auto entry = find(ET_LANG, RT_ICON, GroupEntries[i].nID, group.m_lang);
        if (!entry)
        {
            entry = find(ET_LANG, RT_ICON, GroupEntries[i].nID, BAD_LANG);
            if (!entry)
                continue;
        }

        // GroupEntries[i] --> DirEntries[i]
        DirEntries[i].bWidth = GroupEntries[i].bWidth;
        DirEntries[i].bHeight = GroupEntries[i].bHeight;
        if (GroupEntries[i].wBitCount >= 8)
            DirEntries[i].bColorCount = 0;
        else
            DirEntries[i].bColorCount = GroupEntries[i].bColorCount;
        DirEntries[i].bReserved = 0;
        DirEntries[i].wPlanes = 1;
        DirEntries[i].wBitCount = GroupEntries[i].wBitCount;
        DirEntries[i].dwBytesInRes = (*entry).size();
        DirEntries[i].dwImageOffset = offset;

        // move the offset
        offset += DirEntries[i].dwBytesInRes;
    }

    // write the header
    MByteStreamEx stream;
    if (!stream.WriteRaw(dir))
    {
        assert(0);
        return false;   // unable to write
    }

    // write the dir entries
    DWORD SizeOfDirEntries = sizeof(ICONDIRENTRY) * dir.idCount;
    if (!stream.WriteData(&DirEntries[0], SizeOfDirEntries))
    {
        assert(0);
        return false;   // unable to write
    }

    // write the images
    for (UINT i = 0; i < dir.idCount; ++i)
    {
        // find the RT_ICON entry
        auto entry = find(ET_LANG, RT_ICON, GroupEntries[i].nID, group.m_lang);
        if (!entry)
        {
            entry = find(ET_LANG, RT_ICON, GroupEntries[i].nID, BAD_LANG);
            if (!entry)
                continue;
        }

        // write it
        DWORD dwSize = (*entry).size();
        if (!stream.WriteData(&(*entry)[0], dwSize))
        {
            assert(0);
            return false;   // unable to write
        }
    }

    // save the stream to a file
    return stream.SaveToFile(file_name);
}

EntryBase *EntrySet::add_bitmap(const MIdOrString& name, WORD lang, const MStringW& file)
{
    // load the data from an *.bmp file
    MByteStreamEx stream;
    if (!stream.LoadFromFile(file.c_str()) || stream.size() <= 4)
        return NULL;

    // is it a JPEG, GIF or PNG image?
    if (stream.size() >= 4 &&
        (memcmp(&stream[0], "\xFF\xD8\xFF", 3) == 0 ||    // JPEG
         memcmp(&stream[0], "GIF", 3) == 0 ||             // GIF
         memcmp(&stream[0], "\x89\x50\x4E\x47", 4) == 0)) // PNG
    {
        // create a bitmap object from memory
        MBitmapDx bitmap;
        if (!bitmap.CreateFromMemory(&stream[0], (DWORD)stream.size()))
            return NULL;

        LONG cx, cy;
        HBITMAP hbm = bitmap.GetHBITMAP32(cx, cy);

        // create a packed DIB from bitmap handle
        std::vector<BYTE> PackedDIB;
        if (!PackedDIB_CreateFromHandle(PackedDIB, hbm))
        {
            DeleteObject(hbm);
            return NULL;
        }
        DeleteObject(hbm);

        // add the entry
        return add_lang_entry(RT_BITMAP, name, lang, PackedDIB);
    }

    // check the size
    size_t head_size = sizeof(BITMAPFILEHEADER);
    if (stream.size() < head_size)
        return NULL;    // invalid

    // add the entry
    size_t i0 = head_size, i1 = stream.size();
    EntryBase::data_type HeadLess(&stream[i0], &stream[i0] + (i1 - i0));
    return add_lang_entry(RT_BITMAP, name, lang, HeadLess);
}

EntryBase *
EntrySet::add_group_icon(const MIdOrString& name, WORD lang, 
                         const MStringW& file_name)
{
    // load the data from an *.ico file
    IconFile icon;
    if (!icon.LoadFromFile(file_name.c_str()) || icon.type() != RES_ICON)
        return NULL;

    // get the next icon ID
    UINT LastIconID = get_last_id(RT_ICON, lang);
    UINT NextIconID = LastIconID + 1;

    // add the icon images (RT_ICON)
    int i, nCount = icon.GetImageCount();
    for (i = 0; i < nCount; ++i)
    {
        add_lang_entry(RT_ICON, WORD(NextIconID + i), lang, icon.GetImage(i));
    }

    // add the entry
    IconFile::DataType data(icon.GetIconGroup(NextIconID));
    return add_lang_entry(RT_GROUP_ICON, name, lang, data);
}

EntryBase *
EntrySet::add_group_cursor(const MIdOrString& name, WORD lang, 
                           const MStringW& file_name)
{
    // load the data from an *.cur file
    CursorFile cur;
    if (!cur.LoadFromFile(file_name.c_str()) || cur.type() != RES_CURSOR)
        return NULL;

    // get the next cursor ID
    UINT LastCursorID = get_last_id(RT_CURSOR, lang);
    UINT NextCursorID = LastCursorID + 1;

    // add the cursor images (RT_CURSOR)
    int i, nCount = cur.GetImageCount();
    for (i = 0; i < nCount; ++i)
    {
        add_lang_entry(RT_CURSOR, WORD(NextCursorID + i), lang, cur.GetImage(i));
    }

    // add the entry
    CursorFile::DataType data(cur.GetCursorGroup(NextCursorID));
    return add_lang_entry(RT_GROUP_CURSOR, name, lang, data);
}

HTREEITEM EntrySet::get_insert_parent(EntryBase *entry)
{
    if (m_hwndTV == NULL)
        return NULL;    // no treeview handle

    if (entry->m_et == ET_TYPE)
        return TVI_ROOT;    // the root handle

    auto new_entry = add_type_entry(entry->m_type, false);
    if (!new_entry)
        return NULL;    // unable to add

    switch (entry->m_et)
    {
    case ET_NAME: case ET_STRING: case ET_MESSAGE:
        return new_entry->m_hItem;  // success

    case ET_LANG:
        break;

    default:
        return NULL;
    }

    // add the name entry
    new_entry = add_name_entry(entry->m_type, entry->m_name);
    if (!new_entry)
        return NULL;    // unable to add

    return new_entry->m_hItem;  // success
}

HTREEITEM EntrySet::get_insert_position(EntryBase *entry)
{
    if (m_hwndTV == NULL)
        return NULL;    // no treeview handle

    // get the entries to determine the position
    self_type found;
    switch (entry->m_et)
    {
    case ET_TYPE:
        search(found, ET_TYPE);
        break;

    case ET_NAME:
        search(found, ET_NAME, entry->m_type);
        if (entry->m_type == RT_STRING)
            search(found, ET_STRING, entry->m_type);
        if (entry->m_type == RT_MESSAGETABLE)
            search(found, ET_MESSAGE, entry->m_type);
        break;

    case ET_STRING:
        search(found, ET_STRING, entry->m_type);
        search(found, ET_NAME, entry->m_type);
        break;

    case ET_MESSAGE:
        search(found, ET_MESSAGE, entry->m_type);
        search(found, ET_NAME, entry->m_type);
        break;

    case ET_LANG:
        search(found, ET_LANG, entry->m_type, entry->m_name);
        break;

    default:
        return NULL;
    }

    // determine the target
    EntryBase *target = NULL;
    for (auto e : found)
    {
        if (*e < *entry)
        {
            if (!target)    // there is no target yet
            {
                target = e;     // set the target
            }
            else if (*target < *e)  // check the position
            {
                target = e;     // set the new target
            }
        }
    }

    if (target)
        return target->m_hItem;     // returns the target

    return TVI_FIRST;   // insert as a first
}

EntryBase *EntrySet::get_parent(EntryBase *entry)
{
    if (!entry)
        return NULL;    // no parent

    EntryBase *parent;
    switch (entry->m_et)
    {
    case ET_NAME:
    case ET_STRING:
    case ET_MESSAGE:
        // parent is a type entry
        parent = find(ET_TYPE, entry->m_type);
        break;

    case ET_LANG:
        // parent is a name entry
        parent = find(ET_NAME, entry->m_type, entry->m_name);
        break;

    default:
        parent = NULL;  // no parent
        break;
    }

    return parent;
}

bool EntrySet::is_childless_parent(EntryBase *entry) const
{
    assert(entry);
    switch (entry->m_et)
    {
    case ET_TYPE:
        return !find(ET_NAME, entry->m_type);

    case ET_NAME:
        return !find(ET_LANG, entry->m_type, entry->m_name);

    case ET_STRING:
    case ET_MESSAGE:
    case ET_LANG:
    default:
        return false;   // not parent
    }
}

MStringW EntrySet::get_label(const EntryBase *entry)
{
    MStringW strText;

    // get the preferred label by the entry type
    switch (entry->m_et)
    {
    case ET_TYPE:
        strText = entry->get_type_label();
        break;

    case ET_NAME:
        strText = entry->get_name_label();
        break;

    case ET_LANG:
    case ET_STRING:
    case ET_MESSAGE:
        strText = entry->get_lang_label();
        break;

    default:
        assert(0);
        break;
    }

    return strText;
}

EntryBase *EntrySet::on_insert_entry(EntryBase *entry)
{
    MTRACEW(L"on_insert_entry: %p, %s, %s, %u, %s\n",
        entry, entry->m_type.c_str(), entry->m_name.c_str(),
        entry->m_lang, entry->m_strLabel.c_str());

    if (m_hwndTV == NULL)   // no treeview handle
    {
        entry->m_valid = true;
        insert(entry);
        return NULL;
    }

    // get/insert the insertion parent
    HTREEITEM hParent = get_insert_parent(entry);

    // get the insertion position
    HTREEITEM hPosition = get_insert_position(entry);

    // ok, insert it
    return on_insert_after(hParent, entry, hPosition);
}

EntryBase *
EntrySet::on_insert_after(HTREEITEM hParent, EntryBase *entry, HTREEITEM hInsertAfter)
{
    assert(entry);

    // make it valid
    entry->m_valid = true;

    if (m_hwndTV == NULL)
        return entry;   // no treeview handle

    if (entry->m_hItem && entry == get_entry(entry->m_hItem))
    {
        // it already has its item handle
        insert(entry);
        return entry;
    }

    // initialize the TV_INSERTSTRUCT structure
    TV_INSERTSTRUCTW insert_struct;
    ZeroMemory(&insert_struct, sizeof(insert_struct));

    insert_struct.hParent = hParent;
    insert_struct.hInsertAfter = hInsertAfter;
    insert_struct.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM |
                              TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    insert_struct.item.state = 0;
    insert_struct.item.stateMask = 0;

    entry->m_strLabel = get_label(entry);
    insert_struct.item.pszText = &entry->m_strLabel[0];

    insert_struct.item.lParam = (LPARAM)entry;
    if (entry->m_et == ET_TYPE || entry->m_et == ET_NAME)
    {
        insert_struct.item.iImage = 1;
        insert_struct.item.iSelectedImage = 1;
    }
    else
    {
        insert_struct.item.iImage = 0;
        insert_struct.item.iSelectedImage = 0;
    }

    // insert it to the treeview
    if (auto hItem = TreeView_InsertItem(m_hwndTV, &insert_struct))
    {
        entry->m_hItem = hItem;     // set the item handle

        insert(entry);  // insert the entry pointer to this instance

        return entry;
    }

    return NULL;    // failure
}

bool EntrySet::on_delete_group_icon(EntryBase *entry)
{
    // validate the entry
    if (entry->m_et != ET_LANG || entry->m_type != RT_GROUP_ICON)
        return false;   // invalid

    // store the data to the stream
    MByteStreamEx bs(entry->m_data);

    // read the header from the stream
    ICONDIR dir;
    if (!bs.ReadRaw(dir))
        return false;   // unable to read

    // read the dir entries from the stream
    DWORD size = sizeof(GRPICONDIRENTRY) * dir.idCount;
    if (size == 0)
        return false;  // invalid
    std::vector<GRPICONDIRENTRY> DirEntries(dir.idCount);
    if (!bs.ReadData(&DirEntries[0], size))
        return false;   // unable to read

    // delete the related RT_ICON entries
    DWORD i, nCount = dir.idCount;
    for (i = 0; i < nCount; ++i)
    {
        search_and_delete(ET_LANG, RT_ICON, DirEntries[i].nID, (*entry).m_lang);
    }

    return true;    // success
}

bool EntrySet::on_delete_group_cursor(EntryBase *entry)
{
    // validate the entry
    if (entry->m_et != ET_LANG || entry->m_type != RT_GROUP_CURSOR)
        return false;   // invalid

    // store the data to the stream
    MByteStreamEx bs(entry->m_data);

    // read the header from the stream
    ICONDIR dir;
    if (!bs.ReadRaw(dir))
        return false;   // unable to read

    // read the dir entries from the stream
    DWORD size = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
    if (size == 0)
        return false;   // invalid
    std::vector<GRPCURSORDIRENTRY> DirEntries(dir.idCount);
    if (!bs.ReadData(&DirEntries[0], size))
        return false;   // unable to read

    // delete the related RT_CURSOR entries
    DWORD i, nCount = dir.idCount;
    for (i = 0; i < nCount; ++i)
    {
        search_and_delete(ET_LANG, RT_CURSOR, DirEntries[i].nID, (*entry).m_lang);
    }

    return true;    // success
}

EntryBase *
EntrySet::add_res_entry(HMODULE hMod, LPCWSTR type, LPCWSTR name, WORD lang)
{
    // find the resource in hMod
    HRSRC hResInfo = FindResourceExW(hMod, type, name, lang);
    if (!hResInfo)
        return NULL;

    // get the size and the pointer
    DWORD dwSize = SizeofResource(hMod, hResInfo);
    HGLOBAL hGlobal = LoadResource(hMod, hResInfo);
    LPVOID pv = LockResource(hGlobal);
    if (pv && dwSize)
    {
        // got it. add a language entry
        EntryBase::data_type data((LPBYTE)(pv), (LPBYTE)(pv) + dwSize);
        return add_lang_entry(type, name, lang, data);
    }

    return NULL;    // unable to get
}

EntryBase *EntrySet::get_child(EntryBase *parent) const
{
    if (!parent)
        return NULL;    // no parent, no child

    // get child
    EntryBase *child;
    switch (parent->m_et)
    {
    case ET_TYPE:
        child = find(ET_NAME, parent->m_type);
        break;

    case ET_NAME:
        child = find(ET_LANG, parent->m_type, parent->m_name);
        break;

    default:
        child = NULL;   // no child
        break;
    }

    return child;
}

BOOL EntrySet::copy_group_icon(EntryBase *entry, const MIdOrString& new_name, WORD new_lang)
{
    assert(entry->m_et == ET_LANG);
    assert(entry->m_type == RT_GROUP_ICON);

    // check the size
    ICONDIR dir;
    if (entry->size() < sizeof(dir))
    {
        assert(0);
        return FALSE;   // invalid
    }

    // entry --> dir
    memcpy(&dir, &(*entry)[0], sizeof(dir));

    // check the format
    if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
    {
        assert(0);
        return FALSE;   // invalid
    }

    // check the size
    if (entry->size() < sizeof(dir) + dir.idCount * sizeof(GRPICONDIRENTRY))
    {
        assert(0);
        return FALSE;   // invalid
    }

    // get the pointers of old and new entries
    auto data = entry->m_data;
    auto old_entries = (GRPCURSORDIRENTRY *)&(*entry)[sizeof(dir)];
    auto new_entries = (GRPCURSORDIRENTRY *)&data[sizeof(dir)];

    for (UINT i = 0; i < dir.idCount; ++i)
    {
        // find the RT_ICON entry
        auto e = find(ET_LANG, RT_ICON, old_entries[i].nID, entry->m_lang);
        if (!e)
        {
            e = find(ET_LANG, RT_ICON, old_entries[i].nID, BAD_LANG);
            if (!e)
                return FALSE;
        }

        // get the next ID
        UINT nLastID = get_last_id(RT_ICON, new_lang);
        UINT nNextID = nLastID + 1;

        // add a RT_ICON entry
        add_lang_entry(RT_ICON, WORD(nNextID), new_lang, e->m_data);

        // update the ID in new_entries
        new_entries[i].nID = (WORD)nNextID;
    }

    // add a RT_GROUP_ICON entry
    add_lang_entry(RT_GROUP_ICON, new_name, new_lang, data);
    return TRUE;
}

BOOL EntrySet::copy_group_cursor(EntryBase *entry, const MIdOrString& new_name, WORD new_lang)
{
    assert(entry->m_et == ET_LANG);
    assert(entry->m_type == RT_GROUP_CURSOR);

    // check the size
    ICONDIR dir;
    if (entry->size() < sizeof(dir))
    {
        assert(0);
        return FALSE;   // invalid
    }

    // entry --> dir
    memcpy(&dir, &(*entry)[0], sizeof(dir));

    // check the format
    if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
    {
        assert(0);
        return FALSE;   // invalid
    }

    // check the size
    if (entry->size() < sizeof(dir) + dir.idCount * sizeof(GRPCURSORDIRENTRY))
    {
        assert(0);
        return FALSE;   // invalid
    }

    // get the pointers of old and new entries
    auto data = entry->m_data;
    auto old_entries = (GRPCURSORDIRENTRY *)&(*entry)[sizeof(dir)];
    auto new_entries = (GRPCURSORDIRENTRY *)&data[sizeof(dir)];

    for (UINT i = 0; i < dir.idCount; ++i)
    {
        // find the RT_CURSOR entry
        auto e = find(ET_LANG, RT_CURSOR, old_entries[i].nID, entry->m_lang);
        if (!e)
        {
            e = find(ET_LANG, RT_CURSOR, old_entries[i].nID, BAD_LANG);
            if (!e)
                return FALSE;
        }

        // get the next ID
        UINT nLastID = get_last_id(RT_CURSOR, new_lang);
        UINT nNextID = nLastID + 1;

        add_lang_entry(RT_CURSOR, WORD(nNextID), new_lang, e->m_data);

        // update the ID in new_entries
        new_entries[i].nID = (WORD)nNextID;
    }

    // add a RT_GROUP_CURSOR entry
    add_lang_entry(RT_GROUP_CURSOR, new_name, new_lang, data);
    return TRUE;
}

BOOL EntrySet::extract_res(LPCWSTR pszFileName, const EntryBase *entry) const
{
    EntrySet found;

    switch (entry->m_et)
    {
    case ET_ANY:
        search(found, ET_LANG);
        break;

    case ET_TYPE:
        search(found, ET_LANG, entry->m_type);
        break;

    case ET_STRING:
    case ET_MESSAGE:
        search(found, ET_LANG, entry->m_type, WORD(0), entry->m_lang);
        break;

    case ET_NAME:
        search(found, ET_LANG, entry->m_type, entry->m_name);
        break;

    case ET_LANG:
        search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);
        break;

    default:
        return FALSE;
    }

    return extract_res(pszFileName, found);
}

BOOL EntrySet::extract_res(LPCWSTR pszFileName, const EntrySet& res) const
{
    MByteStreamEx bs;   // the stream

    // write the header to the stream
    ResHeader header;
    if (!header.WriteTo(bs))
        return FALSE;   // unable to write

    // for all the language entries in res
    for (auto entry : res)
    {
        if (entry->m_et != ET_LANG)
            continue;

        header.DataSize = entry->size();

        // check the header size
        header.HeaderSize = header.GetHeaderSize(entry->m_type, entry->m_name);
        if (header.HeaderSize == 0 || header.HeaderSize >= 0x10000)
            return FALSE;   // invalid

        header.type = entry->m_type;
        header.name = entry->m_name;
        header.DataVersion = 0;
        header.MemoryFlags = MEMORYFLAG_DISCARDABLE | MEMORYFLAG_PURE |
                             MEMORYFLAG_MOVEABLE;
        header.LanguageId = entry->m_lang;
        header.Version = 0;
        header.Characteristics = 0;

        // write the header to the stream
        if (!header.WriteTo(bs))
            return FALSE;   // unable to write

        // write the data to the stream
        if (!bs.WriteData(&(*entry)[0], entry->size()))
            return FALSE;   // unable to write

        // adjust the alignment
        bs.WriteDwordAlignment();
    }

    // save the stream to an *.res file
    return bs.SaveToFile(pszFileName);
}

BOOL EntrySet::extract_cursor(LPCWSTR pszFileName, const EntryBase *entry) const
{
    if (entry->m_type == RT_GROUP_CURSOR)
    {
        // RT_GROUP_CURSOR
        return extract_group_cursor(*entry, pszFileName);
    }
    else if (entry->m_type == RT_CURSOR)
    {
        // RT_CURSOR
        return extract_cursor(*entry, pszFileName);
    }
    else if (entry->m_type == RT_ANICURSOR)
    {
        // RT_ANICURSOR
        MFile file;
        DWORD cbWritten = 0;
        if (file.OpenFileForOutput(pszFileName) &&
            file.WriteFile(&(*entry)[0], entry->size(), &cbWritten))
        {
            // written to the file
            file.FlushFileBuffers();
            file.CloseHandle();
            return TRUE;    // success
        }
    }
    return FALSE;   // failure
}

BOOL EntrySet::extract_icon(LPCWSTR pszFileName, const EntryBase *entry) const
{
    if (entry->m_type == RT_GROUP_ICON)
    {
        // RT_GROUP_ICON
        return extract_group_icon(*entry, pszFileName);
    }
    else if (entry->m_type == RT_ICON)
    {
        // RT_ICON
        return extract_icon(*entry, pszFileName);
    }
    else if (entry->m_type == RT_ANIICON)
    {
        // RT_ANIICON
        MFile file;
        DWORD cbWritten = 0;
        if (file.OpenFileForOutput(pszFileName) &&
            file.WriteFile(&(*entry)[0], entry->size(), &cbWritten))
        {
            // written to the file
            file.FlushFileBuffers();
            file.CloseHandle();
            return TRUE;    // success
        }
    }
    return FALSE;   // failure
}

BOOL EntrySet::import_res(LPCWSTR pszResFile)
{
    // load the file to the stream
    MByteStreamEx stream;
    if (!stream.LoadFromFile(pszResFile))
        return FALSE;   // failure

    BOOL bAdded = FALSE;
    ResHeader header;
    while (header.ReadFrom(stream))     // repeat reading
    {
        // header was loaded

        bAdded = TRUE;
        if (header.DataSize == 0)   // no data
        {
            stream.ReadDwordAlignment();
            continue;   // go to next
        }

        if (header.DataSize > stream.remainder())
            return FALSE;   // invalid size

        // read the data
        EntryBase::data_type data;
        if (header.DataSize)
        {
            // store to data
            data.resize(header.DataSize);
            if (!stream.ReadData(&data[0], header.DataSize))
            {
                break;
            }
        }

        // add a language entry with data
        add_lang_entry(header.type, header.name, header.LanguageId, data);

        // adjust the alignment
        stream.ReadDwordAlignment();
    }

    return bAdded;
}

BOOL
EntrySet::load_msg_table(LPCWSTR pszRCFile, MStringA& strOutput, const MString& strMcdxExe,
                         const MStringW& strMacrosDump, const MStringW& strIncludesDump)
{
    // get the temporary file path
    WCHAR szPath3[MAX_PATH];
    StringCchCopyW(szPath3, _countof(szPath3), GetTempFileNameDx(L"R3"));

    // create the temporary file and wait
    MFile r3(szPath3, TRUE);
    r3.CloseHandle();

    AutoDeleteFileW ad3(szPath3);

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += strMcdxExe;
    strCmdLine += L"\" -DMCDX_INVOKED=1 ";
    strCmdLine += strMacrosDump;
    strCmdLine += L' ';
    strCmdLine += strIncludesDump;
    strCmdLine += L" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res \"";
    strCmdLine += pszRCFile;
    strCmdLine += L'\"';
    //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    // create an mcdx.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);

        if (pmaker.GetExitCode() == 0)
        {
            // import from the temporary file
            if (import_res(szPath3))
            {
                bSuccess = TRUE;
            }
        }
    }

    return bSuccess;
}

BOOL EntrySet::IsUTF16File(LPCWSTR pszRCFile) const
{
    if (FILE *fp = _wfopen(pszRCFile, L"rb"))
    {
        BYTE ab[2];
        if (fread(ab, 1, 2, fp) == 2)
        {
            if (memcmp(ab, "\xFF\xFE", 2) == 0)
            {
                fclose(fp);
                return TRUE;
            }
            if (ab[0] && !ab[1])
            {
                fclose(fp);
                return TRUE;
            }
        }
        fclose(fp);
    }
    return FALSE;
}

BOOL EntrySet::load_rc(LPCWSTR pszRCFile, MStringA& strOutput,
    const MString& strWindresExe, const MString& strCppExe,
    const MString& strMcdxExe, const MStringW& strMacrosDump,
    const MStringW& strIncludesDump)
{
    // get the temporary file path
    WCHAR szPath3[MAX_PATH];
    StringCchCopyW(szPath3, _countof(szPath3), GetTempFileNameDx(L"R3"));

    // create the temporary file and wait
    MFile r3(szPath3, TRUE);
    r3.CloseHandle();

    AutoDeleteFileW ad3(szPath3);

    // build the command line text
    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += strWindresExe;
    strCmdLine += L"\" -DRC_INVOKED ";
    strCmdLine += strMacrosDump;
    strCmdLine += L' ';
    strCmdLine += strIncludesDump;
    strCmdLine += L" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res -F pe-i386 \"--preprocessor=";
    strCmdLine += strCppExe;
    strCmdLine += L"\" --preprocessor-arg=\"\" \"";
    strCmdLine += pszRCFile;
    strCmdLine += L'\"';
    //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    // create a windres.exe process
    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT);

    MFile hInputWrite, hOutputRead;

    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        // read all with timeout
        pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);

        if (pmaker.GetExitCode() == 0)
        {
            // import the resource from the temporary file
            bSuccess = import_res(szPath3);
        }
        else if (strOutput.find(": no resources") != MStringA::npos)
        {
            // there is no resource data
            bSuccess = TRUE;
            strOutput.clear();
        }
    }

    if (bSuccess)
    {
        // load the message table if any
        EntrySet es;
        if (es.load_msg_table(pszRCFile, strOutput, strMcdxExe, strMacrosDump, strIncludesDump))
        {
            merge(es);
        }
    }

    return bSuccess;
}

void EntrySet::add_default_TEXTINCLUDE()
{
    EntryBase::data_type data;

    MStringA str1 =
        "resource.h"
        "\0";
    data.resize(str1.size());
    std::memcpy(data.data(), str1.data(), str1.size());
    add_lang_entry(L"TEXTINCLUDE", 1, 0, data);

    MStringA str2 =
        "#define APSTUDIO_HIDDEN_SYMBOLS\r\n"
        "#include <windows.h>\r\n"
        "#include <commctrl.h>\r\n"
        "#undef APSTUDIO_HIDDEN_SYMBOLS\r\n"
        "\0";
    data.resize(str2.size());
    std::memcpy(data.data(), str2.data(), str2.size());
    add_lang_entry(L"TEXTINCLUDE", 2, 0, data);

    MStringA str3 =
        "\r\n"
        "\0";
    data.resize(str3.size());
    std::memcpy(data.data(), str3.data(), str3.size());
    add_lang_entry(L"TEXTINCLUDE", 3, 0, data);
}
