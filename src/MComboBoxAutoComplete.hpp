// MComboBoxAutoComplete.hpp -- Win32API autocomplete combo box -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MCOMBOBOXAUTOCOMP_HPP_
#define MZC4_MCOMBOBOXAUTOCOMP_HPP_     0   /* Version 0 */

class MComboBoxAutoComplete;

#include "MEditCtrl.hpp"
#include "MComboBox.hpp"

////////////////////////////////////////////////////////////////////////////

class MComboBoxEditAutoComplete : public MEditCtrl
{
public:
    MComboBoxEditAutoComplete() : m_bAutoComplete(FALSE)
    {
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (vk == VK_DELETE || vk == VK_BACK)
            m_bAutoComplete = FALSE;
        else
            m_bAutoComplete = TRUE;

        if (fDown)
        {
            FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, DefaultProcDx);
        }
        else
        {
            FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, DefaultProcDx);
        }
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
            HANDLE_MSG(hwnd, WM_KEYUP, OnKey);
        }
        return DefaultProcDx();
    }

public:
    BOOL m_bAutoComplete;
};

class MComboBoxAutoComplete : public MComboBox
{
public:
    MComboBoxAutoComplete()
    {
    }

    virtual void PostSubclassDx(HWND hwnd)
    {
        HWND hEdit = FindWindowEx(hwnd, NULL, TEXT("EDIT"), NULL);
        m_edit.SubclassDx(hEdit);
    }

    void OnEditChange()
    {
        DWORD dwPos;
        OnEditChange(dwPos);
    }

    void OnEditChange(DWORD& dwPos)
    {
        if (!m_edit.m_bAutoComplete)
        {
            return;
        }

        MString strInput = GetWindowText();

        dwPos = GetEditSel();
        MString strRight = strInput.substr(HIWORD(dwPos));
        mstr_trim(strRight);
        if (!strRight.empty())
            return;

        mstr_trim(strInput);
        MString strInputUpper = strInput;
        CharUpper(&strInputUpper[0]);

        INT iItem = FindString(-1, strInput.c_str());
        if (iItem == CB_ERR)
            return;

        TCHAR szText[128];
        GetLBText(iItem, szText);
        MString strCandidate = szText;

        INT nCount = GetCount();
        for (INT i = iItem + 1; i < nCount; ++i)
        {
            GetLBText(i, szText);
            MString strText = szText;
            CharUpper(&strText[0]);
            if (strText.find(strInputUpper) == 0)
            {
                return;
            }
        }

        m_edit.m_bAutoComplete = FALSE;
        SetWindowText(strCandidate.c_str());
        SetEditSel(INT(strInput.size()), INT(strCandidate.size()));
        dwPos = MAKELONG(INT(strInput.size()), INT(strCandidate.size()));
    }

public:
    MComboBoxEditAutoComplete m_edit;
};

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MCOMBOBOXAUTOCOMP_HPP_
