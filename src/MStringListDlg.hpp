// MStringListDlg.hpp
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

#ifndef MSTRINGLISTDLG_HPP_
#define MSTRINGLISTDLG_HPP_

#include "MWindowBase.hpp"
#include "MString.hpp"
#include "DlgInitRes.hpp"

//////////////////////////////////////////////////////////////////////////////

class MStringListDlg : public MDialogBase
{
public:
    DlgInitRes& m_dlginit;
    WORD m_nCtrl;

    MStringListDlg(DlgInitRes& dlginit, WORD nCtrl = -1) :
        MDialogBase(IDD_STRINGLIST), m_dlginit(dlginit), m_nCtrl(nCtrl)
    {
    }

    virtual ~MStringListDlg()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        std::vector<MStringA> vec;
        for (size_t i = 0; i < m_dlginit.size(); ++i)
        {
            if (m_dlginit[i].wCtrl == m_nCtrl)
            {
                vec.push_back(m_dlginit[i].strText);
            }
        }
        MStringA text = mstr_join(vec, "\r\n");
        SetDlgItemTextA(hwnd, edt1, text.c_str());

        MString str = LoadStringDx(IDS_DLGINIT1);
        str += LoadStringDx(IDS_DLGINIT2);
        str += LoadStringDx(IDS_DLGINIT3);
        SetDlgItemText(hwnd, stc1, str.c_str());

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MString str = GetDlgItemText(hwnd, edt1);
        mstr_trim(str);

        if (str.empty())
        {
            m_dlginit.clear();
            EndDialog(IDOK);
            return;
        }

        MStringA strA = MTextToAnsi(CP_ACP, str.c_str()).c_str();

        std::vector<MStringA> lines;
        mstr_replace_all(strA, "\r", "");
        mstr_split(lines, strA, "\n");

        m_dlginit.Erase(m_nCtrl);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            DlgInitEntry entry = { m_nCtrl, WORD(-1), lines[i] };
            m_dlginit.push_back(entry);
        }
        EndDialog(IDOK);
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

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        default:
            return DefaultProcDx();
        }
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MSTRINGLISTDLG_HPP_
