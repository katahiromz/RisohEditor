// MMainWnd.hpp --- MMainWnd class declaration
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "RisohEditor.hpp"
#include "MLangAutoComplete.hpp"

//////////////////////////////////////////////////////////////////////////////
// MMainWnd --- the main window

// the file type
enum FileType
{
	FT_NONE,
	FT_EXECUTABLE,
	FT_RC,
	FT_RES
};

class MMainWnd : public MWindowBase
{
protected:
	INT         m_argc;         // number of command line parameters
	TCHAR **    m_targv;        // command line parameters
	BOOL        m_bLoading;     // loading now?

	// handles
	HINSTANCE   m_hInst;        // the instance handle
	HICON       m_hIcon;        // the icon handle
	HICON       m_hIconSm;      // the small icon handle
	HACCEL      m_hAccel;       // the accelerator handle
	HWND        m_hwndTV;       // the tree control
	HIMAGELIST  m_hImageList;   // the image list for m_hwndTV
	INT         m_nCommandLock; // the lock count of WM_COMMAND message
	HICON       m_hFileIcon;    // the file icon
	HICON       m_hFolderIcon;  // the folder icon
	HFONT       m_hSrcFont;     // the source font
	HFONT       m_hBinFont;     // the binary font
	HWND        m_hToolBar;     // the toolbar window handle
	HWND        m_hStatusBar;   // the status bar handle
	HWND        m_hFindReplaceDlg;  // the find/replace dialog handle
	HIMAGELIST  m_himlTools;        // the image list for the toolbar

	// data and sub-programs
	WCHAR       m_szDataFolder[MAX_PATH];       // the data folder location
	WCHAR       m_szConstantsFile[MAX_PATH];    // the Constants.txt file location
	WCHAR       m_szMCppExe[MAX_PATH];          // the mcpp.exe location
	WCHAR       m_szWindresExe[MAX_PATH];       // the windres.exe location
	WCHAR       m_szUpxExe[MAX_PATH];           // the upx.exe location
	WCHAR       m_szMcdxExe[MAX_PATH];          // the mcdx.exe location
	WCHAR       m_szDFMSC[MAX_PATH];            // the dfmsc.exe location
	WCHAR       m_szOleBow[MAX_PATH];           // the OleBow program location
	WCHAR       m_szMidlWrap[MAX_PATH];         // the midlwrap.bat location
	WCHAR       m_szVCBat[MAX_PATH];            // the vcvarsall.bat location
	WCHAR       m_szIncludeDir[MAX_PATH];       // the include directory
	INT         m_nStatusStringID;

	// file info
	FileType    m_file_type;
	WCHAR       m_szFile[MAX_PATH];             // the file location
	WCHAR       m_szResourceH[MAX_PATH];        // the resource.h file location
	BOOL        m_bUpxCompressed;               // is the real file compressed?

	BOOL UpdateFileInfo(FileType ft, LPCWSTR pszFile, BOOL bCompressed);
	void UpdateTitleBar();

	// selection
	MIdOrString     m_type;
	MIdOrString     m_name;
	WORD            m_lang;

	// classes
	MRadWindow      m_rad_window;               // the RADical window
	MEditCtrl       m_hHexViewer;               // the EDIT control for binary
	HWND            m_hCodeEditor;              // the EDIT control for source
	MBmpView        m_hBmpView;                 // the bitmap view
	MSplitterWnd    m_splitter1;                // 1st splitter window
	MSplitterWnd    m_splitter2;                // 2nd splitter window
	MIDListDlg      m_id_list_dlg;              // the ID List window
	ITEM_SEARCH     m_search;                   // the search options
	MTabCtrl        m_tab;                      // the tab control

	// auto completion
	MLangAutoCompleteEdit   m_auto_comp_edit;
	MLangAutoComplete *m_pAutoComplete;

public:
	MDropdownArrow  m_arrow;                    // the language drop-down arrow
	MStringW m_commands;
	MStringW m_load_options;
	MStringW m_save_options;

	BOOL ParseCommandLine(HWND hwnd, INT argc, WCHAR **targv);

	// constructor
	MMainWnd(int argc, TCHAR **targv, HINSTANCE hInst) :
		m_argc(argc), m_targv(targv), m_bLoading(FALSE),
		m_hInst(hInst), m_hIcon(NULL), m_hIconSm(NULL), m_hAccel(NULL),
		m_hwndTV(NULL), m_hImageList(NULL), m_nCommandLock(0),
		m_hFileIcon(NULL), m_hFolderIcon(NULL), m_hSrcFont(NULL), m_hBinFont(NULL),
		m_hToolBar(NULL), m_hStatusBar(NULL),
		m_hFindReplaceDlg(NULL), m_himlTools(NULL), m_file_type(FT_NONE)
	{
		m_szDataFolder[0] = 0;
		m_szConstantsFile[0] = 0;
		m_szMCppExe[0] = 0;
		m_szWindresExe[0] = 0;
		m_szUpxExe[0] = 0;
		m_szMcdxExe[0] = 0;
		m_szDFMSC[0] = 0;
		m_szOleBow[0] = 0;
		m_szMidlWrap[0] = 0;
		m_szVCBat[0] = 0;
		m_szIncludeDir[0] = 0;
		m_nStatusStringID = 0;
		m_szFile[0] = 0;
		m_szResourceH[0] = 0;

		m_bUpxCompressed = FALSE;

		m_lang = BAD_LANG;
		m_pAutoComplete = NULL;
	}

	// settings
	void SetDefaultSettings(HWND hwnd);
	BOOL LoadSettings(HWND hwnd);
	BOOL SaveSettings(HWND hwnd);
	void UpdatePrefixDB(HWND hwnd);
	BOOL ReCreateSrcEdit(HWND hwnd);

	virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
	{
		MWindowBase::ModifyWndClassDx(wcx);

		// set a class menu
		wcx.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);

		// change the window icon
		wcx.hIcon = m_hIcon;
		wcx.hIconSm = m_hIconSm;
	}

	virtual LPCTSTR GetWndClassNameDx() const
	{
		// the window class name of the main window
		return TEXT("katahiromz's RisohEditor");
	}

	BOOL StartDx();
	INT_PTR RunDx();
	void DoEvents();
	void DoMsg(MSG& msg);

	virtual LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//////////////////////////////////////////////////////////////////////

	// status bar
	void ChangeStatusText(INT nID)
	{
		ChangeStatusText(LoadStringDx(nID));
	}
	void ChangeStatusText(LPCTSTR pszText)
	{
		SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)pszText);
	}

	// utilities
	BOOL CheckDataFolder(VOID);
	INT CheckData(VOID);

	void UpdateMenu();

	enum STV
	{
		STV_RESETTEXTANDMODIFIED,
		STV_RESETTEXT,
		STV_DONTRESET
	};

	void SelectTV(EntryBase *entry, BOOL bDoubleClick, STV stv = STV_RESETTEXTANDMODIFIED);
	void SelectTV(EntryType et, const MIdOrString& type,
				  const MIdOrString& name, WORD lang,
				  BOOL bDoubleClick, STV stv = STV_RESETTEXTANDMODIFIED);

	template <typename T_DIALOG>
	void SelectTV(EntryType et, const T_DIALOG& dialog, BOOL bDoubleClick, STV stv = STV_RESETTEXTANDMODIFIED)
	{
		SelectTV(et, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE, stv);
	}

	BOOL CompileIfNecessary(BOOL bReopen = FALSE);
	BOOL ReCompileOnSelChange(BOOL bReopen = FALSE);
	void SelectString(void);
	void SelectMessage(void);
	BOOL CreateOurToolBar(HWND hwndParent, HIMAGELIST himlTools);
	void UpdateOurToolBarButtons(INT iType);
	void UpdateToolBarStatus();
	bool IsEntryTextEditable(const EntryBase *entry);

	// ID list
	void OnIDList(HWND hwnd);
	void OnIdAssoc(HWND hwnd);
	void OnPredefMacros(HWND hwnd);
	void OnEditLabel(HWND hwnd);
	void OnSetPaths(HWND hwnd);
	void OnShowLangs(HWND hwnd);
	void OnShowHideToolBar(HWND hwnd);

	// show/hide
	void ShowIDList(HWND hwnd, BOOL bShow = TRUE);

	enum SHOW_MODE {
		SHOW_MOVIE, SHOW_CODEONLY, SHOW_CODEANDBMP
	};
	SHOW_MODE m_nShowMode;
	void SetShowMode(SHOW_MODE mode);
	void ShowStatusBar(BOOL bShow = TRUE);
	BOOL ShowLangArrow(BOOL bShow, HTREEITEM hItem = NULL);
	void UpdateLangArrow();
	void PostUpdateLangArrow(HWND hwnd);

	// preview
	VOID HidePreview(STV stv = STV_RESETTEXTANDMODIFIED);
	BOOL Preview(HWND hwnd, const EntryBase *entry, STV stv = STV_RESETTEXTANDMODIFIED);

	// actions
	BOOL DoLoadResH(HWND hwnd, LPCTSTR pszFile);
	void DoLoadLangInfo(VOID);
	BOOL DoLoadFile(HWND hwnd, LPCWSTR pszFileName, DWORD nFilterIndex = 0, BOOL bForceDecompress = FALSE);
	BOOL DoLoadRCEx(HWND hwnd, LPCWSTR szRCFile, EntrySet& res, BOOL bApStudio = FALSE);
	BOOL DoLoadRES(HWND hwnd, LPCWSTR szPath);
	BOOL DoLoadRC(HWND hwnd, LPCWSTR szPath);
	BOOL DoLoadEXE(HWND hwnd, LPCWSTR pszPath, BOOL bForceDecompress);
	BOOL DoExtract(const EntryBase *entry, BOOL bExporting);
	BOOL DoExportRC(LPCWSTR pszRCFile, LPWSTR pszResHFile = NULL);
	BOOL DoExportRC(LPCWSTR pszRCFile, LPWSTR pszResHFile, const EntrySet& found);
	BOOL DoExportRes(LPCWSTR pszResFile);
	void DoIDStat(UINT anValues[5]);
	BOOL DoBackupFile(LPCWSTR pszFileName, UINT nCount = 0);
	BOOL DoBackupFolder(LPCWSTR pszFileName, UINT nCount = 0);
	BOOL DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH);
	BOOL DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH, const EntrySet& found);
	BOOL DoWriteRCLang(MFile& file, ResToText& res2text, WORD lang, const EntrySet& targets);
	BOOL DoWriteRCLangUTF8(MFile& file, ResToText& res2text, WORD lang, const EntrySet& targets);
	BOOL DoWriteRCLangUTF16(MFile& file, ResToText& res2text, WORD lang, const EntrySet& targets);
	BOOL DoWriteResH(LPCWSTR pszResH, LPCWSTR pszRCFile = NULL);
	BOOL DoWriteResHOfExe(LPCWSTR pszExeFile);
	BOOL DoSaveResAs(LPCWSTR pszResFile);
	BOOL DoSaveAs(LPCWSTR pszExeFile);
	BOOL DoSaveAsCompression(LPCWSTR pszExeFile);
	BOOL DoSaveExeAs(LPCWSTR pszExeFile, BOOL bCompression = FALSE);
	BOOL DoSaveInner(LPCWSTR pszExeFile, BOOL bCompression = FALSE);
	BOOL DoSaveFile(HWND hwnd, LPCWSTR pszFile);
	IMPORT_RESULT DoImport(HWND hwnd, LPCWSTR pszFile, LPCWSTR pchDotExt);
	IMPORT_RESULT DoImportRes(HWND hwnd, LPCWSTR pszFile);
	IMPORT_RESULT DoImportRC(HWND hwnd, LPCWSTR pszFile);
	BOOL DoUpxTest(LPCWSTR pszUpx, LPCWSTR pszFile);
	BOOL DoUpxDecompress(LPCWSTR pszUpx, LPCWSTR pszFile);
	BOOL DoUpxCompress(LPCWSTR pszUpx, LPCWSTR pszExeFile);
	void DoRenameEntry(LPWSTR pszText, EntryBase *entry, const MIdOrString& old_name, const MIdOrString& new_name);
	void DoRelangEntry(LPWSTR pszText, EntryBase *entry, WORD old_lang, WORD new_lang);
	void DoRefreshTV(HWND hwnd);
	void DoRefreshIDList(HWND hwnd);
	void DoLangEditAutoComplete(HWND hwnd, HWND hwndEdit);
	void DoLangEditAutoCompleteRelease(HWND hwnd);

	void ReCreateFonts(HWND hwnd);
	void ReSetPaths(HWND hwnd);
	BOOL DoItemSearch(ITEM_SEARCH& search);
	BOOL DoItemSearchBang(HWND hwnd, MItemSearchDlg *pDialog);
	void search_worker_thread_inner(HWND hwnd, MItemSearchDlg* pDialog);
	void search_worker_thread_outer(HWND hwnd, MItemSearchDlg* pDialog);
	void DoEnableControls(BOOL bEnable);

	bool DoResLoad(const MStringW& filename, const MStringW& options = L"");
	bool DoResSave(const MStringW& filename, const MStringW& options = L"");

	EGA::arg_t RES_load(const EGA::args_t& args);
	EGA::arg_t RES_save(const EGA::args_t& args);
	EGA::arg_t RES_search(const EGA::args_t& args);
	EGA::arg_t RES_delete(const EGA::args_t& args);
	EGA::arg_t RES_clone_by_name(const EGA::args_t& args);
	EGA::arg_t RES_clone_by_lang(const EGA::args_t& args);
	EGA::arg_t RES_unload_resh(const EGA::args_t& args);
	EGA::arg_t RES_select(const EGA::args_t& args);
	EGA::arg_t RES_get_binary(const EGA::args_t& args);
	EGA::arg_t RES_set_binary(const EGA::args_t& args);
	EGA::arg_t RES_get_text(EGA::arg_t arg0, EGA::arg_t arg1, EGA::arg_t arg2);
	EGA::arg_t RES_set_text(EGA::arg_t arg0, EGA::arg_t arg1, EGA::arg_t arg2, EGA::arg_t arg3);
	EGA::arg_t RES_const(const EGA::args_t& args);
	EGA::arg_t RES_str_get(EGA::arg_t arg0);
	EGA::arg_t RES_str_get(EGA::arg_t arg0, EGA::arg_t arg1);
	EGA::arg_t RES_str_set(EGA::arg_t arg0, EGA::arg_t arg1);
	EGA::arg_t RES_str_set(EGA::arg_t arg0, EGA::arg_t arg1, EGA::arg_t arg2);

	LRESULT CALLBACK TreeViewWndProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	// parsing resource IDs
	BOOL CompileParts(MStringA& strOutput, const MIdOrString& type, const MIdOrString& name,
					  WORD lang, const MStringW& strWide, BOOL bReopen = FALSE);
	BOOL CompileStringTable(MStringA& strOutput, WORD lang, const MStringW& strWide);
	BOOL CompileMessageTable(MStringA& strOutput, WORD lang, const MStringW& strWide);
	BOOL CompileRCData(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide);
	BOOL CompileTYPELIB(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide);
	BOOL CheckResourceH(HWND hwnd, LPCTSTR pszPath);
	BOOL ParseResH(HWND hwnd, LPCTSTR pszFile, const char *psz, DWORD len);
	BOOL ParseMacros(HWND hwnd, LPCTSTR pszFile, const std::vector<MStringA>& macros, MStringA& str);
	BOOL UnloadResourceH(HWND hwnd);
	void SetErrorMessage(const MStringA& strOutput, BOOL bBox = FALSE);
	MStringW GetMacroDump(BOOL bApStudio = FALSE) const;
	MStringW GetIncludesDump() const;
	MStringW GetIncludesDumpForWindres() const;
	void ReadResHLines(FILE *fp, std::vector<MStringA>& lines);
	void UpdateResHLines(std::vector<MStringA>& lines);

	void JoinLinesByBackslash(std::vector<MStringA>& lines);
	void DeleteIncludeGuard(std::vector<MStringA>& lines);
	void AddAdditionalMacroLines(std::vector<MStringA>& lines);
	void DeleteSpecificMacroLines(std::vector<MStringA>& lines);
	void AddApStudioBlock(std::vector<MStringA>& lines);
	void DeleteApStudioBlock(std::vector<MStringA>& lines);
	void AddHeadComment(std::vector<MStringA>& lines);
	void DeleteHeadComment(std::vector<MStringA>& lines);
	void DoAddRes(HWND hwnd, MAddResDlg& dialog);

	// preview
	void PreviewIcon(HWND hwnd, const EntryBase& entry);
	void PreviewCursor(HWND hwnd, const EntryBase& entry);
	void PreviewGroupIcon(HWND hwnd, const EntryBase& entry);
	void PreviewGroupCursor(HWND hwnd, const EntryBase& entry);
	void PreviewBitmap(HWND hwnd, const EntryBase& entry);
	void PreviewImage(HWND hwnd, const EntryBase& entry);
	void PreviewWAVE(HWND hwnd, const EntryBase& entry);
	void PreviewAVI(HWND hwnd, const EntryBase& entry);
	void PreviewAccel(HWND hwnd, const EntryBase& entry);
	void PreviewMessage(HWND hwnd, const EntryBase& entry);
	void PreviewString(HWND hwnd, const EntryBase& entry);
	void PreviewHtml(HWND hwnd, const EntryBase& entry);
	void PreviewMenu(HWND hwnd, const EntryBase& entry);
	void PreviewToolbar(HWND hwnd, const EntryBase& entry);
	void PreviewVersion(HWND hwnd, const EntryBase& entry);
	void PreviewDialog(HWND hwnd, const EntryBase& entry);
	void PreviewAniIcon(HWND hwnd, const EntryBase& entry, BOOL bIcon);
	void PreviewStringTable(HWND hwnd, const EntryBase& entry);
	void PreviewMessageTable(HWND hwnd, const EntryBase& entry);
	void PreviewRCData(HWND hwnd, const EntryBase& entry);
	void PreviewTypeLib(HWND hwnd, const EntryBase& entry);
	void PreviewDlgInit(HWND hwnd, const EntryBase& entry);
	void PreviewUnknown(HWND hwnd, const EntryBase& entry);

	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
	void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
	void OnSysColorChange(HWND hwnd);
	void OnSetFocus(HWND hwnd, HWND hwndOldFocus);
	void OnKillFocus(HWND hwnd, HWND hwndNewFocus);
	void OnPlay(HWND hwnd);
	void OnDropFiles(HWND hwnd, HDROP hdrop);
	void OnMove(HWND hwnd, int x, int y);
	void OnSize(HWND hwnd, UINT state, int cx, int cy);
	void OnInitMenu(HWND hwnd, HMENU hMenu);
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr);
	void OnClose(HWND hwnd);
	void OnDestroy(HWND hwnd);

	void OnSelChange(HWND hwnd, INT iSelected);
	void OnCancelEdit(HWND hwnd);
	void OnCompile(HWND hwnd);
	void OnGuiEdit(HWND hwnd);
	void OnEdit(HWND hwnd);
	void OnCopyAsNewName(HWND hwnd);
	void OnCopyAsNewLang(HWND hwnd);
	void OnCopyToMultiLang(HWND hwnd);
	void OnItemSearch(HWND hwnd);
	void OnExpandAll(HWND hwnd);
	void OnCollapseAll(HWND hwnd);
	void Expand(HTREEITEM hItem);
	void Collapse(HTREEITEM hItem);
	void OnWordWrap(HWND hwnd);
	void OnSaveAsWithCompression(HWND hwnd);
	void OnClone(HWND hwnd);
	void OnAddBang(HWND hwnd, NMTOOLBAR *pToolBar);
	void OnExtractBang(HWND hwnd);
	void OnGuide(HWND hwnd);
	void OnEncoding(HWND hwnd);
	void OnQueryConstant(HWND hwnd);
	void OnUseBeginEnd(HWND hwnd);
	void OnUseMSMSGTBL(HWND hwnd);
	void OnRefreshAll(HWND hwnd);

	LRESULT OnCompileCheck(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnMoveSizeReport(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnClearStatus(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnReopenRad(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPostSearch(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnIDJumpBang(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnRadSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateDlgRes(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnGetHeadLines(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnDelphiDFMB2T(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnTLB2IDL(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnIDL2TLB(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void OnIDJumpBang2(HWND hwnd, const MString& name, MString& strType);
	LRESULT OnItemSearchBang(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnComplement(HWND hwnd, WPARAM wParam, LPARAM lParam);
	BOOL DoInnerSearch(HWND hwnd);
	LRESULT OnUpdateLangArrow(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnRadDblClick(HWND hwnd, WPARAM wParam, LPARAM lParam);

	void OnAddBitmap(HWND hwnd);
	void OnAddCursor(HWND hwnd);
	void OnAddDialog(HWND hwnd);
	void OnAddIcon(HWND hwnd);
	void OnAddMenu(HWND hwnd);
	void OnAddToolbar(HWND hwnd);
	void OnAddRes(HWND hwnd);
	void OnAddVerInfo(HWND hwnd);
	void OnAddManifest(HWND hwnd);
	void OnAddStringTable(HWND hwnd);
	void OnAddMessageTable(HWND hwnd);
	void OnAddHtml(HWND hwnd);
	void OnAddAccel(HWND hwnd);
	void OnDeleteRes(HWND hwnd);
	void OnExtractBin(HWND hwnd);
	void OnExportRes(HWND hwnd);
	void OnCheckUpdate(HWND hwnd);
	void OnDfmSettings(HWND hwnd);

	void OnExtractRC(HWND hwnd);
	void OnExtractDFM(HWND hwnd);
	void OnExtractTLB(HWND hwnd);
	void OnExtractBitmap(HWND hwnd);
	void OnExtractCursor(HWND hwnd);
	void OnExtractIcon(HWND hwnd);
	void OnReplaceBin(HWND hwnd);
	void OnReplaceBitmap(HWND hwnd);
	void OnReplaceCursor(HWND hwnd);
	void OnReplaceIcon(HWND hwnd);
	void OnUpdateResHBang(HWND hwnd);

	BOOL DoQuerySaveChange(HWND hwnd);

	void OnNew(HWND hwnd);
	void OnOpen(HWND hwnd);
	BOOL OnSave(HWND hwnd);
	BOOL OnSaveAs(HWND hwnd);
	void OnEga(HWND hwnd, LPCWSTR file = NULL);
	void OnEgaProgram(HWND hwnd);
	void OnImport(HWND hwnd);
	void OnLoadResH(HWND hwnd);
	void OnLoadResHBang(HWND hwnd);
	void OnLoadWCLib(HWND hwnd);
	void OnExport(HWND hwnd);
	void OnFonts(HWND hwnd);
	void OnAbout(HWND hwnd);
	void OnConfig(HWND hwnd);
	void OnOpenLocalFile(HWND hwnd, LPCWSTR filename);
	void OnDebugTreeNode(HWND hwnd);
	void OnAdviceResH(HWND hwnd);
	void OnUnloadResH(HWND hwnd);
	void OnHideIDMacros(HWND hwnd);
	void OnUseIDC_STATIC(HWND hwnd);
	void OnTest(HWND hwnd);
	void OnDialogFontSubst(HWND hwnd);
	void OnHelp(HWND hwnd);
	void OnNextPane(HWND hwnd, BOOL bNext);

	// find/replace
	void OnFind(HWND hwnd);
	BOOL OnFindNext(HWND hwnd);
	BOOL OnFindPrev(HWND hwnd);

protected:
	MString GetLanguageStatement(WORD langid)
	{
		return ::GetLanguageStatement(langid, TRUE) + L"\r\n";
	}

	void UpdateNames(BOOL bModified = TRUE);
	void UpdateEntryName(EntryBase *e, LPWSTR pszText = NULL);
	void UpdateEntryLang(EntryBase *e, LPWSTR pszText = NULL);

	std::wstring GetRisohEditorVersion() const;
	std::wstring ParseVersionFile(LPCWSTR pszFile, std::wstring& url) const;
};
