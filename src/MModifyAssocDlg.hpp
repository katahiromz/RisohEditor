// MModifyAssocDlg.hpp
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
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

#ifndef MZC4_MMODIFYASSOCDLG_HPP_
#define MZC4_MMODIFYASSOCDLG_HPP_

#include "MWindowBase.hpp"
#include "MString.hpp"

//////////////////////////////////////////////////////////////////////////////

class MModifyAssocDlg : public MDialogBase
{
public:
    MString& m_text1;
    MString& m_text2;

    MModifyAssocDlg(MString& text1, MString& text2)
        : MDialogBase(IDD_MODIFYASSOC), m_text1(text1), m_text2(text2)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemText(hwnd, edt1, m_text1.c_str());
        SetDlgItemText(hwnd, edt2, m_text2.c_str());
        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MString str = GetDlgItemText(edt2);
        mstr_trim(str);

        if (str.empty())
        {
            HWND hEdt2 = GetDlgItem(hwnd, edt2);
            Edit_SetSel(hEdt2, 0, -1);
            SetFocus(hEdt2);
            ErrorBoxDx(IDS_EMPTYSTR);
            return;
        }

        m_text2 = str;
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

#endif  // ndef MZC4_MMODIFYASSOCDLG_HPP_
