
#pragma once

#include "MEditCtrl.hpp"

#ifndef LNEN_ZOOMIN
#define LNEN_ZOOMIN  10000
#define LNEN_ZOOMOUT 10001
#endif

class MHexEditCtrl : public MEditCtrl
{
	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_MOUSEWHEEL:
			if (GetKeyState(VK_CONTROL) < 0)
			{
				UINT id = GetDlgCtrlID(hwnd);
				if ((SHORT)HIWORD(wParam) < 0)
					PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, LNEN_ZOOMOUT), (LPARAM)hwnd);
				else
					PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, LNEN_ZOOMIN), (LPARAM)hwnd);
				return 0;
			}
			break;
		}
		return DefaultProcDx(hwnd, uMsg, wParam, lParam);
	}
};
