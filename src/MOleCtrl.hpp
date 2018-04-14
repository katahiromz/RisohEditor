// MOleCtrl.hpp -- Win32API OLE control wrapper                 -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MOLECTRL_HPP_
#define MZC4_MOLECTRL_HPP_      27      /* Version 27 */

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"

#include <exdisp.h>
#include <ocidl.h>

struct MVariant;
class MOleCtrl;

struct MDispException;

void MZCAPI
InvokeDx(IDispatch *disp, const WCHAR *pszName, UINT cArgs, VARIANT *pArray,
         WORD wFlags = DISPATCH_METHOD,
         VARIANT *pResult = NULL); // throws MDispException

////////////////////////////////////////////////////////////////////////////
// MVariant --- the variant class (VARIANT wrapper)

struct MVariant : VARIANT
{
    MVariant()
    {
        vt = VT_EMPTY;
    }

    MVariant(LONG i4)
    {
        vt = VT_I4;
        lVal = i4;
    }

    MVariant(const WCHAR *psz)
    {
        vt = VT_BSTR;
        bstrVal = ::SysAllocString(psz);
    }

    MVariant(const void *pv, UINT cb);

    MVariant(IUnknown *punk)
    {
        vt = VT_UNKNOWN;
        punkVal = punk;
        punkVal->AddRef();
    }

    MVariant(IDispatch *pdisp)
    {
        vt = VT_DISPATCH;
        pdispVal = pdisp;
        pdispVal->AddRef();
    }

    MVariant(const MVariant& var)
    {
        Copy(var);
    }

    MVariant& operator=(const MVariant& var);

    void Copy(const MVariant& var);

    bool empty() const
    {
        return vt == VT_EMPTY;
    }

    void clear()
    {
        VariantClear(this);
    }

    ~MVariant()
    {
        clear();
    }

    operator VARIANT *()
    {
        return this;
    }

    operator const VARIANT *() const
    {
        return this;
    }

    operator LONG() const
    {
        assert(vt == VT_I4);
        return lVal;
    }

    operator LONG&()
    {
        assert(vt == VT_I4);
        return lVal;
    }

    operator BSTR()
    {
        return bstrVal;
    }

    operator BSTR *()
    {
        VariantClear(this);
        vt = VT_BSTR;
        return &bstrVal;
    }

    operator BYTE *()
    {
        assert(vt == (VT_ARRAY | VT_UI1));
        return reinterpret_cast<BYTE *>(parray->pvData);
    }

    operator const BYTE *() const
    {
        assert(vt == (VT_ARRAY | VT_UI1));
        return reinterpret_cast<const BYTE *>(parray->pvData);
    }

    operator IUnknown *()
    {
        assert(vt == VT_UNKNOWN);
        return punkVal;
    }

    operator IDispatch *()
    {
        assert(vt == VT_DISPATCH);
        return pdispVal;
    }

    IUnknown *DetachUnknown();
    IDispatch *DetachDispatch();

    void InvokeDx(const WCHAR *pszName, UINT cArgs, VARIANT *pArray,
                  WORD wFlags = DISPATCH_METHOD, VARIANT *pResult = NULL)
    {
        assert(vt == VT_DISPATCH);
        ::InvokeDx(pdispVal, pszName, cArgs, pArray, wFlags, pResult);
    }

    template <typename T_OBJ>
    HRESULT QueryInterfaceDx(REFIID riid, T_OBJ **ppvObj);
};

////////////////////////////////////////////////////////////////////////////
// MDispException --- exception for InvokeDx function

struct MDispException
{
    HRESULT m_hr;
    UINT m_uArgErr;

    MDispException(HRESULT hr) : m_hr(hr)
    {
    }
    MDispException(HRESULT hr, UINT uArgErr) : m_hr(hr), m_uArgErr(uArgErr)
    {
    }
};

////////////////////////////////////////////////////////////////////////////
// MOleCtrl --- MZC4 OLE control class
// NOTE: IMPLEMENT_DYNAMIC(MOleCtrl) is required in a *.cpp source file.

class MOleCtrl :
    public MWindowBase, public IOleClientSite, public IOleInPlaceSite,
    public IOleInPlaceFrame, public IOleControlSite, public IDispatch
{
public:
    DECLARE_DYNAMIC(MOleCtrl)

    MOleCtrl();
    virtual ~MOleCtrl();

    static BOOL RegisterDx()
    {
        MOleCtrl oc;
        return oc.RegisterClassDx();
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("MOleCtrl");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hbrBackground = NULL;
    }

    IDispatch *GetDispatch();
    IUnknown *GetUnknown();

    VOID UIActivate();
    VOID InPlaceActivate();
    VOID Show(BOOL fVisible = TRUE);
    BOOL TranslateAccelerator(LPMSG pMsg);
    void DoVerb(LONG iVerb);

    template <typename T_OBJ>
    HRESULT QueryInterfaceDx(REFIID riid, T_OBJ **ppObj)
    {
        if (m_pUnknown == NULL)
            return E_FAIL;
        return m_pUnknown->QueryInterface(riid, reinterpret_cast<void **>(ppObj));
    }

    // Web Browser actions
    IWebBrowser2 *GetWebBrowser2();
    HRESULT Navigate(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers);
    HRESULT Navigate2(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers);
    HRESULT NavigateDx(const WCHAR *url);
    HRESULT GoBack();
    HRESULT GoForward();
    HRESULT GoHome();
    HRESULT Refresh();
    HRESULT Stop();

    // Web Browser properties
    HRESULT get_Visible(VARIANT_BOOL *pBool);
    HRESULT put_Visible(VARIANT_BOOL Value);
    HRESULT get_FullScreen(VARIANT_BOOL *pbFullScreen);
    HRESULT put_FullScreen(VARIANT_BOOL bFullScreen);
    HRESULT get_LocationName(BSTR *LocationName);
    HRESULT get_LocationURL(BSTR *LocationURL);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IOleClientSite
    STDMETHODIMP SaveObject();
    STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER *ppMk);
    STDMETHODIMP GetContainer(IOleContainer **ppContainer);
    STDMETHODIMP ShowObject();
    STDMETHODIMP OnShowWindow(BOOL fShow);
    STDMETHODIMP RequestNewObjectLayout();

    // IOleWindow Methods
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // IOleInPlaceSite Methods
    STDMETHODIMP CanInPlaceActivate();
    STDMETHODIMP OnInPlaceActivate();
    STDMETHODIMP OnUIActivate();
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT prcPosRect, LPRECT prcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHODIMP Scroll(SIZE scrollExtent);
    STDMETHODIMP OnUIDeactivate(BOOL fUndoable);
    STDMETHODIMP OnInPlaceDeactivate();
    STDMETHODIMP DiscardUndoState();
    STDMETHODIMP DeactivateAndUndo();
    STDMETHODIMP OnPosRectChange(LPCRECT prcPosRect);

    // IOleInPlaceUIWindow Methods
    STDMETHODIMP GetBorder(LPRECT prcBorder);
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS lpborderwidths);
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS lpborderwidths);
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR lpszObjName);

    // IOleInPlaceFrame Methods
    STDMETHODIMP InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHODIMP SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHODIMP RemoveMenus(HMENU hmenuShared);
    STDMETHODIMP SetStatusText(LPCOLESTR pszStatusText);
    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP TranslateAccelerator(LPMSG lpmsg, WORD wID);

    // IOleControlSite Methods
    STDMETHODIMP OnControlInfoChanged();
    STDMETHODIMP LockInPlaceActive(BOOL fLock);
    STDMETHODIMP GetExtendedControl(IDispatch **ppDisp);
    STDMETHODIMP TransformCoords(POINTL *pptlHimetric, POINTF* pptfContainer, DWORD dwFlags);
    STDMETHODIMP TranslateAccelerator(LPMSG pMsg, DWORD grfModifiers);
    STDMETHODIMP OnFocus(BOOL fGotFocus);
    STDMETHODIMP ShowPropertyFrame();

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
    STDMETHODIMP Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexecinfo, UINT *puArgErr);

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    LONG            m_cRefs;
    HWND            m_hwndParent;
    IUnknown *      m_pUnknown;
    IWebBrowser2 *  m_pWebBrowser2;
    RECT            m_rc;
    CLSID           m_clsid;

    BOOL CreateInstanceByString(OLECHAR *str);
    BOOL CreateInstanceByProgID(OLECHAR *progid);
    BOOL CreateInstanceByCLSID(const CLSID& clsid);
    BOOL CreateInstanceByURL(OLECHAR *url);
    VOID DestroyInstance();

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void OnDestroy(HWND hwnd);
    void OnShowWindow(HWND hwnd, BOOL fShow, UINT status);
    void OnPaint(HWND hwnd);
public:
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
};

////////////////////////////////////////////////////////////////////////////

inline void MZCAPI
InvokeDx(IDispatch *disp, const WCHAR *pszName, UINT cArgs, VARIANT *pArray, 
         WORD wFlags/* = DISPATCH_METHOD*/, VARIANT *pResult/* = NULL*/)
{
    MStringW strName = pszName;
    OLECHAR *pName = &strName[0];

    DISPID dispid;
    HRESULT hr;
    hr = disp->GetIDsOfNames(IID_NULL, &pName, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr))
        throw MDispException(hr);

    DISPID     dispid_put = DISPID_PROPERTYPUT;
    DISPPARAMS params;
    params.cArgs = cArgs;
    params.rgvarg = pArray;
    if (wFlags & DISPATCH_PROPERTYPUT)
    {
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &dispid_put;
    }
    else
    {
        params.cNamedArgs = 0;
        params.rgdispidNamedArgs = NULL;
    }

    UINT uArgErr;
    hr = disp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, wFlags,
                      &params, pResult, NULL, &uArgErr);
    if (FAILED(hr))
    {
        if (hr == DISP_E_TYPEMISMATCH || hr == DISP_E_PARAMNOTFOUND)
            throw MDispException(hr, uArgErr);
        throw MDispException(hr);
    }
}

////////////////////////////////////////////////////////////////////////////

inline MVariant::MVariant(const void *pv, UINT cb)
{
    SAFEARRAYBOUND bound = { cb, 0 };
    vt = VT_ARRAY | VT_UI1;
    parray = SafeArrayCreate(VT_UI1, cb, &bound);

    void *data;
    HRESULT hr = SafeArrayAccessData(parray, &data);
    if (SUCCEEDED(hr))
    {
        CopyMemory(data, pv, cb);
        SafeArrayUnaccessData(parray);
    }
}

inline MVariant& MVariant::operator=(const MVariant& var)
{
    if (this != &var)
    {
        VariantClear(this);
        Copy(var);
    }
    return *this;
}

inline void MVariant::Copy(const MVariant& var)
{
    vt = var.vt;
    switch (vt)
    {
    case VT_BSTR:
        bstrVal = ::SysAllocString(var.bstrVal);
        break;
    case VT_UNKNOWN:
        punkVal = var.punkVal;
        punkVal->AddRef();
        break;
    case VT_DISPATCH:
        pdispVal = var.pdispVal;
        pdispVal->AddRef();
        break;
    default:
        if (var.vt & VT_ARRAY)
        {
            SAFEARRAY *psa = NULL;
            SafeArrayCopy(parray, &psa);
            parray = psa;
        }
        else
        {
            CopyMemory(this, &var, sizeof(var));
        }
        break;
    }
}

template <typename T_OBJ>
inline HRESULT MVariant::QueryInterfaceDx(REFIID riid, T_OBJ **ppvObj)
{
    assert(vt == VT_UNKNOWN || vt == VT_DISPATCH);
    if (vt == VT_UNKNOWN)
    {
        return punkVal->QueryInterface(riid, reinterpret_cast<void **>(ppvObj));
    }
    if (vt == VT_DISPATCH)
    {
        return pdispVal->QueryInterface(riid, reinterpret_cast<void **>(ppvObj));
    }
    return E_FAIL;
}

inline IUnknown *MVariant::DetachUnknown()
{
    assert(vt == VT_UNKNOWN);
    IUnknown *ret = punkVal;
    vt = VT_EMPTY;
    punkVal = NULL;
    return ret;
}

inline IDispatch *MVariant::DetachDispatch()
{
    assert(vt == VT_DISPATCH);
    IDispatch *ret = pdispVal;
    vt = VT_EMPTY;
    pdispVal = NULL;
    return ret;
}

////////////////////////////////////////////////////////////////////////////
// inlines


inline MOleCtrl::MOleCtrl() : m_cRefs(1), m_hwndParent(NULL),
    m_pUnknown(NULL), m_pWebBrowser2(NULL)
{
    ZeroMemory(&m_clsid, sizeof(m_clsid));
}

inline MOleCtrl::~MOleCtrl()
{
}

inline BOOL MOleCtrl::CreateInstanceByString(OLECHAR *str)
{
    CLSID clsid;
    ::CLSIDFromString(str, &clsid);
    return CreateInstanceByCLSID(clsid);
}

inline BOOL MOleCtrl::CreateInstanceByProgID(OLECHAR *progid)
{
    CLSID clsid;
    ::CLSIDFromProgID(progid, &clsid);
    return CreateInstanceByCLSID(clsid);
}

inline BOOL MOleCtrl::CreateInstanceByCLSID(const CLSID& clsid)
{
    ::CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                       IID_IUnknown, reinterpret_cast<void **>(&m_pUnknown));
    if (!m_pUnknown)
        return FALSE;

    HRESULT hr;
    if (IsEqualIID(clsid, IID_IWebBrowser2))
    {
        IWebBrowser2 *pwb;
        hr = QueryInterfaceDx(IID_IWebBrowser2, &pwb);
        if (FAILED(hr))
        {
            m_pUnknown->Release();
            m_pUnknown = NULL;
            return FALSE;
        }
        m_pWebBrowser2 = pwb;
    }

    IOleObject *pioo;
    hr = QueryInterfaceDx(IID_IOleObject, &pioo);
    if (FAILED(hr))
        return FALSE;

    pioo->SetClientSite(this);
    pioo->Release();

    IPersistStreamInit *ppsi;
    hr = QueryInterfaceDx(IID_IPersistStreamInit, &ppsi);
    if (SUCCEEDED(hr))
    {
        m_clsid = clsid;
        ppsi->InitNew();
        ppsi->Release();
        return TRUE;
    }
    return FALSE;
}

inline HRESULT MOleCtrl::get_Visible(VARIANT_BOOL *pBool)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->get_Visible(pBool);
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::put_Visible(VARIANT_BOOL Value)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->put_Visible(Value);
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::get_FullScreen(VARIANT_BOOL *pbFullScreen)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->get_FullScreen(pbFullScreen);
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::put_FullScreen(VARIANT_BOOL bFullScreen)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->put_FullScreen(bFullScreen);
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::GoBack()
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->GoBack();
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::GoForward()
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->GoForward();
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::GoHome()
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->GoHome();
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::Navigate(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->Navigate(URL, Flags, TargetFrameName, PostData, Headers);
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::NavigateDx(const WCHAR *url)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        if (url == NULL)
            url = L"about:blank";

        MVariant vURL(url), ve;
        hr = pWebBrowser2->Navigate2(vURL, ve, ve, ve, ve);

        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::Navigate2(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->Navigate2(URL, Flags, TargetFrameName, PostData, Headers);

        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::Refresh()
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->Refresh();
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::Stop()
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->Stop();
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::get_LocationName(BSTR *LocationName)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->get_LocationName(LocationName);
        pWebBrowser2->Release();
    }
    return hr;
}

inline HRESULT MOleCtrl::get_LocationURL(BSTR *LocationURL)
{
    HRESULT hr = E_FAIL;
    if (IWebBrowser2 *pWebBrowser2 = GetWebBrowser2())
    {
        hr = pWebBrowser2->get_LocationURL(LocationURL);
        pWebBrowser2->Release();
    }
    return hr;
}

inline BOOL MOleCtrl::CreateInstanceByURL(OLECHAR *url)
{
    if (!CreateInstanceByCLSID(CLSID_WebBrowser))
        return FALSE;

    HRESULT hr;
    hr = QueryInterfaceDx(IID_IWebBrowser2, &m_pWebBrowser2);
    if (SUCCEEDED(hr))
    {
        hr = NavigateDx(url);
    }
    return SUCCEEDED(hr);
}

inline VOID MOleCtrl::UIActivate()
{
    DoVerb(OLEIVERB_UIACTIVATE);
}

inline VOID MOleCtrl::InPlaceActivate()
{
    DoVerb(OLEIVERB_INPLACEACTIVATE);
}

inline VOID MOleCtrl::Show(BOOL fVisible)
{
    DoVerb(fVisible ? OLEIVERB_SHOW : OLEIVERB_HIDE);
}

inline void MOleCtrl::DoVerb(LONG iVerb)
{
    if (m_pUnknown == NULL)
        return;

    IOleObject *pioo;
    HRESULT hr = QueryInterfaceDx(IID_IOleObject, &pioo);
    if (FAILED(hr))
        return;

    pioo->DoVerb(iVerb, NULL, this, 0, m_hwnd, &m_rc);
    pioo->Release();
}

inline VOID MOleCtrl::DestroyInstance()
{
    if (!m_pUnknown)
        return;

    if (m_pWebBrowser2)
    {
        m_pWebBrowser2->Stop();
        m_pWebBrowser2->Release();
        m_pWebBrowser2 = NULL;
    }

    IOleObject *pioo;
    HRESULT hr = QueryInterfaceDx(IID_IOleObject, &pioo);
    if (SUCCEEDED(hr))
    {
        pioo->Close(OLECLOSE_NOSAVE);
        pioo->SetClientSite(NULL);
        pioo->Release();
    }

    IOleInPlaceObject *pipo;
    hr = QueryInterfaceDx(IID_IOleInPlaceObject, &pipo);
    if (SUCCEEDED(hr))
    {
        pipo->UIDeactivate();
        pipo->InPlaceDeactivate();
        pipo->Release();
    }

    m_pUnknown->Release();
    m_pUnknown = NULL;
}

inline BOOL MOleCtrl::TranslateAccelerator(LPMSG pMsg)
{
    if (!m_pUnknown)
        return FALSE;

    IOleInPlaceActiveObject *pao;
    HRESULT hr = QueryInterfaceDx(IID_IOleInPlaceActiveObject, &pao);
    if (SUCCEEDED(hr))
    {
        hr = pao->TranslateAccelerator(pMsg);
        pao->Release();
    }
    return FAILED(hr);
}

inline IDispatch *MOleCtrl::GetDispatch()
{
    if (!m_pUnknown)
        return NULL;

    IDispatch *pDispatch;
    HRESULT hr = QueryInterfaceDx(IID_IDispatch, &pDispatch);
    if (SUCCEEDED(hr))
        return pDispatch;
    return NULL;
}

inline IUnknown *MOleCtrl::GetUnknown()
{
    if (!m_pUnknown)
        return NULL;

    m_pUnknown->AddRef();
    return m_pUnknown;
}

inline IWebBrowser2 *MOleCtrl::GetWebBrowser2()
{
    if (!m_pWebBrowser2)
        return NULL;

    m_pWebBrowser2->AddRef();
    return m_pWebBrowser2;
}

inline STDMETHODIMP MOleCtrl::QueryInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IOleClientSite))
        *ppvObject = dynamic_cast<IOleClientSite*>(this);
    else if (IsEqualIID(riid, IID_IOleInPlaceSite))
        *ppvObject = dynamic_cast<IOleInPlaceSite*>(this);
    else if (IsEqualIID(riid, IID_IOleInPlaceFrame))
        *ppvObject = dynamic_cast<IOleInPlaceFrame*>(this);
    else if (IsEqualIID(riid, IID_IOleInPlaceUIWindow))
        *ppvObject = dynamic_cast<IOleInPlaceUIWindow*>(this);
    else if (IsEqualIID(riid, IID_IOleControlSite))
        *ppvObject = dynamic_cast<IOleControlSite*>(this);
    else if (IsEqualIID(riid, IID_IOleWindow))
        *ppvObject = this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *ppvObject = dynamic_cast<IDispatch*>(this);
    else if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = this;

    if (*ppvObject)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

inline STDMETHODIMP_(ULONG) MOleCtrl::AddRef()
{
    return InterlockedIncrement(&m_cRefs);
}

inline STDMETHODIMP_(ULONG) MOleCtrl::Release()
{
    if (InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

inline STDMETHODIMP MOleCtrl::SaveObject()
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER *ppMk)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::GetContainer(IOleContainer **ppContainer)
{
    return QueryInterface(IID_IOleContainer,
                          reinterpret_cast<void **>(ppContainer));
}

inline STDMETHODIMP MOleCtrl::ShowObject()
{
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::OnShowWindow(BOOL fShow)
{
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::GetWindow(HWND *phwnd)
{
    *phwnd = m_hwnd;
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::CanInPlaceActivate()
{
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::OnInPlaceActivate()
{
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::OnUIActivate()
{
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::GetWindowContext(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppIIPUIWin, LPRECT prcPosRect, LPRECT prcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    *ppIIPUIWin = NULL;

    if (prcClipRect)
        *prcClipRect = m_rc;
    if (prcPosRect)
        *prcPosRect = m_rc;

    if (ppFrame)
    {
        QueryInterface(IID_IOleInPlaceFrame,
                       reinterpret_cast<void **>(ppFrame));
    }

    if (ppIIPUIWin)
        *ppIIPUIWin = NULL;

    if (lpFrameInfo)
    {
        lpFrameInfo->fMDIApp        = FALSE;
        lpFrameInfo->hwndFrame      = m_hwnd;
        lpFrameInfo->haccel         = NULL;
        lpFrameInfo->cAccelEntries  = 0;
    }

    return S_OK;
}

inline STDMETHODIMP MOleCtrl::Scroll(SIZE scrollExtent)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::OnUIDeactivate(BOOL fUndoable)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::OnInPlaceDeactivate()
{
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::DiscardUndoState()
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::DeactivateAndUndo()
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::OnPosRectChange(LPCRECT prcPosRect)
{
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::GetBorder(LPRECT prcBorder)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::RequestBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR lpszObjName)
{
    return S_OK;
}

inline STDMETHODIMP MOleCtrl::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::RemoveMenus(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::SetStatusText(LPCOLESTR pszStatusText)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::EnableModeless(BOOL fEnable)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::TranslateAccelerator(LPMSG pMsg, WORD wID)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::OnControlInfoChanged(void)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::LockInPlaceActive(BOOL fLock)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::GetExtendedControl(IDispatch **ppDisp)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::TransformCoords(POINTL *pPtlHimetric, POINTF *pPtfContainer, DWORD dwFlags)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::TranslateAccelerator(MSG *pMsg, DWORD grfModifiers)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::OnFocus(BOOL fGotFocus)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::ShowPropertyFrame()
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

inline STDMETHODIMP MOleCtrl::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    *rgdispid = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}

inline STDMETHODIMP MOleCtrl::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexecinfo, UINT *puArgErr)
{
    return DISP_E_MEMBERNOTFOUND;
}

inline LRESULT CALLBACK MOleCtrl::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
    HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    HANDLE_MSG(hwnd, WM_SIZE, OnSize);
    HANDLE_MSG(hwnd, WM_SHOWWINDOW, OnShowWindow);
    default:
        return DefaultProcDx();
    }
}

inline BOOL MOleCtrl::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    m_hwndParent = GetParent(m_hwnd);

    WCHAR szText[512];
    GetWindowTextW(m_hwnd, szText, _countof(szText));

    if (szText[0] == TEXT('{'))
    {
        if (!CreateInstanceByString(szText))
            return FALSE;
    }
    else if (memcmp(szText, L"http://", 7 * sizeof(WCHAR)) == 0 ||
             memcmp(szText, L"https://", 8 * sizeof(WCHAR)) == 0 ||
             memcmp(szText, L"ftp://", 6 * sizeof(WCHAR)) == 0 ||
             memcmp(szText, L"www.", 4 * sizeof(WCHAR)) == 0)
    {
        if (!CreateInstanceByURL(szText))
            return FALSE;
    }
    else
    {
        if (!CreateInstanceByProgID(szText))
            return FALSE;
    }

    OnSize(hwnd, SIZE_RESTORED, 0, 0);
    InPlaceActivate();

    return TRUE;
}

inline void MOleCtrl::OnDestroy(HWND hwnd)
{
    DestroyInstance();
}

inline void MOleCtrl::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    RECT rc;
    ::GetClientRect(hwnd, &rc);
    m_rc = rc;

    if (!m_pUnknown)
        return;

    IOleInPlaceObject *pipo;
    HRESULT hr = QueryInterfaceDx(IID_IOleInPlaceObject, &pipo);
    if (FAILED(hr))
        return;

    pipo->SetObjectRects(&m_rc, &m_rc);
    pipo->Release();
}

inline void MOleCtrl::OnShowWindow(HWND hwnd, BOOL fShow, UINT status)
{
}

inline void MOleCtrl::OnPaint(HWND hwnd)
{
    if (!m_pUnknown)
        return;

    PAINTSTRUCT ps;
    HRESULT hr;
    if (HDC hDC = BeginPaint(m_hwnd, &ps))
    {
        IOleObject *pioo;
        hr = QueryInterfaceDx(IID_IOleObject, &pioo);
        if (SUCCEEDED(hr))
        {
            IViewObject *pView;
            hr = pioo->QueryInterface(IID_IViewObject,
                                      reinterpret_cast<void **>(&pView));
            if (SUCCEEDED(hr))
            {
                RECTL rect;
                rect.left = m_rc.left;
                rect.top = m_rc.top;
                rect.right = m_rc.right;
                rect.bottom = m_rc.bottom;

                pView->Draw(DVASPECT_CONTENT, ~0, NULL, NULL, 0,
                            hDC, &rect, &rect, NULL, 0);
                pView->Release();
            }
            pioo->Release();
        }
        EndPaint(m_hwnd, &ps);
    }
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MOLECTRL_HPP_
