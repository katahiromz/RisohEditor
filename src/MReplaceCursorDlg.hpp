// MReplaceCursorDlg.hpp --- "Replace Cursor" Dialog
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

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MReplaceCursorDlg : public MDialogBase
{
protected:
    HCURSOR   m_hCursor;
public:
    EntryBase *m_entry;
    MIdOrString m_type;
    MIdOrString m_name;
    WORD m_lang;

    MReplaceCursorDlg(EntryBase *entry) :
        MDialogBase(IDD_REPLACECUR), m_entry(entry),
          m_type(entry->m_type), m_name(entry->m_name), m_lang(entry->m_lang)
    {
        m_hCursor = NULL;
    }

    ~MReplaceCursorDlg()
    {
        DestroyCursor(m_hCursor);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for name
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitResNameComboBox(hCmb2, m_entry->m_name, IDTYPE_CURSOR);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, m_entry->m_lang);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MIdOrString type = m_entry->m_type;

        MIdOrString name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!CheckNameComboBox(hCmb2, name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD lang;
        if (!CheckLangComboBox(hCmb3, lang))
            return;

        std::wstring file;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, file))
        {
            Edit_SetSel(hEdt1, 0, -1);  // select all
            SetFocus(hEdt1);    // set focus
            ErrorBoxDx(IDS_FILENOTFOUND);
            return;
        }

        BOOL bAni = FALSE;
        size_t ich = file.find(L'.');
        if (ich != std::wstring::npos && lstrcmpiW(&file[ich], L".ani") == 0)
            bAni = TRUE;

        if (bAni)   // animation cursor (*.ani)
        {
            MByteStream bs;
            if (!bs.LoadFromFile(file.c_str()) ||
                !g_res.add_lang_entry(RT_ANICURSOR, name, lang, bs.data()))
            {
                ErrorBoxDx(IDS_CANTREPLACECUR);
                return;
            }
        }
        else    // normal cursor (*.cur)
        {
            if (auto entry = g_res.find(ET_LANG, m_type, m_name, m_lang))
                g_res.delete_entry(entry);

            if (!g_res.add_group_cursor(name, lang, file))
            {
                ErrorBoxDx(IDS_CANTREPLACECUR);
                return;
            }
        }

        m_type = type;
        m_name = name;
        m_lang = lang;

        EndDialog(IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        MStringW strFile = GetDlgItemText(edt1);
        mstr_trim(strFile);

        WCHAR szFile[MAX_PATH];
        lstrcpynW(szFile, strFile.c_str(), _countof(szFile));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_REPLACECUR);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        if (m_entry->m_type == RT_ANICURSOR)
        {
            ofn.nFilterIndex = 2;
            ofn.lpstrDefExt = L"ani";
        }
        else
        {
            ofn.nFilterIndex = 1;
            ofn.lpstrDefExt = L"cur";
        }
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, szFile);
            if (m_hCursor)
                DestroyCursor(m_hCursor);
            m_hCursor = LoadCursorFromFile(szFile);
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
            EndDialog(IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR file[MAX_PATH];
        DragQueryFileW(hdrop, 0, file, _countof(file));
        SetDlgItemTextW(hwnd, edt1, file);

        if (m_hCursor)
            DestroyCursor(m_hCursor);
        m_hCursor = LoadCursorFromFile(file);
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
