#include "MWindowBase.hpp"
#include "MOleHost.hpp"
#include <oledlg.h>

#define ID_VERBMIN 10000
#define ID_VERBMAX 10031

static const CLSID CLSID_SampleFile =
    {0x9f05f2d2, 0x9ec3, 0x53ac, {0xc6, 0xff, 0x20, 0x7e, 0x3f, 0x8a, 0x97, 0xff}};

static MOleHost *s_pHost = NULL;

MOleHost *DoGetActiveOleHost(void)
{
    assert(s_pHost != NULL);
    return s_pHost;
}

void DoSetActiveOleHost(MOleHost *pHost)
{
    s_pHost = pHost;
}

IMPLEMENT_DYNAMIC(MOleSite)

//////////////////////////////////////////////////////////////////////////////
// MOleSite

MOleSite::MOleSite()
    : m_pNext(NULL)
    , m_cRef(1)
    , m_dwConnection(0)
    , m_bEnableMenu(FALSE)
    , m_pOleObject(NULL)
    , m_pStorage(NULL)
    , m_pHost(NULL)
{
    ::SetRectEmpty(&m_rc);
}

MOleSite::~MOleSite()
{
    DoDestroy();
}

BOOL MOleSite::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    WCHAR szText[40];
    GetWindowTextW(hwnd, szText, _countof(szText));

    CLSID clsid;
    if (szText[0] != L'{' || ::CLSIDFromString(szText, &clsid) != S_OK)
        return FALSE;

    m_pHost = DoGetActiveOleHost();
    if (m_pHost == NULL)
        return FALSE;

    return m_pHost->DoInsertObject(this, clsid);
}

void MOleSite::DoDestroy()
{
    if (m_pOleObject)
    {
        DoCloseObject();
        m_pOleObject->Release();
        m_pOleObject = NULL;
    }

    if (m_pStorage)
    {
        m_pStorage->Release();
        m_pStorage = NULL;
    }
}

void MOleSite::OnDestroy(HWND hwnd)
{
    DoDestroy();
}

void MOleSite::DoEnableMenu(BOOL bEnable)
{
    m_bEnableMenu = bEnable;
}

void MOleSite::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    if (fDoubleClick)
    {
        if (m_bEnableMenu)
            DoRunObject(OLEIVERB_PRIMARY);
    }
}

void MOleSite::OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    if (!m_bEnableMenu)
        return;

    HMENU hMenu = CreatePopupMenu(), hmenuTmp;
    ::OleUIAddVerbMenu(m_pOleObject, NULL, hMenu, 0, ID_VERBMIN, ID_VERBMAX, FALSE, 0, &hmenuTmp);

    POINT pt = { x, y };
    ::ClientToScreen(hwnd, &pt);

    SetForegroundWindow(hwnd);
    INT nCmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
    PostMessageW(hwnd, WM_NULL, 0, 0);

    PostMessageW(hwnd, WM_COMMAND, nCmd, 0);
}

void MOleSite::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (!m_bEnableMenu)
        return;

    if (ID_VERBMIN <= id && id <= ID_VERBMAX)
        DoRunObject(id);
}

void MOleSite::OnMove(HWND hwnd, int x, int y)
{
    RECT rc;
    ::GetClientRect(hwnd, &rc);
    m_rc = rc;

    if (!m_pOleObject)
        return;

    IOleInPlaceObject *pipo = NULL;
    m_pOleObject->QueryInterface(IID_IOleInPlaceObject, (void **)&pipo);
    if (pipo)
    {
        pipo->SetObjectRects(&m_rc, &m_rc);
        pipo->Release();
    }

    DoUpdateRect();
}

void MOleSite::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    RECT rc;
    ::GetClientRect(hwnd, &rc);
    m_rc = rc;

    if (!m_pOleObject)
        return;

    IOleInPlaceObject *pipo = NULL;
    m_pOleObject->QueryInterface(IID_IOleInPlaceObject, (void **)&pipo);
    if (pipo)
    {
        pipo->SetObjectRects(&m_rc, &m_rc);
        pipo->Release();
    }

    DoUpdateRect();
}

void MOleSite::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    if (HDC hdc = ::BeginPaint(hwnd, &ps))
    {
        ::OleDraw(m_pOleObject, DVASPECT_CONTENT, hdc, &m_rc);
        ::EndPaint(hwnd, &ps);
    }
}

STDMETHODIMP MOleSite::QueryInterface(REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (riid == IID_IUnknown || riid == IID_IOleClientSite)
        *ppvObject = static_cast<IOleClientSite *>(this);
    else if (riid == IID_IAdviseSink)
        *ppvObject = static_cast<IAdviseSink *>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) MOleSite::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) MOleSite::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

STDMETHODIMP MOleSite::SaveObject()
{
    DoSave();
    DoUpdateRect();
    InvalidateRect(m_hwnd, NULL, TRUE);
    return S_OK;
}

STDMETHODIMP MOleSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    return E_NOTIMPL;
}

STDMETHODIMP MOleSite::GetContainer(IOleContainer **ppContainer)
{
    *ppContainer = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP MOleSite::ShowObject()
{
    return S_OK;
}

STDMETHODIMP MOleSite::OnShowWindow(BOOL fShow)
{
    return S_OK;
}

STDMETHODIMP MOleSite::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

STDMETHODIMP_(void) MOleSite::OnDataChange(FORMATETC *pFormatetc, STGMEDIUM *pStgmed)
{
}

STDMETHODIMP_(void) MOleSite::OnViewChange(DWORD dwAspect, LONG lindex)
{
    InvalidateRect(m_hwnd, NULL, TRUE);
}

STDMETHODIMP_(void) MOleSite::OnRename(IMoniker *pmk)
{
}

STDMETHODIMP_(void) MOleSite::OnSave()
{
}

STDMETHODIMP_(void) MOleSite::OnClose()
{
}

void MOleSite::DoInit(MOleHost *pHost, IOleObject *pOleObject, IStorage *pStorage,
                      LPCWSTR lpszObjectName)
{
    m_pHost = pHost;
    m_pStorage = pStorage;
    m_pOleObject = pOleObject;
    m_pOleObject->SetClientSite(static_cast<IOleClientSite *>(this));
    m_pOleObject->SetHostNames(L"RisohEditor", lpszObjectName);

    OnSize(m_hwnd, SIZE_RESTORED, 0, 0);
}

void MOleSite::DoSave()
{
    IPersistStorage *pPersistStorage = NULL;
    m_pOleObject->QueryInterface(IID_IPersistStorage, (void **)&pPersistStorage);
    if (pPersistStorage)
    {
        ::OleSave(pPersistStorage, m_pStorage, TRUE);

        pPersistStorage->SaveCompleted(NULL);
        pPersistStorage->Release();
    }
}

void MOleSite::DoRunObject(LONG nVerb)
{
    m_pOleObject->Advise(static_cast<IAdviseSink *>(this), &m_dwConnection);

    IViewObject *pViewObject = NULL;
    m_pOleObject->QueryInterface(IID_IViewObject, (void **)&pViewObject);
    if (pViewObject)
    {
        pViewObject->SetAdvise(DVASPECT_CONTENT, 0, static_cast<IAdviseSink *>(this));
        pViewObject->Release();
    }

    m_pOleObject->DoVerb(nVerb, NULL, static_cast<IOleClientSite *>(this), 0, m_hwnd, &m_rc);
}

void MOleSite::DoCloseObject()
{
    if (m_pOleObject && ::OleIsRunning(m_pOleObject))
    {
        IOleInPlaceObject *pipo = NULL;
        m_pOleObject->QueryInterface(IID_IOleInPlaceObject, (void **)&pipo);
        if (pipo)
        {
            pipo->UIDeactivate();
            pipo->InPlaceDeactivate();
            pipo->Release();
        }

        m_pOleObject->Close(OLECLOSE_NOSAVE);
        m_pOleObject->Unadvise(m_dwConnection);
        m_pOleObject->SetClientSite(NULL);
    }
}

void MOleSite::DoUpdateRect()
{
    SIZEL sizel;
    m_pOleObject->GetExtent(DVASPECT_CONTENT, &sizel);

    DoHIMETRICtoDP(&sizel);
    m_rc.right = m_rc.left + sizel.cx;
    m_rc.bottom = m_rc.top + sizel.cy;
}

void MOleSite::DoHIMETRICtoDP(LPSIZEL lpSizel)
{
    const INT HIMETRIC_INCH = 2540;

    HDC hdc = GetDC(NULL);
    lpSizel->cx = lpSizel->cx * GetDeviceCaps(hdc, LOGPIXELSX) / HIMETRIC_INCH;
    lpSizel->cy = lpSizel->cy * GetDeviceCaps(hdc, LOGPIXELSY) / HIMETRIC_INCH;
    ReleaseDC(NULL, hdc);
}

//////////////////////////////////////////////////////////////////////////////
// MOleHost

MOleHost::MOleHost()
    : m_cObjects(0)
    , m_pRootStorage(NULL)
    , m_pStream(NULL)
    , m_pSite(NULL)
{
    DoCreateRootStorage(NULL);
}

MOleHost::~MOleHost()
{
    DoDestroy();
}

BOOL MOleHost::DoCreateRootStorage(LPCWSTR lpszFileName)
{
    static const WCHAR szStreamName[] = L"stream";
    if (lpszFileName == NULL)
    {
        StgCreateDocfile(NULL, STGM_READWRITE | STGM_TRANSACTED, 0, &m_pRootStorage);
        if (m_pRootStorage == NULL)
            return FALSE;

        WriteClassStg(m_pRootStorage, CLSID_SampleFile);

        m_pRootStorage->CreateStream(szStreamName, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStream);
        if (m_pStream == NULL)
            return FALSE;

        return TRUE;
    }

    IStorage *pStorage = NULL;
    StgOpenStorage(lpszFileName, NULL, STGM_READWRITE | STGM_TRANSACTED, NULL, 0, &pStorage);
    if (pStorage == NULL)
        return FALSE;

    CLSID clsid;
    ReadClassStg(pStorage, &clsid);
    if (!IsEqualCLSID(clsid, CLSID_SampleFile))
    {
        pStorage->Release();
        return FALSE;
    }

    IStream *pStream = NULL;
    pStorage->OpenStream(szStreamName, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStream);
    if (pStream == NULL)
    {
        pStorage->Release();
        return FALSE;
    }

    DoDestroy();

    m_pRootStorage = pStorage;
    m_pStream = pStream;

    return TRUE;
}

BOOL MOleHost::DoInsertObject(MOleSite *pSite, REFCLSID clsid)
{
    WCHAR szName[MAX_PATH];
    StringCbPrintfW(szName, sizeof(szName), L"Object %d", m_cObjects + 1);

    IStorage *pStorage = NULL;
    m_pRootStorage->CreateStorage(szName, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &pStorage);
    if (pStorage == NULL)
        return FALSE;

    IOleObject *pOleObject = NULL;
    HRESULT hr = ::OleCreate(clsid, IID_IOleObject, OLERENDER_DRAW, NULL, NULL,
                             pStorage, (void **)&pOleObject);
    if (FAILED(hr))
    {
        pStorage->Release();
        return FALSE;
    }

    m_cObjects++;
    pSite->DoInit(this, pOleObject, pStorage, szName);

    if (m_pSite)
        pSite->m_pNext = m_pSite;
    m_pSite = pSite;

    return TRUE;
}

void MOleHost::DoEnableMenu(BOOL bEnable)
{
    MOleSite *pSite = m_pSite;
    while (pSite)
    {
        pSite->DoEnableMenu(bEnable);
        pSite = pSite->m_pNext;
    }
}

void MOleHost::DoDestroy()
{
    if (m_pStream)
    {
        m_pStream->Release();
        m_pStream = NULL;
    }
    if (m_pRootStorage)
    {
        m_pRootStorage->Release();
        m_pRootStorage = NULL;
    }
}
