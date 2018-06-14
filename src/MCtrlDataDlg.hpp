// MCtrlDataDlg.hpp --- "Edit Control Data" Dialog
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

#ifndef MZC4_MCTRLDATADLG_HPP_
#define MZC4_MCTRLDATADLG_HPP_

#include "MWindowBase.hpp"
#include "MFile.hpp"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////

class MCtrlDataDlg : public MDialogBase
{
public:
    std::vector<BYTE>& m_data;

    MCtrlDataDlg(std::vector<BYTE>& data)
        : MDialogBase(IDD_CTRLDATA), m_data(data)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        UpdateData(hwnd);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MString str = GetDlgItemText(edt1);

        std::vector<MString> vec;
        mstr_split(vec, str, TEXT(" \t\n\r\f\v, "));

        if (vec.size() > 0x10000 / sizeof(WORD))
        {
            ErrorBoxDx(IDS_DATATOOLONG);
            return;
        }

        std::vector<WORD> words;
        for (size_t i = 0; i < vec.size(); ++i)
        {
            mstr_trim(vec[i]);
            if (vec[i].empty())
                continue;

            words.push_back(mstr_parse_int(vec[i].c_str(), true));
        }

        m_data.clear();
        if (words.size())
        {
            BYTE *pb0 = (BYTE *)&words[0];
            BYTE *pb1 = (BYTE *)(&words[0] + words.size());
            m_data.assign(pb0, pb1);
        }

        EndDialog(IDOK);
    }

    void UpdateData(HWND hwnd)
    {
        MString str;
        if (m_data.size() >= 2)
        {
            const WORD *pw = (const WORD *)&m_data[0];
            for (size_t i = 0; i < m_data.size() / 2; ++i)
            {
                str += mstr_hex_word(*pw++);
                str += L" ";
            }
        }
        SetDlgItemTextW(hwnd, edt1, str.c_str());
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
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR szFile[MAX_PATH];
        DragQueryFileW(hdrop, 0, szFile, _countof(szFile));

        std::string str;

        MFile file(szFile);

        if (!file)
            return;

        CHAR buf[512];
        DWORD cbRead;
        while (file.ReadFile(buf, 512, &cbRead) && cbRead)
        {
            str.append(buf, cbRead);
        }
        file.CloseHandle();

        m_data.clear();
        if (str.size() >= 2)
        {
            BYTE *pb0 = (BYTE *)&str[0];
            BYTE *pb1 = (BYTE *)(&str[0] + str.size() / 2 * 2);
            m_data.assign(pb0, pb1);
        }

        UpdateData(hwnd);
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

#endif  // ndef MZC4_MCTRLDATADLG_HPP_
