// MTestDialog
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MTESTDIALOG_HPP_
#define MZC4_MTESTDIALOG_HPP_

#include "MWindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

class MTestDialog : public MDialogBase
{
public:
    MTestDialog()
    {
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DWORD dwStyle;
        switch (uMsg)
        {
        case WM_INITDIALOG:
            CenterWindowDx();
            return TRUE;
        case WM_LBUTTONDOWN:
            dwStyle = GetWindowStyle(hwnd);
            if (!(dwStyle & WS_SYSMENU))
            {
                EndDialog(IDOK);
            }
            break;
        case WM_LBUTTONDBLCLK:
            EndDialog(IDOK);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK: case IDCANCEL:
                EndDialog(LOWORD(wParam));
                break;
            }
            break;
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTESTDIALOG_HPP_

//////////////////////////////////////////////////////////////////////////////
