#include <windows.h>
#include <windowsx.h>

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
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

INT WINAPI
WinMain(HINSTANCE   hInstance,
		HINSTANCE   hPrevInstance,
		LPSTR       lpCmdLine,
		INT         nCmdShow)
{
	DialogBox(hInstance, MAKEINTRESOURCE(1), NULL, DialogProc);
	return 0;
}
