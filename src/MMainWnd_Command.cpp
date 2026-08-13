// MMainWnd_Command.cpp --- RisohEditor commands
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MMainWnd.hpp"
#define LINENUMEDIT_IMPL
#include "LineNumEdit.hpp"

#include "AccelRes.hpp"
#include "DialogRes.hpp"
#include "DlgInitRes.hpp"
#include "IconRes.hpp"
#include "MenuRes.hpp"
#include "MessageRes.hpp"
#include "StringRes.hpp"
#include "ToolbarRes.hpp"
#include "VersionRes.hpp"

#include "MAddBitmapDlg.hpp"
#include "MAddCursorDlg.hpp"
#include "MAddIconDlg.hpp"
#include "MAdviceResHDlg.hpp"
#include "MChooseLangDlg.hpp"
#include "MCloneInNewLangDlg.hpp"
#include "MCloneInNewNameDlg.hpp"
#include "MCloneInNewTypeDlg.hpp"
#include "MConfigPropSheet.hpp"
#include "MConstantDlg.hpp"
#include "MCopyToMultiLangDlg.hpp"
#include "MDfmSettingsDlg.hpp"
#include "MDialogFontSubstDlg.hpp"
#include "MDlgInitDlg.hpp"
#include "MEditAccelDlg.hpp"
#include "MEditMenuDlg.hpp"
#include "MEditToolbarDlg.hpp"
#include "MEncodingDlg.hpp"
#include "MExportOptionsDlg.hpp"
#include "MFontsDlg.hpp"
#include "MGoToLineDlg.hpp"
#include "MIdAssocDlg.hpp"
#include "MLangsDlg.hpp"
#include "MMacrosDlg.hpp"
#include "MMessagesDlg.hpp"
#include "MPathsDlg.hpp"
#include "MReplaceBinDlg.hpp"
#include "MReplaceBitmapDlg.hpp"
#include "MReplaceCursorDlg.hpp"
#include "MReplaceIconDlg.hpp"
#include "MStringsDlg.hpp"
#include "MTestDialog.hpp"
#include "MTestMenuDlg.hpp"
#include "MTestParentWnd.hpp"
#include "MVersionInfoDlg.hpp"

#include "MenuRes.hpp"
#include "Utils.h"
#include "resource.h"

extern BOOL s_bModified;
extern INT s_ret;

// ID_NEW: clear all the resource data
BOOL MMainWnd::OnNew(HWND hwnd)
{
	if (!DoQuerySaveChange(hwnd))
		return FALSE;

	// close preview
	HidePreview();

	// unload the resource.h file
	OnUnloadResH(hwnd);

	// update the file info
	UpdateFileInfo(FT_NONE, NULL, FALSE);

	// unselect
	SelectTV(NULL, FALSE);

	// clean up
	g_res.delete_all();

	// update modified flag
	DoSetFileModified(FALSE);

	// update drop-down arrow
	PostUpdateArrow(hwnd);
	return TRUE;
}

// ID_OPEN: open a file
void MMainWnd::OnOpen(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	if (!DoQuerySaveChange(hwnd))
		return;

	// store the nominal path
	WCHAR szFile[MAX_PATH];
	StringCchCopyW(szFile, _countof(szFile), m_szFile);

	// if path was not valid, make it empty
	if (!PathFileExistsW(szFile))
		szFile[0] = 0;

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXERESRCFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_OPEN);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"exe";       // the default extension

	// let the user choose the path
	if (GetOpenFileNameW(&ofn))
	{
		// load the file
		DoLoadFile(hwnd, ofn.lpstrFile, ofn.nFilterIndex);
	}
}

// ID_SAVEAS: save as a file or files
BOOL MMainWnd::OnSaveAs(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return FALSE;

	// store m_szFile to szFile
	WCHAR szFile[MAX_PATH];
	StringCchCopyW(szFile, _countof(szFile), m_szFile);

	// if not found, then make it empty
	if (!PathFileExistsW(szFile))
		szFile[0] = 0;

	// was it an executable?
	BOOL bWasExecutable = (m_file_type == FT_EXECUTABLE);

	// get and delete the filename extension
	WCHAR szExt[MAX_PATH] = L"";
	if (LPWSTR pch = wcsrchr(szFile, L'.'))
	{
		static const LPCWSTR s_DotExts[] =
		{
			L".exe", L".dll", L".ocx", L".cpl", L".scr", L".mui", L".rc", L".rc2", L".res"
		};
		for (auto ext : s_DotExts)
		{
			if (lstrcmpiW(pch, ext) == 0)
			{
				StringCbCopyW(szExt, sizeof(szExt), ext + 1);
				*pch = 0;
				break;
			}
		}
	}

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXERESFILTER));

	// use the prefered filter by the entry
	ofn.nFilterIndex = g_settings.nSaveFilterIndex;
	if (bWasExecutable)
	{
		if (ofn.nFilterIndex != RFFI_EXECUTABLE)
			ofn.nFilterIndex = RFFI_EXECUTABLE;
	}
	else
	{
		if (ofn.nFilterIndex == RFFI_EXECUTABLE)
			ofn.nFilterIndex = RFFI_RC;
	}

	// use the preferred extension
	switch (ofn.nFilterIndex)
	{
	case RFFI_EXECUTABLE:
		if (szExt[0])
		{
			ofn.lpstrDefExt = szExt;
		}
		else
		{
			ofn.lpstrDefExt = L"exe";       // the default extension
		}
		break;

	case RFFI_RC:
		ofn.lpstrDefExt = L"rc";        // the default extension
		break;

	case RFFI_RES:
	default:
		ofn.lpstrDefExt = L"res";       // the default extension
		break;
	}

	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_SAVEAS);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// save the filter index to the settings
		g_settings.nSaveFilterIndex = ofn.nFilterIndex;

		if (ofn.nFilterIndex == RFFI_ALL)
		{
			ofn.nFilterIndex = RFFI_EXECUTABLE;
		}

		switch (ofn.nFilterIndex)
		{
		case RFFI_EXECUTABLE:
			// save it
			if (DoSaveAs(szFile))
			{
				m_nStatusStringID = IDS_FILESAVED;
				return TRUE;
			}
			ErrorBoxDx(IDS_CANNOTSAVE);
			break;

		case RFFI_RC:
			// export and save it
			{
				// show "save options" dialog
				MExportOptionsDlg save_options(FALSE);
				if (save_options.DialogBoxDx(hwnd) != IDOK)
					return FALSE;

				// export
				WCHAR szResH[MAX_PATH] = L"";
				if (DoExportRC(szFile, szResH))   // succeeded
				{
					// save the resource.h path
					StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);

					// update the file info
					UpdateFileInfo(FT_RC, szFile, FALSE);

					m_nStatusStringID = IDS_FILESAVED;
					return TRUE;
				}
				else
				{
					ErrorBoxDx(IDS_CANNOTSAVE);
				}
			}
			break;

		case RFFI_RES:
			// save the *.res file
			if (DoSaveResAs(szFile))
			{
				m_nStatusStringID = IDS_FILESAVED;
				return TRUE;
			}
			ErrorBoxDx(IDS_CANNOTSAVE);
			break;

		default:
			assert(0);
			break;
		}
	}

	return FALSE;
}

// ID_SAVE
BOOL MMainWnd::OnSave(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return FALSE;

	if (!m_szFile[0])
	{
		return OnSaveAs(hwnd);
	}

	LPWSTR pchDotExt = PathFindExtensionW(m_szFile);
	if (lstrcmpiW(pchDotExt, L".res") == 0)
	{
		g_settings.nSaveFilterIndex = RFFI_RES;
	}
	else if (lstrcmpiW(pchDotExt, L".rc") == 0 || lstrcmpiW(pchDotExt, L".rc2") == 0)
	{
		g_settings.nSaveFilterIndex = RFFI_RC;
	}
	else
	{
		g_settings.nSaveFilterIndex = RFFI_EXECUTABLE;
	}

	switch (g_settings.nSaveFilterIndex)
	{
	case RFFI_EXECUTABLE:
		// save it
		if (DoSaveAs(m_szFile))
		{
			m_nStatusStringID = IDS_FILESAVED;
			return TRUE;
		}
		ErrorBoxDx(IDS_CANNOTSAVE);
		break;

	case RFFI_RC:
		// export and save it
		{
			// show "save options" dialog
			MExportOptionsDlg save_options(FALSE);
			if (save_options.DialogBoxDx(hwnd) != IDOK)
				return FALSE;

			// export
			WCHAR szResH[MAX_PATH] = L"";
			if (DoExportRC(m_szFile, szResH))   // succeeded
			{
				// save the resource.h path
				StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);

				// update the file info
				UpdateFileInfo(FT_RC, m_szFile, FALSE);

				m_nStatusStringID = IDS_FILESAVED;
				return TRUE;
			}
			else
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
		}
		break;

	case RFFI_RES:
		// save the *.res file
		if (DoSaveResAs(m_szFile))
		{
			m_nStatusStringID = IDS_FILESAVED;
			return TRUE;
		}
		ErrorBoxDx(IDS_CANNOTSAVE);
		break;

	default:
		assert(0);
		break;
	}

	return FALSE;
}

// ID_IMPORT: import the resource data additionally
void MMainWnd::OnImport(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	WCHAR file[MAX_PATH] = TEXT("");

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_IMPORTFILTER));
	ofn.lpstrFile = file;
	ofn.nMaxFile = _countof(file);
	ofn.lpstrTitle = LoadStringDx(IDS_IMPORTRES);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"res";       // the default extension

	// let the user choose the path
	if (GetOpenFileNameW(&ofn))
	{
		// find the file title
		LPCWSTR pch = wcsrchr(file, L'\\');
		if (pch == NULL)
			pch = wcsrchr(file, L'/');
		if (pch == NULL)
			pch = file;
		else
			++pch;

		// find the dot extension
		pch = wcsrchr(pch, L'.');

		if (IMPORT_FAILED == DoImport(hwnd, file, pch))
		{
			ErrorBoxDx(IDS_CANNOTIMPORT);
		}
		DoSetFileModified(TRUE);
	}
}

// ID_ADDICON: add an icon resource
void MMainWnd::OnAddIcon(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddIconDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// refresh the ID list window
		DoRefreshIDList(hwnd);

		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// ID_REPLACEICON: replace the icon resource
void MMainWnd::OnReplaceIcon(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	// show the dialog
	MReplaceIconDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// ID_REPLACECURSOR: replace the cursor resource
void MMainWnd::OnReplaceCursor(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	// show the dialog
	MReplaceCursorDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDBITMAP: add a bitmap resource
void MMainWnd::OnAddBitmap(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddBitmapDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// refresh the ID list window
		DoRefreshIDList(hwnd);

		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// ID_REPLACEBITMAP: replace the bitmap resource
void MMainWnd::OnReplaceBitmap(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	// show the dialog
	MReplaceBitmapDlg dialog(*entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDCURSOR: add a cursor
void MMainWnd::OnAddCursor(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddCursorDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// refresh the ID list window
		DoRefreshIDList(hwnd);

		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDRES: add a resource item
void MMainWnd::OnAddRes(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDMENU: add a menu
void MMainWnd::OnAddMenu(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_MENU;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDTOOLBAR: add a TOOLBAR
void MMainWnd::OnAddToolbar(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_TOOLBAR;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDSTRINGTABLE: add a string table
void MMainWnd::OnAddStringTable(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_STRING;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDMESSAGETABLE: add a message table resource
void MMainWnd::OnAddMessageTable(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_MESSAGETABLE;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDHTML: add an HTML resource
void MMainWnd::OnAddHtml(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_HTML;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDACCEL: add an RT_ACCELERATOR resource
void MMainWnd::OnAddAccel(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_ACCELERATOR;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDVERINFO: add a version info resource
void MMainWnd::OnAddVerInfo(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_VERSION;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDMANIFEST: add a manifest resource
void MMainWnd::OnAddManifest(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_MANIFEST;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_ADDDIALOG: add a dialog template
void MMainWnd::OnAddDialog(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_DIALOG;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// ID_EXTRACTBIN: extract the binary as a file
void MMainWnd::OnExtractBin(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// get the selected entry
	auto e = g_res.get_entry();
	if (!e)
		return;     // not selected

	if (e->is_delphi_dfm())
		return OnExtractDFM(hwnd);

	if (e->m_type == L"TYPELIB")
		return OnExtractTLB(hwnd);

	ResToText res2text;
	MString filename;
	res2text.GetEntryFileNameEx(*e, filename);

	WCHAR szFile[MAX_PATH] = L"";
	if (filename.size())
		StringCchCopyW(szFile, _countof(szFile), filename.c_str());
	PathRemoveExtensionW(szFile);

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);

	// use the prefered filter by the entry
	switch (e->m_et)
	{
	case ET_STRING:
	case ET_TYPE:
	case ET_NAME:
		ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
		ofn.lpstrDefExt = L"res";
		break;
	case ET_LANG:
		if (e->m_type == L"PNG")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_PNGRESBINFILTER));
			ofn.lpstrDefExt = L"png";
		}
		else if (e->m_type == L"JPEG")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_JPEGRESBINFILTER));
			ofn.lpstrDefExt = L"jpg";
		}
		else if (e->m_type == L"GIF")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_GIFRESBINFILTER));
			ofn.lpstrDefExt = L"gif";
		}
		else if (e->m_type == L"TIFF")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_TIFFRESBINFILTER));
			ofn.lpstrDefExt = L"tif";
		}
		else if (e->m_type == L"AVI")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_AVIRESBINFILTER));
			ofn.lpstrDefExt = L"avi";
		}
		else if (e->m_type == L"WAVE")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_WAVERESBINFILTER));
			ofn.lpstrDefExt = L"wav";
		}
		else if (e->m_type == L"MP3")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_MP3RESBINFILTER));
			ofn.lpstrDefExt = L"mp3";
		}
		else
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESBINFILTER));
			ofn.lpstrDefExt = L"res";
		}
		break;
	default:
		return;
	}

	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract it to a file
		if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
		{
			// it was a *.res file
			if (!g_res.extract_res(ofn.lpstrFile, e))
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
		}
		else
		{
			// it was not a *.res file
			if (!g_res.extract_bin(ofn.lpstrFile, e))
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
			else
			{
				// Notify change of file icon
				MyChangeNotify(ofn.lpstrFile);
			}
		}
	}
}

// ID_EXTRACTRC
void MMainWnd::OnExtractRC(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// get the selected entry
	auto e = g_res.get_entry();
	if (!e)
		return;     // not selected

	// initialize OPENFILENAME structure
	WCHAR szFile[MAX_PATH] = L"";
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RCFILTER));
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"rc";   // the default extension

	// use the prefered filter by the entry
	EntrySet found;
	MIdOrString type = e->m_type;
	switch (e->m_et)
	{
	case ET_STRING:
		g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, e->m_lang);
		break;
	case ET_TYPE:
		if (type == RT_ICON)
			type = RT_GROUP_ICON;
		else if (type == RT_CURSOR)
			type = RT_GROUP_CURSOR;
		g_res.search(found, ET_LANG, type, BAD_NAME, BAD_LANG);
		break;
	case ET_NAME:
		g_res.search(found, ET_LANG, type, e->m_name, BAD_LANG);
		break;
	case ET_LANG:
		g_res.search(found, ET_LANG, type, e->m_name, e->m_lang);
		break;
	default:
		return;
	}

	if (found.empty())
	{
		ErrorBoxDx(IDS_DATAISEMPTY);
		return;
	}

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// show the "export options" dialog
		MExportOptionsDlg dialog(TRUE);
		if (dialog.DialogBoxDx(hwnd) != IDOK)
			return;

		if (!DoExportRC(szFile, NULL, found))
		{
			ErrorBoxDx(IDS_CANTEXPORT);
		}
	}
}

// ID_EXTRACTICON: extract an icon as an *.ico file
void MMainWnd::OnExtractIcon(HWND hwnd)
{
	enum IconFilterIndex  // see also: IDS_ICOFILTER
	{
		IFI_NONE = 0,
		IFI_ICO = 1,
		IFI_ANI = 2,
		IFI_ALL = 3
	};

	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	WCHAR szFile[MAX_PATH] = L"";
	ResToText res2text;
	MString strFile = res2text.GetEntryFileName(*entry);
	if (strFile.size())
	{
		StringCbCopyW(szFile, sizeof(szFile), strFile.c_str());
	}

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTICO);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// use the prefered filter by the entry
	if (entry->m_type == RT_ANIICON)
	{
		ofn.nFilterIndex = IFI_ANI;
		ofn.lpstrDefExt = L"ani";   // the default extension
	}
	else
	{
		ofn.nFilterIndex = IFI_ICO;
		ofn.lpstrDefExt = L"ico";   // the default extension
	}

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract it to an *.ico or *.ani file
		if (!g_res.extract_icon(ofn.lpstrFile, entry))
		{
			ErrorBoxDx(IDS_CANTEXTRACTICO);
		}
		else
		{
			// Notify change of file icon
			MyChangeNotify(ofn.lpstrFile);
		}
	}
}

// ID_EXTRACTCURSOR: extract a cursor as an *.cur or *.ani file
void MMainWnd::OnExtractCursor(HWND hwnd)
{
	enum CursorFilterIndex      // see also: IDS_CURFILTER
	{
		CFI_NONE = 0,
		CFI_CUR = 1,
		CFI_ANI = 2,
		CFI_ALL = 3
	};

	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)     // not selected
		return;

	WCHAR szFile[MAX_PATH] = L"";
	ResToText res2text;
	MString strFile = res2text.GetEntryFileName(*entry);
	if (strFile.size())
	{
		StringCbCopyW(szFile, sizeof(szFile), strFile.c_str());
	}

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTCUR);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// use the prefered filter by the entry
	if (entry->m_type == RT_ANICURSOR)
	{
		ofn.nFilterIndex = CFI_ANI;
		ofn.lpstrDefExt = L"ani";   // the default extension
	}
	else
	{
		ofn.nFilterIndex = CFI_CUR;
		ofn.lpstrDefExt = L"cur";   // the default extension
	}

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract it to an *.cur or *.ani file
		if (!g_res.extract_cursor(ofn.lpstrFile, entry))
		{
			ErrorBoxDx(IDS_CANTEXTRACTCUR);
		}
		else
		{
			// Notify change of file icon
			MyChangeNotify(ofn.lpstrFile);
		}
	}
}

// ID_EXTRACTBITMAP: extract a bitmap as an *.bmp file
void MMainWnd::OnExtractBitmap(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)     // not selected
		return;

	WCHAR szFile[MAX_PATH] = L"";
	ResToText res2text;
	MString strFile = res2text.GetEntryFileName(*entry);
	if (strFile.size())
	{
		StringCbCopyW(szFile, sizeof(szFile), strFile.c_str());
	}

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_BMPFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTBMP);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"bmp";   // the default extension

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract a bitmap as an *.bmp or *.png file
		BOOL bPNG = (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"png") == 0);
		if (!PackedDIB_Extract(ofn.lpstrFile, &(*entry)[0], (*entry).size(), bPNG))
		{
			ErrorBoxDx(IDS_CANTEXTRACTBMP);
		}
		else
		{
			// Notify change of file icon
			MyChangeNotify(ofn.lpstrFile);
		}
	}
}

// ID_REPLACEBIN: replace the resource data by a binary file
void MMainWnd::OnReplaceBin(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	// show the dialog
	MReplaceBinDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);
	}

	DoSetFileModified(TRUE);
}

// ID_ABOUT: version info
void MMainWnd::OnAbout(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MVersionInfoDlg dialog;
	dialog.DialogBoxDx(hwnd);
}

void MMainWnd::RefreshFonts()
{
	ReCreateFonts(m_hwnd);
}

// ID_EXPORT: export all the resource items to an RC file
void MMainWnd::OnExport(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// initialize OPENFILENAME structure
	WCHAR file[MAX_PATH] = TEXT("");
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXPORTFILTER));
	ofn.lpstrFile = file;
	ofn.nMaxFile = _countof(file);
	ofn.lpstrTitle = LoadStringDx(IDS_EXPORT);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"rc";    // the default extension

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// do export!
		if (ofn.nFilterIndex == 2) // .res
		{
			if (!DoExportRes(file))
			{
				ErrorBoxDx(IDS_CANTEXPORT);
			}
		}
		else // .rc or .rc2
		{
			// show the "export options" dialog
			MExportOptionsDlg dialog(TRUE);
			if (dialog.DialogBoxDx(hwnd) != IDOK)
				return;

			if (!DoExportRC(file))
			{
				ErrorBoxDx(IDS_CANTEXPORT);
			}
		}
	}
}

void MMainWnd::OnCopyAsNewType(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected type entry
	auto entry = g_res.get_entry();
	if (!entry || entry->m_et != ET_TYPE)
		return;

	// show the dialog
	MCloneInNewTypeDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// search the ET_LANG entries
		EntrySet found;
		g_res.search(found, ET_LANG, dialog.m_old_type, BAD_NAME, BAD_LANG);

		for (auto e : found)
		{
			g_res.add_lang_entry(dialog.m_new_type, e->m_name, e->m_lang, e->m_data);
		}

		SelectTV(ET_TYPE, dialog.m_new_type, BAD_NAME, BAD_LANG, FALSE);

		auto entry = g_res.find(ET_TYPE, dialog.m_new_type);
		if (entry)
			Expand(entry->m_hItem);

		DoSetFileModified(TRUE);
	}
}

// ID_COPYASNEWNAME: clone the resource item in new name
void MMainWnd::OnCopyAsNewName(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected name entry
	auto entry = g_res.get_entry();
	if (!entry || entry->m_et != ET_NAME)
		return;

	// show the dialog
	MCloneInNewNameDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// search the ET_LANG entries
		EntrySet found;
		g_res.search(found, ET_LANG, entry->m_type, entry->m_name);

		if (entry->m_type == RT_GROUP_ICON)     // group icon
		{
			for (auto e : found)
			{
				g_res.copy_group_icon(e, dialog.m_name, e->m_lang);
			}
		}
		else if (entry->m_type == RT_GROUP_CURSOR)  // group cursor
		{
			for (auto e : found)
			{
				g_res.copy_group_cursor(e, dialog.m_name, e->m_lang);
			}
		}
		else    // otherwise
		{
			for (auto e : found)
			{
				g_res.add_lang_entry(e->m_type, dialog.m_name, e->m_lang, e->m_data);
			}
		}

		// select the entry
		SelectTV(ET_NAME, dialog.m_type, dialog.m_name, BAD_LANG, FALSE);
		DoSetFileModified(TRUE);
	}
}

// ID_COPYASNEWLANG: clone the resource item in new language
void MMainWnd::OnCopyAsNewLang(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_LANG: case ET_STRING:
		break;      // ok

	default:
		return;     // unable to copy the language
	}

	// show the dialog
	MCloneInNewLangDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		if (entry->m_type == RT_GROUP_ICON)     // group icon
		{
			// search the group icons
			EntrySet found;
			g_res.search(found, ET_LANG, RT_GROUP_ICON, entry->m_name, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.copy_group_icon(e, e->m_name, dialog.m_lang);
			}

			// select the entry
			SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
		}
		else if (entry->m_type == RT_GROUP_CURSOR)
		{
			// search the group cursors
			EntrySet found;
			g_res.search(found, ET_LANG, RT_GROUP_CURSOR, entry->m_name, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.copy_group_cursor(e, e->m_name, dialog.m_lang);
			}

			// select the entry
			SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
		}
		else if (entry->m_et == ET_STRING)
		{
			// search the strings
			EntrySet found;
			g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.add_lang_entry(e->m_type, e->m_name, dialog.m_lang, e->m_data);
			}

			// select the entry
			SelectTV(ET_STRING, dialog.m_type, BAD_NAME, dialog.m_lang, FALSE);
		}
		else
		{
			// search the entries
			EntrySet found;
			g_res.search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.add_lang_entry(e->m_type, e->m_name, dialog.m_lang, e->m_data);
			}

			// select the entry
			SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
		}
		DoSetFileModified(TRUE);
	}
}

// ID_COPYTOMULTILANG
void MMainWnd::OnCopyToMultiLang(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_LANG: case ET_STRING:
		break;      // ok

	default:
		return;     // unable to copy the language
	}

	// show the dialog
	MCopyToMultiLangDlg dialog(entry);
	LANGID wLang = BAD_LANG;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		for (auto& lang : dialog.m_langs)
		{
			wLang = lang;
			if (entry->m_type == RT_GROUP_ICON)     // group icon
			{
				// search the group icons
				EntrySet found;
				g_res.search(found, ET_LANG, RT_GROUP_ICON, entry->m_name, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.copy_group_icon(e, e->m_name, lang);
				}
			}
			else if (entry->m_type == RT_GROUP_CURSOR)
			{
				// search the group cursors
				EntrySet found;
				g_res.search(found, ET_LANG, RT_GROUP_CURSOR, entry->m_name, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.copy_group_cursor(e, e->m_name, lang);
				}
			}
			else if (entry->m_et == ET_STRING)
			{
				// search the strings
				EntrySet found;
				g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.add_lang_entry(e->m_type, e->m_name, lang, e->m_data);
				}
			}
			else
			{
				// search the entries
				EntrySet found;
				g_res.search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.add_lang_entry(e->m_type, e->m_name, lang, e->m_data);
				}
			}
		}

		DoSetFileModified(TRUE);

		// select the entry
		SelectTV(ET_LANG, entry->m_type, entry->m_name, wLang, FALSE);
	}
}

// ID_EXPORTRES
void MMainWnd::OnExportRes(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// get the selected entry
	auto e = g_res.get_entry();
	if (!e)
		return;     // not selected

	if (e->is_delphi_dfm())
		return OnExtractDFM(hwnd);

	// initialize OPENFILENAME structure
	WCHAR szFile[MAX_PATH] = L"";
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"res";   // the default extension

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract it to a file
		if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
		{
			// it was a *.res file
			if (!g_res.extract_res(ofn.lpstrFile, e))
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
		}
		else
		{
			// it was not a *.res file
			if (!g_res.extract_bin(ofn.lpstrFile, e))
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
		}
	}
}

// ID_DELETERES: delete a resource item
void MMainWnd::OnDeleteRes(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected entry
	if (auto entry = g_res.get_entry())
	{
		g_res.delete_entry(entry);
	}

	DoRefreshIDList(hwnd);

	DoSetFileModified(TRUE);

	PostUpdateArrow(hwnd);
}

// ID_PLAY: play the sound
void MMainWnd::OnPlay(HWND hwnd)
{
	// is it a WAVE sound?
	auto entry = g_res.get_lang_entry();
	if (entry && entry->m_type == L"WAVE")
	{
		// play the sound
		PlaySound(reinterpret_cast<LPCTSTR>(&(*entry)[0]), NULL,
				  SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
		return;
	}
	// Is it an MP3 sound?
	if (entry && entry->m_type == L"MP3")
	{
		PlayMP3(&(*entry)[0], entry->size());
		return;
	}
}

// ID_COMPILE: compile the source text
void MMainWnd::OnCompile(HWND hwnd)
{
	// needs reopen?
	BOOL bReopen = IsWindowVisible(m_rad_window);

	if (bReopen)
		GetWindowRect(m_rad_window, &m_rcRadWindow);
	else
		SetRectEmpty(&m_rcRadWindow);

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	// is it not modified?
	if (!Edit_GetModify(m_hCodeEditor))
	{
		// select the entry
		SelectTV(entry, FALSE);
		return;
	}

	ChangeStatusText(IDS_COMPILING);

	// m_hCodeEditor --> strWide
	MStringW strWide = MWindowBase::GetWindowTextW(m_hCodeEditor);

	// compile the strWide text
	MStringA strOutput;
	if (CompileParts(strOutput, entry->m_type, entry->m_name, entry->m_lang, strWide, bReopen))
	{
		m_nStatusStringID = IDS_RECOMPILEOK;

		// clear the control selection
		MRadCtrl::GetTargets().clear();
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);

		// clear the modification flag
		Edit_SetModify(m_hCodeEditor, FALSE);

		// select the entry
		SelectTV(entry, FALSE);
	}
	else
	{
		// failed
		m_nStatusStringID = IDS_RECOMPILEFAILED;
		SetErrorMessage(strOutput);
	}
}

// ID_GUIEDIT: do GUI edit
void MMainWnd::OnGuiEdit(HWND hwnd)
{
	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry || !entry->is_editable(m_szVCBat))
		return;     // not editable

	if (!entry->can_gui_edit())
		return;     // unable to edit by GUI?

	// compile if necessary
	if (!CompileIfNecessary(FALSE))
	{
		return;
	}

	if (entry->m_type == RT_ACCELERATOR)
	{
		// entry->m_data --> accel_res
		AccelRes accel_res;
		MByteStreamEx stream(entry->m_data);
		if (accel_res.LoadFromStream(stream))
		{
			// editing...
			ChangeStatusText(IDS_EDITINGBYGUI);

			// show the dialog
			MEditAccelDlg dialog(accel_res);
			INT nID = (INT)dialog.DialogBoxDx(hwnd);
			if (nID == IDOK && entry == g_res.get_entry())
			{
				DoSetFileModified(TRUE);

				// update accel_res
				accel_res.Update();

				// accel_res --> entry->m_data
				entry->m_data = accel_res.data();
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// select the entry
		if (entry == g_res.get_entry())
			SelectTV(entry, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_MENU)
	{
		// entry->m_data --> menu_res
		MenuRes menu_res;
		MByteStreamEx stream(entry->m_data);
		if (menu_res.LoadFromStream(stream))
		{
			// editing...
			ChangeStatusText(IDS_EDITINGBYGUI);

			// show the dialog
			MEditMenuDlg dialog(menu_res);
			INT nID = (INT)dialog.DialogBoxDx(hwnd);
			if (nID == IDOK && entry == g_res.get_entry())
			{
				DoSetFileModified(TRUE);

				// update menu_res
				menu_res.Update();

				// menu_res --> entry->m_data
				entry->m_data = menu_res.data();
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// select the entry
		if (entry == g_res.get_entry())
			SelectTV(entry, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_TOOLBAR)
	{
		// entry->m_data --> toolbar_res
		ToolbarRes toolbar_res;
		MByteStreamEx stream(entry->m_data);
		if (toolbar_res.LoadFromStream(stream))
		{
			// editing...
			ChangeStatusText(IDS_EDITINGBYGUI);

			// show the dialog
			MEditToolbarDlg dialog(toolbar_res);
			INT nID = (INT)dialog.DialogBoxDx(hwnd);
			if (nID == IDOK && entry == g_res.get_entry())
			{
				DoSetFileModified(TRUE);

				// toolbar_res --> entry->m_data
				entry->m_data = toolbar_res.data();
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// select the entry
		if (entry == g_res.get_entry())
			SelectTV(entry, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_DIALOG)
	{
		// editing...
		ChangeStatusText(IDS_EDITINGBYGUI);

		// entry->m_data --> m_rad_window.m_dialog_res
		MByteStreamEx stream(entry->m_data);
		m_rad_window.m_dialog_res.LoadFromStream(stream);
		m_rad_window.m_dialog_res.m_lang = entry->m_lang;
		m_rad_window.m_dialog_res.m_name = entry->m_name;
		m_rad_window.clear_maps();
		m_rad_window.create_maps(entry->m_lang);

		// load RT_DLGINIT
		if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, entry->m_lang))
		{
			m_rad_window.m_dialog_res.LoadDlgInitData(e->m_data);
		}
		else if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, BAD_LANG))
		{
			m_rad_window.m_dialog_res.LoadDlgInitData(e->m_data);
		}

		// Opening the GUI editor does not change the dialog source text.
		// Avoid any forced repaint of m_hCodeEditor: a WM_SETREDRAW +
		// RedrawWindow pair was itself flashing the LineNumEdit even when
		// the text was identical. Just drop a leftover update timer and
		// clear ES_READONLY only when it is actually set.
		::KillTimer(hwnd, CODE_REFRESH_TIMER_ID);

		// recreate the RADical dialog
		if (::IsWindow(m_rad_window))
		{
			m_rad_window.ReCreateRadDialog(m_rad_window);
		}
		else
		{
			if (!m_rad_window.CreateDx(m_hwnd))
			{
				ErrorBoxDx(IDS_DLGFAIL);
				return;
			}
		}

		// Swallow any MYWM_UPDATEDLGRES timer that may have been armed
		// while the RAD window was shown/sized.
		::KillTimer(hwnd, CODE_REFRESH_TIMER_ID);

		if (GetWindowStyle(m_hCodeEditor) & ES_READONLY)
			Edit_SetReadOnly(m_hCodeEditor, FALSE);
	}
	else if (entry->m_type == RT_DLGINIT)
	{
		// entry->m_data --> dlginit_res
		DlgInitRes dlginit_res;
		MByteStreamEx stream(entry->m_data);
		if (dlginit_res.LoadFromStream(stream))
		{
			// editing...
			ChangeStatusText(IDS_EDITINGBYGUI);

			// show the dialog
			MDlgInitDlg dialog(dlginit_res);
			INT nID = (INT)dialog.DialogBoxDx(hwnd);
			if (nID == IDOK && entry == g_res.get_entry())
			{
				DoSetFileModified(TRUE);

				// dlginit_res --> entry->m_data
				entry->m_data = dlginit_res.data();
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// select the entry
		if (entry == g_res.get_entry())
			SelectTV(entry, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_STRING && entry->m_et == ET_STRING)
	{
		// g_res --> found
		LANGID lang = entry->m_lang;
		EntrySet found;
		g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, lang);

		// found --> str_res
		StringRes str_res;
		for (auto e : found)
		{
			MByteStreamEx stream(e->m_data);
			if (!str_res.LoadFromStream(stream, e->m_name.m_id))
			{
				ErrorBoxDx(IDS_CANNOTLOAD);
				return;
			}
		}

		// editing...
		ChangeStatusText(IDS_EDITINGBYGUI);

		// show the dialog
		MStringsDlg dialog(str_res);
		INT nID = (INT)dialog.DialogBoxDx(hwnd);
		if (nID == IDOK)
		{
			DoSetFileModified(TRUE);

			// dialog --> str_res
			str_res = dialog.m_str_res;

			// dump (with disabling macro IDs)
			bool shown = !g_settings.bHideID;
			g_settings.bHideID = false;
			MStringW strWide = str_res.Dump();
			g_settings.bHideID = !shown;

			// compile the dumped result
			MStringA strOutput;
			if (CompileParts(strOutput, RT_STRING, BAD_NAME, lang, strWide))
			{
				m_nStatusStringID = IDS_RECOMPILEOK;

				// select the entry to update the source
				SelectTV(ET_STRING, RT_STRING, BAD_NAME, lang, FALSE);
			}
			else
			{
				m_nStatusStringID = IDS_RECOMPILEFAILED;
				SetErrorMessage(strOutput);
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_MESSAGETABLE && entry->m_et == ET_LANG)
	{
		// g_res --> found
		LANGID lang = entry->m_lang;
		EntrySet found;
		g_res.search(found, ET_LANG, RT_MESSAGETABLE, entry->m_name, lang);

		// found --> msg_res
		MessageRes msg_res;
		for (auto e : found)
		{
			MByteStreamEx stream(e->m_data);
			if (!msg_res.LoadFromStream(stream))
			{
				ErrorBoxDx(IDS_CANNOTLOAD);
				return;
			}
		}

		// editing...
		ChangeStatusText(IDS_EDITINGBYGUI);

		// show the dialog
		MMessagesDlg dialog(msg_res);
		INT nID = (INT)dialog.DialogBoxDx(hwnd);
		if (nID == IDOK)
		{
			DoSetFileModified(TRUE);

			// dialog --> msg_res
			msg_res = dialog.m_msg_res;

			// dump (with disabling macro IDs)
			bool shown = !g_settings.bHideID;
			g_settings.bHideID = false;
			MStringW strWide = msg_res.Dump(entry->m_name);
			g_settings.bHideID = !shown;

			// compile the dumped result
			MStringA strOutput;
			if (CompileParts(strOutput, RT_MESSAGETABLE, entry->m_name, lang, strWide))
			{
				m_nStatusStringID = IDS_RECOMPILEOK;

				// select the entry
				SelectTV(ET_LANG, RT_MESSAGETABLE, entry->m_name, lang, FALSE);
			}
			else
			{
				m_nStatusStringID = IDS_RECOMPILEFAILED;
				SetErrorMessage(strOutput);
			}
		}

		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
}

// ID_LOADRESH: open the dialog to load the resource.h
void MMainWnd::OnLoadResH(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
	{
		return;
	}

	// (resource.h file path) --> szFile
	WCHAR szFile[MAX_PATH];
	if (m_szResourceH[0])
		StringCchCopyW(szFile, _countof(szFile), m_szResourceH);
	else
		StringCchCopyW(szFile, _countof(szFile), L"resource.h");

	// if it does not exist, clear the file path
	if (!PathFileExistsW(szFile))
		szFile[0] = 0;

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_LOADRESH);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"h";     // the default extension

	// let the user choose the path
	if (GetOpenFileNameW(&ofn))
	{
		// load the resource.h file
		DoLoadResH(hwnd, szFile);

		// is the resource.h file loaded?
		if (m_szResourceH[0])
		{
			// show the ID list window
			ShowIDList(hwnd, TRUE);
		}

		// update the names
		UpdateNames();

		// re-select the selected entry
		auto entry = g_res.get_entry();
		SelectTV(entry, FALSE);
	}
}

// ID_ADVICERESH: advice the change for resource.h file
void MMainWnd::OnAdviceResH(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	MString str;

	if (!g_settings.removed_ids.empty() &&
		(g_settings.removed_ids.size() != 1 ||
		 g_settings.removed_ids.find("IDC_STATIC") == g_settings.removed_ids.end()))
	{
		str += LoadStringDx(IDS_DELETENEXTIDS);

		for (auto& pair : g_settings.removed_ids)
		{
			if (pair.first == "IDC_STATIC")
				continue;

			str += TEXT("#define ");
			str += MAnsiToText(CP_ACP, pair.first).c_str();
			str += TEXT(" ");
			str += MAnsiToText(CP_ACP, pair.second).c_str();
			str += TEXT("\r\n");
		}
		str += TEXT("\r\n");
	}

	if (!g_settings.added_ids.empty() &&
		(g_settings.added_ids.size() != 1 ||
		 g_settings.added_ids.find("IDC_STATIC") == g_settings.added_ids.end() ||
		 !g_settings.bUseIDC_STATIC))
	{
		str += LoadStringDx(IDS_ADDNEXTIDS);

		for (auto& pair : g_settings.added_ids)
		{
			str += TEXT("#define ");
			str += MAnsiToText(CP_ACP, pair.first).c_str();
			str += TEXT(" ");
			str += MAnsiToText(CP_ACP, pair.second).c_str();
			str += TEXT("\r\n");
		}
		str += TEXT("\r\n");
	}

	if (str.empty())
	{
		str += LoadStringDx(IDS_NOCHANGE);
		str += TEXT("\r\n");
	}

	// show the dialog
	MAdviceResHDlg dialog(str);
	dialog.DialogBoxDx(hwnd);
}

// ID_UNLOADRESH: unload the resource.h info
void MMainWnd::OnUnloadResH(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// unload the resource.h file
	UnloadResourceH(hwnd);
}

// ID_CONFIG: the configuration dialog
void MMainWnd::OnConfig(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MConfigPropSheet dialog;
	dialog.DoModalDx(hwnd, PAGE_CONFIG);
}

// ID_USEIDC_STATIC: use IDC_STATIC macro or not
void MMainWnd::OnUseIDC_STATIC(HWND hwnd)
{
	// toggle the flag
	g_settings.bUseIDC_STATIC = !g_settings.bUseIDC_STATIC;

	// select the entry to update the text
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE, STV_RESETTEXT);
}

// ID_HIDEIDMACROS: show/hide the ID macros
void MMainWnd::OnHideIDMacros(HWND hwnd)
{
	BOOL bListOpen = IsWindow(m_id_list_dlg);

	// toggle the flag
	g_settings.bHideID = !g_settings.bHideID;

	UpdateNames(FALSE);

	ShowIDList(hwnd, bListOpen);

	// select the entry to update the text
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE);
}

// ID_IDLIST: show the ID list window
void MMainWnd::OnIDList(HWND hwnd)
{
	ShowIDList(hwnd, TRUE);
}

// ID_SHOWLANGS: show the language list
void MMainWnd::OnShowLangs(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MLangsDlg dialog;
	dialog.DialogBoxDx(hwnd);
}

// ID_SHOWHIDETOOLBAR: show/hide the toolbar
void MMainWnd::OnShowToolBar(HWND hwnd)
{
	// toggle the flag
	g_settings.bShowToolBar = !g_settings.bShowToolBar;

	if (g_settings.bShowToolBar)
		ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);
	else
		ShowWindow(m_hToolBar, SW_HIDE);

	// recalculate the splitter
	PostMessageDx(WM_SIZE);
}

// ID_EDITLABEL: start changing the resource type/name/language
void MMainWnd::OnEditLabel(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected type entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
	{
		if (entry->m_type == RT_STRING)
			return;
	}

	HTREEITEM hItem = TreeView_GetSelection(m_hwndTV);
	TreeView_EditLabel(m_hwndTV, hItem);
}

// ID_TREEITEMHELP
void MMainWnd::OnTreeItemHelp(HWND hwnd)
{
	HTREEITEM hItem = TreeView_GetSelection(m_hwndTV);
	EntryBase *entry = g_res.get_entry();
	if (!entry)
		return;

	PCSTR pszHelp = GetTreeItemHelp(entry);
	if (pszHelp)
		ShellExecuteA(hwnd, NULL, pszHelp, NULL, NULL, SW_SHOWNORMAL);
}

// ID_EXPAND_ALL: expand all the tree control items
void MMainWnd::OnExpandAll(HWND hwnd)
{
	ShowTreeViewArrow(FALSE);

	// get the selected entry
	auto entry = g_res.get_entry();

	HTREEITEM hItem = TreeView_GetRoot(m_hwndTV);
	do
	{
		Expand(hItem);
		hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
	} while (hItem);

	// select the entry
	SelectTV(entry, FALSE);

	// update drop-down arrow
	PostUpdateArrow(hwnd);
}

// ID_COLLAPSE_ALL: unexpand all the tree control items
void MMainWnd::OnCollapseAll(HWND hwnd)
{
	ShowTreeViewArrow(FALSE);

	HTREEITEM hItem = TreeView_GetRoot(m_hwndTV);
	do
	{
		Collapse(hItem);
		hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
	} while (hItem);

	// select the entry
	SelectTV(NULL, FALSE);

	// update drop-down arrow
	PostUpdateArrow(hwnd);
}

// ID_CLONE
void MMainWnd::OnClone(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_TYPE:
		OnCopyAsNewType(hwnd);
		break;

	case ET_NAME:
		OnCopyAsNewName(hwnd);
		break;

	case ET_LANG:
	case ET_STRING:
		OnCopyAsNewLang(hwnd);
		break;

	default:
		break;
	}
}

// ID_GUIDE
void MMainWnd::OnGuide(HWND hwnd)
{
	static const WCHAR szJapaneseURL[] =
		L"https://katahiromz.web.fc2.com/colony3rd/risoheditor/";
	static const WCHAR szEnglishURL[] =
		L"https://katahiromz.web.fc2.com/colony3rd/risoheditor/en/";

	if (PRIMARYLANGID(GetThreadUILanguage()) == LANG_JAPANESE)
		ShellExecuteW(hwnd, NULL, szJapaneseURL, NULL, NULL, SW_SHOWNORMAL);
	else
		ShellExecuteW(hwnd, NULL, szEnglishURL, NULL, NULL, SW_SHOWNORMAL);
}

// ID_QUERYCONSTANT
void MMainWnd::OnQueryConstant(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	MConstantDlg dialog;
	dialog.DialogBoxDx(hwnd);
}

// ID_EXTRACTBANG
void MMainWnd::OnExtractBang(HWND hwnd)
{
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_TYPE:
	case ET_NAME:
	case ET_STRING:
		OnExtractBin(hwnd);
		break;

	case ET_LANG:
		if (entry->m_type == RT_ICON || entry->m_type == RT_GROUP_ICON ||
			entry->m_type == RT_ANIICON)
		{
			OnExtractIcon(hwnd);
		}
		else if (entry->m_type == RT_CURSOR || entry->m_type == RT_GROUP_CURSOR ||
				 entry->m_type == RT_ANICURSOR)
		{
			OnExtractCursor(hwnd);
		}
		else if (entry->m_type == RT_BITMAP)
		{
			OnExtractBitmap(hwnd);
		}
		else if (entry->m_type == RT_RCDATA && entry->is_delphi_dfm())
		{
			OnExtractDFM(hwnd);
		}
		else if (entry->m_type == L"TYPELIB")
		{
			OnExtractTLB(hwnd);
		}
		else
		{
			OnExtractBin(hwnd);
		}
		break;

	default:
		break;
	}
}

// ID_WORD_WRAP: toggle the word wrapping of the source EDIT control
void MMainWnd::OnWordWrap(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// save the modified flag
	BOOL bModified = Edit_GetModify(m_hCodeEditor);

	// switch the flag
	g_settings.bWordWrap = !g_settings.bWordWrap;

	// get text
	MString strText = GetWindowTextW(m_hCodeEditor);

	// create the source EDIT control
	ReCreateSrcEdit(m_splitter2);

	// reset fonts
	ReCreateFonts(hwnd);

	// restore text
	SetWindowTextW(m_hCodeEditor, strText.c_str());

	// restore the modified flag
	Edit_SetModify(m_hCodeEditor, bModified);

	// select the entry to refresh
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE, STV_DONTRESET);
}

// ID_USEBEGINEND
void MMainWnd::OnUseBeginEnd(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	g_settings.bUseBeginEnd = !g_settings.bUseBeginEnd;

	// select the entry to refresh
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE, STV_RESETTEXT);
}

// ID_REFRESHALL
void MMainWnd::OnRefreshAll(HWND hwnd)
{
	BOOL bModifiedOld = s_bModified;
	DoRefreshTV(hwnd);
	DoRefreshIDList(hwnd);
	SelectTV(g_res.get_entry(), FALSE);

	s_bModified = bModifiedOld;

	PostUpdateArrow(hwnd);
}

// ID_HELP
void MMainWnd::OnHelp(HWND hwnd)
{
	ShellExecuteW(hwnd, NULL, LoadStringDx(IDS_HOMEPAGE), NULL, NULL, SW_SHOWNORMAL);
}

// ID_TEST: do resource test
void MMainWnd::OnTest(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(::IsWindowVisible(m_rad_window)))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	if (entry->m_type == RT_DIALOG)
	{
		// entry->m_data --> stream --> dialog_res
		DialogRes dialog_res;
		MByteStreamEx stream(entry->m_data);
		dialog_res.LoadFromStream(stream);

		if (!dialog_res.m_class.empty())
		{
			// TODO: support the classed dialogs
			ErrorBoxDx(IDS_CANTTESTCLASSDLG);
		}
		else
		{
			// detach the dialog template menu (it will be used after)
			MIdOrString menu = dialog_res.m_menu;
			dialog_res.m_menu.clear();
			stream.clear();

			// fixup for "AtlAxWin*" and/or "{...}" window classes.
			// see also: DialogRes::FixupForTest
			dialog_res.FixupForTest(false);
			dialog_res.SaveToStream(stream);
			dialog_res.FixupForTest(true);

			// load RT_DLGINIT if any
			std::vector<BYTE> dlginit_data;
			if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, entry->m_lang))
			{
				dlginit_data = e->m_data;
			}

			// show test dialog
			if (dialog_res.m_style & WS_CHILD)
			{
				// dialog_res is a child dialog. create its parent (with menu if any)
				auto window = new MTestParentWnd(dialog_res, menu, entry->m_lang,
												 stream, dlginit_data);
				window->CreateWindowDx(hwnd, LoadStringDx(IDS_PARENTWND),
					WS_DLGFRAME | WS_POPUPWINDOW, WS_EX_APPWINDOW);

				// show it
				ShowWindow(*window, g_bNoGuiMode ? SW_HIDE : SW_SHOWNORMAL);
				UpdateWindow(*window);
			}
			else
			{
				// it's a non-child dialog. show the test dialog (with menu if any)
				MTestDialog dialog(dialog_res, menu, entry->m_lang, dlginit_data);
				dialog.DialogBoxIndirectDx(hwnd, stream.ptr());
				stream = std::move(stream);
			}
		}
	}
	else if (entry->m_type == RT_MENU)
	{
		// load a menu from memory
		HMENU hMenu = LoadMenuIndirect(&(*entry)[0]);
		if (hMenu)
		{
			// show the dialog
			MTestMenuDlg dialog(hMenu);
			dialog.DialogBoxDx(hwnd, IDD_MENUTEST);

			// unload the menu
			DestroyMenu(hMenu);
		}
	}
}

// ID_GOTOLINE
void MMainWnd::OnGoToLine(HWND hwnd)
{
	MGoToLineDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
		SetShowMode(SHOW_CODEONLY, FALSE);
		INT line = dialog.m_line;
		INT ich = Edit_LineIndex(m_hCodeEditor, (line <= 0) ? 0 : (line - 1));
		INT cch = Edit_GetTextLength(m_hCodeEditor);
		if (ich == -1)
			Edit_SetSel(m_hCodeEditor, cch, cch);
		else
			Edit_SetSel(m_hCodeEditor, ich, ich);
		Edit_ScrollCaret(m_hCodeEditor);
		SetFocus(m_hCodeEditor);
	}
}

// ID_FIND: find the text
void MMainWnd::OnFind(HWND hwnd)
{
	m_search.bDownward = TRUE;
	OnItemSearch(hwnd);
}

// ID_FINDDOWNWARD: find next
BOOL MMainWnd::OnFindNext(HWND hwnd)
{
	m_search.bDownward = TRUE;
	if (m_search.strText.empty())
	{
		OnItemSearch(hwnd);
		return TRUE;
	}
	DoItemSearchBang(hwnd, NULL);
	return TRUE;
}

// ID_FINDUPWARD: find previous
BOOL MMainWnd::OnFindPrev(HWND hwnd)
{
	m_search.bDownward = FALSE;
	if (m_search.strText.empty())
	{
		OnItemSearch(hwnd);
		return TRUE;
	}
	DoItemSearchBang(hwnd, NULL);
	return TRUE;
}

// ID_ITEMSEARCH: show the item search dialog
void MMainWnd::OnItemSearch(HWND hwnd)
{
	// is there "item search" dialogs?
	if (MItemSearchDlg::Dialog())
	{
		HWND hDlg = *MItemSearchDlg::Dialog();
		if (::IsWindow(hDlg))
		{
			// bring it to the top
			SetForegroundWindow(hDlg);
			SetFocus(hDlg);
			return;
		}
		// erase hDlg from dialogs
		MItemSearchDlg::Dialog() = nullptr;
	}

	// create dialog
	auto pDialog = std::make_shared<MItemSearchDlg>(m_search);
	pDialog->CreateDialogDx(hwnd);
	MItemSearchDlg::Dialog() = pDialog;

	// set the window handles to m_search.res2text
	m_search.res2text.m_hwnd = hwnd;
	m_search.res2text.m_hwndDialog = *pDialog;

	// show it
	ShowWindow(*pDialog, SW_SHOWNORMAL);
	UpdateWindow(*pDialog);
}

// ID_CHECKUPDATE
void MMainWnd::OnCheckUpdate(HWND hwnd)
{
	std::wstring local_version = GetRisohEditorVersion();
	if (local_version.empty())
	{
		ErrorBoxDx(IDS_CANTCHECKUPDATE);
		return;
	}

	WCHAR szPath[MAX_PATH], szFile[MAX_PATH];
	GetTempPathW(_countof(szPath), szPath);
	GetTempFileNameW(szPath, L"Upd", 0, szFile);

	std::wstring page = L"https://katahiromz.web.fc2.com/re/version.html";
	DeleteUrlCacheEntryW(page.c_str());
	HRESULT hr = URLDownloadToFileW(NULL, page.c_str(), szFile, 0, NULL);
	if (FAILED(hr))
	{
		ErrorBoxDx(IDS_CANTCHECKUPDATE);
		return;
	}

	std::wstring url;
	std::wstring remote_version = ParseVersionFile(szFile, url);
	DeleteFileW(szFile);
	if (remote_version.empty() || url.empty())
	{
		ErrorBoxDx(IDS_CANTCHECKUPDATE);
		return;
	}

	std::vector<std::wstring> vecLocalVersion, vecRemoteVersion;
	mstr_split(vecLocalVersion, local_version, L".");
	mstr_split(vecRemoteVersion, remote_version, L".");

	// comparing version
	int compare = 0;
	for (size_t i = 0; i < 4; ++i)
	{
		if (i >= vecLocalVersion.size() || i >= vecRemoteVersion.size())
		{
			compare = (int)vecLocalVersion.size() - (int)vecRemoteVersion.size();
			break;
		}
		int a0 = _wtoi(vecLocalVersion[i].c_str());
		int a1 = _wtoi(vecRemoteVersion[i].c_str());
		if (a0 < a1)
		{
			compare = -1;
			break;
		}
		if (a0 > a1)
		{
			compare = +1;
			break;
		}
	}

	WCHAR szText[256];
	if (compare < 0)
	{
		StringCchPrintfW(szText, _countof(szText), LoadStringDx(IDS_THEREISUPDATE),
		                 remote_version.c_str());
		if (MsgBoxDx(szText, MB_ICONINFORMATION | MB_YESNOCANCEL) == IDYES)
		{
			ShellExecuteW(hwnd, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
	}
	else
	{
		MsgBoxDx(IDS_NOUPDATE, MB_ICONINFORMATION);
	}
}

IMPORT_RESULT MMainWnd::DoImport(HWND hwnd, LPCWSTR pszFile, LPCWSTR pchDotExt)
{
	if (!pchDotExt)
		return NOT_IMPORTABLE;

	if (lstrcmpiW(pchDotExt, L".rc") == 0 || lstrcmpiW(pchDotExt, L".rc2") == 0)
	{
		return DoImportRC(hwnd, pszFile);
	}
	else if (lstrcmpiW(pchDotExt, L".res") == 0)
	{
		return DoImportRes(hwnd, pszFile);
	}
	else if (lstrcmpiW(pchDotExt, L".ico") == 0)
	{
		// show the dialog
		MAddIconDlg dialog;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// refresh the ID list window
			DoRefreshIDList(hwnd);

			// select the entry
			SelectTV(ET_LANG, dialog, FALSE);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".cur") == 0 || lstrcmpiW(pchDotExt, L".ani") == 0)
	{
		// show the dialog
		MAddCursorDlg dialog;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// refresh the ID list window
			DoRefreshIDList(hwnd);

			// select the entry
			SelectTV(ET_LANG, dialog, FALSE);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".html") == 0 || lstrcmpiW(pchDotExt, L".htm") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_type = RT_HTML;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".manifest") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_type = RT_MANIFEST;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".wav") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_type = L"WAVE";
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".mp3") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_type = L"MP3";
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".bmp") == 0 || lstrcmpiW(pchDotExt, L".dib") == 0)
	{
		// show the dialog
		MAddBitmapDlg dialog;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// refresh the ID list window
			DoRefreshIDList(hwnd);

			// select the entry
			SelectTV(ET_LANG, dialog, FALSE);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".png") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"PNG";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".gif") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"GIF";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".jpg") == 0 || lstrcmpiW(pchDotExt, L".jpeg") == 0 ||
		lstrcmpiW(pchDotExt, L".jpe") == 0 || lstrcmpiW(pchDotExt, L".jfif") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"JPEG";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".tif") == 0 || lstrcmpiW(pchDotExt, L".tiff") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"TIFF";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".avi") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"AVI";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".wmf") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"WMF";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".emf") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"EMF";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".dfm") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = RT_RCDATA;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".tlb") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"TYPELIB";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}

	return NOT_IMPORTABLE;
}

// ID_EDIT: do text edit
void MMainWnd::OnEdit(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(::IsWindowVisible(m_rad_window)))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!IsEntryTextEditable(entry))
		return;

	// Tree double-click / ID_EDIT on an already-selected, unmodified entry
	// must not run HidePreview→Preview. That path clears m_hCodeEditor and
	// writes the same text back, which is the remaining flicker even with
	// PreviewBatch (SetWindowText still rebuilds the edit buffer).
	HTREEITEM hSel = TreeView_GetSelection(m_hwndTV);
	if (entry->m_hItem == hSel &&
		!Edit_GetModify(m_hCodeEditor) &&
		GetWindowTextLengthW(m_hCodeEditor) > 0)
	{
		if (GetWindowStyle(m_hCodeEditor) & ES_READONLY)
			Edit_SetReadOnly(m_hCodeEditor, FALSE);

		if (entry->is_testable())
			UpdateOurToolBarButtons(0);
		else if (entry->can_gui_edit())
			UpdateOurToolBarButtons(4);
		else
			UpdateOurToolBarButtons(3);
		return;
	}

	// make it non-read-only and (re)load the entry text once
	Edit_SetReadOnly(m_hCodeEditor, FALSE);
	SelectTV(entry, TRUE);
}

// ID_UPDATERESHBANG: do save or update the resource.h file
void MMainWnd::OnUpdateResHBang(HWND hwnd)
{
	// check whether the ID list window is open or not
	BOOL bListOpen = IsWindow(m_id_list_dlg);

	// destroy the ID list window
	DestroyWindow(m_id_list_dlg);

	// // query update to the user
	// if (MsgBoxDx(IDS_UPDATERESH, MB_ICONINFORMATION | MB_YESNO) == IDNO)    // don't update
	// {
	//     ShowIDList(hwnd, bListOpen);
	//     return;
	// }

	// build new "resource.h" file path
	WCHAR szResH[MAX_PATH];

	if (m_szResourceH[0])
	{
		StringCchCopyW(szResH, _countof(szResH), m_szResourceH);
	}
	else if (m_szFile[0])
	{
		StringCchCopyW(szResH, _countof(szResH), m_szFile);

		WCHAR *pch = wcsrchr(szResH, L'\\');
		if (pch == NULL)
			pch = wcsrchr(szResH, L'/');
		if (pch == NULL)
			return; // failure

		*pch = 0;
		StringCchCatW(szResH, _countof(szResH), L"\\resource.h");
	}
	else
	{
		StringCchCopyW(szResH, _countof(szResH), L"resource.h");
	}

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
	ofn.lpstrFile = szResH;
	ofn.nMaxFile = _countof(szResH);
	ofn.lpstrTitle = LoadStringDx(IDS_SAVERESH);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"h";

	// let the user choose the path
	if (!GetSaveFileNameW(&ofn))
	{
		return;     // cancelled
	}

	// create new
	if (!DoWriteResH(szResH))
	{
		ErrorBoxDx(IDS_CANTWRITERESH);
		ShowIDList(hwnd, bListOpen);
		return;     // failure
	}

	// szResH --> m_szResourceH
	StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);

	// clear modification of IDs
	g_settings.added_ids.clear();
	g_settings.removed_ids.clear();

	// reopen the ID list window if necessary
	ShowIDList(hwnd, bListOpen);

	DoSetFileModified(TRUE);
}

// ID_LOADRESHBANG: load the resource.h file
void MMainWnd::OnLoadResHBang(HWND hwnd)
{
	if (m_szResourceH[0])
	{
		MString strFile = m_szResourceH;
		DoLoadResH(hwnd, strFile.c_str());

		if (m_szResourceH[0])
		{
			ShowIDList(hwnd, TRUE);
		}
	}
}

void MMainWnd::OnOpenLocalFile(HWND hwnd, LPCWSTR filename)
{
	// compile if necessary
	if (!CompileIfNecessary(::IsWindowVisible(m_rad_window)))
		return;

	// get the module path filename of this application module
	WCHAR szPath[MAX_PATH];
	GetModuleFileNameW(NULL, szPath, _countof(szPath));

	LPWSTR pch = PathFindFileNameW(szPath);
	*pch = 0;

	for (INT m = 0; m <= 3; ++m)
	{
		MString strPath = szPath;
		for (INT n = 0; n < m; ++n)
			strPath += L"..\\";

		strPath += filename;

		if (PathFileExistsW(strPath.c_str()))
		{
			// open it
			ShellExecuteW(hwnd, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			return;
		}
	}
}

// ID_LOADWCLIB: load a window class library
void MMainWnd::OnLoadWCLib(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	WCHAR file[MAX_PATH] = TEXT("");

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_DLLFILTER));
	ofn.lpstrFile = file;
	ofn.nMaxFile = _countof(file);
	ofn.lpstrTitle = LoadStringDx(IDS_LOADWCLIB);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"dll";       // the default extension

	// let the user choose the path
	if (GetOpenFileNameW(&ofn))
	{
		// load the window class library
		HMODULE hMod = LoadLibraryW(file);
		if (!hMod)
		{
			ErrorBoxDx(IDS_CANNOTLOAD);
			return;
		}

		// success. add it
		s_wclib.insert(hMod);
	}
}

// ID_CANCELEDIT: cancel edit
void MMainWnd::OnCancelEdit(HWND hwnd)
{
	// A pending MYWM_UPDATEDLGRES timer can still fire after the RAD window
	// is gone and rewrite m_hCodeEditor a second time, which is one of the
	// sources of the flicker seen when closing the GUI editor.
	::KillTimer(hwnd, CODE_REFRESH_TIMER_ID);

	// clear modification flag
	Edit_SetModify(m_hCodeEditor, FALSE);
	Edit_SetReadOnly(m_hCodeEditor, FALSE);

	// Collapse the SelectTV/Preview path (and the line-mark clear) into a
	// single paint of the LineNumEdit, matching the anti-flicker approach
	// already used for tree-view selection changes.
	BeginPreviewBatch();
	::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE);
	EndPreviewBatch();
}

// ID_NEXTPANE, ID_PREVPANE
void MMainWnd::OnNextPane(HWND hwnd, BOOL bNext)
{
	HWND hwndCodeEditor = m_hCodeEditor;
	HWND hwndHexViewer = m_hHexViewer;
	HWND hwndRad = IsWindow(m_rad_window) ? (HWND)m_rad_window : NULL;
	HWND hwndIDList = IsWindow(m_id_list_dlg) ? (HWND)m_id_list_dlg : NULL;
	HWND hwndFind = IsWindow(m_hFindReplaceDlg) ? (HWND)m_hFindReplaceDlg : NULL;

	HWND hwndFocus = GetFocus();

	if (hwndRad != NULL && GetParent(hwndFocus) == hwndRad)
		hwndFocus = hwndRad;

	if (hwndIDList != NULL && GetParent(hwndFocus) == hwndIDList)
		hwndFocus = hwndIDList;

	if (hwndFind != NULL && GetParent(hwndFocus) == hwndFind)
		hwndFocus = hwndFind;

	if (hwndFocus == NULL)
	{
		SetFocus(m_hwndTV);
		return;
	}

	HWND ahwnd[] =
	{
		m_hwndTV, m_hCodeEditor, m_hHexViewer, hwndRad, m_hFindReplaceDlg, hwndIDList
	};

	UINT i;
	for (i = 0; i < _countof(ahwnd); ++i)
	{
		if (ahwnd[i] == hwndFocus)
			break;
	}

	if (i == _countof(ahwnd))
	{
		SetFocus(m_hwndTV);
		return;
	}

	if (bNext)
	{
		do
		{
			++i;
			if (i == _countof(ahwnd))
				i = 0;
		} while (ahwnd[i] == NULL);
	}
	else
	{
		do
		{
			if (i == 0)
				i = _countof(ahwnd) - 1;
			else
				--i;
		} while (ahwnd[i] == NULL);
	}

	if (hwndCodeEditor == ahwnd[i])
	{
		OnSelChange(hwnd, 0);
		SetFocus(m_hCodeEditor);
	}
	else if (hwndHexViewer == ahwnd[i])
	{
		OnSelChange(hwnd, 1);
		SetFocus(m_hHexViewer);
	}
	else
	{
		SetFocus(ahwnd[i]);
	}
}

PCSTR MMainWnd::GetWordHelp(const MStringW& str)
{
	auto entry = g_res.get_entry();
	if (str.empty())
		return NULL;
	if (str == L"ACCELERATORS")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/accelerators-resource";
	if (str == L"AUTO3STATE")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/auto3state-control";
	if (str == L"AUTOCHECKBOX")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/autocheckbox-control";
	if (str == L"AUTORADIOBUTTON")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/autoradiobutton-control";
	if (str == L"BITMAP")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/bitmap-resource";
	if (str == L"BUTTON")
	{
		if (entry && entry->m_type == RT_TOOLBAR)
			return "https://github.com/katahiromz/RisohEditor/blob/master/src/Toolbar.h";
		return "https://learn.microsoft.com/en-us/windows/win32/controls/buttons";
	}
	if (str == L"CAPTION")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/caption-statement";
	if (str == L"CHARACTERISTICS")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/characteristics-statement";
	if (str == L"CHECKBOX")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/checkbox-control";
	if (str == L"CLASS")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/class-statement";
	if (str == L"COMBOBOX")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/combobox-control";
	if (str == L"CONTROL")
	{
		if (entry && entry->m_type == RT_DIALOG)
			return "https://learn.microsoft.com/en-us/windows/win32/menurc/control-control";
		if (entry && entry->m_type == RT_ACCELERATOR)
			return "https://learn.microsoft.com/en-us/windows/win32/menurc/accelerators-resource";
	}
	if (str == L"CTEXT")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/ctext-control";
	if (str == L"CURSOR")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/cursor-resource";
	if (str == L"DEFPUSHBUTTON")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/defpushbutton-control";
	if (str == L"DIALOG")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/dialog-resource";
	if (str == L"DIALOGEX")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/dialogex-resource";
	if (str == L"EDITTEXT")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/edittext-control";
	if (str == L"EXSTYLE")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/exstyle-statement";
	if (str == L"FONT")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/font-statement";
	if (str == L"GROUPBOX")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/groupbox-control";
	if (str == L"HTML")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/html-resource";
	if (str == L"ICON")
	{
		if (entry && entry->m_type == RT_DIALOG)
			return "https://learn.microsoft.com/en-us/windows/win32/menurc/icon-control";
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/icon-resource";
	}
	if (str == L"LANGUAGE")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/language-statement";
	if (str == L"LISTBOX")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/listbox-control";
	if (str == L"LTEXT")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/ltext-control";
	if (str == L"MENU")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/menu-resource";
	if (str == L"MENUEX")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/menuex-resource";
	if (str == L"MENUITEM")
	{
		if (entry && entry->m_type == RT_MENU)
		{
			MByteStreamEx stream(entry->m_data);
			MenuRes menu_res;
			if (menu_res.LoadFromStream(stream) && menu_res.IsExtended())
				return "https://learn.microsoft.com/en-us/windows/win32/menurc/menuex-resource";
		}
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/menuitem-statement";
	}
	if (str == L"MESSAGETABLE")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/messagetable-resource";
	if (str == L"POPUP")
	{
		if (entry && entry->m_type == RT_MENU)
		{
			MByteStreamEx stream(entry->m_data);
			MenuRes menu_res;
			if (menu_res.LoadFromStream(stream) && menu_res.IsExtended())
				return "https://learn.microsoft.com/en-us/windows/win32/menurc/menuex-resource";
		}
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/popup-resource";
	}
	if (str == L"PUSHBOX")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/pushbox-control";
	if (str == L"PUSHBUTTON")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/pushbutton-control";
	if (str == L"RADIOBUTTON")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/radiobutton-control";
	if (str == L"RCDATA")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/rcdata-resource";
	if (str == L"RTEXT")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/rtext-control";
	if (str == L"SCROLLBAR")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/scrollbar-control";
	if (str == L"STATE3")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/state3-control";
	if (str == L"STRINGTABLE")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/stringtable-resource";
	if (str == L"STYLE")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/style-statement";
	if (str == L"VERSION")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/version-statement";
	if (str == L"VERSIONINFO")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource";

	if (str == L"MESSAGETABLEDX")
		return "https://github.com/katahiromz/RisohEditor/blob/master/mcdx/MESSAGETABLEDX.md";

	if (str == L"VIRTKEY" || str == L"ASCII" || str == L"NOINVERT" || str == L"ALT" || str == L"SHIFT")
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/accelerators-resource";

	if (str == L"SEPARATOR")
	{
		if (entry && entry->m_type == RT_TOOLBAR)
			return "https://github.com/katahiromz/RisohEditor/blob/master/src/Toolbar.h";
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/menuitem-statement";
	}
	if (str == L"TOOLBAR")
		return "https://github.com/katahiromz/RisohEditor/blob/master/src/Toolbar.h";

	if (str == L"GRAYED" || str == L"CHECKED" || str == L"INACTIVE" ||
		str == L"MENUBARBREAK" || str == L"MENUBREAK")
	{
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/menuitem-statement";
	}

	if (str == L"FILEVERSION" || str == L"PRODUCTVERSION" || str == L"FILEFLAGSMASK" ||
		str == L"FILEFLAGS" || str == L"FILEOS" || str == L"FILETYPE" ||
		str == L"FILESUBTYPE" || str == L"BLOCK" || str == L"VALUE")
	{
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource";
	}

	ConstantsDB::ValueType value;
	if (g_db.GetValueOfName(str.c_str(), value) || g_db.GetResIDValue(str.c_str()))
	{
		WCHAR url[MAX_PATH];
		StringCchPrintfW(url, _countof(url), LoadStringDx(IDS_SEARCHURL), str.c_str());
		ShellExecuteW(m_hwnd, nullptr, url, nullptr, nullptr, SW_SHOWNORMAL);

		MConstantDlg dialog(str.c_str());
		dialog.DialogBoxDx(m_hwnd);
		return NULL;
	}

	return NULL;
}

void MMainWnd::DoWordHelp(const MStringW& str)
{
	PCSTR pszHelp = GetWordHelp(str);
	if (pszHelp)
		ShellExecuteA(m_hwnd, NULL, pszHelp, NULL, NULL, SW_SHOWNORMAL);
}

void MMainWnd::DoHelp()
{
	if (::GetFocus() == m_hCodeEditor && GetWindowTextLengthW(m_hCodeEditor) > 0)
	{
		MStringW str = GetWindowText(m_hCodeEditor);
		DWORD ichStart, ichEnd;
		::SendMessage(m_hCodeEditor, EM_GETSEL, (WPARAM)&ichStart, (LPARAM)&ichEnd);
		ichStart = ichEnd = (ichStart + ichEnd) / 2;
		while (0 < ichStart && (mchr_is_alnum(str[ichStart - 1]) || str[ichStart - 1] == L'_'))
			--ichStart;
		while (ichEnd + 1 < str.size() && (mchr_is_alnum(str[ichEnd]) || str[ichEnd] == L'_'))
			++ichEnd;
		MStringW strSelected = str.substr(ichStart, ichEnd - ichStart);
		if (strSelected.size())
		{
			SendMessageW(m_hCodeEditor, EM_SETSEL, ichStart, ichEnd);
			DoWordHelp(strSelected);
		}
	}
	else if (::GetFocus() == m_hwndTV && g_res.get_entry())
	{
		::PostMessageW(g_hMainWnd, WM_COMMAND, ID_TREEITEMHELP, 0);
	}
	else
	{
		::PostMessageW(g_hMainWnd, WM_COMMAND, ID_HELP, 0);
	}
}

// WM_INITMENU: update the menus
void MMainWnd::OnInitMenu(HWND hwnd, HMENU hMenu)
{
	if (g_settings.bWordWrap)
		CheckMenuItem(hMenu, ID_WORD_WRAP, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_WORD_WRAP, MF_BYCOMMAND | MF_UNCHECKED);

	if (m_szResourceH[0])
		EnableMenuItem(hMenu, ID_UNLOADRESH, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(hMenu, ID_UNLOADRESH, MF_BYCOMMAND | MF_GRAYED);

	// search the language entries
	EntrySet found;
	g_res.search(found, ET_LANG);

	if (found.empty())
	{
		EnableMenuItem(hMenu, ID_ITEMSEARCH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXPAND_ALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COLLAPSE_ALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FIND, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FINDUPWARD, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACE, MF_GRAYED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_ITEMSEARCH, MF_ENABLED);
		EnableMenuItem(hMenu, ID_EXPAND_ALL, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COLLAPSE_ALL, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FIND, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FINDUPWARD, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_ENABLED);
		EnableMenuItem(hMenu, ID_REPLACE, MF_ENABLED);
	}

	if (g_settings.bShowToolBar)
		CheckMenuItem(hMenu, ID_SHOWHIDETOOLBAR, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_SHOWHIDETOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);

	if (g_settings.bUseBeginEnd)
		CheckMenuItem(hMenu, ID_USEBEGINEND, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_USEBEGINEND, MF_BYCOMMAND | MF_UNCHECKED);

	BOOL bCanEditLabel = TRUE;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (bCanEditLabel)
	{
		if (entry)
		{
			if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
			{
				if (entry->m_type == RT_STRING)
				{
					bCanEditLabel = FALSE;
				}
			}
		}
		else
		{
			bCanEditLabel = FALSE;
		}
	}

	if (bCanEditLabel)
		EnableMenuItem(hMenu, ID_EDITLABEL, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(hMenu, ID_EDITLABEL, MF_BYCOMMAND | MF_GRAYED);

	if (IsWindowVisible(m_hStatusBar))
		CheckMenuItem(hMenu, ID_STATUSBAR, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_STATUSBAR, MF_UNCHECKED);

	if (m_bShowBinEdit)
		CheckMenuItem(hMenu, ID_BINARYPANE, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_BINARYPANE, MF_UNCHECKED);

	if (g_settings.bAlwaysControl)
		CheckMenuItem(hMenu, ID_ALWAYSCONTROL, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_ALWAYSCONTROL, MF_UNCHECKED);

	if (g_settings.bHideID)
		CheckMenuItem(hMenu, ID_HIDEIDMACROS, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_HIDEIDMACROS, MF_UNCHECKED);

	if (g_settings.bUseIDC_STATIC)
		CheckMenuItem(hMenu, ID_USEIDC_STATIC, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_USEIDC_STATIC, MF_UNCHECKED);

	if (!entry || !entry->m_hItem)
	{
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
		EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EDIT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CLONE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTRC, MF_GRAYED);
		return;
	}

	EnableMenuItem(hMenu, ID_EXTRACTRC, MF_ENABLED);

	BOOL bEditable = entry && entry->is_editable(m_szVCBat);
	if (bEditable)
	{
		EnableMenuItem(hMenu, ID_EDIT, MF_ENABLED);
		if (entry->can_gui_edit())
		{
			EnableMenuItem(hMenu, ID_GUIEDIT, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
		}
	}
	else
	{
		EnableMenuItem(hMenu, ID_EDIT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
	}

	switch (entry ? entry->m_et : ET_ANY)
	{
	case ET_TYPE:
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
		EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CLONE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		break;

	case ET_NAME:
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
		EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CLONE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		break;

	case ET_LANG:
		if (entry->m_type == RT_GROUP_ICON || entry->m_type == RT_ICON ||
			entry->m_type == RT_ANIICON)
		{
			EnableMenuItem(hMenu, ID_EXTRACTICON, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		}
		if (entry->m_type == RT_GROUP_ICON || entry->m_type == RT_ANIICON)
		{
			EnableMenuItem(hMenu, ID_REPLACEICON, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		}

		if (entry->m_type == RT_BITMAP)
		{
			EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_ENABLED);
			EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
			EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		}

		if (entry->m_type == RT_GROUP_CURSOR || entry->m_type == RT_CURSOR ||
			entry->m_type == RT_ANICURSOR)
		{
			EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		}
		if (entry->m_type == RT_GROUP_CURSOR || entry->m_type == RT_ANICURSOR)
		{
			EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		}

		if (entry->is_testable())
		{
			EnableMenuItem(hMenu, ID_TEST, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		}

		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		if (entry->m_type == RT_STRING)
		{
			EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
			EnableMenuItem(hMenu, ID_CLONE, MF_GRAYED);
			EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_ENABLED);
			EnableMenuItem(hMenu, ID_CLONE, MF_ENABLED);
			EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_ENABLED);
		}
		break;

	case ET_STRING:
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
		EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CLONE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		break;

	default:
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CLONE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		break;
	}
}

// WM_COMMAND
void MMainWnd::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	if (m_hCodeEditor == hwndCtl)
	{
		switch (codeNotify)
		{
		case EN_HSCROLL:
		case EN_VSCROLL:
		case EN_UPDATE:
		case EN_SETFOCUS:
		case EN_KILLFOCUS:
			return;
		}
	}

	if (hwndCtl == m_hCodeEditor)
	{
		if (codeNotify == EN_CHANGE)
		{
			// the source EDIT control was modified.
			// change the toolbar
			UpdateOurToolBarButtons(2);

			// show "ready" status
			ChangeStatusText(IDS_READY);
			return;
		}
		if (codeNotify == LNEN_ZOOMIN)
		{
			g_settings.nSrcFontSize += 2;
			if (g_settings.nSrcFontSize >= 48)
				g_settings.nSrcFontSize = 48;
			ReCreateFonts(hwnd);
			return;
		}
		if (codeNotify == LNEN_ZOOMOUT)
		{
			g_settings.nSrcFontSize -= 2;
			if (g_settings.nSrcFontSize < 8)
				g_settings.nSrcFontSize = 8;
			ReCreateFonts(hwnd);
			return;
		}
	}

	if (hwndCtl == m_hHexViewer)
	{
		if (codeNotify == LNEN_ZOOMIN)
		{
			g_settings.nBinFontSize += 2;
			if (g_settings.nBinFontSize >= 48)
				g_settings.nBinFontSize = 48;
			ReCreateFonts(hwnd);
			return;
		}
		if (codeNotify == LNEN_ZOOMOUT)
		{
			g_settings.nBinFontSize -= 2;
			if (g_settings.nBinFontSize < 8)
				g_settings.nBinFontSize = 8;
			ReCreateFonts(hwnd);
			return;
		}
	}

	MWaitCursor wait;

	// show "executing command..." status
	if (!::IsWindow(m_rad_window) && id >= 100)
		ChangeStatusText(IDS_EXECUTINGCMD);

	// add a command lock
	++m_nCommandLock;

	// execute the command
	BOOL bUpdateStatus = TRUE;
	switch (id)
	{
	case ID_NEW:
		OnNew(hwnd);
		break;
	case ID_OPEN:
		OnOpen(hwnd);
		break;
	case ID_SAVEAS:
		OnSaveAs(hwnd);
		break;
	case ID_IMPORT:
		OnImport(hwnd);
		break;
	case ID_EXIT:
		DestroyWindow(hwnd);
		break;
	case ID_ADDICON:
		OnAddIcon(hwnd);
		break;
	case ID_ADDCURSOR:
		OnAddCursor(hwnd);
		break;
	case ID_ADDBITMAP:
		OnAddBitmap(hwnd);
		break;
	case ID_ADDRES:
		OnAddRes(hwnd);
		break;
	case ID_REPLACEICON:
		OnReplaceIcon(hwnd);
		break;
	case ID_REPLACECURSOR:
		OnReplaceCursor(hwnd);
		break;
	case ID_REPLACEBITMAP:
		OnReplaceBitmap(hwnd);
		break;
	case ID_REPLACEBIN:
		OnReplaceBin(hwnd);
		break;
	case ID_DELETERES:
		OnDeleteRes(hwnd);
		break;
	case ID_EDIT:
		OnEdit(hwnd);
		break;
	case ID_EXTRACTICON:
		OnExtractIcon(hwnd);
		break;
	case ID_EXTRACTCURSOR:
		OnExtractCursor(hwnd);
		break;
	case ID_EXTRACTBITMAP:
		OnExtractBitmap(hwnd);
		break;
	case ID_EXTRACTBIN:
		OnExtractBin(hwnd);
		break;
	case ID_ABOUT:
		OnAbout(hwnd);
		break;
	case ID_TEST:
		OnTest(hwnd);
		break;
	case ID_CANCELEDIT:
		OnCancelEdit(hwnd);
		break;
	case ID_COMPILE:
		OnCompile(hwnd);
		break;
	case ID_GUIEDIT:
		OnGuiEdit(hwnd);
		break;
	case ID_DESTROYRAD:
		OnCancelEdit(hwnd);
		break;
	case ID_DELCTRL:
		MRadCtrl::DeleteSelection();
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
	case ID_ADDCTRL:
		m_rad_window.OnAddCtrl(m_rad_window);
		break;
	case ID_CTRLPROP:
		m_rad_window.OnCtrlProp(m_rad_window);
		break;
	case ID_DLGPROP:
		m_rad_window.OnDlgProp(m_rad_window);
		break;
	case ID_CTRLINDEXTOP:
		m_rad_window.IndexTop(m_rad_window);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
	case ID_CTRLINDEXBOTTOM:
		m_rad_window.IndexBottom(m_rad_window);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
	case ID_CTRLINDEXMINUS:
		m_rad_window.IndexMinus(m_rad_window);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
	case ID_CTRLINDEXPLUS:
		m_rad_window.IndexPlus(m_rad_window);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
	case ID_SHOWINDEXLABELS:
		m_rad_window.OnShowIndex(m_rad_window);
		break;
	case ID_TOPALIGN:
		m_rad_window.OnTopAlign(m_rad_window);
		break;
	case ID_BOTTOMALIGN:
		m_rad_window.OnBottomAlign(m_rad_window);
		break;
	case ID_LEFTALIGN:
		m_rad_window.OnLeftAlign(m_rad_window);
		break;
	case ID_RIGHTALIGN:
		m_rad_window.OnRightAlign(m_rad_window);
		break;
	case ID_FITTOGRID:
		m_rad_window.OnFitToGrid(m_rad_window);
		bUpdateStatus = FALSE;
		break;
	case ID_STATUSBAR:
		// toggle the flag
		g_settings.bShowStatusBar = !g_settings.bShowStatusBar;

		// show/hide the scroll bar
		ShowStatusBar(g_settings.bShowStatusBar);

		// relayout
		PostMessageDx(WM_SIZE);
		break;
	case ID_BINARYPANE:
		// toggle the flag
		m_bShowBinEdit = !m_bShowBinEdit;
		// show/hide the binary
		m_tab.SetCurSel(!!m_bShowBinEdit);
		OnSelChange(hwnd, !!m_bShowBinEdit);
		break;
	case ID_ALWAYSCONTROL:
		{
			// toggle the flag
			g_settings.bAlwaysControl = !g_settings.bAlwaysControl;

			// select the entry to update the text
			auto entry = g_res.get_entry();
			SelectTV(entry, FALSE);
		}
		break;
	case ID_MRUFILE0:
	case ID_MRUFILE1:
	case ID_MRUFILE2:
	case ID_MRUFILE3:
	case ID_MRUFILE4:
	case ID_MRUFILE5:
	case ID_MRUFILE6:
	case ID_MRUFILE7:
	case ID_MRUFILE8:
	case ID_MRUFILE9:
	case ID_MRUFILE10:
	case ID_MRUFILE11:
	case ID_MRUFILE12:
	case ID_MRUFILE13:
	case ID_MRUFILE14:
	case ID_MRUFILE15:
		{
			DWORD i = id - ID_MRUFILE0;
			if (i < g_settings.vecRecentlyUsed.size())
			{
				DoLoadFile(hwnd, g_settings.vecRecentlyUsed[i].c_str());
			}
		}
		break;
	case ID_PLAY:
		OnPlay(hwnd);
		break;
	case ID_READY:
		if (g_bNoGuiMode)
		{
			std::vector<MStringW> commands;
			mstr_split(commands, m_commands, L"\n");
			for (auto& command : commands)
			{
				if (command.empty())
					continue;

				if (command.find(L"load:") == 0)
				{
					command = command.substr(5);
					if (!DoResLoad(command, m_load_options))
						s_ret = 3;
					continue;
				}

				if (command.find(L"save:") == 0)
				{
					command = command.substr(5);
					if (!DoResSave(command, m_save_options))
						s_ret = 4;
					continue;
				}
			}

			PostMessageW(hwnd, WM_CLOSE, 0, 0);
		}
		break;
	case ID_LOADRESH:
		OnLoadResH(hwnd);
		break;
	case ID_IDLIST:
		OnIDList(hwnd);
		break;
	case ID_UNLOADRESH:
		OnUnloadResH(hwnd);
		break;
	case ID_HIDEIDMACROS:
		OnHideIDMacros(hwnd);
		break;
	case ID_USEIDC_STATIC:
		OnUseIDC_STATIC(hwnd);
		break;
	case ID_CONFIG:
		OnConfig(hwnd);
		break;
	case ID_ADVICERESH:
		OnAdviceResH(hwnd);
		break;
	case ID_UPDATEID:
		UpdateNames();
		break;
	case ID_OPENREADME:
		OnOpenLocalFile(hwnd, L"README.txt");
		break;
	case ID_OPENREADMEJP:
		OnOpenLocalFile(hwnd, L"README_ja.txt");
		break;
	case ID_OPENREADMEES:
		OnOpenLocalFile(hwnd, L"README_es.txt");
		break;
	case ID_OPENREADMERU:
		OnOpenLocalFile(hwnd, L"README_ru.txt");
		break;
	case ID_LOADWCLIB:
		OnLoadWCLib(hwnd);
		break;
	case ID_FIND:
		OnFind(hwnd);
		break;
	case ID_FINDDOWNWARD:
		OnFindNext(hwnd);
		break;
	case ID_FINDUPWARD:
		OnFindPrev(hwnd);
		break;
	case ID_REPLACE:
		break;
	case ID_ADDMENU:
		OnAddMenu(hwnd);
		break;
	case ID_ADDTOOLBAR:
		OnAddToolbar(hwnd);
		break;
	case ID_ADDVERINFO:
		OnAddVerInfo(hwnd);
		break;
	case ID_ADDMANIFEST:
		OnAddManifest(hwnd);
		break;
	case ID_ADDDIALOG:
		OnAddDialog(hwnd);
		break;
	case ID_ADDSTRINGTABLE:
		OnAddStringTable(hwnd);
		break;
	case ID_ADDMESSAGETABLE:
		OnAddMessageTable(hwnd);
		break;
	case ID_ADDHTML:
		OnAddHtml(hwnd);
		break;
	case ID_ADDACCEL:
		OnAddAccel(hwnd);
		break;
	case ID_COPYASNEWNAME:
		OnCopyAsNewName(hwnd);
		break;
	case ID_COPYASNEWLANG:
		OnCopyAsNewLang(hwnd);
		break;
	case ID_COPYTOMULTILANG:
		OnCopyToMultiLang(hwnd);
		break;
	case ID_ITEMSEARCH:
		OnItemSearch(hwnd);
		break;
	case ID_UPDATERESHBANG:
		OnUpdateResHBang(hwnd);
		break;
	case ID_OPENLICENSE:
		OnOpenLocalFile(hwnd, L"LICENSE.txt");
		break;
	case ID_OPENHISTORY:
		OnOpenLocalFile(hwnd, L"ChangeLog.txt");
		break;
	case ID_OPENHISTORYITA:
		OnOpenLocalFile(hwnd, L"ChangeLog_it.txt");
		break;
	case ID_OPENHISTORYJPN:
		OnOpenLocalFile(hwnd, L"ChangeLog_ja.txt");
		break;
	case ID_OPENHISTORYKOR:
		OnOpenLocalFile(hwnd, L"ChangeLog_ko.txt");
		break;
	case ID_OPENHISTORYES:
		OnOpenLocalFile(hwnd, L"ChangeLog_es.txt");
		break;
	case ID_OPENHYOJUNKA:
		OnOpenLocalFile(hwnd, L"HYOJUNKA.md");
		break;
	case ID_DEBUGTREENODE:
		OnDebugTreeNode(hwnd);
		break;
	case ID_LOADRESHBANG:
		OnLoadResHBang(hwnd);
		break;
	case ID_REFRESHDIALOG:
		m_rad_window.OnRefresh(m_rad_window);
		break;
	case ID_REFRESHALL:
		OnRefreshAll(hwnd);
		break;
	case ID_EXPORT:
		OnExport(hwnd);
		break;
	case ID_EDITLABEL:
		OnEditLabel(hwnd);
		break;
	case ID_SETDEFAULTS:
		SetDefaultSettings(hwnd);
		break;
	case ID_SHOWLANGS:
		OnShowLangs(hwnd);
		break;
	case ID_SHOWHIDETOOLBAR:
		OnShowToolBar(hwnd);
		break;
	case ID_EXPAND_ALL:
		OnExpandAll(hwnd);
		break;
	case ID_COLLAPSE_ALL:
		OnCollapseAll(hwnd);
		break;
	case ID_WORD_WRAP:
		OnWordWrap(hwnd);
		break;
	case ID_CLONE:
		OnClone(hwnd);
		break;
	case ID_ADDBANG:
		break;
	case ID_EXTRACTBANG:
		OnExtractBang(hwnd);
		break;
	case ID_GUIDE:
		OnGuide(hwnd);
		break;
	case ID_QUERYCONSTANT:
		OnQueryConstant(hwnd);
		break;
	case ID_USEBEGINEND:
		OnUseBeginEnd(hwnd);
		break;
	case ID_SAVE:
		OnSave(hwnd);
		break;
	case ID_EGA:
		OnStartEgaConsole(hwnd, nullptr);
		break;
	case ID_EGA_PROGRAM:
		OnAskEgaProgramAndExecute(hwnd);
		break;
	case ID_OPENREADMEIT:
		OnOpenLocalFile(hwnd, L"README_it.txt");
		break;
	case ID_OPEN_EGA_MANUAL:
		OnOpenLocalFile(hwnd, L"EGA\\EGA-Manual.pdf");
		break;
	case ID_HELP:
		OnHelp(hwnd);
		break;
	case ID_NEXTPANE:
		OnNextPane(hwnd, TRUE);
		break;
	case ID_PREVPANE:
		OnNextPane(hwnd, FALSE);
		break;
	case ID_EXTRACTRC:
		OnExtractRC(hwnd);
		break;
	case ID_EXPORTRES:
		OnExportRes(hwnd);
		break;
	case ID_CHECKUPDATE:
		OnCheckUpdate(hwnd);
		break;
	case ID_OPENREADMEKO:
		OnOpenLocalFile(hwnd, L"README_ko.txt");
		break;
	case ID_CHOOSEUILANG:
		{
			MChooseLangDlg dialog;
			if (dialog.DialogBoxDx(hwnd) == IDOK)
				g_settings.ui_lang = dialog.m_langid;
		}
		break;
	case ID_OPENREADMETR:
		OnOpenLocalFile(hwnd, L"README_tr.txt");
		break;
	case ID_OPENREADMEID:
		OnOpenLocalFile(hwnd, L"README_id.txt");
		break;
	case ID_OPENHISTORYID:
		OnOpenLocalFile(hwnd, L"ChangeLog_id.txt");
		break;
	case ID_OPENREADMEPTB:
		OnOpenLocalFile(hwnd, L"README_pt_BR.txt");
		break;
	case ID_OPENHISTORYPTB:
		OnOpenLocalFile(hwnd, L"ChangeLog_pt_BR.txt");
		break;
	case ID_INTERNALTEST:
		OnInternalTest(hwnd);
		break;
	case ID_GOTOLINE:
		OnGoToLine(hwnd);
		break;
	case ID_OPENREADMEUK:
		OnOpenLocalFile(hwnd, L"README_uk.txt");
		break;
	case ID_OPENHISTORYUK:
		OnOpenLocalFile(hwnd, L"ChangeLog_uk.txt");
		break;
	case ID_OPENREADMEPL:
		OnOpenLocalFile(hwnd, L"README_pl.txt");
		break;
	case ID_TREEITEMHELP:
		OnTreeItemHelp(hwnd);
		break;
	case ID_RESETSETTINGS:
		if (!OnNew(hwnd))
			break;
		if (MsgBoxDx(IDS_WARNRESETSETTINGS, MB_ICONWARNING | MB_YESNOCANCEL) == IDYES)
		{
			DestroyRadWindow();
			DestroyWindow(m_id_list_dlg);
			DestroyWindow(s_hwndEga);
			SetDefaultSettings(hwnd);
		}
		break;
	default:
		bUpdateStatus = FALSE;
		break;
	}

	// remove the command lock
	--m_nCommandLock;

	if (m_nCommandLock == 0)
	{
		g_res.delete_invalid();     // clean up invalids
		if (g_res.empty())
			HidePreview(STV_RESETTEXT);
	}

	UpdateToolBarStatus();

	// show "ready" status if ready
	if (m_nCommandLock == 0 && bUpdateStatus && !::IsWindow(m_rad_window))
	{
		if (m_nStatusStringID != 0)
		{
			ChangeStatusText(m_nStatusStringID);
			m_nStatusStringID = 0;
		}
		else
		{
			ChangeStatusText(IDS_READY);
		}
	}
}

// ID_DEBUGTREENODE: for debugging purpose
void MMainWnd::OnDebugTreeNode(HWND hwnd)
{
	WCHAR sz[MAX_PATH * 2 + 32];

	// show the file paths
	StringCchPrintfW(sz, _countof(sz), L"%s\n\n%s", m_szFile, m_szResourceH);
	MsgBoxDx(sz, MB_ICONINFORMATION);

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	static const LPCWSTR apszI_[] =
	{
		L"ET_ANY",
		L"ET_TYPE",
		L"ET_STRING",
		L"ET_NAME",
		L"ET_LANG"
	};

	MStringW type = entry->m_type.str();
	MStringW name = entry->m_name.str();
	StringCchPrintfW(sz, _countof(sz),
		L"%s: type:%s, name:%s, lang:0x%04X, entry:%p, hItem:%p, strLabel:%s", apszI_[entry->m_et],
		type.c_str(), name.c_str(), entry->m_lang, entry, entry->m_hItem, entry->m_strLabel.c_str());
	MsgBoxDx(sz, MB_ICONINFORMATION);
}
