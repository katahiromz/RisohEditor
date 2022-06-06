#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "../../src/Toolbar.h"
#include "resource.h"

HINSTANCE g_hInstance = NULL;
HWND g_hwndTB = NULL;

static INT CALLBACK CommandIdToImageIndex(INT id)
{
    switch (id)
    {
    case 100: return 0;
    case 101: return 1;
    case 102: return 2;
    }
    return -1;
}

static INT CALLBACK CommandIdToStringId(INT id)
{
    switch (id)
    {
    case 100: return 100;
    case 101: return 101;
    case 102: return 102;
    }
    return -1;
}

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    DWORD style = WS_CHILD | WS_VISIBLE | CCS_TOP | TBS_HORZ | TBS_TOOLTIPS | TBSTYLE_LIST;
    DWORD exstyle = 0;
    UINT id = IDW_TOOLBAR;
    g_hwndTB = CreateWindowEx(exstyle, TOOLBARCLASSNAME, NULL,
                              style, 0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)id, g_hInstance, NULL);

    LoadToolbarResource(g_hwndTB, g_hInstance, MAKEINTRESOURCE(IDB_TOOLBAR), CommandIdToImageIndex, CommandIdToStringId);
    //LoadToolbarResource(g_hwndTB, g_hInstance, MAKEINTRESOURCE(IDB_TOOLBAR), CommandIdToImageIndex, NULL);

    return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    }
}

void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    SendMessage(g_hwndTB, TB_AUTOSIZE, 0, 0);
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    g_hInstance = hInstance;
    InitCommonControls();

    DialogBox(hInstance, MAKEINTRESOURCE(1), NULL, DialogProc);
    return 0;
}
