// MMainWnd_Preview.cpp --- Preview function implementations for MMainWnd
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MMainWnd.hpp"
#define LINENUMEDIT_IMPL
#include "LineNumEdit.hpp"

#include "AccelRes.hpp"
#include "MenuRes.hpp"
#include "ToolbarRes.hpp"
#include "VersionRes.hpp"

#include "Utils.h"
#include "MStrBin.hpp"

// preview the icon resource
BOOL MMainWnd::PreviewIcon(HWND hwnd, const EntryBase& entry)
{
	// create a bitmap object from the entry and set it to m_hBmpView
	BITMAP bm;
	HBITMAP hbm = CreateBitmapFromIconOrPngDx(hwnd, entry, bm);
	if (!hbm)
		return FALSE;

	m_hBmpView.SetBitmap(hbm);

	// create the icon
	MStringW str;
	HICON hIcon = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, TRUE);

	// dump info to m_hCodeEditor
	if (hIcon)
	{
		str = DumpIconInfo(bm, TRUE);
		DestroyIcon(hIcon);
	}
	else
	{
		str = DumpBitmapInfo(m_hBmpView.m_hBitmap);
	}
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the cursor resource
BOOL MMainWnd::PreviewCursor(HWND hwnd, const EntryBase& entry)
{
	// create a cursor object from the entry and set it to m_hBmpView
	BITMAP bm;
	HCURSOR hCursor = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, FALSE);
	if (!hCursor)
		return FALSE;
	m_hBmpView.SetBitmap(CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE));

	// dump info to m_hCodeEditor
	MStringW str = DumpIconInfo(bm, FALSE);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// destroy the cursor
	DestroyCursor(hCursor);

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the group icon resource
BOOL MMainWnd::PreviewGroupIcon(HWND hwnd, const EntryBase& entry)
{
	HBITMAP hbm = CreateBitmapFromIconsDx(hwnd, entry);
	if (!hbm)
		return FALSE;
	// create a bitmap object from the entry and set it to m_hBmpView
	m_hBmpView.SetBitmap(hbm);

	// dump the text to m_hCodeEditor
	ResToText res2text;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the group cursor resource
BOOL MMainWnd::PreviewGroupCursor(HWND hwnd, const EntryBase& entry)
{
	HBITMAP hbm = CreateBitmapFromCursorsDx(hwnd, entry);
	if (!hbm)
		return FALSE;

	// create a bitmap object from the entry and set it to m_hBmpView
	m_hBmpView.SetBitmap(hbm);
	assert(m_hBmpView);

	// dump the text to m_hCodeEditor
	ResToText res2text;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the bitmap resource
BOOL MMainWnd::PreviewBitmap(HWND hwnd, const EntryBase& entry)
{
	// create a bitmap object from the entry and set it to m_hBmpView
	HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&entry[0], entry.size());
	if (!hbm)
		return FALSE;
	m_hBmpView.SetBitmap(hbm);

	// dump the text to m_hCodeEditor
	ResToText res2text;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the image resource
BOOL MMainWnd::PreviewImage(HWND hwnd, const EntryBase& entry)
{
	// dump the text to m_hCodeEditor
	ResToText res2text;
	MStringW str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// set the entry image to m_hBmpView
	if (!m_hBmpView.SetImage(&entry[0], entry.size()))
		return FALSE;

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the WAVE resource
BOOL MMainWnd::PreviewWAVE(HWND hwnd, const EntryBase& entry)
{
	// dump the text to m_hCodeEditor
	ResToText res2text;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// make it playable
	m_hBmpView.SetPlay();

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the MP3 resource
BOOL MMainWnd::PreviewMP3(HWND hwnd, const EntryBase& entry)
{
	// dump the text to m_hCodeEditor
	ResToText res2text;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// make it playable
	m_hBmpView.SetPlay();

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the AVI resource
BOOL MMainWnd::PreviewAVI(HWND hwnd, const EntryBase& entry)
{
	// dump the text to m_hCodeEditor
	ResToText res2text;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// set the AVI
	if (!m_hBmpView.SetMedia(&entry[0], entry.size(), L"avi"))
		return FALSE;

	// show movie
	SetShowMode(SHOW_MOVIE);
	return TRUE;
}

// preview the RT_ACCELERATOR resource
BOOL MMainWnd::PreviewAccel(HWND hwnd, const EntryBase& entry)
{
	// entry.m_data --> stream --> accel
	AccelRes accel;
	MByteStreamEx stream(entry.m_data);
	if (accel.LoadFromStream(stream))
	{
		// dump the text to m_hCodeEditor
		MString str = GetLanguageStatement(entry.m_lang);
		str += accel.Dump(entry.m_name);
		SetWindowTextW(m_hCodeEditor, str.c_str());
		SetShowMode(SHOW_CODEONLY);
		return TRUE;
	}
	return FALSE;
}

// preview the message table resource
BOOL MMainWnd::PreviewMessage(HWND hwnd, const EntryBase& entry)
{
	// entry.m_data --> stream --> mes
	MessageRes mes;
	MByteStreamEx stream(entry.m_data);
	if (mes.LoadFromStream(stream))
	{
		// dump the text to m_hCodeEditor
		MString str = GetLanguageStatement(entry.m_lang);
		str += mes.Dump(entry.m_name);
		SetWindowTextW(m_hCodeEditor, str.c_str());
		SetShowMode(SHOW_CODEONLY);
		return TRUE;
	}
	return FALSE;
}

// preview the string resource
BOOL MMainWnd::PreviewString(HWND hwnd, const EntryBase& entry)
{
	// entry.m_data --> stream --> str_res
	StringRes str_res;
	MByteStreamEx stream(entry.m_data);
	WORD nNameID = entry.m_name.m_id;
	if (str_res.LoadFromStream(stream, nNameID))
	{
		// dump the text to m_hCodeEditor
		MStringW str = str_res.Dump(nNameID);
		SetWindowTextW(m_hCodeEditor, str.c_str());
		SetShowMode(SHOW_CODEONLY);
		return TRUE;
	}
	return FALSE;
}

// preview the HTML resource
BOOL MMainWnd::PreviewHtml(HWND hwnd, const EntryBase& entry)
{
	// load a text file
	MTextType type;
	type.nNewLine = MNEWLINE_CRLF;
	MStringW str;
	if (entry.size())
		str = mstr_from_bin(&entry.m_data[0], entry.m_data.size(), &type);

	// dump the text to m_hCodeEditor
	SetWindowTextW(m_hCodeEditor, str.c_str());
	SetShowMode(SHOW_CODEONLY);
	return TRUE;
}

// preview the menu resource
BOOL MMainWnd::PreviewMenu(HWND hwnd, const EntryBase& entry)
{
	// entry.m_data --> stream --> menu_res
	MenuRes menu_res;
	MByteStreamEx stream(entry.m_data);
	if (menu_res.LoadFromStream(stream))
	{
		// dump the text to m_hCodeEditor
		MString str = GetLanguageStatement(entry.m_lang);
		str += menu_res.Dump(entry.m_name);
		SetWindowTextW(m_hCodeEditor, str.c_str());
		SetShowMode(SHOW_CODEONLY);
		return TRUE;
	}
	return FALSE;
}

// preview the TOOLBAR resource
BOOL MMainWnd::PreviewToolbar(HWND hwnd, const EntryBase& entry)
{
	// entry.m_data --> stream --> toolbar_res
	ToolbarRes toolbar_res;
	MByteStreamEx stream(entry.m_data);
	if (toolbar_res.LoadFromStream(stream))
	{
		// dump the text to m_hCodeEditor
		MString str = GetLanguageStatement(entry.m_lang);
		str += toolbar_res.Dump(entry.m_name);
		SetWindowTextW(m_hCodeEditor, str.c_str());
		SetShowMode(SHOW_CODEONLY);
		return TRUE;
	}
	return FALSE;
}

// preview the version resource
BOOL MMainWnd::PreviewVersion(HWND hwnd, const EntryBase& entry)
{
	// entry.m_data --> ver_res
	VersionRes ver_res;
	if (ver_res.LoadFromData(entry.m_data))
	{
		// dump the text to m_hCodeEditor
		MString str = GetLanguageStatement(entry.m_lang);
		str += ver_res.Dump(entry.m_name);
		SetWindowTextW(m_hCodeEditor, str.c_str());
		SetShowMode(SHOW_CODEONLY);
		return TRUE;
	}
	return FALSE;
}

// preview the unknown resource
BOOL MMainWnd::PreviewUnknown(HWND hwnd, const EntryBase& entry)
{
	// dump the text to m_hCodeEditor
	ResToText res2text;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());
	SetShowMode(SHOW_CODEONLY);
	return TRUE;
}

BOOL MMainWnd::PreviewTypeLib(HWND hwnd, const EntryBase& entry)
{
	// dump the text to m_hCodeEditor
	ResToText res2text;
	res2text.m_hwnd = m_hwnd;
	res2text.m_bHumanReadable = TRUE;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());
	SetShowMode(SHOW_CODEONLY);
	return TRUE;
}

// preview the RT_RCDATA resource
BOOL MMainWnd::PreviewRCData(HWND hwnd, const EntryBase& entry)
{
	// dump the text to m_hCodeEditor
	ResToText res2text;
	res2text.m_hwnd = m_hwnd;
	res2text.m_bHumanReadable = TRUE;
	MString str = res2text.DumpEntry(entry);
	SetWindowTextW(m_hCodeEditor, str.c_str());
	SetShowMode(SHOW_CODEONLY);
	return TRUE;
}

// preview the DLGINIT resource
BOOL MMainWnd::PreviewDlgInit(HWND hwnd, const EntryBase& entry)
{
	// dump the text to m_hCodeEditor
	ResToText res2text;
	MString str = res2text.DumpEntry(entry);
	if (str.empty())
		return FALSE;
	SetWindowTextW(m_hCodeEditor, str.c_str());
	SetShowMode(SHOW_CODEONLY);
	return TRUE;
}

// preview the dialog template resource
BOOL MMainWnd::PreviewDialog(HWND hwnd, const EntryBase& entry)
{
	// entry.m_data --> stream --> dialog_res
	DialogRes dialog_res;
	MByteStreamEx stream(entry.m_data);
	if (dialog_res.LoadFromStream(stream))
	{
		// dump the text to m_hCodeEditor
		MString str = GetLanguageStatement(entry.m_lang);
		str += dialog_res.Dump(entry.m_name, !!g_settings.bAlwaysControl);
		SetWindowTextW(m_hCodeEditor, str.c_str());
		SetShowMode(SHOW_CODEONLY);
		return TRUE;
	}
	return FALSE;
}

// preview the animation icon resource
BOOL MMainWnd::PreviewAniIcon(HWND hwnd, const EntryBase& entry, BOOL bIcon)
{
	HICON hIcon = NULL;

	{
		WCHAR szPath[MAX_PATH], szTempFile[MAX_PATH];
		GetTempPathW(_countof(szPath), szPath);
		GetTempFileNameW(szPath, L"ani", 0, szTempFile);

		MFile file;
		DWORD cbWritten = 0;
		if (file.OpenFileForOutput(szTempFile) &&
			file.WriteFile(&entry[0], entry.size(), &cbWritten))
		{
			file.FlushFileBuffers();	// flush
			file.CloseHandle();		 // close the handle

			if (bIcon)
			{
				hIcon = (HICON)LoadImage(NULL, szTempFile, IMAGE_ICON,
					0, 0, LR_LOADFROMFILE);
			}
			else
			{
				hIcon = (HICON)LoadImage(NULL, szTempFile, IMAGE_CURSOR,
					0, 0, LR_LOADFROMFILE);
			}
		}
		DeleteFileW(szTempFile);
	}

	if (hIcon)
	{
		m_hBmpView.SetIcon(hIcon, bIcon);

		ResToText res2text;
		MString str = res2text.DumpEntry(entry);
		SetWindowTextW(m_hCodeEditor, str.c_str());
	}
	else
	{
		m_hBmpView.DestroyView();
		return FALSE;
	}

	// show
	SetShowMode(SHOW_CODEANDBMP);
	return TRUE;
}

// preview the string table resource
BOOL MMainWnd::PreviewStringTable(HWND hwnd, const EntryBase& entry)
{
	// search the strings
	EntrySet found;
	g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, entry.m_lang);
	if (found.empty())
		return FALSE;

	// found --> str_res
	StringRes str_res;
	for (auto e : found)
	{
		MByteStreamEx stream(e->m_data);
		if (!str_res.LoadFromStream(stream, e->m_name.m_id))
			return FALSE;
	}

	// dump the text to m_hCodeEditor
	MString str = GetLanguageStatement(entry.m_lang);
	str += str_res.Dump();
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// show code only
	SetShowMode(SHOW_CODEONLY);
	return TRUE;
}

// Begin/EndPreviewBatch --- see the comment next to m_nPreviewBatchDepth in
// MMainWnd.hpp. These bracket a whole Preview() call so that the several
// SetWindowText/ShowWindow/WM_SIZE steps it triggers internally collapse
// into a single repaint/relayout of m_hCodeEditor instead of several,
// visible ones.
//
// In addition to suppressing redraw on m_hCodeEditor itself, we also
// freeze m_splitter2 (the parent that owns the code / bitmap / hex panes).
// Without that, SetShowMode()'s ShowWindow/SetPane calls still produce
// intermediate paints of the splitter client area (empty background or
// partially-moved panes) even while the editor's own WM_SETREDRAW is
// FALSE -- which is the residual flicker that remained after the first
// batching pass. Freezing the splitter makes those visibility/layout
// mutations invisible until EndPreviewBatch restores redraw and issues
// one coherent invalidate (plus a single WM_SIZE when the layout really
// changed).
void MMainWnd::BeginPreviewBatch()
{
	if (m_nPreviewBatchDepth++ == 0)
	{
		::SendMessageW(m_hCodeEditor, WM_SETREDRAW, FALSE, 0);
		if (::IsWindow(m_splitter2))
			m_splitter2.SendMessageDx(WM_SETREDRAW, FALSE, 0);
	}
}

void MMainWnd::EndPreviewBatch()
{
	assert(m_nPreviewBatchDepth > 0);
	if (--m_nPreviewBatchDepth == 0)
	{
		::SendMessageW(m_hCodeEditor, WM_SETREDRAW, TRUE, 0);
		if (::IsWindow(m_splitter2))
			m_splitter2.SendMessageDx(WM_SETREDRAW, TRUE, 0);

		// Redraw the editor *and* its line-number child in one shot.
		// RDW_NOERASE avoids a blank background flash; RDW_ALLCHILDREN
		// reaches the LineNumStatic that lives inside LineNumEdit.
		// (InvalidateRect alone does not reliably dirty the static.)
		::RedrawWindow(m_hCodeEditor, NULL, NULL,
		               RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE);

		// Only relayout if SetShowMode() actually changed m_nShowMode/
		// m_bShowBinEdit somewhere inside this batch. Switching between
		// tree items of the same kind (the common, high-frequency case)
		// never touches the layout, so skipping this avoids a pointless
		// MoveWindow(..., TRUE) on m_splitter1/m_splitter2 -- and the
		// full repaint that comes with it -- on every single selection
		// change. This is what eliminates the flicker.
		if (m_bPreviewLayoutDirty)
		{
			m_bPreviewLayoutDirty = FALSE;
			PostMessageDx(WM_SIZE);
		}
		else if (::IsWindow(m_splitter2))
		{
			// No layout change, but the splitter was frozen; give it a
			// single clean paint so any ShowWindow that happened under
			// the freeze becomes visible together with the new text.
			::RedrawWindow(m_splitter2, NULL, NULL,
			               RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE);
		}
	}
}

// close the preview
VOID MMainWnd::HidePreview(STV stv, BOOL bWillRePreview/* = FALSE*/,
                           BOOL bDestroyRad/* = TRUE*/)
{
	if (bDestroyRad)
	{
		// destroy the RADical window if any
		DestroyRadWindow();
	}

	// clear m_hHexViewer
	SetWindowTextW(m_hHexViewer, NULL);
	Edit_SetModify(m_hHexViewer, FALSE);

	// clear m_hCodeEditor
	if (stv == STV_RESETTEXT || stv == STV_RESETTEXTANDMODIFIED)
	{
		SetWindowTextW(m_hCodeEditor, NULL);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		Edit_SetReadOnly(m_hCodeEditor, TRUE);
	}
	if (stv != STV_DONTRESET)
	{
		Edit_SetModify(m_hCodeEditor, FALSE);
	}

	// close and hide m_hBmpView
	m_hBmpView.DestroyView();

	// Code Viewer only -- but only as the resting state. If the caller is
	// about to (re)establish the real target mode itself right after this
	// (bWillRePreview), skip this: forcing CODEONLY here and then switching
	// again a moment later is exactly the redundant transition that used to
	// show up as m_hCodeEditor/m_hBmpView flicker.
	if (!bWillRePreview)
		SetShowMode(SHOW_CODEONLY);

	// recalculate the splitter (skipped while Preview() is batching; it will
	// post a single WM_SIZE itself once the whole batch is done, via
	// EndPreviewBatch, instead of one per internal step)
	if (!m_nPreviewBatchDepth)
		PostMessageDx(WM_SIZE);
}

// do preview the resource item
BOOL MMainWnd::Preview(HWND hwnd, const EntryBase *entry, STV stv, BOOL bDestroyRad)
{
	BeginPreviewBatch();

	BOOL bWillRePreview = (stv != STV_DONTRESET);

	if (stv == STV_DONTRESET)
	{
		// Only the non-repreview path needs a full HidePreview (resting
		// CODEONLY mode). The common path below does the minimal cleanup
		// inline so we avoid the extra HidePreview -> SetShowMode/
		// PostMessageDx hop that used to contribute to m_hCodeEditor
		// flicker even under the batch.
		HidePreview(stv, /*bWillRePreview=*/FALSE, bDestroyRad);
		EndPreviewBatch();
		return IsEntryTextEditable(entry);
	}

	// Minimal cleanup for the re-preview case (replaces HidePreview with
	// bWillRePreview=TRUE). Skips SetShowMode / PostMessageDx entirely;
	// each PreviewXxx() sets the real target mode, and EndPreviewBatch()
	// posts a single WM_SIZE only if the layout actually changed.
	if (bDestroyRad)
		DestroyRadWindow();

	SetWindowTextW(m_hHexViewer, NULL);
	Edit_SetModify(m_hHexViewer, FALSE);
	ClearHexCache();

	if (stv == STV_RESETTEXT || stv == STV_RESETTEXTANDMODIFIED)
	{
		SetWindowTextW(m_hCodeEditor, NULL);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
	}
	Edit_SetModify(m_hCodeEditor, FALSE);

	m_hBmpView.DestroyView();

	// NOTE: no SetShowMode(SHOW_CODEONLY) here anymore -- each PreviewXxx()
	// call below now sets the mode it actually needs directly (including
	// SHOW_CODEONLY for the text-only ones), so there is at most one real
	// mode transition per selection change instead of two.

	// do preview the resource item
	BOOL bEditable = TRUE;
	if (entry->m_type.m_id != 0)
	{
		WORD wType = entry->m_type.m_id;
		if (wType == (WORD)(UINT_PTR)RT_ICON)
		{
			bEditable = PreviewIcon(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_CURSOR)
		{
			bEditable = PreviewCursor(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_GROUP_ICON)
		{
			bEditable = PreviewGroupIcon(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_GROUP_CURSOR)
		{
			bEditable = PreviewGroupCursor(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_BITMAP)
		{
			bEditable = PreviewBitmap(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_ACCELERATOR)
		{
			bEditable = PreviewAccel(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_STRING)
		{
			bEditable = PreviewString(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_MENU)
		{
			bEditable = PreviewMenu(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_TOOLBAR)
		{
			bEditable = PreviewToolbar(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_DIALOG)
		{
			bEditable = PreviewDialog(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_ANIICON)
		{
			bEditable = PreviewAniIcon(hwnd, *entry, TRUE);
		}
		else if (wType == (WORD)(UINT_PTR)RT_ANICURSOR)
		{
			bEditable = PreviewAniIcon(hwnd, *entry, FALSE);
		}
		else if (wType == (WORD)(UINT_PTR)RT_MESSAGETABLE)
		{
			bEditable = PreviewMessage(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_MANIFEST || wType == (WORD)(UINT_PTR)RT_HTML)
		{
			bEditable = PreviewHtml(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_VERSION)
		{
			bEditable = PreviewVersion(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_RCDATA)
		{
			bEditable = PreviewRCData(hwnd, *entry);
		}
		else if (wType == (WORD)(UINT_PTR)RT_DLGINIT)
		{
			bEditable = PreviewDlgInit(hwnd, *entry);
		}
		else
		{
			bEditable = PreviewUnknown(hwnd, *entry);
		}
	}
	else
	{
		if (entry->m_type == L"PNG" || entry->m_type == L"GIF" ||
			entry->m_type == L"JPEG" || entry->m_type == L"TIFF" ||
			entry->m_type == L"JPG" || entry->m_type == L"TIF" ||
			entry->m_type == L"EMF" || entry->m_type == L"ENHMETAFILE" ||
			entry->m_type == L"ENHMETAPICT" ||
			entry->m_type == L"WMF" || entry->m_type == L"IMAGE")
		{
			bEditable = PreviewImage(hwnd, *entry);
		}
		else if (entry->m_type == L"WAVE")
		{
			bEditable = PreviewWAVE(hwnd, *entry);
		}
		else if (entry->m_type == L"MP3")
		{
			bEditable = PreviewMP3(hwnd, *entry);
		}
		else if (entry->m_type == L"AVI")
		{
			bEditable = PreviewAVI(hwnd, *entry);
		}
		else if (entry->m_type == L"TYPELIB")
		{
			bEditable = PreviewTypeLib(hwnd, *entry);
		}
		else
		{
			bEditable = PreviewUnknown(hwnd, *entry);
		}
	}

	// Fallback: if the specific PreviewXxx() above failed before it got to
	// its own SetShowMode() call (e.g. LoadFromStream() failed on a
	// corrupt resource), make sure we don't leave a stale mode (e.g. still
	// showing m_hBmpView from the previous, unrelated selection) behind.
	if (!bEditable)
		SetShowMode(SHOW_CODEONLY);

	// re-enable redraw on m_hCodeEditor and recalculate the splitter
	EndPreviewBatch();

	return bEditable && IsEntryTextEditable(entry);
}
