#pragma once

class MOleSite;
class MOleHost;

class MOleSite : public MWindowBase, public IOleClientSite, public IAdviseSink
{
public:
    MOleSite *m_pNext;

    DECLARE_DYNAMIC(MOleSite)

    // NOTE: Please call MOleSite::RegisterDx before use.
    static BOOL RegisterDx()
    {
        MOleSite oc;
        return oc.RegisterClassDx();
    }

    // MWindowBase interface
    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("MOleSite");
    }
    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = NULL;
    }

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IOleClientSite interface
    STDMETHODIMP SaveObject() override;
    STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk) override;
    STDMETHODIMP GetContainer(IOleContainer **ppContainer) override;
    STDMETHODIMP ShowObject() override;
    STDMETHODIMP OnShowWindow(BOOL fShow) override;
    STDMETHODIMP RequestNewObjectLayout() override;

    // IAdviseSink interface
    STDMETHODIMP_(void) OnDataChange(FORMATETC *pFormatetc, STGMEDIUM *pStgmed) override;
    STDMETHODIMP_(void) OnViewChange(DWORD dwAspect, LONG lindex) override;
    STDMETHODIMP_(void) OnRename(IMoniker *pmk) override;
    STDMETHODIMP_(void) OnSave() override;
    STDMETHODIMP_(void) OnClose() override;

    MOleSite();
    ~MOleSite();
    void DoInit(MOleHost *pHost, IOleObject *pOleObject, IStorage *pStorage,
                LPCWSTR lpszObjectName);
    void DoRunObject(LONG nVerb);
    void DoCloseObject();
    void DoUpdateRect();
    void DoEnableMenu(BOOL bEnable);

protected:
    LONG        m_cRef;
    DWORD       m_dwConnection;
    BOOL        m_bEnableMenu;
    IOleObject *m_pOleObject;
    IStorage   *m_pStorage;
    MOleHost   *m_pHost;
    RECT        m_rc;

    void DoDestroy();
    void DoSave();
    void DoHIMETRICtoDP(LPSIZEL lpSizel);
    void DoDPtoHIMETRIC(LPSIZEL lpSizel);

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void OnMove(HWND hwnd, int x, int y);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    void OnPaint(HWND hwnd);
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void OnDestroy(HWND hwnd);

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_MOVE, OnMove);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_RBUTTONDOWN, OnRButtonDown);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        default:
            return DefaultProcDx(hwnd, uMsg, wParam, lParam);
        }
    }
};

class MOleHost
{
public:
    MOleHost();
    ~MOleHost();

    BOOL DoInsertObject(MOleSite *pSite, REFCLSID clsid);
    void DoEnableMenu(BOOL bEnable);

protected:
    INT       m_cObjects;
    IStorage *m_pRootStorage;
    IStream  *m_pStream;
    MOleSite *m_pSite; /* linked list */

    BOOL DoCreateRootStorage(LPCWSTR lpszFileName = NULL);
    void DoDestroy();
};

MOleHost *DoGetActiveOleHost(void);
void DoSetActiveOleHost(MOleHost *pHost);
