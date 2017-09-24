#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dlgs.h>
#include <tchar.h>

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        EndDialog(hwnd, IDOK);
        break;
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
}

extern "C" INT WINAPI
_tWinMain(HINSTANCE   hInstance,
          HINSTANCE   hPrevInstance,
          LPTSTR      lpCmdLine,
          INT         nCmdShow)
{
    InitCommonControls();
    DialogBox(hInstance, MAKEINTRESOURCE(1), NULL, DialogProc);
}
