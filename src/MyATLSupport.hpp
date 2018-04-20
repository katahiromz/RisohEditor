// TODO: OLE support

#include "MWindowBase.hpp"

class MyATLSupport;
//MyATLSupport& GetMyATLSupport();
class AtlAxWin;
class AtlAxWin71;
class AtlAxWin80;
class AtlAxWin90;
class AtlAxWin100;
class AtlAxWin110;

// NOTE: Don't forget the following lines:
// IMPLEMENT_DYNAMIC(AtlAxWin)
// IMPLEMENT_DYNAMIC(AtlAxWin71)
// IMPLEMENT_DYNAMIC(AtlAxWin80)
// IMPLEMENT_DYNAMIC(AtlAxWin90)
// IMPLEMENT_DYNAMIC(AtlAxWin100)
// IMPLEMENT_DYNAMIC(AtlAxWin110)

//////////////////////////////////////////////////////////////////////////////

class MyATLSupport
{
public:
    HMODULE m_hATL;
    const WCHAR *m_pszClass;

    typedef BOOL (WINAPI *ATLAXWININIT)();
    typedef HRESULT (WINAPI *ATLAXGETCONTROL)(HWND, IUnknown **);
    typedef HRESULT (WINAPI *ATLAXCREATECONTROLEX)(LPCOLESTR, HWND,
        IStream *, IUnknown **, IUnknown **, IID *, IUnknown *);

    ATLAXWININIT m_pAtlAxWinInit;
    ATLAXGETCONTROL m_pAtlAxGetControl;
    ATLAXCREATECONTROLEX m_pAtlAxCreateControlEx;

    MyATLSupport() : m_hATL(NULL), m_pszClass(NULL),
                     m_pAtlAxWinInit(NULL), m_pAtlAxGetControl(NULL),
                     m_pAtlAxCreateControlEx(NULL)
    {
    }

    virtual ~MyATLSupport()
    {
        FreeATL();
    }

    void FreeATL()
    {
        FreeLibrary(m_hATL);
        m_hATL = NULL;
        m_pszClass = NULL;
    }

    HMODULE LoadATL(LPCTSTR pszOptional)
    {
        static const struct {
            const WCHAR *dll;
            const WCHAR *wndclass;
        } s_entries[] = {
            { L"atl110.dll", L"AtlAxWin110" },
            { L"atl100.dll", L"AtlAxWin100" },
            { L"atl90.dll", L"AtlAxWin90" },
            { L"atl80.dll", L"AtlAxWin80" },
            { L"atl71.dll", L"AtlAxWin71" },
            { L"atl.dll", L"AtlAxWin" },
        };
        size_t i;
        HMODULE hATL = NULL;
        const WCHAR *pszWndClass = NULL;
        for (i = 0; i < _countof(s_entries); ++i)
        {
            hATL = LoadLibraryW(s_entries[i].dll);
            if (hATL)
            {
                pszWndClass = s_entries[i].wndclass;
                break;
            }
        }
        if (!hATL)
        {
            hATL = LoadLibrary(pszOptional);
            pszWndClass = L"AtlAxWin";
        }
        if (hATL)
        {
            m_pAtlAxWinInit = (ATLAXWININIT)GetProcAddress(hATL, "AtlAxWinInit");
            m_pAtlAxGetControl = (ATLAXGETCONTROL)GetProcAddress(hATL, "AtlAxGetControl");
            m_pAtlAxCreateControlEx = (ATLAXCREATECONTROLEX)GetProcAddress(hATL, "AtlAxCreateControlEx");

            if (m_pAtlAxWinInit)
            {
                (*m_pAtlAxWinInit)();
            }

            m_hATL = hATL;
            m_pszClass = pszWndClass;
        }
        return hATL;
    }

    void RegisterAll();
};

inline MyATLSupport& GetMyATLSupport()
{
    static MyATLSupport s_support;
    return s_support;
}

//////////////////////////////////////////////////////////////////////////////

class AtlAxWin : public MWindowBase
{
public:
    DECLARE_DYNAMIC(AtlAxWin)

    AtlAxWin() : m_pUnknown(NULL)
    {
    }

    virtual ~AtlAxWin()
    {
        if (m_pUnknown)
        {
            m_pUnknown->Release();
            m_pUnknown = NULL;
        }
    }

    static BOOL RegisterDx()
    {
        AtlAxWin ctrl;
        return ctrl.RegisterClassDx();
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("AtlAxWin");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = NULL;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        WCHAR szText[MAX_PATH];
        GetWindowTextW(hwnd, szText, MAX_PATH);

        HRESULT hr;
        hr = GetMyATLSupport().m_pAtlAxCreateControlEx(szText, hwnd, NULL, NULL, NULL, NULL, NULL);
        assert(hr == S_OK);
        return TRUE;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        default:
            return DefaultProcDx();
        }
    }

protected:
    IUnknown *m_pUnknown;
};

//////////////////////////////////////////////////////////////////////////////

class AtlAxWin71 : public MWindowBase
{
public:
    DECLARE_DYNAMIC(AtlAxWin71)

    AtlAxWin71() : m_pUnknown(NULL)
    {
    }

    virtual ~AtlAxWin71()
    {
        if (m_pUnknown)
        {
            m_pUnknown->Release();
            m_pUnknown = NULL;
        }
    }

    static BOOL RegisterDx()
    {
        AtlAxWin71 ctrl;
        return ctrl.RegisterClassDx();
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("AtlAxWin71");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = NULL;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        WCHAR szText[MAX_PATH];
        GetWindowTextW(hwnd, szText, MAX_PATH);

        HRESULT hr;
        hr = GetMyATLSupport().m_pAtlAxCreateControlEx(szText, hwnd, NULL, NULL, NULL, NULL, NULL);
        assert(hr == S_OK);
        return TRUE;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        default:
            return DefaultProcDx();
        }
    }

protected:
    IUnknown *m_pUnknown;
};

//////////////////////////////////////////////////////////////////////////////

class AtlAxWin80 : public MWindowBase
{
public:
    DECLARE_DYNAMIC(AtlAxWin80)

    AtlAxWin80() : m_pUnknown(NULL)
    {
    }

    virtual ~AtlAxWin80()
    {
        if (m_pUnknown)
        {
            m_pUnknown->Release();
            m_pUnknown = NULL;
        }
    }

    static BOOL RegisterDx()
    {
        AtlAxWin80 ctrl;
        return ctrl.RegisterClassDx();
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("AtlAxWin80");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = NULL;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        WCHAR szText[MAX_PATH];
        GetWindowTextW(hwnd, szText, MAX_PATH);

        HRESULT hr;
        hr = GetMyATLSupport().m_pAtlAxCreateControlEx(szText, hwnd, NULL, NULL, NULL, NULL, NULL);
        assert(hr == S_OK);
        return TRUE;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        default:
            return DefaultProcDx();
        }
    }

protected:
    IUnknown *m_pUnknown;
};

//////////////////////////////////////////////////////////////////////////////

class AtlAxWin90 : public MWindowBase
{
public:
    DECLARE_DYNAMIC(AtlAxWin90)

    AtlAxWin90() : m_pUnknown(NULL)
    {
    }

    virtual ~AtlAxWin90()
    {
        if (m_pUnknown)
        {
            m_pUnknown->Release();
            m_pUnknown = NULL;
        }
    }

    static BOOL RegisterDx()
    {
        AtlAxWin90 ctrl;
        return ctrl.RegisterClassDx();
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("AtlAxWin90");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = NULL;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        WCHAR szText[MAX_PATH];
        GetWindowTextW(hwnd, szText, MAX_PATH);

        HRESULT hr;
        hr = GetMyATLSupport().m_pAtlAxCreateControlEx(szText, hwnd, NULL, NULL, NULL, NULL, NULL);
        assert(hr == S_OK);
        return TRUE;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        default:
            return DefaultProcDx();
        }
    }

protected:
    IUnknown *m_pUnknown;
};

//////////////////////////////////////////////////////////////////////////////

class AtlAxWin100 : public MWindowBase
{
public:
    DECLARE_DYNAMIC(AtlAxWin100)

    AtlAxWin100() : m_pUnknown(NULL)
    {
    }

    virtual ~AtlAxWin100()
    {
        if (m_pUnknown)
        {
            m_pUnknown->Release();
            m_pUnknown = NULL;
        }
    }

    static BOOL RegisterDx()
    {
        AtlAxWin100 ctrl;
        return ctrl.RegisterClassDx();
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("AtlAxWin100");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = NULL;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        WCHAR szText[MAX_PATH];
        GetWindowTextW(hwnd, szText, MAX_PATH);

        HRESULT hr;
        hr = GetMyATLSupport().m_pAtlAxCreateControlEx(szText, hwnd, NULL, NULL, NULL, NULL, NULL);
        assert(hr == S_OK);
        return TRUE;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        default:
            return DefaultProcDx();
        }
    }

protected:
    IUnknown *m_pUnknown;
};

//////////////////////////////////////////////////////////////////////////////

class AtlAxWin110 : public MWindowBase
{
public:
    DECLARE_DYNAMIC(AtlAxWin110)

    AtlAxWin110() : m_pUnknown(NULL)
    {
    }

    virtual ~AtlAxWin110()
    {
        if (m_pUnknown)
        {
            m_pUnknown->Release();
            m_pUnknown = NULL;
        }
    }

    static BOOL RegisterDx()
    {
        AtlAxWin110 ctrl;
        return ctrl.RegisterClassDx();
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("AtlAxWin110");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = NULL;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        WCHAR szText[MAX_PATH];
        GetWindowTextW(hwnd, szText, MAX_PATH);

        HRESULT hr;
        hr = GetMyATLSupport().m_pAtlAxCreateControlEx(szText, hwnd, NULL, NULL, NULL, NULL, NULL);
        assert(hr == S_OK);
        return TRUE;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        default:
            return DefaultProcDx();
        }
    }

protected:
    IUnknown *m_pUnknown;
};

//////////////////////////////////////////////////////////////////////////////

inline void MyATLSupport::RegisterAll()
{
    AtlAxWin::RegisterDx();
    AtlAxWin71::RegisterDx();
    AtlAxWin80::RegisterDx();
    AtlAxWin90::RegisterDx();
    AtlAxWin100::RegisterDx();
    AtlAxWin110::RegisterDx();
}

//////////////////////////////////////////////////////////////////////////////
