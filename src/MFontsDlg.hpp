// MFontsDlg.hpp --- font settings dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MFONTSDLG_HPP_
#define MZC4_MFONTSDLG_HPP_

#include "RisohEditor.hpp"
#include "MString.hpp"
#include <commdlg.h>
#include "resource.h"

class MFontsDlg;

//////////////////////////////////////////////////////////////////////////////

class MFontsDlg : public MDialogBase
{
public:
    RisohSettings& m_settings;
    HFONT m_hSrcFont;
    HFONT m_hBinFont;

    MFontsDlg(RisohSettings& settings) :
        MDialogBase(IDD_FONTS), m_settings(settings),
        m_hSrcFont(NULL), m_hBinFont(NULL)
    {
    }

    virtual ~MFontsDlg()
    {
        DestroySrcFont();
        DestroyBinFont();
    }

    HFONT DetachSrcFont()
    {
        HFONT hFont = m_hSrcFont;
        m_hSrcFont = NULL;
        return hFont;
    }
    HFONT DetachBinFont()
    {
        HFONT hFont = m_hBinFont;
        m_hBinFont = NULL;
        return hFont;
    }

    void DestroySrcFont()
    {
        if (m_hSrcFont)
        {
            DeleteObject(m_hSrcFont);
            m_hSrcFont = NULL;
        }
    }
    void DestroyBinFont()
    {
        if (m_hBinFont)
        {
            DeleteObject(m_hBinFont);
            m_hBinFont = NULL;
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        MString str;

        str = m_settings.strSrcFont;
        str += L", ";
        str += mstr_dec_short(m_settings.nSrcFontSize);
        str += L"pt";
        SetDlgItemText(hwnd, edt1, str.c_str());
        m_hSrcFont = CreateMyFont(m_settings.strSrcFont.c_str(), m_settings.nSrcFontSize);
        SetWindowFont(GetDlgItem(hwnd, stc1), m_hSrcFont, TRUE);

        str = m_settings.strBinFont;
        str += L", ";
        str += mstr_dec_short(m_settings.nBinFontSize);
        str += L"pt";
        SetDlgItemText(hwnd, edt2, str.c_str());
        m_hBinFont = CreateMyFont(m_settings.strBinFont.c_str(), m_settings.nBinFontSize);
        SetWindowFont(GetDlgItem(hwnd, stc2), m_hBinFont, TRUE);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        WCHAR szText1[128], szText2[128];

        GetDlgItemText(hwnd, edt1, szText1, _countof(szText1));
        GetDlgItemText(hwnd, edt2, szText2, _countof(szText2));

        MString str1 = szText1;
        MString str2 = szText2;
        mstr_trim(str1);
        mstr_trim(str2);

        size_t k1 = str1.find(L", ");
        size_t k2 = str2.find(L", ");
        if (k1 == MString::npos || k2 == MString::npos)
        {
            return;
        }

        m_settings.strSrcFont = str1.substr(0, k1);
        m_settings.nSrcFontSize = mstr_parse_int(str1.substr(k1 + 2).c_str());
        DestroySrcFont();
        m_hSrcFont = CreateMyFont(m_settings.strSrcFont.c_str(), m_settings.nSrcFontSize);

        m_settings.strBinFont = str2.substr(0, k2);
        m_settings.nBinFontSize = mstr_parse_int(str2.substr(k2 + 2).c_str());
        DestroyBinFont();
        m_hBinFont = CreateMyFont(m_settings.strBinFont.c_str(), m_settings.nBinFontSize);

        EndDialog(IDOK);
    }

    HFONT CreateMyFont(const TCHAR *pszName, INT nPointSize)
    {
        HFONT hFont = NULL;
        LOGFONT lf;
        ZeroMemory(&lf, sizeof(lf));
        if (HDC hDC = CreateCompatibleDC(NULL))
        {
            lf.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
            lstrcpy(lf.lfFaceName, pszName);
            hFont = CreateFontIndirect(&lf);
            DeleteDC(hDC);
        }
        return hFont;
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR szText1[128];
        GetDlgItemText(hwnd, edt1, szText1, _countof(szText1));
        MString str1 = szText1;
        mstr_trim(str1);
        size_t k1 = str1.find(L", ");

        LOGFONT lf;
        ZeroMemory(&lf, sizeof(lf));

        if (k1 != MString::npos)
        {
            lstrcpy(lf.lfFaceName, str1.substr(0, k1).c_str());
            INT nPointSize = mstr_parse_int(str1.substr(k1 + 2).c_str());

            HDC hDC = CreateCompatibleDC(NULL);
            lf.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
            DeleteDC(hDC);
        }

        CHOOSEFONT cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.lStructSize = sizeof(cf);
        cf.hwndOwner = hwnd;
        cf.lpLogFont = &lf;
        cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL |
                   CF_NOVERTFONTS | CF_SCREENFONTS | CF_FIXEDPITCHONLY;
        if (ChooseFont(&cf))
        {
            INT nPointSize = (cf.iPointSize + 5) / 10;

            str1 = lf.lfFaceName;
            str1 += L", ";
            str1 += mstr_dec_short(nPointSize);
            str1 += L"pt";
            SetDlgItemText(hwnd, edt1, str1.c_str());

            DestroySrcFont();
            m_hSrcFont = CreateMyFont(lf.lfFaceName, nPointSize);

            SetWindowFont(GetDlgItem(hwnd, stc1), m_hSrcFont, TRUE);
        }
    }

    void OnPsh2(HWND hwnd)
    {
        WCHAR szText2[128];
        GetDlgItemText(hwnd, edt2, szText2, _countof(szText2));
        MString str2 = szText2;
        mstr_trim(str2);
        size_t k2 = str2.find(L", ");

        LOGFONT lf;
        ZeroMemory(&lf, sizeof(lf));

        if (k2 != MString::npos)
        {
            lstrcpy(lf.lfFaceName, str2.substr(0, k2).c_str());
            INT nPointSize = mstr_parse_int(str2.substr(k2 + 2).c_str());

            HDC hDC = CreateCompatibleDC(NULL);
            lf.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
            DeleteDC(hDC);
        }

        CHOOSEFONT cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.lStructSize = sizeof(cf);
        cf.hwndOwner = hwnd;
        cf.lpLogFont = &lf;
        cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL |
                   CF_NOVERTFONTS | CF_SCREENFONTS | CF_FIXEDPITCHONLY;
        if (ChooseFont(&cf))
        {
            INT nPointSize = (cf.iPointSize + 5) / 10;

            str2 = lf.lfFaceName;
            str2 += L", ";
            str2 += mstr_dec_short(nPointSize);
            str2 += L"pt";
            SetDlgItemText(hwnd, edt2, str2.c_str());

            DestroyBinFont();
            m_hBinFont = CreateMyFont(lf.lfFaceName, nPointSize);

            SetWindowFont(GetDlgItem(hwnd, stc2), m_hBinFont, TRUE);
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        case psh2:
            OnPsh2(hwnd);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        default:
            return DefaultProcDx();
        }
    }

protected:
    MHyperLinkCtrl m_hyperlink;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MFONTSDLG_HPP_
