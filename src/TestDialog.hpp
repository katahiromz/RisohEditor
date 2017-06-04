// TestDialog
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_TESTDIALOG_HPP_
#define MZC4_TESTDIALOG_HPP_

#include "WindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

struct TestDialog : DialogBase
{
    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DWORD dwStyle;
        switch (uMsg)
        {
        case WM_LBUTTONDOWN:
            dwStyle = GetWindowStyle(hwnd);
            if (!(dwStyle & WS_SYSMENU))
            {
                EndDialog(hwnd, IDOK);
            }
            break;
        case WM_LBUTTONDBLCLK:
            EndDialog(hwnd, IDOK);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK: case IDCANCEL:
                EndDialog(hwnd, LOWORD(wParam));
                break;
            }
            break;
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_TESTDIALOG_HPP_

//////////////////////////////////////////////////////////////////////////////
