// TextInclude1Test.cpp
// Test case for TEXTINCLUDE 1 support (Issue #301)
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include "custom_resource.h"

#ifdef _MSC_VER
#pragma comment(linker,"/manifestdependency:\"type='win32' \
    name='Microsoft.Windows.Common-Controls' \
    version='6.0.0.0' \
    processorArchitecture='*' \
    publicKeyToken='6595b64144ccf1df' \
    language='*'\"")
#endif

//////////////////////////////////////////////////////////////////////////////

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_BTN_OK:
    case IDOK:
        EndDialog(hwnd, IDOK);
        break;
    case IDC_BTN_CANCEL:
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

extern "C" INT WINAPI
_tWinMain(HINSTANCE   hInstance,
          HINSTANCE   hPrevInstance,
          LPTSTR      lpCmdLine,
          INT         nCmdShow)
{
    InitCommonControls();
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
