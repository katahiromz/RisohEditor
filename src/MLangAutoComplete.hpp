#pragma once

#include <shldisp.h>
#include <shlguid.h>

class MLangAutoComplete : public IEnumString
{
public:
    MLangAutoComplete();

    virtual ~MLangAutoComplete()
    {
        unbind();
    }

    void push_back(const std::wstring& text)
    {
        auto it = std::find(m_list.begin(), m_list.end(), text);
        if (it == m_list.end())
            m_list.push_back(text);
    }
    void erase(const std::wstring& text)
    {
        auto it = std::find(m_list.begin(), m_list.end(), text);
        if (it != m_list.end())
            m_list.erase(it);
    }
    size_t size() const
    {
        return m_list.size();
    }
    bool empty() const
    {
        return size() == 0;
    }

    bool bind(HWND hwndEdit)
    {
        assert(::IsWindow(hwndEdit));

        if (m_fBound && m_pAC)
            return false;

        ::CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pAC));
        if (m_pAC)
        {
            IAutoComplete2 *pAC2 = NULL;
            m_pAC->QueryInterface(IID_PPV_ARGS(&pAC2));
            if (pAC2)
            {
                pAC2->SetOptions(ACO_AUTOSUGGEST | ACO_UPDOWNKEYDROPSLIST);
                pAC2->Release();
            }

            m_pAC->Init(hwndEdit, this, NULL, NULL);
            m_fBound = TRUE;
            return true;
        }

        assert(0);
        return false;
    }
    void unbind()
    {
        if (!m_fBound)
            return;

        if (m_pAC)
        {
            m_pAC->Release();
            m_pAC = NULL;
            m_fBound = FALSE;
        }
    }

    // IUnknown interface
    STDMETHODIMP_(ULONG) AddRef()
    {
        return ++m_nRefCount;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        ULONG nCount = --m_nRefCount;
        if (nCount == 0)
            delete this;
        return nCount;
    }
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject)
    {
        if (ppvObject == NULL)
            return E_POINTER;

        *ppvObject = NULL;

        IUnknown *punk = NULL;
        if (riid == IID_IUnknown)
            punk = static_cast<IUnknown *>(this);
        else if (riid == IID_IEnumString)
            punk = static_cast<IEnumString *>(this);

        if (punk == NULL)
            return E_NOINTERFACE;

        punk->AddRef();
        *ppvObject = punk;
        return S_OK;
    }

    // IEnumString interface
    STDMETHODIMP Next(ULONG celt, LPOLESTR* rgelt, ULONG* pceltFetched)
    {
        HRESULT hr = S_FALSE;
        if (!celt)
            celt = 1;

        ULONG i;
        for (i = 0; i < celt; i++)
        {
            if (m_nCurrentElement == m_list.size())
                break;

            size_t cb = (m_list[m_nCurrentElement].size() + 1) * sizeof(WCHAR);
            rgelt[i] = reinterpret_cast<LPWSTR>(::CoTaskMemAlloc(cb));
            memcpy(rgelt[i], m_list[m_nCurrentElement].c_str(), cb);

            if (pceltFetched)
                *pceltFetched++;

            m_nCurrentElement++;
        }

        if (i == celt)
            hr = S_OK;

        return hr;
    }
    STDMETHODIMP Skip(ULONG celt)
    {
        m_nCurrentElement += celt;
        if (m_nCurrentElement > m_list.size())
            m_nCurrentElement = 0;
        return S_OK;
    }
    STDMETHODIMP Reset(void)
    {
        m_nCurrentElement = 0;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumString** ppenum)
    {
        if (!ppenum)
            return E_POINTER;
        
        MLangAutoComplete *cloned = new MLangAutoComplete();

        cloned->AddRef();
        cloned->m_list = m_list;
        *ppenum = cloned;

        return S_OK;
    }

protected:
    IAutoComplete *m_pAC;
    std::vector<std::wstring> m_list;
    ULONG m_nCurrentElement;
    ULONG m_nRefCount;
    BOOL m_fBound;
};

class MLangAutoCompleteEdit : public MEditCtrl
{
public:
    HWND m_hwndTV;
    BOOL m_bHooked;
    BOOL m_bAdjustSize;

    MLangAutoCompleteEdit()
        : m_hwndTV(NULL)
        , m_bHooked(FALSE)
        , m_bAdjustSize(FALSE)
    {
    }

    void hook(HWND hwndEdit, HWND hwndTV = NULL)
    {
        if (!m_bHooked)
        {
            SubclassDx(hwndEdit);
            m_hwndTV = hwndTV;
            m_bHooked = TRUE;
            SetTimer(hwndEdit, 99999, 100, NULL);
        }
    }

    void unhook()
    {
        KillTimer(m_hwnd, 99999);
        if (m_bHooked)
        {
            UnsubclassDx();
        }
        m_bHooked = FALSE;
        m_hwndTV = NULL;
    }

    void AdjustSize()
    {
        if (!m_bHooked || !m_bAdjustSize)
            return;

        RECT rc, rcTV;
        GetWindowRect(m_hwnd, &rc);
        MapWindowRect(NULL, m_hwndTV, &rc);
        GetClientRect(m_hwndTV, &rcTV);

        INT cx = rcTV.right - rc.left;
        INT cy = rc.bottom - rc.top;

        if (HWND hwndDropdown = FindWindowW(L"Auto-Suggest Dropdown", NULL))
        {
            GetWindowRect(hwndDropdown, &rc);
            cy = rc.bottom - rc.top;
            MoveWindow(hwndDropdown, rc.left, rc.top, cx, cy, TRUE);
        }
    }

protected:
    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT ret = 0;
        switch (uMsg)
        {
        case WM_TIMER:
            if (wParam == 99999)
            {
                AdjustSize();
                break;
            }
            ret = DefaultProcDx(hwnd, uMsg, wParam, lParam);
            break;
        default:
            ret = DefaultProcDx(hwnd, uMsg, wParam, lParam);
        }
        return ret;
    }
};
