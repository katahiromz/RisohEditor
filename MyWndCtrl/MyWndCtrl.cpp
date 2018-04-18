// MyWndCtrl --- private window class
//////////////////////////////////////////////////////////////////////////////

#define NO_STRSAFE
#include "MWindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

class MyWndCtrl : public MWindowBase
{
public:
    DECLARE_DYNAMIC(MyWndCtrl)

    MyWndCtrl()
    {
    }

    virtual ~MyWndCtrl()
    {
    }

    static BOOL RegisterDx()
    {
        MyWndCtrl ctrl;
        return ctrl.RegisterClassDx();
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("MyWndCtrl");
    }

    static HBRUSH& GetBackBrush()
    {
        static HBRUSH s_hbrBack = ::CreateSolidBrush(RGB(255, 0, 0));
        return s_hbrBack;
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = GetBackBrush();
        wcx.hCursor = LoadCursor(NULL, IDC_CROSS);
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        return TRUE;
    }

    void OnPaint(HWND hwnd)
    {
        PAINTSTRUCT ps;
        if (HDC hDC = BeginPaint(hwnd, &ps))
        {
            SelectObject(hDC, GetStockObject(WHITE_BRUSH));
            SelectObject(hDC, GetStockObject(BLACK_PEN));
            RECT rc;
            GetClientRect(hwnd, &rc);
            Ellipse(hDC, rc.left, rc.top, rc.right, rc.bottom);
            EndPaint(hwnd, &ps);
        }
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        default:
            return DefaultProcDx();
        }
    }

protected:
    HBRUSH m_hbrBack;
};

IMPLEMENT_DYNAMIC(MyWndCtrl)

//////////////////////////////////////////////////////////////////////////////

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        MyWndCtrl::RegisterDx();
        break;
    case DLL_PROCESS_DETACH:
        DeleteObject(MyWndCtrl::GetBackBrush());
        MyWndCtrl::GetBackBrush() = NULL;
        break;
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
