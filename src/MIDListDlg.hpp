// MIDListDlg.hpp --- "ID List" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include <algorithm> // std::find, std::sort, std::unique
#include <unordered_map>

#include "resource.h"
#include "MWindowBase.hpp"
#include "MTextToText.hpp"
#include "MIdOrString.hpp"
#include "MAddResIDDlg.hpp"
#include "MModifyResIDDlg.hpp"
#include "MResizable.hpp"
#include "Common.hpp"

class MSubclassedListView;
class MIDListDlg;
void Res_ReplaceResTypeString(MString& str, bool bRevert);

#define MYWM_IDJUMPBANG (WM_USER + 238)
#define MYWM_REFRESHIDLIST (WM_USER + 239)

//////////////////////////////////////////////////////////////////////////////

// Let the listview subclassed to get Enter key
class MSubclassedListView : public MWindowBase
{
public:
	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};

struct RowKey {
	MString name;
	MString value;
	bool operator==(const RowKey& other) const;
};

namespace std {
	template<> struct hash<RowKey> {
		size_t operator()(const RowKey& k) const;
	};
}

class MIDListDlg : public MDialogBase
{
public:
	struct ItemRow
	{
		MString col0, col1, col2;
		INT value;
		bool numeric;
		bool has_real_type;
	};
	std::vector<ItemRow> m_items; // Sorted
	std::unordered_map<RowKey, INT> m_rowIndexMap;

	HWND m_hMainWnd;
	LPWSTR m_pszResH;
	INT m_nBase;
	HWND m_hCmb1;
	HWND m_hLst1;
	BOOL m_bChanging;
	BOOL m_bRefreshing;
	HICON m_hIconDiamond;
	MSubclassedListView m_lv;
	MResizable m_resizable;
	std::unordered_map<INT, MStringW> m_map1; // IDTYPE_ --> MStringW
	std::unordered_map<MStringW, INT> m_map2; // MStringW --> IDTYPE_

	MIDListDlg();
	~MIDListDlg();

	void InitMaps();
	MString FormatByBase(INT value) const;

	void NormalizeValueText(MString& text, INT& value, bool& numeric) const;

	bool IsTypeMatched(const MString& rowTypes, LPCTSTR pszIDType) const;

	void MergeType(ItemRow& row, const MString& type, bool bRealType);

	void AddOrMergeRow(const MString& text1, const MString& type, const MString& text3, bool bRealType);

	MString GetTypeTextFromEntry(const EntryBase *entry) const;

	std::vector<MString> GetPrefixTypeTexts(const MString& name) const;

	void AddResourceRow(const EntryBase *entry);

	void AddMacroRow(const std::pair<MStringA, MStringA>& pair);

	void UpdateComboBox();
	void UpdateItems();
	void UpdateListView(LPCTSTR pszIDType);
	void BuildItemsIntoCache();
	void UpdateResHIfAsk();

	void RebuildNumericColumn();

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

protected:
	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);
	void OnMove(HWND hwnd, int x, int y);
	void OnSize(HWND hwnd, UINT state, int cx, int cy);
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
	void OnCmb1(HWND hwnd);
	void OnRefreshList(HWND hwnd);
	void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem);
	void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem);
	int OnCompareItem(HWND hwnd, const COMPAREITEMSTRUCT * lpCompareItem);
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void OnIdJump(HWND hwnd, INT nIndex = -1);
};
