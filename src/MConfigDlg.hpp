// MConfigDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MCONFIGDLG_HPP_
#define MZC4_MCONFIGDLG_HPP_

#include "RisohEditor.hpp"

//////////////////////////////////////////////////////////////////////////////

class MConfigDlg : public MDialogBase
{
public:
    RisohSettings& m_settings;

    MConfigDlg(RisohSettings& settings)
        : MDialogBase(IDD_CONFIG), m_settings(settings)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        CheckDlgButton(hwnd, chx1, m_settings.bAlwaysControl ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx2, m_settings.bHideID ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx3, m_settings.bResumeWindowPos ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx4, m_settings.bAutoLoadNearbyResH ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx5, m_settings.bAutoShowIDList ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemInt(hwnd, edt1, m_settings.nComboHeight, FALSE);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        BOOL bTranslated = FALSE;
        INT nHeight = GetDlgItemInt(hwnd, edt1, &bTranslated, FALSE);
        if (!bTranslated)
        {
            HWND hEdt1 = GetDlgItem(hwnd, edt1);
            Edit_SetSel(hEdt, 0, -1);
            SetFocus(hEdt1);
            ErrorBoxDx(IDS_ENTERINT);
            return 0;
        }
        m_settings.nComboHeight = nHeight;

        m_settings.bAlwaysControl = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        m_settings.bHideID = (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
        m_settings.bResumeWindowPos = (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
        m_settings.bAutoLoadNearbyResH = (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
        m_settings.bAutoShowIDList = (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);

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
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            DO_MSG(WM_INITDIALOG, OnInitDialog);
            DO_MSG(WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MCONFIGDLG_HPP_
