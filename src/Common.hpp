#pragma once

enum LANG_TYPE
{
    LANG_TYPE_0,
    LANG_TYPE_1,
    LANG_TYPE_2
};

// Helper function to get ComboBox edit text without buffer size limitations
inline MStringW GetComboBoxText(HWND hwndCombo)
{
    INT cch = ComboBox_GetTextLength(hwndCombo);
    if (cch <= 0)
        return MStringW();

    MStringW str;
    str.resize(cch + 1);
    ComboBox_GetText(hwndCombo, &str[0], cch + 1);
    str.resize(cch);  // Remove null terminator from string length
    return str;
}

// Helper function to get ComboBox listbox text without buffer size limitations
inline MStringW GetComboBoxLBText(HWND hwndCombo, INT nIndex)
{
    INT cch = ComboBox_GetLBTextLen(hwndCombo, nIndex);
    if (cch == CB_ERR)
        return MStringW();

    MStringW str;
    str.resize(cch);
    cch = ComboBox_GetLBText(hwndCombo, nIndex, &str[0]);
    if (cch == CB_ERR)
        return MStringW();

    return str;
}

// Helper function to get ListBox text without buffer size limitations
inline MStringW GetListBoxText(HWND hwndListBox, INT nIndex)
{
    INT cch = ListBox_GetTextLen(hwndListBox, nIndex);
    if (cch == LB_ERR)
        return MStringW();

    MStringW str;
    str.resize(cch);
    cch = ListBox_GetText(hwndListBox, nIndex, &str[0]);
    if (cch == LB_ERR)
        return MStringW();

    return str;
}

// Helper function to get window text without buffer size limitations
inline MStringW GetWindowTextW(HWND hwnd)
{
    INT cch = ::GetWindowTextLengthW(hwnd);
    if (cch <= 0)
        return MStringW();

    MStringW str;
    str.resize(cch);
    ::GetWindowTextW(hwnd, &str[0], cch + 1);
    return str;
}

// Helper function to get dialog item text without buffer size limitations
inline MStringW GetDlgItemTextW(HWND hwnd, INT nCtrlID)
{
    return GetWindowTextW(::GetDlgItem(hwnd, nCtrlID));
}

// Helper function to get ListView item text without buffer size limitations
inline MStringW GetListViewItemText(HWND hwndListView, INT iItem, INT iSubItem)
{
    // First try with a reasonable initial buffer size
    MStringW str;
    INT cchBuffer = 256;
    
    for (;;)
    {
        str.resize(cchBuffer);
        LVITEMW lvi;
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.iSubItem = iSubItem;
        lvi.pszText = &str[0];
        lvi.cchTextMax = cchBuffer;
        INT cch = (INT)SendMessageW(hwndListView, LVM_GETITEMTEXTW, iItem, (LPARAM)&lvi);
        if (cch < cchBuffer - 1)
        {
            str.resize(cch);
            break;
        }
        // Buffer was too small, double it and try again
        cchBuffer *= 2;
        if (cchBuffer > 65536)  // Safety limit
        {
            str.resize(cch);
            break;
        }
    }
    return str;
}

BOOL CheckCommand(MString strCommand);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang, LANG_TYPE type);
BOOL CheckNameComboBox(HWND hCmb2, MIdOrString& name);
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type);
BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, std::wstring& str);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& file);
BOOL InitLangListBox(HWND hwnd);
BOOL ChooseLangListBoxLang(HWND hwnd, LANGID wLangId);
BOOL IsThereWndClass(const WCHAR *pszName);
BYTE GetCharSetFromComboBox(HWND hCmb);
DWORD AnalyseStyleDiff(DWORD dwValue, ConstantsDB::TableType& table, std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
MString GetAssoc(const MString& name);
MString GetLanguageStatement(WORD langid, BOOL bOldStyle);
MStringW GetRisohTemplate(const MIdOrString& type, const MIdOrString& name, WORD wLang);
MStringW TextFromLang(WORD lang);
WORD LangFromText(LPWSTR pszLang);
std::vector<INT> GetPrefixIndexes(const MString& prefix);
std::wstring GetKeyID(UINT wId);
void Cmb1_InitVirtualKeys(HWND hCmb1);
void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel);
void GetStyleSelect(std::vector<BYTE>& sel, const ConstantsDB::TableType& table, DWORD dwValue);
void InitCaptionComboBox(HWND hCmb, LPCTSTR pszCaption);
void InitCharSetComboBox(HWND hCmb, BYTE CharSet);
void InitClassComboBox(HWND hCmb, LPCTSTR pszClass);
void InitComboBoxPlaceholder(HWND hCmb, UINT nStringID);
void InitConstantComboBox(HWND hCmb);
void InitCtrlIDComboBox(HWND hCmb);
void InitFontComboBox(HWND hCmb);
void InitLangComboBox(HWND hCmb3, DWORD langid);
void InitLangComboBox(HWND hCmb3, DWORD langid, BOOL bUILanguage);
void InitLangListView(HWND hLst1, LPCTSTR pszText);
void InitMessageComboBox(HWND hCmb, const MString& strString);
void InitResNameComboBox(HWND hCmb, const MIdOrString& id, IDTYPE_ nIDTYPE_);
void InitResNameComboBox(HWND hCmb, const MIdOrString& id, INT nIDTYPE_1, INT nIDTYPE_2);
void InitResTypeComboBox(HWND hCmb1, const MIdOrString& type);
void InitStringComboBox(HWND hCmb, const MString& strString);
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitWndClassComboBox(HWND hCmb, LPCTSTR pszWndClass);
void ReplaceFullWithHalf(LPWSTR pszText);
void ReplaceFullWithHalf(MStringW& strText);
DWORD GetDefaultResLanguage(VOID);
HRESULT FileSystemAutoComplete(HWND hwnd);
