#ifndef MZC4_MIDLISTDLG_HPP_
#define MZC4_MIDLISTDLG_HPP_

#include "MWindowBase.hpp"
#include "resource.h"

class MIDListDlg : public MDialogBase
{
public:
    MIDListDlg() : MDialogBase(IDD_IDLIST)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_hLst1 = GetDlgItem(hwnd, lst1);
        ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 160;
        column.pszText = LoadStringDx(IDS_NAME);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 90;
        column.pszText = LoadStringDx(IDS_IDTYPE);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 80;
        column.pszText = LoadStringDx(IDS_VALUE);
        column.iSubItem = 2;
        ListView_InsertColumn(m_hLst1, 2, &column);

        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDCANCEL:
            ::DestroyWindow(hwnd);
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
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        default:
            return DefaultProcDx();
        }
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        MoveWindow(m_hLst1, 0, 0, cx, cy, TRUE);
    }

protected:
    HWND m_hLst1;
};

#endif  // ndef MZC4_MIDLISTDLG_HPP_
