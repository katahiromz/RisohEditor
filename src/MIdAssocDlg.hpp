// MTestMenuDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MIDASSOCDLG_HPP_
#define MZC4_MIDASSOCDLG_HPP_

#include "MWindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

class MIdAssocDlg : public MDialogBase
{
public:
    typedef std::map<MString, MString> map_type;
    map_type& m_map;

    MIdAssocDlg(map_type& map) : MDialogBase(IDD_IDASSOC), m_map(map)
    {
    }

    virtual ~MIdAssocDlg()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            EndDialog(IDOK);
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

#endif  // ndef MZC4_MIDASSOCDLG_HPP_
