// MModifyMItemDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MMODIFYMITEMDLG_HPP_
#define MZC4_MMODIFYMITEMDLG_HPP_

#include "RisohEditor.hpp"
#include "MenuRes.hpp"

//////////////////////////////////////////////////////////////////////////////

struct MModifyMItemDlg : MDialogBase
{
    MENU_ENTRY& m_entry;

    MModifyMItemDlg(MENU_ENTRY& entry) : MDialogBase(IDD_MODIFYMITEM), m_entry(entry)
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

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MMODIFYMITEMDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
