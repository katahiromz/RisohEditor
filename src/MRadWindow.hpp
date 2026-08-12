// MRadWindow.hpp --- RADical development window
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "MRubberBand.hpp"
#include "MOleHost.hpp"
#include "Res.hpp"
#include "DialogRes.hpp"
#include <map>
#include <unordered_set>     // for std::unordered_set
#include <vector>            // for std::vector

class MRadCtrl;
class MRadDialog;
class MRadWindow;

//////////////////////////////////////////////////////////////////////////////
// constants

// user-defined window messages
#define MYWM_CTRLMOVE           (WM_USER + 100)     // control was moved
#define MYWM_CTRLSIZE           (WM_USER + 101)     // control was resized
#ifndef MYWM_SELCHANGE
	#define MYWM_SELCHANGE      (WM_USER + 102)     // selection was changed
#endif
#define MYWM_DLGSIZE            (WM_USER + 103)     // dialog was resized
#define MYWM_DELETESEL          (WM_USER + 104)     // selection was deleted
#define MYWM_MOVESIZEREPORT     (WM_USER + 105)     // report moving/resizing
#define MYWM_CLEARSTATUS        (WM_USER + 106)     // clear status
#define MYWM_COMPILECHECK       (WM_USER + 107)     // compilation check
#define MYWM_REOPENRAD          (WM_USER + 108)     // reopen the RADical window
#define MYWM_GETUNITS           (WM_USER + 109)     // get the dialog base units
#define MYWM_UPDATEDLGRES       (WM_USER + 110)     // update the dialog res
#define MYWM_REDRAW             (WM_USER + 111)     // redraw MRadDialog
#define MYWM_RADDBLCLICK       (WM_USER + 115)     // Double-clicked on control

#define GRID_SIZE   5   // grid size

//////////////////////////////////////////////////////////////////////////////
// MRadCtrl --- the RADical controls
// NOTE: An MRadCtrl is a dialog control, subclassed by MRadDialog.

class MRadCtrl : public MWindowBase
{
public:
	DWORD           m_dwMagic;          // magic number to verify the instance
	BOOL            m_bTopCtrl;         // is it a top control?
	MRubberBand     m_hwndRubberBand;   // the rubber band window
	BOOL            m_bMoving;          // is it moving?
	BOOL            m_bSizing;          // is it resizing?
	BOOL            m_bLocking;         // is it locked?
	INT             m_nIndex;           // the control index
	POINT           m_pt;               // the position
	INT             m_nImageType;       // the image type

	// constructor
	MRadCtrl();

	// the default icon
	static HICON& Icon();
	// the default bitmap
	static HBITMAP& Bitmap();
	// is the window a group box?
	static BOOL IsGroupBox(HWND hCtrl);

	// call me after subclassing
	void PostSubclass();

	// the targets (the selected window handles)
	typedef std::unordered_set<HWND> set_type;
	static set_type& GetTargets();

	// the last selection (the selected window handle)
	static HWND& GetLastSel();

	// get the target control indexes
	static std::unordered_set<INT> GetTargetIndeces();

	// the index-to-control mapping
	typedef std::map<INT, HWND> map_type;
	static map_type& IndexToCtrlMap();

	// get the rubber band that is associated to the MRadCtrl
	MRubberBand *GetRubberBand();

	// get the MRadCtrl from a window handle
	static MRadCtrl *GetRadCtrl(HWND hwnd);

	// is the user dragging on the dialog face
	static BOOL& GetRangeSelect(void);

	// Keep every group box pinned at the very back of the Z order. Group
	// boxes in this app paint a solid opaque fill over their whole client
	// rect (see OnEraseBkgnd below), so if an ordinary control ever ends up
	// Z-order-behind one, that control gets completely hidden -- calling
	// this after any Z-order change that sends something to HWND_BOTTOM
	// guarantees a group box can never be "passed" by a regular control.
	//
	// If a group box happens to be selected (has its own rubber band) when
	// this runs, its band is kept directly above it -- just enough for the
	// selection outline to still be visible over the group box's own
	// opaque fill -- but still below every ordinary control. Selecting a
	// group box must never visually cover anything else, the same as the
	// group box's own body never does.
	static void PinGroupBoxesToBack(HWND hwndParent);

	// Undo the Z-order elevation that Select()/OnNCLButtonDown() gave a
	// control while it was selected. Without this, a control that was once
	// selected (then deselected, e.g. by Ctrl-click, or by clicking
	// elsewhere) stays parked at the very top of the Z order forever, and
	// can end up covering another control's rubber band later even though
	// it isn't selected itself anymore -- that's the "sometimes the rubber
	// band gets partially hidden" bug.
	static void UnelevateZOrder(HWND hwnd);

	// deselect the selection
	static BOOL DeselectSelection();

	static LRESULT DoSendMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// delete the selection
	static void DeleteSelection();

	// deselect this control
	void Deselect();

	// is it selected?
	BOOL IsSelected() const;

	// select the control
	static void Select(HWND hwnd);

	static void SelectByIndex(INT nIndex);

	// move the selected RADical controls
	static void MoveSelection(HWND hwnd, INT dx, INT dy);

	// resize the selected RADical controls
	static void ResizeSelection(HWND hwnd, INT cx, INT cy);

	// range selection
	struct RANGE_SELECT
	{
		RECT rc;
		BOOL bCtrlDown;
	};

	// callback function for DoRangeSelect
	static BOOL CALLBACK RangeSelectProc(HWND hwnd, LPARAM lParam);

	// do range selection
	static void DoRangeSelect(HWND hwndParent, const RECT *prc, BOOL bCtrlDown);

	// the window procedure of MRadCtrl
	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// MRadCtrl MYWM_REDRAW
	LRESULT OnRedraw(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadCtrl WM_ERASEBKGND
	BOOL OnEraseBkgnd(HWND hwnd, HDC hdc);

	// MRadCtrl WM_NCRBUTTONDOWN/WM_NCRBUTTONDBLCLK
	void OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest);

	// MRadCtrl WM_NCRBUTTONUP
	void OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest);

	// MRadCtrl WM_LBUTTONDOWN/WM_LBUTTONDBLCLK
	void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);

	// MRadCtrl WM_LBUTTONUP
	void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);

	// MRadCtrl WM_MOUSEMOVE
	void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);

	// MRadCtrl WM_MOVE
	void OnMove(HWND hwnd, int x, int y);

	// MRadCtrl WM_SIZE
	void OnSize(HWND hwnd, UINT state, int cx, int cy);

	// MRadCtrl WM_KEYDOWN/WM_KEYUP
	void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);

	// MRadCtrl WM_NCLBUTTONDOWN/WM_NCLBUTTONDBLCLK
	void OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest);

	// MRadCtrl WM_NCMOUSEMOVE
	void OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest);

	// MRadCtrl WM_NCLBUTTONUP
	void OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest);

	struct MYHITTEST
	{
		HWND hParent;
		HWND hCandidate;
		HWND hLast;
		HWND hGroupBox;
		POINT pt;
		std::vector<HWND> matches;  // filled in front-to-back (Z-order) order
	};

	// the helper function for hittest: just collect every overlapping
	// descendant that could matter, in Z-order (EnumChildWindows visits
	// front-to-back, i.e. topmost/frontmost window first).
	static BOOL CALLBACK EnumHitTestChildProc(HWND hwnd, LPARAM lParam);

	// resolve the collected matches into hCandidate/hLast/hGroupBox, giving
	// priority to the frontmost (topmost in Z-order) one. Selected controls
	// are kept at the front of the Z order (see Select() and
	// OnNCLButtonDown()), so the frontmost match is the one the user
	// actually sees on top and expects clicks to go to.
	static void ResolveHitTestMatches(MYHITTEST *pmht);

	// MRadCtrl WM_NCHITTEST
	UINT OnNCHitTest(HWND hwnd, int x, int y);

	void DoTest();
};

//////////////////////////////////////////////////////////////////////////////
// MRadDialog --- RADical dialog

class MRadDialog : public MDialogBase
{
public:
	BOOL            m_index_visible;        // indeces are visible
	POINT           m_ptClicked;            // the clicked position
	POINT           m_ptDragging;           // the dragging position
	HFONT           m_hFontLabel;           // the font used to draw the index labels
	HBRUSH          m_hLabelBrush;
	BOOL            m_bMovingSizing;        // the lock of moving and/or resizing
	INT             m_xDialogBaseUnit;      // the X dialog base unit
	INT             m_yDialogBaseUnit;      // the Y dialog base unit
	HBRUSH          m_hbrBack;              // the background brush
	std::vector<std::shared_ptr<MRadCtrl>> m_rad_ctrls;

	MRadDialog();
	~MRadDialog();

	// the target types to get
	enum TARGET_TYPE
	{
		TARGET_NEXT,        // get the next target
		TARGET_PREV,        // get the previous target
		TARGET_FIRST,       // get the first target
		TARGET_LAST         // get the last target
	};

	// the structure to get the target control
	struct GET_TARGET
	{
		TARGET_TYPE target;
		HWND hwndTarget;
		INT m_nIndex;
		INT m_nCurrentIndex;
	};

	// EnumChildWindows' callback function to get the target
	static BOOL CALLBACK GetTargetProc(HWND hwnd, LPARAM lParam);

	HWND GetNextCtrl(HWND hwndCtrl) const;
	HWND GetPrevCtrl(HWND hwndCtrl) const;
	static HWND GetFirstCtrl(HWND hwndParent);
	static HWND GetLastCtrl(HWND hwndParent);
	void DeleteBackBrush();

	// the dialog procedure of MRadDialog
	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	LRESULT DoSendMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// MRadDialog WM_RBUTTONDOWN
	void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);

	// MRadDialog MYWM_REDRAW
	LRESULT OnRedraw(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadDialog MYWM_SELCHANGE
	LRESULT OnSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// get the normalized rectangle from two points
	void NormalizeRect(RECT *prc, POINT pt0, POINT pt1);

	// draw the dragging rectangle
	void DrawDragSelect(HWND hwnd);

	// MRadDialog WM_LBUTTONDOWN/WM_LBUTTONDBLCLK
	void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);

	// MRadDialog WM_MOUSEMOVE
	void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);

	// MRadDialog WM_CAPTURECHANGED
	void OnCaptureChanged(HWND hwnd);

	// MRadDialog WM_LBUTTONUP
	void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);

	// MRadDialog WM_ERASEBKGND
	BOOL OnEraseBkgnd(HWND hwnd, HDC hdc);

	// the window procedure of MRadDialog
	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// MRadDialog MYWM_RADDBLCLICK
	LRESULT OnRadDblClick(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadDialog WM_SIZE
	void OnSize(HWND hwnd, UINT state, int cx, int cy);

	// MRadDialog MYWM_DELETESEL
	LRESULT OnDeleteSel(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadDialog MYWM_CTRLMOVE
	LRESULT OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadDialog MYWM_CTRLSIZE
	LRESULT OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadDialog WM_NCLBUTTONDOWN/WM_NCLBUTTONDBLCLK
	void OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest);

	// MRadDialog WM_NCLBUTTONUP
	void OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest);

	// MRadDialog WM_NCRBUTTONDOWN/WM_NCRBUTTONDBLCLK
	void OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest);

	// MRadDialog WM_NCRBUTTONUP
	void OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest);

	// MRadDialog WM_NCMOUSEMOVE
	void OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest);

	// MRadDialog WM_KEYDOWN/WM_KEYUP
	void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);

	// NOTE: We have to do subclassing all the children controls and their descendants
	//       to modify the hittesting.

	// do subclassing a control and its descendants
	void DoSubclass(HWND hCtrl, INT nIndex);

	// do subclassing the children
	void DoSubclassChildren(HWND hwnd, BOOL bTop = FALSE);

	// create the background brush
	BOOL ReCreateBackBrush();

	// MRadDialog WM_SYSCOLORCHANGE
	void OnSysColorChange(HWND hwnd);

	// MRadDialog WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

	// show/hide the index labels
	void ShowIndexLabels(BOOL bShow = TRUE);

	// Stably "overwrite" the index-number overlay on top of hwndDialog
	// (an MRadDialog window). There is no separate window for the
	// numbers, no hit-testing, and no Z-order to maintain -- this just
	// forces hwndDialog and all of its children to finish painting
	// themselves, then (if the owning MRadDialog's m_index_visible is
	// TRUE) draws the numbers onto hwndDialog's own client DC, obtained
	// via GetDCEx(hwndDialog, NULL, DCX_CACHE) -- deliberately without
	// DCX_USESTYLE (what plain GetDC() passes under the hood) or
	// DCX_CLIPCHILDREN, so the WS_CLIPCHILDREN style (see OnInitDialog)
	// doesn't exclude the area under every child control. Prefer
	// posting MYWM_REDRAW to the dialog over calling this directly from
	// hot paths (WM_MOVE/WM_SIZE/UpdateRes): each call can RDW_ERASE the
	// whole dialog, so stacking synchronous calls was the main source of
	// index-label flicker while dragging or multi-selecting.
	static void PaintIndexLabels(HWND hwndDialog);
};

//////////////////////////////////////////////////////////////////////////////
// MRadWindow --- the RADical window
// NOTE: An MRadWindow contains an MRadDialog.

class MRadWindow : public MWindowBase
{
public:
	INT             m_xDialogBaseUnit;      // the X dialog base unit
	INT             m_yDialogBaseUnit;      // the Y dialog base unit
	MRadDialog      m_rad_dialog;           // the MRadDialog instance
	DialogRes       m_dialog_res;           // the dialog resource
	HICON           m_hIcon;                // the icon
	HICON           m_hIconSm;              // the small icon
	MTitleToBitmap  m_title_to_bitmap;      // a title-to-bitmap mapping
	MTitleToIcon    m_title_to_icon;        // a title-to-icon mapping
	DialogItemClipboard m_clipboard;        // a clipboard manager
	std::shared_ptr<MOleHost> m_pOleHost;

	MRadWindow();
	~MRadWindow();

	// create the mappings
	void create_maps(LANGID lang);

	// clear the mappings
	void clear_maps();

	VOID DestroyWindow();

	// create an MRadWindow window
	BOOL CreateDx(HWND hwndParent);

	// convert the coordinates
	void ClientToDialog(POINT *ppt);
	void ClientToDialog(SIZE *psiz);
	void ClientToDialog(RECT *prc);
	void DialogToClient(POINT *ppt);
	void DialogToClient(SIZE *psiz);
	void DialogToClient(RECT *prc);

	static HWND GetPrimaryControl(HWND hwnd, HWND hwndDialog);

	// adjust MRadWindow's position and size to MRadDialog's client area
	void FitToRadDialog();

	// the window class name
	LPCTSTR GetWndClassNameDx() const override;

	void ModifyWndClassDx(WNDCLASSEX& wcx) override;

	LRESULT DoSendMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// recreate the MRadDialog window
	BOOL ReCreateRadDialog(HWND hwnd, INT nSelectStartIndex = -1);

	// update the mappings
	void update_maps();

	// MRadWindow WM_CREATE
	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

	// MRadWindow WM_DESTROY
	void OnDestroy(HWND hwnd);

	// the window procedure of MRadWindow
	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// MRadWindow WM_SYSCOLORCHANGE
	void OnSysColorChange(HWND hwnd);

	// MRadWindow MYWM_RADDBLCLICK
	LRESULT OnRadDblClick(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadWindow MYWM_GETUNITS
	LRESULT OnGetUnits(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadWindow WM_ACTIVATE
	void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);

	// MRadWindow MYWM_SELCHANGE
	LRESULT OnSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadWindow WM_INITMENUPOPUP
	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);

	// MRadWindow MYWM_DELETESEL
	LRESULT OnDeleteSel(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadWindow MYWM_CTRLMOVE
	LRESULT OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// MRadWindow MYWM_CTRLSIZE
	LRESULT OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// update the resource
	void UpdateRes();

	// MRadWindow MYWM_DLGSIZE
	LRESULT OnDlgSize(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// called from MMainWnd WM_COMMAND ID_ADDCTRL
	void OnAddCtrl(HWND hwnd);

	// called from MMainWnd WM_COMMAND ID_CTRLPROP
	void OnCtrlProp(HWND hwnd);

	// called from MMainWnd WM_COMMAND ID_DLGPROP
	void OnDlgProp(HWND hwnd);

	// refresh
	void OnRefresh(HWND hwnd);

	// show/hide the indeces
	void OnShowIndex(HWND hwnd);

	// get the selected dialog items
	BOOL GetSelectedItems(DialogItems& items);

	// MRadWindow WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

	// align selected controls to a common edge (needs >= 2 targets)
	enum AlignEdge { ALIGN_TOP, ALIGN_BOTTOM, ALIGN_LEFT, ALIGN_RIGHT };

	void AlignSelection(AlignEdge edge);
	void OnTopAlign(HWND hwnd);
	void OnBottomAlign(HWND hwnd);
	void OnLeftAlign(HWND hwnd);
	void OnRightAlign(HWND hwnd);

	// able to make it top index? (true if any selected item follows an unselected one)
	BOOL CanIndexTop() const;

	// partition items into (unselected, selected) preserving relative order
	void PartitionBySelection(DialogItems& unselected, DialogItems& selected) const;

	// make it top index
	void IndexTop(HWND hwnd);

	// able to make it bottom index? (true if any selected item precedes an unselected one)
	BOOL CanIndexBottom() const;

	// make it bottom index
	void IndexBottom(HWND hwnd);

	// able to decrement the control index?
	BOOL CanIndexMinus() const;

	// decrement the control index
	void IndexMinus(HWND hwnd);

	// able to increment the control index?
	BOOL CanIndexPlus() const;

	// increment the control index
	void IndexPlus(HWND hwnd);

	// MRadWindow WM_KEYDOWN/WM_KEYUP
	void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);

	// select all the RADical controls
	void SelectAll(HWND hwnd);

	// MRadWindow WM_CONTEXTMENU
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);

	// get the dialog base units
	BOOL GetBaseUnits(INT& xDialogBaseUnit, INT& yDialogBaseUnit);

	// MRadWindow WM_MOVE
	void OnMove(HWND hwnd, int x, int y);

	// MRadWindow WM_SIZE
	void OnSize(HWND hwnd, UINT state, int cx, int cy);

	// fit the coordinates to the grid
	void FitToGrid(POINT *ppt);
	void FitToGrid(SIZE *psiz);
	void FitToGrid(RECT *prc);

	void OnFitToGrid(HWND hwnd);
};
