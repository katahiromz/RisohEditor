// MCtrlPropDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MCTRLPROPDLG_HPP_
#define MZC4_MCTRLPROPDLG_HPP_

#include "RisohEditor.hpp"
#include "DialogRes.hpp"
#include "MString.hpp"
#include "resource.h"
#include <set>

//////////////////////////////////////////////////////////////////////////////

void GetSelection(HWND hLst, std::vector<BYTE>& sel);
void GetSelection(std::vector<BYTE>& sel,
                  const ConstantsDB::TableType& table, DWORD dwValue);
DWORD AnalyseDifference(DWORD dwValue, ConstantsDB::TableType& table,
    std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitClassComboBox(HWND hCmb, ConstantsDB& db, LPCTSTR pszClass);
void InitWndClassComboBox(HWND hCmb, ConstantsDB& db, LPCTSTR pszWndClass);
void InitCtrlIDComboBox(HWND hCmb, ConstantsDB& db);

//////////////////////////////////////////////////////////////////////////////

class MCtrlPropDlg : public MDialogBase
{
public:
    enum Flags
    {
        F_NONE = 0,
        F_HELP = 0x0001,
        F_STYLE = 0x0002,
        F_EXSTYLE = 0x0004,
        F_X = 0x0008,
        F_Y = 0x0010,
        F_CX = 0x0020,
        F_CY = 0x0040,
        F_ID = 0x0080,
        F_CLASS = 0x0100,
        F_TITLE = 0x0200,
        F_ALL = 0x03FF
    };
    DialogRes&          m_dialog_res;
    BOOL                m_bUpdating;
    std::set<INT>       m_indeces;
    ConstantsDB&        m_db;
    DWORD               m_flags;
    DWORD               m_dwStyle;
    DWORD               m_dwExStyle;
    DialogItem          m_item;
    ConstantsDB::TableType  m_style_table;
    ConstantsDB::TableType  m_exstyle_table;
    std::vector<BYTE>       m_style_selection;
    std::vector<BYTE>       m_exstyle_selection;

    MCtrlPropDlg(DialogRes& dialog_res, const std::set<INT>& indeces, ConstantsDB& db)
        : MDialogBase(IDD_CTRLPROP), m_dialog_res(dialog_res),
          m_bUpdating(FALSE), m_indeces(indeces), m_db(db)
    {
    }

    void GetInfo()
    {
        if (m_indeces.empty())
            return;

        m_flags = F_ALL;
        std::set<INT>::iterator it, end = m_indeces.end();
        it = m_indeces.begin();
        {
            DialogItem& item = m_dialog_res.Items[*it];
            m_item = item;
        }
        for (++it; it != end; ++it)
        {
            DialogItem& item = m_dialog_res.Items[*it];
            if (m_item.m_HelpID != item.m_HelpID)
                m_flags &= ~F_HELP;
            if (m_item.m_Style != item.m_Style)
                m_flags &= ~F_STYLE;
            if (m_item.m_ExStyle != item.m_ExStyle)
                m_flags &= ~F_EXSTYLE;
            if (m_item.m_pt.x != item.m_pt.x)
                m_flags &= ~F_X;
            if (m_item.m_pt.y != item.m_pt.y)
                m_flags &= ~F_Y;
            if (m_item.m_siz.cx != item.m_siz.cx)
                m_flags &= ~F_CX;
            if (m_item.m_siz.cy != item.m_siz.cy)
                m_flags &= ~F_CY;
            if (m_item.m_ID != item.m_ID)
                m_flags &= ~F_ID;
            if (m_item.m_Class != item.m_Class)
                m_flags &= ~F_CLASS;
            if (m_item.m_Title  != item.m_Title)
                m_flags &= ~F_TITLE;
        }

        if (m_flags & F_CLASS)
        {
            if (m_item.m_Class.is_int())
            {
                std::wstring cls;
                if (IDToPredefClass(m_item.m_Class.m_ID, cls))
                    m_item.m_Class = cls.c_str();
            }
        }
    }

    DWORD GetItemAndFlags(DialogItem& item)
    {
        DWORD flags = m_flags;

        MString strCaption = GetDlgItemText(cmb2);
        mstr_trim(strCaption);
        if (!strCaption.empty())
            flags |= F_TITLE;
        item.m_Title = strCaption.c_str();

        MString strX = GetDlgItemText(edt1);
        mstr_trim(strX);
        if (!strX.empty())
            flags |= F_X;
        item.m_pt.x = _tcstol(strX.c_str(), NULL, 0);

        MString strY = GetDlgItemText(edt2);
        mstr_trim(strY);
        if (!strY.empty())
            flags |= F_Y;
        item.m_pt.y = _tcstol(strY.c_str(), NULL, 0);

        MString strCX = GetDlgItemText(edt3);
        mstr_trim(strCX);
        if (!strCX.empty())
            flags |= F_CX;
        item.m_siz.cx = _tcstol(strCX.c_str(), NULL, 0);

        MString strCY = GetDlgItemText(edt4);
        mstr_trim(strCY);
        if (!strCY.empty())
            flags |= F_CY;
        item.m_siz.cy = _tcstol(strCY.c_str(), NULL, 0);

        MString strID = GetDlgItemText(cmb3);
        mstr_trim(strID);
        if (!strID.empty())
            flags |= F_ID;
        UINT id;
        if ((TEXT('0') <= strID[0] && strID[0] <= TEXT('9')) ||
            strID[0] == TEXT('-') || strID[0] == TEXT('+'))
        {
            id = _tcstol(strID.c_str(), NULL, 0);
        }
        else if (m_db.HasCtrlID(strID))
        {
            id = m_db.GetCtrlIDValue(strID);
        }
        else if (m_db.HasResID(strID))
        {
            id = m_db.GetResIDValue(strID);
        }
        else
        {
            HWND hCmb3 = GetDlgItem(m_hwnd, cmb3);
            Edit_SetSel(hCmb3, 0, -1);
            SetFocus(hCmb3);
            ErrorBoxDx(IDS_NOSUCHID);
            return 0xFFFFFFFF;
        }
        item.m_ID = id;

        MString strClass = GetDlgItemText(cmb4);
        mstr_trim(strClass);
        if (!strClass.empty())
            flags |= F_CLASS;
        item.m_Class = strClass.c_str();

        MString strHelp = GetDlgItemText(cmb5);
        mstr_trim(strHelp);
        if (!strHelp.empty())
            flags |= F_HELP;
        if (m_db.HasResID(strHelp))
        {
            item.m_HelpID = m_db.GetResIDValue(strHelp);
        }
        else
        {
            item.m_HelpID = _tcstol(strHelp.c_str(), NULL, 0);
        }

        MString strStyle = GetDlgItemText(edt6);
        mstr_trim(strStyle);
        if (!strStyle.empty())
            flags |= F_STYLE;
        item.m_Style = _tcstoul(strStyle.c_str(), NULL, 16);

        MString strExStyle = GetDlgItemText(edt7);
        mstr_trim(strExStyle);
        if (!strExStyle.empty())
            flags |= F_EXSTYLE;
        item.m_ExStyle = _tcstoul(strExStyle.c_str(), NULL, 16);

        return flags;
    }

    BOOL SetInfo(DWORD flags)
    {
        if (m_indeces.empty())
            return TRUE;

        std::set<INT>::iterator it, end = m_indeces.end();
        for (it = m_indeces.begin(); it != end; ++it)
        {
            DialogItem& item = m_dialog_res.Items[*it];
            if ((m_flags & F_HELP) || (flags & F_HELP))
                item.m_HelpID = m_item.m_HelpID;
            if ((m_flags & F_STYLE) || (flags & F_STYLE))
                item.m_Style = m_item.m_Style;
            if ((m_flags & F_EXSTYLE) || (flags & F_EXSTYLE))
                item.m_ExStyle = m_item.m_ExStyle;
            if ((m_flags & F_X) || (flags & F_X))
                item.m_pt.x = m_item.m_pt.x;
            if ((m_flags & F_Y) || (flags & F_Y))
                item.m_pt.y = m_item.m_pt.y;
            if ((m_flags & F_CX) || (flags & F_CX))
                item.m_siz.cx = m_item.m_siz.cx;
            if ((m_flags & F_CY) || (flags & F_CY))
                item.m_siz.cy = m_item.m_siz.cy;
            if ((m_flags & F_ID) || (flags & F_ID))
                item.m_ID = m_item.m_ID;
            if ((m_flags & F_CLASS) || (flags & F_CLASS))
            {
                item.m_Class = m_item.m_Class;
                WNDCLASSEX cls;
                if (!item.m_Class.empty() &&
                    !GetClassInfoEx(NULL, item.m_Class.str().c_str(), &cls))
                {
                    HWND hCmb4 = GetDlgItem(m_hwnd, cmb4);
                    ComboBox_SetEditSel(hCmb4, 0, -1);
                    SetFocus(hCmb4);
                    ErrorBoxDx(IDS_ENTERCLASS);
                    return FALSE;
                }
            }
            if ((m_flags & F_TITLE) || (flags & F_TITLE))
                item.m_Title = m_item.m_Title;
        }
        return TRUE;
    }

    void InitTables(LPCTSTR pszClass)
    {
        ConstantsDB::TableType table;

        m_style_table.clear();
        if (pszClass && pszClass[0])
        {
            table = m_db.GetTable(pszClass);
            if (table.size())
            {
                m_style_table.insert(m_style_table.end(),
                    table.begin(), table.end());
            }
        }
        table = m_db.GetTable(TEXT("STYLE"));
        if (table.size())
        {
            m_style_table.insert(m_style_table.end(),
                table.begin(), table.end());
        }
        m_style_selection.resize(m_style_table.size());

        m_exstyle_table.clear();
        table = m_db.GetTable(TEXT("EXSTYLE"));
        if (table.size())
        {
            m_exstyle_table.insert(m_exstyle_table.end(),
                table.begin(), table.end());
        }
        m_exstyle_selection.resize(m_exstyle_table.size());
    }

    void ApplySelection(HWND hLst, std::vector<BYTE>& sel)
    {
        m_bUpdating = TRUE;
        INT iTop = ListBox_GetTopIndex(hLst);
        for (size_t i = 0; i < sel.size(); ++i)
        {
            ListBox_SetSel(hLst, sel[i], (DWORD)i);
        }
        ListBox_SetTopIndex(hLst, iTop);
        m_bUpdating = FALSE;
    }

    void ApplySelection(HWND hLst, ConstantsDB::TableType& table,
                        std::vector<BYTE>& sel, DWORD dwValue)
    {
        m_bUpdating = TRUE;
        INT iTop = ListBox_GetTopIndex(hLst);
        for (size_t i = 0; i < table.size(); ++i)
        {
            sel[i] = ((dwValue & table[i].mask) == table[i].value);
            ListBox_SetSel(hLst, sel[i], (DWORD)i);
        }
        ListBox_SetTopIndex(hLst, iTop);
        m_bUpdating = FALSE;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        InitClassComboBox(hCmb1, m_db, TEXT(""));

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitCtrlIDComboBox(hCmb3, m_db);

        GetInfo();

        if (m_flags & F_CLASS)
        {
            InitTables(m_item.m_Class.c_str());
        }
        else
        {
            InitTables(NULL);
        }

        HWND hCmb4 = GetDlgItem(hwnd, cmb4);
        InitWndClassComboBox(hCmb4, m_db, m_item.m_Class.c_str());

        TCHAR szText[64];

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        if (m_flags & F_STYLE)
            m_dwStyle = m_item.m_Style;
        else
            m_dwStyle = 0;
        GetSelection(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        if (m_flags & F_STYLE)
        {
            m_bUpdating = TRUE;
            wsprintf(szText, TEXT("%08lX"), m_dwStyle);
            SetDlgItemText(hwnd, edt6, szText);
            ::SendDlgItemMessage(hwnd, edt6, EM_SETLIMITTEXT, 8, 0);
            m_bUpdating = FALSE;
        }

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        if (m_flags & F_EXSTYLE)
            m_dwExStyle = m_item.m_ExStyle;
        else
            m_dwExStyle = 0;
        GetSelection(m_exstyle_selection, m_exstyle_table, m_dwExStyle);
        InitStyleListBox(hLst2, m_exstyle_table);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        if (m_flags & F_EXSTYLE)
        {
            m_bUpdating = TRUE;
            wsprintf(szText, TEXT("%08lX"), m_dwExStyle);
            SetDlgItemText(hwnd, edt7, szText);
            ::SendDlgItemMessage(hwnd, edt7, EM_SETLIMITTEXT, 8, 0);
            m_bUpdating = FALSE;
        }

        if (m_flags & F_HELP)
        {
            MStringW name = m_db.GetNameOfResID(IDTYPE_HELP, m_item.m_HelpID);
            SetDlgItemTextW(hwnd, cmb5, name.c_str());
        }
        if (m_flags & F_X)
        {
            SetDlgItemInt(hwnd, edt1, m_item.m_pt.x, TRUE);
        }
        if (m_flags & F_Y)
        {
            SetDlgItemInt(hwnd, edt2, m_item.m_pt.y, TRUE);
        }
        if (m_flags & F_CX)
        {
            SetDlgItemInt(hwnd, edt3, m_item.m_siz.cx, TRUE);
        }
        if (m_flags & F_CY)
        {
            SetDlgItemInt(hwnd, edt4, m_item.m_siz.cy, TRUE);
        }
        if (m_flags & F_ID)
        {
            MStringW name = m_db.GetNameOfResID(IDTYPE_CONTROL, m_item.m_HelpID);
            SetDlgItemTextW(hwnd, cmb3, name.c_str());
        }
        if (m_flags & F_CLASS)
        {
            SetDlgItemText(hwnd, cmb4, m_item.m_Class.c_str());
        }
        if (m_flags & F_TITLE)
        {
            SetDlgItemText(hwnd, cmb2, m_item.m_Title.c_str_or_empty());
        }

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        DWORD flags = GetItemAndFlags(m_item);
        if (flags == 0xFFFFFFFF)
        {
            return;
        }
        if (flags & F_CLASS)
        {
            MString strClass = GetDlgItemText(cmb4);
            mstr_trim(strClass);
            if (strClass.empty())
            {
                SetFocus(GetDlgItem(hwnd, cmb4));
                MsgBoxDx(LoadStringDx(IDS_ENTERCLASS), MB_ICONERROR);
                return;
            }
        }

        if (!SetInfo(flags))
            return;

        EndDialog(IDOK);
    }

    void OnLst1(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst1 = GetDlgItem(hwnd, lst1);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetSelection(hLst1, m_style_selection);

        m_dwStyle = AnalyseDifference(m_dwStyle, m_style_table,
                                      old_style_selection, m_style_selection);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        wsprintf(szText, TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        m_bUpdating = FALSE;
    }

    void OnLst2(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst2 = GetDlgItem(hwnd, lst2);

        std::vector<BYTE> old_exstyle_selection = m_exstyle_selection;
        GetSelection(hLst2, m_exstyle_selection);

        m_dwExStyle = AnalyseDifference(m_dwExStyle, m_exstyle_table,
                                        old_exstyle_selection, m_exstyle_selection);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        wsprintf(szText, TEXT("%08lX"), m_dwExStyle);
        SetDlgItemText(hwnd, edt7, szText);
        m_bUpdating = FALSE;
    }

    void OnEdt6(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        MString text = GetDlgItemText(hwnd, edt6);
        mstr_trim(text);
        DWORD dwStyle = _tcstoul(text.c_str(), NULL, 16);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetSelection(m_style_selection, m_style_table, dwStyle);

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        m_dwStyle = dwStyle;
        ApplySelection(hLst1, m_style_table, m_style_selection, dwStyle);
    }

    void OnEdt7(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        MString text = GetDlgItemText(hwnd, edt7);
        mstr_trim(text);
        DWORD dwExStyle = _tcstoul(text.c_str(), NULL, 16);

        std::vector<BYTE> old_exstyle_selection = m_exstyle_selection;
        GetSelection(m_exstyle_selection, m_exstyle_table, dwExStyle);

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        m_dwExStyle = dwExStyle;
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, dwExStyle);
    }

    void UpdateClass(HWND hwnd, HWND hLst1, const MString& strClass)
    {
        MString strSuper;
        DWORD dwType = m_db.GetValue(TEXT("CONTROL.CLASSES"), strClass);
        if (dwType >= 3)
        {
            ConstantsDB::TableType table;
            table = m_db.GetTable(strClass + TEXT(".SUPERCLASS"));
            if (table.size())
            {
                strSuper = table[0].name;
            }
        }

        if (strSuper.size())
            InitTables(strSuper.c_str());
        else
            InitTables(strClass.c_str());

        MString str = strClass + TEXT(".DEFAULT.STYLE");
        m_dwStyle = m_db.GetValue(str, TEXT("STYLE"));

        GetSelection(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        wsprintf(szText, TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        m_bUpdating = FALSE;

        ListBox_SetTopIndex(hLst1, 0);

        if (strSuper.size())
            SetDlgItemText(hwnd, cmb4, strSuper.c_str());
        else
            SetDlgItemText(hwnd, cmb4, strClass.c_str());
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        HWND hCmb4 = GetDlgItem(hwnd, cmb4);
        TCHAR szText[64];
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        case cmb1:
            if (codeNotify == CBN_SELCHANGE)
            {
                INT nIndex = ComboBox_GetCurSel(hCmb1);
                ComboBox_GetLBText(hCmb1, nIndex, szText);
                MString text = szText;
                mstr_trim(text);
                UpdateClass(hwnd, hLst1, text);
            }
            else if (codeNotify == CBN_EDITCHANGE)
            {
                MString text = GetDlgItemText(hwnd, cmb1);
                mstr_trim(text);
                InitTables(text.c_str());
                UpdateClass(hwnd, hLst1, text);
            }
            break;
        case cmb4:
            if (codeNotify == CBN_SELCHANGE)
            {
                INT nIndex = ComboBox_GetCurSel(hCmb4);
                ComboBox_GetLBText(hCmb4, nIndex, szText);
                MString text = szText;
                mstr_trim(text);
                UpdateClass(hwnd, hLst1, text);
            }
            else if (codeNotify == CBN_EDITCHANGE)
            {
                MString text = GetDlgItemText(hwnd, cmb4);
                mstr_trim(text);
                InitTables(text.c_str());
                UpdateClass(hwnd, hLst1, text);
            }
            break;
        case lst1:
            if (codeNotify == LBN_SELCHANGE)
            {
                OnLst1(hwnd);
            }
            break;
        case lst2:
            if (codeNotify == LBN_SELCHANGE)
            {
                OnLst2(hwnd);
            }
            break;
        case edt6:
            if (codeNotify == EN_CHANGE)
            {
                OnEdt6(hwnd);
            }
            break;
        case edt7:
            if (codeNotify == EN_CHANGE)
            {
                OnEdt7(hwnd);
            }
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
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MCTRLPROPDLG_HPP_
