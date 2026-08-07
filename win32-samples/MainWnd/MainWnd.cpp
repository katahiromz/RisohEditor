// MainWnd.cpp --- A protected Windows application
// Author: katahiromz
// License: CC0

#include "stdafx.h"

HINSTANCE g_hInst = NULL;
HWND g_hMainWnd = NULL;
static const TCHAR s_szClassName[] = TEXT("RisohEditor MainWnd");
static TCHAR s_szText[64] = TEXT("");
HWND g_hButton = NULL;

// button size
#define CX_BUTTON 100
#define CY_BUTTON 24

// WM_CREATE
BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    g_hMainWnd = hwnd;

    LoadString(g_hInst, IDS_TEXT1, s_szText, _countof(s_szText));

    RECT rc;
    GetClientRect(hwnd, &rc);

    INT x = (rc.left + rc.right - CX_BUTTON) / 2;
    INT y = (rc.top + rc.bottom - CY_BUTTON) / 2;

    DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    HMENU id = reinterpret_cast<HMENU>(static_cast<INT_PTR>(psh1));
    g_hButton = CreateWindow(TEXT("BUTTON"), TEXT("psh1"), style,
                             x, y, CX_BUTTON, CY_BUTTON,
                             hwnd, id, g_hInst, NULL);
    if (!g_hButton)
        return FALSE;

    return TRUE;
}

// WM_PAINT
void OnPaint(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    PAINTSTRUCT ps;
    if (HDC hdc = BeginPaint(hwnd, &ps))
    {
        MoveToEx(hdc, rc.left, rc.top, NULL);
        LineTo(hdc, rc.right, rc.bottom);

        MoveToEx(hdc, rc.right, rc.top, NULL);
        LineTo(hdc, rc.left, rc.bottom);

        UINT uFormat = DT_LEFT | DT_TOP | DT_SINGLELINE;
        DrawText(hdc, s_szText, -1, &rc, uFormat);

        EndPaint(hwnd, &ps);
    }
}

// WM_COMMAND
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case psh1:
        LoadString(g_hInst, IDS_TEXT2, s_szText, _countof(s_szText));
        MessageBox(hwnd, s_szText, TEXT("OnCommand"), MB_ICONINFORMATION);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
}

// WM_DESTROY
void OnDestroy(HWND hwnd)
{
    DestroyWindow(g_hButton);
    g_hButton = NULL;

    PostQuitMessage(EXIT_SUCCESS);
}

// WM_SIZE
void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    if (!g_hButton)
        return;

    RECT rc;
    GetClientRect(hwnd, &rc);

#if 1
    INT x = (rc.left + rc.right - CX_BUTTON) / 2;
    INT y = (rc.top + rc.bottom - CY_BUTTON) / 2;
    MoveWindow(g_hButton, x, y, CX_BUTTON, CY_BUTTON, TRUE);
#endif
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL InitInstance(HINSTANCE hInstance, INT nCmdShow)
{
    InitCommonControls();

    g_hInst = hInstance;

#if defined(NDEBUG) && defined(PROTECTION)
    // TODO: Conceal the final string values trickly from digital forensic
    WCHAR password[] = L"my password";
    WCHAR salt[] = L"my salt my salt my salt my salt";
    WonSetEncryptionPasswordW(password, salt, lstrlenW(salt) * sizeof(WCHAR), WON_ENCRYPTION_MIN_ITERATIONS);
    SecureZeroMemory(password, sizeof(password));
    SecureZeroMemory(salt, sizeof(salt));
#endif

    WNDCLASSEX wcx = { sizeof(wcx) };
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = WindowProc;
    wcx.hInstance = hInstance;
    //wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = GetSysColorBrush(COLOR_3DFACE + 1);
    wcx.lpszClassName = s_szClassName;
    //wcx.hIconSm = NULL;
    wcx.hIconSm = reinterpret_cast<HICON>(
        LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    if (!RegisterClassEx(&wcx))
    {
        MessageBoxA(NULL, "RegisterClassEx failed", NULL, MB_ICONERROR);
        return FALSE;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_ACCEPTFILES;
    CreateWindowEx(exstyle, s_szClassName, TEXT("The main window"), style,
                   CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
                   NULL, NULL, hInstance, NULL);
    if (!g_hMainWnd)
    {
        MessageBoxA(NULL, "CreateWindowEx failed", NULL, MB_ICONERROR);
        return FALSE;
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    return TRUE;
}

INT ExitInstance(MSG& msg)
{
    return INT(msg.wParam);
}

INT Run(VOID)
{
    MSG msg;

    for (;;)
    {
        INT ret = GetMessage(&msg, NULL, 0, 0);
        if (ret == -1)
        {
            DebugBreak();
            break;
        }
        if (ret == 0)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return ExitInstance(msg);
}

////////////////////////////////////////////////////////////////////////////
// Digital signature

#if defined(NDEBUG) && defined(PROTECTION)
#include <wintrust.h>
#include <softpub.h>
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")
BOOL IsSelfSigned(VOID)
{
    WCHAR szPath[MAX_PATH] = {0};
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH) == 0)
        return FALSE;

    WINTRUST_FILE_INFO fileInfo = {0};
    fileInfo.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = szPath;
    fileInfo.hFile         = NULL;
    fileInfo.pgKnownSubject = NULL;

    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    WINTRUST_DATA trustData = {0};
    trustData.cbStruct            = sizeof(WINTRUST_DATA);
    trustData.dwUIChoice          = WTD_UI_NONE;
    trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    trustData.dwUnionChoice       = WTD_CHOICE_FILE;
    trustData.pFile               = &fileInfo;
    trustData.dwStateAction       = WTD_STATEACTION_VERIFY;
    trustData.hWVTStateData       = NULL;

    LONG status = WinVerifyTrust(NULL, &action, &trustData);

    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &action, &trustData);

    return (status == ERROR_SUCCESS);
}
#endif // defined(NDEBUG) && defined(PROTECTION)

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
#if defined(NDEBUG) && defined(PROTECTION)
	if (IsDebuggerPresent())
		return 1;
	if (!IsSelfSigned())
	{
		MessageBoxA(NULL, "No valid digital signature.", NULL, MB_ICONERROR);
		return 1;
	}
#endif // defined(NDEBUG) && defined(PROTECTION)

#if defined(_MSC_VER) && !defined(NDEBUG)
	// for detecting memory leak (MSVC only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if (!InitInstance(hInstance, nCmdShow))
        return EXIT_FAILURE;

    return Run();
}
