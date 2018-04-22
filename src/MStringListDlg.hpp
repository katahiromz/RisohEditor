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
#include "DialogRes.hpp"

//////////////////////////////////////////////////////////////////////////////

class MStringListDlg : public MDialogBase
{
public:
    DialogRes& m_dialog_res;
    WORD m_nCtrl;

    MStringListDlg(DialogRes& dialog_res, WORD nCtrl = -1) :
        MDialogBase(IDD_STRINGLIST), m_dialog_res(dialog_res), m_nCtrl(nCtrl)
    {
    }

    virtual ~MStringListDlg()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        std::vector<MStringA> vec;
        for (size_t i = 0; i < m_dialog_res.m_dlginit.size(); ++i)
        {
            if (m_dialog_res.m_dlginit[i].wCtrl == m_nCtrl)
            {
                vec.push_back(m_dialog_res.m_dlginit[i].strText);
            }
        }
        MStringA text = mstr_join(vec, "\r\n");
        SetDlgItemTextA(hwnd, edt1, text.c_str());

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MString str = GetDlgItemText(hwnd, edt1);
        mstr_trim(str);

		MStringA strA = MTextToAnsi(CP_ACP, str.c_str()).c_str();

        std::vector<MStringA> lines;
        mstr_split(lines, strA, "\n");

        for (size_t i = 0; i < lines.size(); ++i)
        {
            DlgInitEntry entry = { m_nCtrl, WORD(-1), lines[i] };
            m_dialog_res.m_dlginit.push_back(entry);
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
