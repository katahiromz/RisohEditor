// MAddKeyDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDKEYDLG_HPP_
#define MZC4_MADDKEYDLG_HPP_

#include "RisohEditor.hpp"
#include "AccelRes.hpp"

void Cmb1_InitVirtualKeys(HWND hCmb1);
BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, std::wstring& str);

//////////////////////////////////////////////////////////////////////////////

struct MAddKeyDlg : MDialogBase
{
    ACCEL_ENTRY& m_entry;

    MAddKeyDlg(ACCEL_ENTRY& entry) : MDialogBase(IDD_ADDKEY), m_entry(entry)
    {
    }

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

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADDKEYDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
