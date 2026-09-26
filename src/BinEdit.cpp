// BinEdit.cpp --- Binary edit control Implementation
// Author: katahiromz
// License: MIT

#include "BinEdit.h"
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <cwctype>
#include <cwchar>
#include <strsafe.h>
#include "resource.h"

namespace {

constexpr WCHAR HEX_DIGITS[] = L"0123456789ABCDEF";
constexpr UINT CP_SJIS = 932; // Shift_JIS

constexpr UINT_PTR kDragAutoScrollTimerId = 1;
constexpr UINT kDragAutoScrollIntervalMs = 50;

int HexValue(WCHAR ch)
{
    if (ch >= L'0' && ch <= L'9') return ch - L'0';
    if (ch >= L'a' && ch <= L'f') return ch - L'a' + 10;
    if (ch >= L'A' && ch <= L'F') return ch - L'A' + 10;
    return -1;
}

size_t SelectionCaretFromHit(size_t hitOff, size_t anchor, size_t dataSize)
{
    if (hitOff >= dataSize)
        return dataSize;
    if (hitOff >= anchor)
        return hitOff + 1;
    return hitOff;
}

bool HexColToByte(int col, int& byteIndex, int& withinCell)
{
    if (col <= 0)
    {
        byteIndex = withinCell = 0;
        return true;
    }
    if (col < BYTES_PER_LINE * 3 / 2)
    {
        byteIndex  = col / 3;
        withinCell = col % 3;
    }
    else
    {
        int adj = col - 1;
        byteIndex  = adj / 3;
        withinCell = adj % 3;
    }
    return 0 <= byteIndex && byteIndex < BYTES_PER_LINE;
}

UINT GetWindowDpi(HWND hwnd)
{
    using GetDpiForWindow_t = UINT (WINAPI *)(HWND);
    static GetDpiForWindow_t s_pGetDpiForWindow = nullptr;
    if (!s_pGetDpiForWindow)
    {
        FARPROC fn = GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
        memcpy(&s_pGetDpiForWindow, &fn, sizeof(fn));
    }

    if (s_pGetDpiForWindow)
    {
        if (UINT dpi = s_pGetDpiForWindow(hwnd))
            return dpi;
    }

    UINT dpi = 96;
    if (HDC hdc = GetDC(hwnd))
    {
        dpi = static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSY));
        ReleaseDC(hwnd, hdc);
    }
    return dpi ? dpi : 96;
}

struct GotoOffsetDlgParams
{
    size_t current;
    size_t maxOffset;
    size_t result;
    bool  accepted;
};

INT_PTR CALLBACK GotoOffsetDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        SetWindowLongPtrW(hDlg, DWLP_USER, static_cast<LONG_PTR>(lParam));
        auto* p = reinterpret_cast<GotoOffsetDlgParams*>(lParam);

        CheckRadioButton(hDlg, rad1, rad2, rad1);

        WCHAR buf[32];
        StringCchPrintfW(buf, _countof(buf), L"%llX", static_cast<unsigned long long>(p->current));
        SetDlgItemTextW(hDlg, edt1, buf);

        WCHAR info[128];
        StringCchPrintfW(info, _countof(info), L"範囲: 0 - %llX (%llu バイト)",
                         static_cast<unsigned long long>(p->maxOffset),
                         static_cast<unsigned long long>(p->maxOffset));
        SetDlgItemTextW(hDlg, stc2, info);

        SendDlgItemMessageW(hDlg, edt1, EM_SETSEL, 0, -1);
        SetFocus(GetDlgItem(hDlg, edt1));

        if (HWND owner = GetWindow(hDlg, GW_OWNER))
        {
            RECT rc, rcOwner;
            if (GetWindowRect(hDlg, &rc) && GetWindowRect(owner, &rcOwner))
            {
                int w = rc.right - rc.left, h = rc.bottom - rc.top;
                int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - w) / 2;
                int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - h) / 2;
                SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
        }
        return FALSE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            auto* p = reinterpret_cast<GotoOffsetDlgParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
            WCHAR buf[64] = L"";
            GetDlgItemTextW(hDlg, edt1, buf, 64);

            const bool hex = (IsDlgButtonChecked(hDlg, rad1) == BST_CHECKED);
            WCHAR* endp = nullptr;
            unsigned __int64 val = _wcstoui64(buf, &endp, hex ? 16 : 10);
            if (endp == buf)
            {
                MessageBeep(MB_ICONWARNING);
                SendDlgItemMessageW(hDlg, edt1, EM_SETSEL, 0, -1);
                SetFocus(GetDlgItem(hDlg, edt1));
                return TRUE;
            }
            if (val > static_cast<unsigned __int64>(p->maxOffset))
                val = static_cast<unsigned __int64>(p->maxOffset);
            p->result = static_cast<size_t>(val);
            p->accepted = true;
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        default:
            break;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;

    default:
        break;
    }
    return FALSE;
}

size_t DecodeAnsiOne(const BYTE* data, size_t size, size_t pos, std::wstring& glyph, bool& printable, UINT nCodePage = CP_ACP)
{
    BYTE b0 = data[pos];
    if (b0 == 0)
    {
        glyph = L" ";
        printable = true;
        return 1;
    }

    size_t len = 1;
    if (IsDBCSLeadByteEx(nCodePage, b0) && pos + 1 < size)
        len = 2;

    WCHAR wch = 0;
    int converted = MultiByteToWideChar(nCodePage, MB_ERR_INVALID_CHARS,
                                        reinterpret_cast<const char*>(data + pos), static_cast<int>(len),
                                        &wch, 1);
    if (converted != 1)
    {
        if (len == 2)
        {
            len = 1;
            converted = MultiByteToWideChar(nCodePage, MB_ERR_INVALID_CHARS,
                                            reinterpret_cast<const char*>(data + pos), 1, &wch, 1);
        }
        if (converted != 1)
        {
            glyph = L".";
            printable = false;
            return 1;
        }
    }

    if (wch < 0x20 || (wch >= 0x7F && wch < 0xA0))
    {
        glyph = L".";
        printable = false;
    }
    else
    {
        glyph.assign(1, wch);
        printable = true;
    }
    return len;
}

size_t DecodeUtf8One(const BYTE* data, size_t size, size_t pos, std::wstring& glyph, bool& printable)
{
    BYTE b0 = data[pos];
    if (b0 == 0)
    {
        glyph = L" ";
        printable = true;
        return 1;
    }

    size_t need = 0;
    size_t cp = 0;
    if ((b0 & 0x80) == 0x00)      { need = 1; cp = b0; }
    else if ((b0 & 0xE0) == 0xC0) { need = 2; cp = b0 & 0x1F; }
    else if ((b0 & 0xF0) == 0xE0) { need = 3; cp = b0 & 0x0F; }
    else if ((b0 & 0xF8) == 0xF0) { need = 4; cp = b0 & 0x07; }
    else
    {
        glyph = L".";
        printable = false;
        return 1;
    }

    if (pos + need > size)
    {
        glyph = L".";
        printable = false;
        return 1;
    }

    for (size_t i = 1; i < need; ++i)
    {
        BYTE b = data[pos + i];
        if ((b & 0xC0) != 0x80)
        {
            glyph = L".";
            printable = false;
            return 1;
        }
        cp = (cp << 6) | (b & 0x3F);
    }

    constexpr size_t kMinForLen[5] = { 0, 0, 0x80, 0x800, 0x10000 };
    if ((need > 1 && cp < kMinForLen[need]) || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF)
    {
        glyph = L".";
        printable = false;
        return 1;
    }

    if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
    {
        glyph = L".";
        printable = false;
        return need;
    }

    if (cp <= 0xFFFF)
    {
        glyph.assign(1, static_cast<WCHAR>(cp));
    }
    else
    {
        size_t v = cp - 0x10000;
        WCHAR hi = static_cast<WCHAR>(0xD800 + (v >> 10));
        WCHAR lo = static_cast<WCHAR>(0xDC00 + (v & 0x3FF));
        glyph.assign(1, hi);
        glyph.push_back(lo);
    }
    printable = true;
    return need;
}

size_t DecodeUtf16One(const BYTE* data, size_t size, size_t pos, std::wstring& glyph, bool& printable)
{
    if (pos + 2 > size)
    {
        glyph = L".";
        printable = false;
        return 1;
    }

    WORD w0 = static_cast<WORD>(data[pos]) | (static_cast<WORD>(data[pos + 1]) << 8);
    if (w0 == 0)
    {
        glyph = L" ";
        printable = true;
        return 2;
    }

    if (w0 >= 0xD800 && w0 <= 0xDBFF)
    {
        if (pos + 4 <= size)
        {
            WORD w1 = static_cast<WORD>(data[pos + 2]) | (static_cast<WORD>(data[pos + 3]) << 8);
            if (w1 >= 0xDC00 && w1 <= 0xDFFF)
            {
                glyph.assign(1, static_cast<WCHAR>(w0));
                glyph.push_back(static_cast<WCHAR>(w1));
                printable = true;
                return 4;
            }
        }
        glyph = L".";
        printable = false;
        return 2;
    }

    if ((w0 >= 0xDC00 && w0 <= 0xDFFF) || w0 < 0x20 || (w0 >= 0x7F && w0 < 0xA0))
    {
        glyph = L".";
        printable = false;
        return 2;
    }

    glyph.assign(1, static_cast<WCHAR>(w0));
    printable = true;
    return 2;
}

// テキストモードに応じて1文字分をデコードする共通ディスパッチ
// (RebuildDecodeCache とインクリメンタル更新の両方から使う)
size_t DecodeOneChar(BinEditTextMode mode, const BYTE* data, size_t size, size_t pos,
                    std::wstring& glyph, bool& printable)
{
    switch (mode)
    {
    case BinEditTextMode::ANSI:  return DecodeAnsiOne(data, size, pos, glyph, printable, CP_ACP);
    case BinEditTextMode::UTF8:  return DecodeUtf8One(data, size, pos, glyph, printable);
    case BinEditTextMode::UTF16: return DecodeUtf16One(data, size, pos, glyph, printable);
    case BinEditTextMode::SJIS:  return DecodeAnsiOne(data, size, pos, glyph, printable, CP_SJIS);
    }
    glyph = L".";
    printable = false;
    return 1;
}

} // namespace

// ===========================================================================
// ウィンドウ登録・作成
// ===========================================================================

const WCHAR* BinEdit::ClassName()
{
    return L"katahiromz's BinEdit";
}

BOOL BinEdit::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW existing;
    if (GetClassInfoExW(hInstance, ClassName(), &existing))
        return TRUE;

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = BinEdit::StaticWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_IBEAM);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = ClassName();
    return !!RegisterClassExW(&wc);
}

HWND BinEdit::Create(HWND hParent, int controlId,
                     int x, int y, int width, int height,
                     HINSTANCE hInstance, DWORD extraStyle)
{
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | WS_BORDER | extraStyle;
    return CreateWindowExW(WS_EX_CLIENTEDGE, ClassName(), L"",
                           style, x, y, width, height,
                           hParent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
                           hInstance, nullptr);
}

BinEdit* BinEdit::FromHwnd(HWND hwnd)
{
    if (!hwnd) return nullptr;
    return reinterpret_cast<BinEdit*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

BinEdit::BinEdit(HWND hwnd) : m_hwnd(hwnd)
{
    m_data_src = &m_data;
}

BinEdit::~BinEdit()
{
    if (m_hFont && m_ownFont)
        DeleteObject(m_hFont);
    if (m_hHeaderLinePen)
        DeleteObject(m_hHeaderLinePen);
}

LRESULT CALLBACK BinEdit::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto self = FromHwnd(hwnd);

    if (msg == WM_NCCREATE)
    {
        self = new BinEdit(hwnd);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    if (!self)
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    LRESULT result = self->WndProc(msg, wParam, lParam);

    if (msg == WM_NCDESTROY)
    {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        delete self;
    }

    return result;
}

void BinEdit::OnCut(HWND hwnd)
{
    if (Cut())
        NotifyChanged();
}

void BinEdit::OnCopy(HWND hwnd)
{
    Copy();
}

void BinEdit::OnPaste(HWND hwnd)
{
    if (Paste())
        NotifyChanged();
}

void BinEdit::OnClear(HWND hwnd)
{
    if (!DeleteSelection())
        return;

    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
    NotifyChanged();
}

void BinEdit::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_BINEDIT_CUT:
        SendMessageW(m_hwnd, WM_CUT, 0, 0);
        break;
    case ID_BINEDIT_COPY:
        SendMessageW(m_hwnd, WM_COPY, 0, 0);
        break;
    case ID_BINEDIT_PASTE:
        SendMessageW(m_hwnd, WM_PASTE, 0, 0);
        break;
    case ID_BINEDIT_DELETE:
        SendMessageW(m_hwnd, WM_CLEAR, 0, 0);
        break;
    case ID_BINEDIT_SELECTALL:
        SelectAll();
        break;
    case ID_BINEDIT_GOTO:
        GoToOffsetDialog();
        break;
    case ID_BINEDIT_ANSI:
        SetTextMode(BinEditTextMode::ANSI);
        break;
    case ID_BINEDIT_UTF8:
        SetTextMode(BinEditTextMode::UTF8);
        break;
    case ID_BINEDIT_UTF16:
        SetTextMode(BinEditTextMode::UTF16);
        break;
    case ID_BINEDIT_SJIS:
        SetTextMode(BinEditTextMode::SJIS);
        break;
    case ID_BINEDIT_COPYDUMPTEXT:
        CopyDumpText();
        break;
    default:
        break;
    }
}

LRESULT BinEdit::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    HANDLE_MSG(m_hwnd, WM_CREATE,      OnCreate);
    HANDLE_MSG(m_hwnd, WM_DESTROY,     OnDestroy);
    HANDLE_MSG(m_hwnd, WM_SIZE,        OnSize);
    HANDLE_MSG(m_hwnd, WM_PAINT,       OnPaint);
    HANDLE_MSG(m_hwnd, WM_ERASEBKGND,  OnEraseBkgnd);
    HANDLE_MSG(m_hwnd, WM_LBUTTONDOWN, OnLButtonDown);
    HANDLE_MSG(m_hwnd, WM_LBUTTONUP,   OnLButtonUp);
    HANDLE_MSG(m_hwnd, WM_MOUSEMOVE,   OnMouseMove);
    HANDLE_MSG(m_hwnd, WM_KEYDOWN,     OnKey);
    HANDLE_MSG(m_hwnd, WM_CHAR,        OnChar);
    HANDLE_MSG(m_hwnd, WM_VSCROLL,     OnVScroll);
    HANDLE_MSG(m_hwnd, WM_HSCROLL,     OnHScroll);
    HANDLE_MSG(m_hwnd, WM_SETFOCUS,    OnSetFocus);
    HANDLE_MSG(m_hwnd, WM_KILLFOCUS,   OnKillFocus);
    HANDLE_MSG(m_hwnd, WM_SETCURSOR,   OnSetCursor);
    HANDLE_MSG(m_hwnd, WM_SETFONT,     OnSetFont);
    HANDLE_MSG(m_hwnd, WM_GETFONT,     OnGetFont);
    HANDLE_MSG(m_hwnd, WM_MOUSEWHEEL,  OnMouseWheel);
    HANDLE_MSG(m_hwnd, WM_CONTEXTMENU, OnContextMenu);
    HANDLE_MSG(m_hwnd, WM_ENABLE,      OnEnable);
    HANDLE_MSG(m_hwnd, WM_CUT,         OnCut);
    HANDLE_MSG(m_hwnd, WM_COPY,        OnCopy);
    HANDLE_MSG(m_hwnd, WM_PASTE,       OnPaste);
    HANDLE_MSG(m_hwnd, WM_CLEAR,       OnClear);
    HANDLE_MSG(m_hwnd, WM_COMMAND,     OnCommand);

    case WM_CAPTURECHANGED:
        m_trackingMouse = false;
        m_gutterDrag = false;
        KillTimer(m_hwnd, kDragAutoScrollTimerId);
        break;

    case WM_TIMER:
        if (wParam == kDragAutoScrollTimerId)
        {
            OnDragAutoScrollTimer();
        }
        break;

#ifndef WM_DPICHANGED
    #define WM_DPICHANGED 0x02E0
#endif
    case WM_DPICHANGED:
        OnDpiChanged(wParam, lParam);
        break;

    case WM_IME_STARTCOMPOSITION: return OnImeStartComposition();
    case WM_IME_COMPOSITION:      return OnImeComposition(wParam, lParam);
    case WM_IME_ENDCOMPOSITION:   return OnImeEndComposition();
    case WM_IME_SETCONTEXT:       return OnImeSetContext(wParam, lParam);
    case WM_IME_NOTIFY:           return OnImeNotify(wParam, lParam);
    case WM_IME_CHAR:             return OnImeChar(wParam, lParam);

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_WANTTAB;

    default:
        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ===========================================================================
// ライフサイクル & フォント/レイアウト
// ===========================================================================

BOOL BinEdit::OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
{
    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
    m_showHeader = (style & BES_NOHEADER) == 0;
    m_readOnly   = (style & BES_READONLY) != 0;

    CreateEditFont();
    RecalcLayout();
    RebuildDecodeCache();
    UpdateScrollInfo();
    return TRUE;
}

BOOL BinEdit::OnEraseBkgnd(HWND hwnd, HDC hdc)
{
    return TRUE;
}

void BinEdit::OnDestroy(HWND hwnd)
{
    KillTimer(m_hwnd, kDragAutoScrollTimerId);

    if (m_hFont && m_ownFont)
    {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
        m_ownFont = false;
    }
}

void BinEdit::MeasureFontMetrics()
{
    HDC hdc = GetDC(m_hwnd);
    HFONT hOld = static_cast<HFONT>(SelectObject(hdc, m_hFont ? m_hFont : static_cast<HFONT>(GetStockObject(SYSTEM_FIXED_FONT))));
    TEXTMETRICW tm;
    GetTextMetricsW(hdc, &tm);
    m_charWidth = tm.tmAveCharWidth;
    m_lineHeight = tm.tmHeight + tm.tmExternalLeading + MulDiv(2, static_cast<int>(m_dpi), 96);
    SelectObject(hdc, hOld);
    ReleaseDC(m_hwnd, hdc);
}

void BinEdit::CreateEditFont()
{
    if (m_hFont && m_ownFont)
        DeleteObject(m_hFont);

    m_dpi = GetWindowDpi(m_hwnd);
    int pointSize = 10;
    int heightPx = -MulDiv(pointSize, static_cast<int>(m_dpi), 72);

    m_hFont = CreateFontW(heightPx, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"MS Gothic");
    m_ownFont = true;

    MeasureFontMetrics();
}

void BinEdit::OnSetFont(HWND /*hwnd*/, HFONT hfont, BOOL fRedraw)
{
    if (m_hFont && m_ownFont)
        DeleteObject(m_hFont);

    if (hfont)
    {
        m_hFont = hfont;
        m_ownFont = false;
    }
    else
    {
        m_hFont = nullptr;
        m_ownFont = false;
        CreateEditFont();
    }

    MeasureFontMetrics();
    RecalcLayout();
    UpdateScrollInfo();
    EnsureCaretVisible();
    RecreateCaret();

    if (fRedraw)
        InvalidateAll();
}

HFONT BinEdit::OnGetFont(HWND /*hwnd*/)
{
    return m_hFont;
}

int BinEdit::AddressDigits() const
{
#ifdef _WIN64
    // 通常は32ビット幅 (8桁) で表示し、4GBを超えるバッファのみ64ビット幅 (16桁) にする
    return (size() > 0xFFFFFFFFull) ? 16 : 8;
#else
    return 8;
#endif
}

void BinEdit::RecalcLayout()
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    m_clientWidth  = rc.right - rc.left;
    m_clientHeight = rc.bottom - rc.top;

    const int cw = m_charWidth;
    const int margin4 = MulDiv(4, static_cast<int>(m_dpi), 96);
    const int margin6 = MulDiv(6, static_cast<int>(m_dpi), 96);
    m_headerGap = MulDiv(4, static_cast<int>(m_dpi), 96);

    m_addrColX = margin4;
    m_hexColX  = m_addrColX + (AddressDigits() + 2) * cw;
    m_textColX = m_hexColX + (BYTES_PER_LINE * 3 + 1) * cw + cw;

    m_contentWidth = m_textColX + BYTES_PER_LINE * cw + margin4;
    m_headerHeight = m_showHeader ? (m_lineHeight + margin6 + m_headerGap) : 0;
    m_visibleLines = __max(1, (m_clientHeight - m_headerHeight) / m_lineHeight);

    const int maxSX = GetMaxScrollX();
    if (m_scrollX > maxSX) m_scrollX = maxSX;
    if (m_scrollX < 0)     m_scrollX = 0;
}

void BinEdit::OnSize(HWND hwnd, UINT /*state*/, int cx, int cy)
{
    RecalcLayout();
    UpdateScrollInfo();
    EnsureCaretVisible();
    InvalidateAll();
}

void BinEdit::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
    m_dpi = HIWORD(wParam);

    if (m_ownFont)
        CreateEditFont();
    else
        MeasureFontMetrics();

    if (const RECT* suggested = reinterpret_cast<const RECT*>(lParam))
    {
        SetWindowPos(m_hwnd, nullptr,
                     suggested->left, suggested->top,
                     suggested->right - suggested->left,
                     suggested->bottom - suggested->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    RecalcLayout();
    UpdateScrollInfo();
    EnsureCaretVisible();
    RecreateCaret();
    InvalidateAll();
}

void BinEdit::InvalidateAll()
{
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// ===========================================================================
// カーソル & コンテキストメニュー
// ===========================================================================

BOOL BinEdit::OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
    if (codeHitTest == HTCLIENT)
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_hwnd, &pt);

        size_t off; PANE pane; bool hi;
        if (HitTest(pt.x, pt.y, off, pane, hi))
        {
            if (pane == PANE_ADDRESS)
            {
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
                return TRUE;
            }
        }
        SetCursor(LoadCursor(nullptr, IDC_IBEAM));
        return TRUE;
    }
    return FALSE;
}

void BinEdit::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    if ((SHORT)xPos == -1 && (SHORT)yPos == -1)
    {
        POINT pt = { 0, 0 };
        ClientToScreen(hwnd, &pt);
        ShowContextMenu(pt.x, pt.y);
    }
    else
    {
        ShowContextMenu(static_cast<int>(xPos), static_cast<int>(yPos));
    }
}

void BinEdit::SelectAll()
{
    m_anchorOffset = 0;
    m_caretOffset = size();
    m_caretHiNibble = true;
    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
}

bool BinEdit::CopyDumpText()
{
    if (!OpenClipboard(m_hwnd))
        return false;

    EmptyClipboard();
    auto text = GetDumpText();
    SetClipboardUnicodeText(text);
    CloseClipboard();
    return true;
}

void BinEdit::ShowContextMenu(int screenX, int screenY)
{
    HMENU hMenu = LoadMenuW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDR_BINEDIT_CTX_MENU));
    if (!hMenu) return;
    HMENU hSubMenu = GetSubMenu(hMenu, 0);

    const bool hasSel = HasSelection();
    const bool canEdit = IsEditable();

    if (canEdit && hasSel)
    {
        EnableMenuItem(hMenu, ID_BINEDIT_CUT, MF_ENABLED);
        EnableMenuItem(hMenu, ID_BINEDIT_DELETE, MF_ENABLED);
    }
    else
    {
        EnableMenuItem(hMenu, ID_BINEDIT_CUT, MF_GRAYED);
        EnableMenuItem(hMenu, ID_BINEDIT_DELETE, MF_GRAYED);
    }

    EnableMenuItem(hMenu, ID_BINEDIT_COPY, hasSel ? MF_ENABLED : MF_GRAYED);

    if (canEdit && (IsClipboardFormatAvailable(CF_UNICODETEXT) ||
                    IsClipboardFormatAvailable(GetBinEditBytesFormat())))
    {
        EnableMenuItem(hMenu, ID_BINEDIT_PASTE, MF_ENABLED);
    }
    else
    {
        EnableMenuItem(hMenu, ID_BINEDIT_PASTE, MF_GRAYED);
    }

    switch (GetTextMode())
    {
    case BinEditTextMode::ANSI:
        CheckMenuRadioItem(hMenu, ID_BINEDIT_ANSI, ID_BINEDIT_SJIS, ID_BINEDIT_ANSI, MF_BYCOMMAND);
        break;
    case BinEditTextMode::UTF8:
        CheckMenuRadioItem(hMenu, ID_BINEDIT_ANSI, ID_BINEDIT_SJIS, ID_BINEDIT_UTF8, MF_BYCOMMAND);
        break;
    case BinEditTextMode::UTF16:
        CheckMenuRadioItem(hMenu, ID_BINEDIT_ANSI, ID_BINEDIT_SJIS, ID_BINEDIT_UTF16, MF_BYCOMMAND);
        break;
    case BinEditTextMode::SJIS:
        CheckMenuRadioItem(hMenu, ID_BINEDIT_ANSI, ID_BINEDIT_SJIS, ID_BINEDIT_SJIS, MF_BYCOMMAND);
        break;
    }

    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(hSubMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screenX, screenY, 0, m_hwnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd)
        PostMessage(m_hwnd, WM_COMMAND, cmd, 0);
}

void BinEdit::NotifyChanged()
{
    HWND hParent = GetParent(m_hwnd);
    if (hParent)
    {
        int id = GetWindowLong(m_hwnd, GWL_ID);
        SendMessageW(hParent, WM_COMMAND, MAKEWPARAM(id, BEN_CHANGE), reinterpret_cast<LPARAM>(m_hwnd));
    }
}

// ===========================================================================
// データ管理 & デコード
// ===========================================================================

void BinEdit::SetDataSrc(data_type* data_src)
{
    m_data_src = (data_src ? data_src : &m_data);
    m_anchorOffset = 0;
    m_caretOffset = 0;
    m_caretHiNibble = true;
    RecalcLayout();
    UpdateScrollInfo();
    EnsureCaretVisible();
    InvalidateAll();
    RebuildDecodeCache();
}

void BinEdit::SetData(data_type data)
{
    *m_data_src = std::move(data);
    m_caretOffset = __min(m_caretOffset, size());
    m_anchorOffset = m_caretOffset;
    m_caretHiNibble = true;
    m_topLine = 0;
    RebuildDecodeCache();
    UpdateScrollInfo();
    InvalidateAll();

    // SetLimit() で設定された範囲外なら合わせる
    if (size() < m_minLen)
        resize(m_minLen);
    else if (size() > m_maxLen)
        resize(m_maxLen);
}

void BinEdit::resize(size_t cb)
{
    if (cb < m_minLen) cb = m_minLen;
    if (cb > m_maxLen) cb = m_maxLen;

    if (cb == size()) return;

    // 拡張時はゼロ埋め、縮小時は末尾を切り詰める (std::vector::resize がどちらも行う)
    m_data_src->resize(cb, 0);

    m_caretOffset  = __min(m_caretOffset, size());
    m_anchorOffset = __min(m_anchorOffset, size());
    m_caretHiNibble = true;

    RebuildDecodeCache();
    RecalcLayout();
    UpdateScrollInfo();
    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
    NotifyChanged();
}

void BinEdit::SetLimit(size_t min_len, size_t max_len)
{
    if (min_len > max_len)
        std::swap(min_len, max_len); // 安全側に倒す (逆転していたら入れ替える)

    m_minLen = min_len;
    m_maxLen = max_len;

    const size_t cur = size();
    if (cur < m_minLen)
        resize(m_minLen);
    else if (cur > m_maxLen)
        resize(m_maxLen);
}

void BinEdit::SetTextMode(BinEditTextMode mode)
{
    if (m_textMode == mode) return;
    m_textMode = mode;
    RebuildDecodeCache();
    InvalidateAll();
}

void BinEdit::SetShowHeader(bool show)
{
    if (m_showHeader == show) return;
    m_showHeader = show;

    LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
    style = show ? (style & ~static_cast<LONG_PTR>(BES_NOHEADER))
                 : (style | static_cast<LONG_PTR>(BES_NOHEADER));
    SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);

    RecalcLayout();
    UpdateScrollInfo();
    EnsureCaretVisible();
    InvalidateAll();
}

void BinEdit::SetReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly) return;
    m_readOnly = readOnly;

    LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
    style = readOnly ? (style | static_cast<LONG_PTR>(BES_READONLY))
                     : (style & ~static_cast<LONG_PTR>(BES_READONLY));
    SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);

    if (readOnly)
    {
        HIMC hImc = ImmGetContext(m_hwnd);
        if (hImc)
        {
            ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
            ImmReleaseContext(m_hwnd, hImc);
        }
    }
    InvalidateAll();
}

bool BinEdit::IsEditable() const
{
    return !m_readOnly && IsWindowEnabled(m_hwnd);
}

void BinEdit::SetCaretOffset(size_t offset)
{
    m_caretOffset = __min(offset, size());
    m_anchorOffset = m_caretOffset;
    m_caretHiNibble = true;
    EnsureCaretVisible();
    InvalidateAll();
}

bool BinEdit::GoToOffsetDialog()
{
    GotoOffsetDlgParams params = {};
    params.current   = m_caretOffset;
    params.maxOffset = size();
    params.result    = m_caretOffset;
    params.accepted  = false;

    HWND owner = GetAncestor(m_hwnd, GA_ROOT);
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(
        static_cast<LONG_PTR>(GetWindowLongPtrW(m_hwnd, GWLP_HINSTANCE)));

    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_GOTO_OFFSET), owner, GotoOffsetDlgProc,
                    reinterpret_cast<LPARAM>(&params));
    if (!params.accepted)
        return false;

    SetCaretOffset(params.result);
    UpdateCaretShape();
    SetFocus(m_hwnd);
    return true;
}

std::wstring BinEdit::GetDumpText() const
{
    std::wstring result;
    const size_t dataSize = size();
    if (dataSize == 0) return result;

    const BYTE* pData = m_data_src->data();
    const size_t totalLines = (dataSize + BYTES_PER_LINE - 1) / BYTES_PER_LINE;
    result.reserve(totalLines * 80);

    const int addrDigits = AddressDigits();
    for (size_t addr = 0; addr < dataSize; addr += BYTES_PER_LINE)
    {
        WCHAR addrBuf[24];
        StringCchPrintfW(addrBuf, _countof(addrBuf), L"%0*llX  ", addrDigits, static_cast<unsigned long long>(addr));
        result += addrBuf;

        size_t lineCount = __min(BYTES_PER_LINE, dataSize - addr);

        for (size_t i = 0; i < BYTES_PER_LINE; ++i)
        {
            if (i < lineCount)
            {
                BYTE b = pData[addr + i];
                result.push_back(HEX_DIGITS[b >> 4]);
                result.push_back(HEX_DIGITS[b & 0x0F]);
                result.push_back(L' ');
            }
            else
            {
                result += L"   ";
            }
            if (i == 7) result.push_back(L' ');
        }
        result.push_back(L' ');

        for (size_t i = 0; i < lineCount; ++i)
        {
            size_t pos = addr + i;
            if (pos < m_byteToDecodedIndex.size())
            {
                ptrdiff_t idx = m_byteToDecodedIndex[pos];
                if (idx >= 0 && idx < static_cast<ptrdiff_t>(m_decoded.size()))
                {
                    const DecodedChar& dc = m_decoded[idx];
                    result += dc.printable ? dc.glyph : L".";
                    if (dc.length > 1) i += (dc.length - 1);
                    continue;
                }
            }
            BYTE b = pData[pos];
            if (b >= 0x20 && b <= 0x7E)
                result.push_back(static_cast<WCHAR>(b));
            else
                result.push_back(L'.');
        }
        result += L"\r\n";
    }
    return result;
}

void BinEdit::RebuildDecodeCache()
{
    m_decoded.clear();
    m_byteToDecodedIndex.assign(size(), -1);

    const BYTE* p = m_data_src->data();
    const size_t size = this->size();

    size_t pos = 0;
    while (pos < size)
    {
        DecodedChar dc;
        dc.offset = pos;
        dc.length = DecodeOneChar(m_textMode, p, size, pos, dc.glyph, dc.printable);
        if (dc.length == 0) dc.length = 1;

        m_byteToDecodedIndex[pos] = static_cast<ptrdiff_t>(m_decoded.size());
        m_decoded.push_back(dc);
        pos += dc.length;
    }
}

void BinEdit::UpdateDecodeCacheAfterEdit(size_t editPos, size_t oldLen, size_t newLen)
{
    const size_t newSize = size();
    const ptrdiff_t delta = static_cast<ptrdiff_t>(newLen) - static_cast<ptrdiff_t>(oldLen);

    // 直前まで m_byteToDecodedIndex は「編集前」のバッファを指しているはずなので、
    // そのサイズから逆算した旧サイズが整合しない場合は前提が崩れているのでフルリビルド。
    if (static_cast<ptrdiff_t>(m_byteToDecodedIndex.size()) != static_cast<ptrdiff_t>(newSize) - delta)
    {
        RebuildDecodeCache();
        return;
    }
    const size_t oldSize = m_byteToDecodedIndex.size();
    if (oldSize == 0 || m_decoded.empty())
    {
        RebuildDecodeCache();
        return;
    }

    // --- 編集点を含む「旧」文字の先頭まで巻き戻る (それ以前は編集の影響を受けない) ---
    size_t p = __min(editPos, oldSize - 1);
    while (p > 0 && m_byteToDecodedIndex[p] < 0)
        --p;
    const size_t oldStart = p;
    const ptrdiff_t firstIdx = (m_byteToDecodedIndex[p] >= 0) ? m_byteToDecodedIndex[p] : 0;

    // --- 「新」バッファ上を oldStart から再デコードし、旧キャッシュと再同期できる点を探す ---
    const BYTE* pData = m_data_src->data();
    const size_t resumeAfter = editPos + newLen;               // 編集(挿入)領域の直後の新位置
    const size_t giveUpAt = __min(newSize, resumeAfter + 65536); // 病的ケース (再同期不能) の暴走防止

    std::vector<DecodedChar> freshChars;
    size_t q = oldStart;
    bool resynced = false;
    ptrdiff_t resyncOldIdx = static_cast<ptrdiff_t>(m_decoded.size());

    while (q < newSize)
    {
        if (q >= resumeAfter)
        {
            const ptrdiff_t qOldL = static_cast<ptrdiff_t>(q) - delta;
            if (qOldL >= 0 && static_cast<size_t>(qOldL) < oldSize && m_byteToDecodedIndex[qOldL] >= 0)
            {
                resynced = true;
                resyncOldIdx = m_byteToDecodedIndex[qOldL];
                break;
            }
            if (q >= giveUpAt)
                break;
        }

        DecodedChar dc;
        dc.offset = q;
        dc.length = DecodeOneChar(m_textMode, pData, newSize, q, dc.glyph, dc.printable);
        if (dc.length == 0) dc.length = 1;
        freshChars.push_back(dc);
        q += dc.length;
    }

    if (!resynced && q >= newSize)
    {
        // バッファの本当の終端まで再デコードし切った場合は、それ自体が有効な終端 (それ以降は何もない)
        resynced = true;
        resyncOldIdx = static_cast<ptrdiff_t>(m_decoded.size());
    }

    if (!resynced)
    {
        // 安全に再同期できなかった (稀なケース) ので、常に正しいフルリビルドにフォールバック
        RebuildDecodeCache();
        return;
    }

    // --- 継ぎ合わせ: [0, firstIdx) はそのまま維持 / 中間は freshChars に置換 / 残りはオフセットを delta だけシフト ---
    std::vector<DecodedChar> result;
    result.reserve(m_decoded.size() + freshChars.size());
    result.insert(result.end(), m_decoded.begin(), m_decoded.begin() + firstIdx);
    result.insert(result.end(), freshChars.begin(), freshChars.end());
    for (size_t i = static_cast<size_t>(resyncOldIdx); i < m_decoded.size(); ++i)
    {
        DecodedChar dc = m_decoded[i];
        dc.offset = static_cast<size_t>(static_cast<ptrdiff_t>(dc.offset) + delta);
        result.push_back(std::move(dc));
    }

    m_decoded.swap(result);
    m_byteToDecodedIndex.assign(newSize, -1);
    for (size_t i = 0; i < m_decoded.size(); ++i)
        m_byteToDecodedIndex[m_decoded[i].offset] = static_cast<ptrdiff_t>(i);
}

size_t BinEdit::DecodedLengthAt(size_t pos) const
{
    if (pos >= m_byteToDecodedIndex.size()) return 1;
    ptrdiff_t idx = m_byteToDecodedIndex[pos];
    if (idx < 0) return 1;
    return m_decoded[idx].length;
}

// ===========================================================================
// 描画処理
// ===========================================================================

void BinEdit::DrawHeader(HDC hdc)
{
    if (!m_showHeader || m_headerHeight <= 0) return;

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    RECT headerRc = { 0, 0, rc.right, m_headerHeight - m_headerGap };
    FillRect(hdc, &headerRc, reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1));

    const int cw = m_charWidth;
    const COLORREF colLabel = RGB(0x40, 0x40, 0x40);
    const int sx = m_scrollX;

    INT y = MulDiv(3, static_cast<int>(m_dpi), 96);

    SetTextColor(hdc, colLabel);
    TextOutW(hdc, m_addrColX - sx, y, L"@ADDRESS", 8);

    for (int i = 0; i < BYTES_PER_LINE; ++i)
    {
        int gap = (i >= BYTES_PER_LINE / 2) ? 1 : 0;
        int x = m_hexColX + (i * 3 + gap) * cw - sx;
        WCHAR label[3] = { L'+', HEX_DIGITS[i], 0 };
        TextOutW(hdc, x, y, label, 2);
    }

    for (int i = 0; i < BYTES_PER_LINE; ++i)
    {
        int x = m_textColX + i * cw - sx;
        WCHAR label[2] = { HEX_DIGITS[i], 0 };
        TextOutW(hdc, x, y, label, 1);
    }

    // ヘッダー区切り線用のペンは毎回 Create/Delete せずキャッシュして使い回す
    if (!m_hHeaderLinePen)
        m_hHeaderLinePen = CreatePen(PS_SOLID, 1, RGB(0x99, 0x99, 0x99));
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, m_hHeaderLinePen));
    MoveToEx(hdc, 0, m_headerHeight - m_headerGap - 1, nullptr);
    LineTo(hdc, rc.right, m_headerHeight - m_headerGap - 1);
    SelectObject(hdc, hOldPen);
}

void BinEdit::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    if (!IsWindowVisible(hwnd) || IsRectEmpty(&ps.rcPaint))
    {
        EndPaint(m_hwnd, &ps);
        return;
    }

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(memDC, memBmp));

    const bool enabled = IsWindowEnabled(m_hwnd);
    const bool normalBk = enabled && !m_readOnly;
    FillRect(memDC, &rc, reinterpret_cast<HBRUSH>((normalBk ? COLOR_WINDOW : COLOR_3DFACE) + 1));

    HFONT oldFont = static_cast<HFONT>(SelectObject(memDC, m_hFont));
    SetBkMode(memDC, TRANSPARENT);

    const COLORREF colGrayText = GetSysColor(COLOR_GRAYTEXT);
    const COLORREF colAddr = enabled ? RGB(0x60, 0x60, 0x60) : colGrayText;
    const COLORREF colHex  = enabled ? RGB(0x00, 0x00, 0x00) : colGrayText;
    const COLORREF colText = enabled ? RGB(0x00, 0x00, 0x80) : colGrayText;
    const COLORREF colSel  = RGB(0x33, 0x66, 0xCC);
    const COLORREF colSelInactive = RGB(0xC0, 0xC0, 0xC0);
    const COLORREF colRange = (m_hasFocus && enabled) ? RGB(0x33, 0x66, 0xCC) : RGB(0xC0, 0xC0, 0xC0);
    const COLORREF colRangeText = (m_hasFocus && enabled) ? RGB(255, 255, 255) : colHex;

    const size_t size = this->size();
    const int cw = m_charWidth;
    const int sx = m_scrollX;

    size_t selStart = 0, selEnd = 0;
    GetSelection(selStart, selEnd);
    const bool hasSel = (selStart < selEnd);

    // 選択/キャレット表示用のブラシは色の種類が高々3つなので、行ごとに
    // CreateSolidBrush/DeleteObject を繰り返さず、描画開始時に一度だけ作って使い回す。
    HBRUSH brRange = CreateSolidBrush(colRange);
    HBRUSH brSel = CreateSolidBrush(colSel);
    HBRUSH brSelInactive = CreateSolidBrush(colSelInactive);

    DrawHeader(memDC);

    const int addrDigits = AddressDigits();

    for (int row = 0; row < m_visibleLines + 1; ++row)
    {
        size_t addr = static_cast<size_t>(m_topLine + row) * BYTES_PER_LINE;
        if (addr > size) break;

        int y = m_headerHeight + row * m_lineHeight;

        WCHAR addrText[24];
        StringCchPrintfW(addrText, _countof(addrText), L"%0*llX", addrDigits, static_cast<unsigned long long>(addr));

        SetTextColor(memDC, colAddr);
        TextOutW(memDC, m_addrColX - sx, y, addrText, addrDigits);

        size_t remain = (addr < size) ? (size - addr) : 0;
        size_t count = (remain < BYTES_PER_LINE) ? remain : BYTES_PER_LINE;

        auto hexCellX = [&](size_t i) -> int {
            int gap = (i >= BYTES_PER_LINE / 2) ? 1 : 0;
            return m_hexColX + static_cast<int>(i * 3 + gap) * cw - sx;
        };

        if (hasSel && count > 0)
        {
            const size_t lineSelStart = __max(selStart, addr);
            const size_t lineSelEnd   = __min(selEnd, addr + count);
            if (lineSelStart < lineSelEnd)
            {
                const size_t i0 = lineSelStart - addr;
                const size_t i1 = lineSelEnd - addr - 1;
                const int left  = hexCellX(i0);
                const int right = hexCellX(i1) + 2 * cw;
                RECT band = { left, y, right, y + m_lineHeight };
                FillRect(memDC, &band, brRange);
            }
        }

        for (size_t i = 0; i < count; ++i)
        {
            size_t off = addr + i;
            int x = hexCellX(i);

            const bool inRange = hasSel && off >= selStart && off < selEnd;
            const bool isCaret = !hasSel && m_hasFocus && m_focusPane == PANE_HEX && off == m_caretOffset;
            const bool isCaretRow = !hasSel && (!m_hasFocus || m_focusPane != PANE_HEX) && off == m_caretOffset;

            if (!inRange && (isCaret || isCaretRow))
            {
                RECT sel = { x, y, x + 2 * cw, y + m_lineHeight };
                FillRect(memDC, &sel, isCaret ? brSel : brSelInactive);
            }

            BYTE b = (*m_data_src)[off];
            WCHAR hexStr[3] = { HEX_DIGITS[b >> 4], HEX_DIGITS[b & 0xF], 0 };
            COLORREF textCol = inRange ? colRangeText : (isCaret ? RGB(255, 255, 255) : colHex);
            SetTextColor(memDC, textCol);
            TextOutW(memDC, x, y, hexStr, 2);
        }

        // テキスト列描画
        {
            const int textColLeft  = m_textColX - sx;
            const int textColRight = m_textColX + BYTES_PER_LINE * cw - sx;
            const size_t lineEnd = addr + count;

            ptrdiff_t di = 0;
            if (addr < m_byteToDecodedIndex.size())
            {
                size_t p = addr;
                while (p > 0 && m_byteToDecodedIndex[p] < 0) --p;
                if (m_byteToDecodedIndex[p] >= 0) di = m_byteToDecodedIndex[p];
            }

            for (ptrdiff_t ci = di; ci < static_cast<ptrdiff_t>(m_decoded.size()); ++ci)
            {
                const DecodedChar& dc = m_decoded[ci];
                if (dc.offset >= lineEnd) break;
                if (dc.offset + dc.length <= addr) continue;

                const bool charSelected = hasSel && dc.offset < selEnd && (dc.offset + dc.length) > selStart;
                const bool isCaret = !hasSel && m_hasFocus && m_focusPane == PANE_TEXT
                    && m_caretOffset >= dc.offset && m_caretOffset < dc.offset + dc.length;
                const bool isCaretRow = !hasSel && (!m_hasFocus || m_focusPane != PANE_TEXT)
                    && m_caretOffset >= dc.offset && m_caretOffset < dc.offset + dc.length;

                if (!charSelected && !isCaret && !isCaretRow) continue;

                const size_t cellFrom = __max(dc.offset, addr);
                const size_t cellTo   = __min(dc.offset + dc.length, lineEnd);
                if (cellFrom >= cellTo) continue;

                const int left  = m_textColX + static_cast<int>(cellFrom - addr) * cw - sx;
                const int right = m_textColX + static_cast<int>(cellTo   - addr) * cw - sx;
                RECT band = { left, y, right, y + m_lineHeight };
                HBRUSH bandBr = charSelected ? brRange : (isCaret ? brSel : brSelInactive);
                FillRect(memDC, &band, bandBr);
            }

            for (ptrdiff_t ci = di; ci < static_cast<ptrdiff_t>(m_decoded.size()); ++ci)
            {
                const DecodedChar& dc = m_decoded[ci];
                if (dc.offset >= lineEnd) break;
                if (dc.offset + dc.length <= addr) continue;

                const int startCell = static_cast<int>(dc.offset) - static_cast<int>(addr);
                const int x = m_textColX + startCell * cw - sx;
                const int glyphRight = x + static_cast<int>(dc.length) * cw;

                const int clipLeft  = __max(x, textColLeft);
                const int clipRight = __min(glyphRight, textColRight);
                if (clipRight <= clipLeft) continue;

                const std::wstring& glyph = dc.printable ? dc.glyph : L".";
                const bool charSelected = hasSel && dc.offset < selEnd && (dc.offset + dc.length) > selStart;
                const bool isCaret = !hasSel && m_hasFocus && m_focusPane == PANE_TEXT
                    && m_caretOffset >= dc.offset && m_caretOffset < dc.offset + dc.length;

                COLORREF textCol = charSelected ? colRangeText : (isCaret ? RGB(255, 255, 255) : colText);
                SetTextColor(memDC, textCol);
                RECT clip = { clipLeft, y, clipRight, y + m_lineHeight };
                ExtTextOutW(memDC, x, y, ETO_CLIPPED, &clip, glyph.c_str(),
                            static_cast<UINT>(glyph.size()), nullptr);
            }
        }
        if (addr >= size) break;
    }

    SelectObject(memDC, oldFont);
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);

    DeleteObject(brRange);
    DeleteObject(brSel);
    DeleteObject(brSelInactive);

    EndPaint(m_hwnd, &ps);
}

// ===========================================================================
// マウス＆ヒットテスト
// ===========================================================================

bool BinEdit::HitTest(int x, int y, size_t& byteOffset, PANE& pane, bool& hiNibble) const
{
    if (y < m_headerHeight) return false;

    int row = (y - m_headerHeight) / m_lineHeight;
    size_t lineAddr = static_cast<size_t>(m_topLine + row) * BYTES_PER_LINE;
    const int cw = m_charWidth;

    x += m_scrollX;
    const size_t dataSize = size();

    if (lineAddr >= dataSize)
    {
        byteOffset = dataSize;
        hiNibble = true;
        pane = (x >= m_textColX) ? PANE_TEXT : PANE_HEX;
        return true;
    }

    if (x < m_hexColX - 2 * cw)
    {
        byteOffset = lineAddr;
        if (byteOffset > dataSize) byteOffset = dataSize;
        pane = PANE_ADDRESS;
        hiNibble = true;
        return true;
    }

    if (m_hexColX - 2 * cw <= x && x < m_textColX)
    {
        const int localX = x - m_hexColX;
        int col = (localX + cw / 2) / cw;
        int byteIndex = 0, within = 0;
        if (!HexColToByte(col, byteIndex, within)) return false;

        if (within == 2)
        {
            const int gapWidth = (byteIndex == 7) ? 2 * cw : cw;
            const int gapStartCol = byteIndex * 3 + ((byteIndex >= BYTES_PER_LINE / 2) ? 1 : 0) + 2;
            const int offsetInGap = localX - gapStartCol * cw;
            if (offsetInGap >= gapWidth / 2)
            {
                byteIndex += 1;
                within = 0;
            }
            else
            {
                within = 1;
            }
        }

        byteOffset = lineAddr + static_cast<size_t>(byteIndex);
        if (byteOffset > dataSize) byteOffset = dataSize;
        pane = PANE_HEX;
        hiNibble = (byteOffset < dataSize) ? (within == 0) : true;
        return true;
    }

    if (x >= m_textColX)
    {
        int localX = x - m_textColX;
        int byteIndex = (localX + cw / 2) / cw;

        if (byteIndex < 0) byteIndex = 0;
        if (byteIndex >= BYTES_PER_LINE) byteIndex = BYTES_PER_LINE;

        byteOffset = lineAddr + static_cast<size_t>(byteIndex);
        if (byteOffset > dataSize) byteOffset = dataSize;

        if (byteOffset < dataSize &&
            byteOffset < m_byteToDecodedIndex.size() &&
            m_byteToDecodedIndex[byteOffset] < 0)
        {
            size_t p = byteOffset;
            while (p > 0 && (p >= m_byteToDecodedIndex.size() || m_byteToDecodedIndex[p] < 0)) --p;
            if (p < m_byteToDecodedIndex.size() && m_byteToDecodedIndex[p] >= 0)
                byteOffset = p;
        }

        pane = PANE_TEXT;
        hiNibble = true;
        return true;
    }

    return false;
}

void BinEdit::OnLButtonDown(HWND hwnd, BOOL /*fDoubleClick*/, int x, int y, UINT /*keyFlags*/)
{
    size_t off; PANE pane; bool hi;
    if (!HitTest(x, y, off, pane, hi)) return;

    const bool shift = (GetKeyState(VK_SHIFT) < 0);
    const size_t dataSize = size();
    off = __min(off, dataSize);

    if (pane == PANE_ADDRESS)
    {
        const size_t lineStart = off;
        const size_t lineEnd = __min(lineStart + BYTES_PER_LINE, dataSize);

        size_t anchorLine = lineStart;
        if (shift)
            anchorLine = (m_anchorOffset / BYTES_PER_LINE) * BYTES_PER_LINE;

        m_gutterAnchorLine = anchorLine;

        const size_t anchorLineEnd = __min(anchorLine + BYTES_PER_LINE, dataSize);
        if (lineStart >= anchorLine)
        {
            m_anchorOffset = anchorLine;
            m_caretOffset  = lineEnd;
        }
        else
        {
            m_anchorOffset = anchorLineEnd;
            m_caretOffset  = lineStart;
        }

        m_focusPane = PANE_HEX;
        m_caretHiNibble = true;
        m_gutterDrag = true;

        SetCapture(m_hwnd);
        m_trackingMouse = true;

        EnsureCaretVisible();
        UpdateCaretShape();
        InvalidateAll();
        return;
    }

    m_gutterDrag = false;
    m_focusPane = pane;
    m_caretHiNibble = hi;

    if (!shift)
    {
        m_caretOffset = off;
        m_anchorOffset = off;
    }
    else
    {
        m_caretOffset = SelectionCaretFromHit(off, m_anchorOffset, dataSize);
        m_caretHiNibble = true;
    }

    SetCapture(m_hwnd);
    m_trackingMouse = true;

    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
}

void BinEdit::OnLButtonUp(HWND hwnd, int x, int y, UINT /*keyFlags*/)
{
    if (m_trackingMouse)
    {
        ReleaseCapture();
        m_trackingMouse = false;
    }
    m_gutterDrag = false;
    KillTimer(m_hwnd, kDragAutoScrollTimerId);
}

void BinEdit::OnMouseMove(HWND hwnd, int x, int y, UINT /*keyFlags*/)
{
    if (!m_trackingMouse) return;

    if (m_gutterDrag)
        UpdateGutterDragSelection(x, y);
    else
        UpdateByteDragSelection(x, y);
}

bool BinEdit::AutoScrollDragIfNeeded(int y)
{
    bool scrolled = false;
    int scrollLines = 0;

    int dist = 0;
    if (y < m_headerHeight)
    {
        dist = m_headerHeight - y;
    }
    else if (y >= m_headerHeight + m_visibleLines * m_lineHeight)
    {
        dist = y - (m_headerHeight + m_visibleLines * m_lineHeight - 1);
    }

    if (dist > 0)
    {
        scrollLines = 1 + (dist / 20);
        scrollLines = __min(scrollLines, 5);
    }

    if (y < m_headerHeight)
    {
        if (m_topLine > 0)
        {
            m_topLine = __max(0, m_topLine - scrollLines);
            scrolled = true;
        }
        SetTimer(m_hwnd, kDragAutoScrollTimerId, kDragAutoScrollIntervalMs, nullptr);
    }
    else if (y >= m_headerHeight + m_visibleLines * m_lineHeight)
    {
        const int maxTop = GetMaxTopLine();
        if (m_topLine < maxTop)
        {
            m_topLine = __min(maxTop, m_topLine + scrollLines);
            scrolled = true;
        }
        SetTimer(m_hwnd, kDragAutoScrollTimerId, kDragAutoScrollIntervalMs, nullptr);
    }
    else
    {
        KillTimer(m_hwnd, kDragAutoScrollTimerId);
    }

    if (scrolled) UpdateScrollInfo();
    return scrolled;
}

void BinEdit::UpdateGutterDragSelection(int x, int y)
{
    const size_t dataSize = size();
    const bool scrolled = AutoScrollDragIfNeeded(y);

    int row = (y >= m_headerHeight) ? (y - m_headerHeight) / m_lineHeight : 0;
    if (row < 0) row = 0;

    size_t curLineStart = static_cast<size_t>(m_topLine + row) * BYTES_PER_LINE;
    const size_t lastLineStart = (dataSize / BYTES_PER_LINE) * BYTES_PER_LINE;
    if (curLineStart > lastLineStart) curLineStart = lastLineStart;

    const size_t curLineEnd = __min(curLineStart + BYTES_PER_LINE, dataSize);
    const size_t anchorLineStart = m_gutterAnchorLine;
    const size_t anchorLineEnd = __min(anchorLineStart + BYTES_PER_LINE, dataSize);

    size_t newAnchor, newCaret;
    if (curLineStart >= anchorLineStart)
    {
        newAnchor = anchorLineStart;
        newCaret  = curLineEnd;
    }
    else
    {
        newAnchor = anchorLineEnd;
        newCaret  = curLineStart;
    }

    if (newAnchor != m_anchorOffset || newCaret != m_caretOffset || scrolled)
    {
        m_anchorOffset = newAnchor;
        m_caretOffset  = newCaret;
        m_caretHiNibble = true;
        EnsureCaretVisible();
        UpdateCaretShape();
        InvalidateAll();
    }
}

void BinEdit::UpdateByteDragSelection(int x, int y)
{
    const size_t dataSize = size();
    const bool scrolled = AutoScrollDragIfNeeded(y);

    int clampedY = y;
    if (clampedY < m_headerHeight)
        clampedY = m_headerHeight;
    else if (clampedY >= m_headerHeight + m_visibleLines * m_lineHeight)
        clampedY = m_headerHeight + m_visibleLines * m_lineHeight - 1;

    size_t off = 0;
    bool hi = true;
    PANE pane = m_focusPane;

    if (HitTest(x, clampedY, off, pane, hi))
    {
        off = __min(off, dataSize);
    }
    else
    {
        int row = (clampedY - m_headerHeight) / m_lineHeight;
        size_t lineAddr = static_cast<size_t>(m_topLine + row) * BYTES_PER_LINE;
        if (lineAddr > dataSize) lineAddr = dataSize;

        const int contentX = x + m_scrollX;
        const int cw = m_charWidth;

        if (m_focusPane == PANE_HEX && contentX >= m_hexColX)
        {
            int col = __max(0, contentX - m_hexColX) / cw;
            int byteIndex = 0, within = 0;
            if (!HexColToByte(col, byteIndex, within)) byteIndex = BYTES_PER_LINE;
            if (byteIndex < 0) byteIndex = 0;
            if (byteIndex > BYTES_PER_LINE) byteIndex = BYTES_PER_LINE;
            off = __min(lineAddr + static_cast<size_t>(byteIndex), dataSize);
        }
        else if (m_focusPane == PANE_TEXT && contentX >= m_textColX)
        {
            int byteIndex = (contentX - m_textColX) / cw;
            if (byteIndex < 0) byteIndex = 0;
            if (byteIndex > BYTES_PER_LINE) byteIndex = BYTES_PER_LINE;
            off = __min(lineAddr + static_cast<size_t>(byteIndex), dataSize);
        }
        else
        {
            return;
        }
        hi = true;
    }

    size_t newCaret = SelectionCaretFromHit(off, m_anchorOffset, dataSize);
    if (newCaret != m_caretOffset || hi != m_caretHiNibble || scrolled)
    {
        m_caretOffset = newCaret;
        m_caretHiNibble = true;
        EnsureCaretVisible();
        UpdateCaretShape();
        InvalidateAll();
    }
}

void BinEdit::OnDragAutoScrollTimer()
{
    if (!m_trackingMouse)
    {
        KillTimer(m_hwnd, kDragAutoScrollTimerId);
        return;
    }

    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(m_hwnd, &pt);

    if (m_gutterDrag)
        UpdateGutterDragSelection(pt.x, pt.y);
    else
        UpdateByteDragSelection(pt.x, pt.y);
}

// ===========================================================================
// スクロール処理
// ===========================================================================

void BinEdit::UpdateScrollInfo()
{
    // バッファサイズが変わった可能性がある (4GB境界をまたいでアドレス桁数が変わる等) ので、
    // スクロール情報を出す前に列レイアウトを再計算しておく。
    RecalcLayout();

    int totalLines = GetTotalLines();
    const int maxTop = __max(0, totalLines - m_visibleLines);
    if (m_topLine > maxTop) m_topLine = maxTop;
    if (m_topLine < 0)      m_topLine = 0;

    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
    si.nMin  = 0;
    si.nMax  = totalLines - 1;
    si.nPage = static_cast<UINT>(__max(1, m_visibleLines));
    si.nPos  = m_topLine;
    SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);

    const int maxSX = GetMaxScrollX();
    if (m_scrollX > maxSX) m_scrollX = maxSX;
    if (m_scrollX < 0)     m_scrollX = 0;

    SCROLLINFO hsi = { sizeof(hsi) };
    hsi.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
    hsi.nMin  = 0;
    hsi.nMax  = __max(0, m_contentWidth - 1);
    hsi.nPage = static_cast<UINT>(__max(1, m_clientWidth));
    hsi.nPos  = m_scrollX;
    SetScrollInfo(m_hwnd, SB_HORZ, &hsi, TRUE);
}

int BinEdit::GetMaxTopLine() const
{
    return __max(0, GetTotalLines() - m_visibleLines);
}

int BinEdit::GetContentWidth() const
{
    return m_contentWidth;
}

int BinEdit::GetMaxScrollX() const
{
    return __max(0, m_contentWidth - m_clientWidth);
}

int BinEdit::GetTotalLines() const
{
    // SCROLLINFO の各フィールドは32ビット (int) までしか表現できないため、
    // 巨大なバッファではここで頭打ちにする (スクロールバー自体の限界であり、
    // 編集・アドレス表示・データサイズは引き続き64ビットで正しく扱われる)。
    const unsigned long long lines = static_cast<unsigned long long>(size() / BYTES_PER_LINE) + 1;
    return (lines > static_cast<unsigned long long>((std::numeric_limits<int>::max)()))
        ? (std::numeric_limits<int>::max)()
        : static_cast<int>(lines);
}

void BinEdit::OnVScroll(HWND hwnd, HWND /*hwndCtl*/, UINT code, int pos)
{
    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_ALL;
    GetScrollInfo(m_hwnd, SB_VERT, &si);

    const int maxTop = GetMaxTopLine();
    int nPos = m_topLine;

    switch (code)
    {
    case SB_LINEUP:        nPos -= 1; break;
    case SB_LINEDOWN:      nPos += 1; break;
    case SB_PAGEUP:        nPos -= __max(1, m_visibleLines - 1); break;
    case SB_PAGEDOWN:      nPos += __max(1, m_visibleLines - 1); break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: nPos = si.nTrackPos; break;
    case SB_TOP:            nPos = 0; break;
    case SB_BOTTOM:         nPos = maxTop; break;
    default: return;
    }

    nPos = __max(0, __min(nPos, maxTop));
    if (nPos != m_topLine)
    {
        m_topLine = nPos;
        SCROLLINFO posSi = { sizeof(posSi) };
        posSi.fMask = SIF_POS;
        posSi.nPos = m_topLine;
        SetScrollInfo(m_hwnd, SB_VERT, &posSi, TRUE);
        InvalidateAll();
        UpdateCaretShape();
    }
}

void BinEdit::OnHScroll(HWND hwnd, HWND /*hwndCtl*/, UINT code, int pos)
{
    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_ALL;
    GetScrollInfo(m_hwnd, SB_HORZ, &si);

    const int maxSX = GetMaxScrollX();
    const int lineStep = __max(1, m_charWidth);
    const int pageStep = __max(lineStep, m_clientWidth - lineStep);
    int nPos = m_scrollX;

    switch (code)
    {
    case SB_LINELEFT:      nPos -= lineStep; break;
    case SB_LINERIGHT:     nPos += lineStep; break;
    case SB_PAGELEFT:      nPos -= pageStep; break;
    case SB_PAGERIGHT:     nPos += pageStep; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: nPos = si.nTrackPos; break;
    case SB_LEFT:          nPos = 0; break;
    case SB_RIGHT:         nPos = maxSX; break;
    default: return;
    }

    nPos = __max(0, __min(nPos, maxSX));
    if (nPos != m_scrollX)
    {
        m_scrollX = nPos;
        SCROLLINFO posSi = { sizeof(posSi) };
        posSi.fMask = SIF_POS;
        posSi.nPos = m_scrollX;
        SetScrollInfo(m_hwnd, SB_HORZ, &posSi, TRUE);
        InvalidateAll();
        UpdateCaretShape();
    }
}

#ifndef SPI_GETWHEELSCROLLCHARS
    #define SPI_GETWHEELSCROLLCHARS 0x006C
#endif

void BinEdit::OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
{
	if (GetKeyState(VK_CONTROL) < 0)
	{
		UINT id = GetDlgCtrlID(hwnd);
		if (zDelta < 0)
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, BEN_ZOOMOUT), (LPARAM)hwnd);
		else
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(id, BEN_ZOOMIN), (LPARAM)hwnd);
		return;
	}

    if (fwKeys & MK_SHIFT)
    {
        UINT wheelChars = 3;
        SystemParametersInfoW(SPI_GETWHEELSCROLLCHARS, 0, &wheelChars, 0);
        if (wheelChars == 0) wheelChars = 3;

        int step = (zDelta * static_cast<int>(wheelChars) * m_charWidth) / WHEEL_DELTA;
        if (step == 0 && zDelta != 0)
            step = (zDelta > 0) ? m_charWidth : -m_charWidth;

        if (GetKeyState(VK_SHIFT) < 0)
            step = -step;

        const int maxSX = GetMaxScrollX();
        int newSX = __max(0, __min(m_scrollX + step, maxSX));
        if (newSX == m_scrollX) return;

        m_scrollX = newSX;
        UpdateScrollInfo();
        InvalidateAll();
        UpdateCaretShape();
        return;
    }

    UINT wheelLines = 3;
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &wheelLines, 0);
    if (wheelLines == WHEEL_PAGESCROLL)
        wheelLines = static_cast<UINT>(__max(1, m_visibleLines));
    else if (wheelLines == 0)
        wheelLines = 3;

    int lines = -(zDelta * static_cast<int>(wheelLines)) / WHEEL_DELTA;
    if (lines == 0 && zDelta != 0)
        lines = (zDelta > 0) ? -1 : 1;

    const int maxTop = GetMaxTopLine();
    int newTop = __max(0, __min(m_topLine + lines, maxTop));
    if (newTop == m_topLine) return;

    m_topLine = newTop;
    UpdateScrollInfo();
    InvalidateAll();
    UpdateCaretShape();
}

// ===========================================================================
// フォーカス・キャレット制御
// ===========================================================================

void BinEdit::OnSetFocus(HWND hwnd, HWND /*hwndOldFocus*/)
{
    m_hasFocus = true;
    RecreateCaret();
    InvalidateAll();
}

void BinEdit::OnKillFocus(HWND hwnd, HWND /*hwndNewFocus*/)
{
    HIMC hImc = ImmGetContext(m_hwnd);
    if (hImc)
    {
        ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
        ImmReleaseContext(m_hwnd, hImc);
    }

    m_hasFocus = false;
    m_suppressImeChar = false;
    HideCaret(m_hwnd);
    DestroyCaret();
    InvalidateAll();
}

void BinEdit::OnEnable(HWND hwnd, BOOL fEnable)
{
    if (!fEnable)
    {
        if (m_trackingMouse || m_gutterDrag)
        {
            ReleaseCapture();
            m_trackingMouse = false;
            m_gutterDrag = false;
            KillTimer(m_hwnd, kDragAutoScrollTimerId);
        }
        HIMC hImc = ImmGetContext(m_hwnd);
        if (hImc)
        {
            ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
            ImmReleaseContext(m_hwnd, hImc);
        }
    }
    InvalidateAll();
}

void BinEdit::RecreateCaret()
{
    if (!m_hasFocus) return;

    HideCaret(m_hwnd);
    DestroyCaret();

    const int caretW = m_insertMode ? __max(2, m_charWidth / 4) : m_charWidth;
    CreateCaret(m_hwnd, nullptr, caretW, m_lineHeight - 2);
    UpdateCaretShape();
    ShowCaret(m_hwnd);
}

void BinEdit::SetInsertMode(bool insert)
{
    if (m_insertMode == insert) return;
    m_insertMode = insert;
    RecreateCaret();
}

void BinEdit::UpdateCaretShape()
{
    if (!m_hasFocus) return;

    int line = static_cast<int>(m_caretOffset / BYTES_PER_LINE) - m_topLine;
    int col  = static_cast<int>(m_caretOffset % BYTES_PER_LINE);
    int y = m_headerHeight + line * m_lineHeight;
    int x = 0;

    if (m_focusPane == PANE_HEX)
    {
        int gap = (col >= BYTES_PER_LINE / 2) ? 1 : 0;
        x = m_hexColX + (col * 3 + gap) * m_charWidth + (m_caretHiNibble ? 0 : m_charWidth);
    }
    else
    {
        x = m_textColX + col * m_charWidth;
    }
    x -= m_scrollX;

    SetCaretPos(x, y);
    UpdateImeCompositionWindow();
}

void BinEdit::EnsureCaretVisible()
{
    int line = static_cast<int>(m_caretOffset / BYTES_PER_LINE);
    if (line < m_topLine)
        m_topLine = line;
    else if (line >= m_topLine + m_visibleLines)
        m_topLine = line - m_visibleLines + 1;

    m_topLine = __max(0, __min(m_topLine, GetMaxTopLine()));

    int col = static_cast<int>(m_caretOffset % BYTES_PER_LINE);
    int caretX = 0;
    if (m_focusPane == PANE_HEX)
    {
        int gap = (col >= BYTES_PER_LINE / 2) ? 1 : 0;
        caretX = m_hexColX + (col * 3 + gap) * m_charWidth + (m_caretHiNibble ? 0 : m_charWidth);
    }
    else
    {
        caretX = m_textColX + col * m_charWidth;
    }

    const int margin = m_charWidth;
    if (caretX - margin < m_scrollX)
        m_scrollX = __max(0, caretX - margin);
    else if (caretX + margin >= m_scrollX + m_clientWidth)
        m_scrollX = caretX + margin - m_clientWidth;

    m_scrollX = __max(0, __min(m_scrollX, GetMaxScrollX()));

    UpdateScrollInfo();
    UpdateCaretShape();
}

// ===========================================================================
// IME サポート
// ===========================================================================

void BinEdit::UpdateImeCompositionWindow()
{
    HIMC hImc = ImmGetContext(m_hwnd);
    if (!hImc) return;

    int line = static_cast<int>(m_caretOffset / BYTES_PER_LINE) - m_topLine;
    int col  = static_cast<int>(m_caretOffset % BYTES_PER_LINE);

    COMPOSITIONFORM cf;
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = m_textColX + col * m_charWidth - m_scrollX;
    cf.ptCurrentPos.y = m_headerHeight + line * m_lineHeight;

    ImmSetCompositionWindow(hImc, &cf);

    LOGFONTW lf = {};
    if (m_hFont)
        GetObjectW(m_hFont, sizeof(lf), &lf);
    ImmSetCompositionFontW(hImc, &lf);

    ImmReleaseContext(m_hwnd, hImc);
}

LRESULT BinEdit::OnImeStartComposition()
{
    if (!IsEditable() || m_focusPane != PANE_TEXT)
        return DefWindowProcW(m_hwnd, WM_IME_STARTCOMPOSITION, 0, 0);

    UpdateImeCompositionWindow();
    return DefWindowProcW(m_hwnd, WM_IME_STARTCOMPOSITION, 0, 0);
}

LRESULT BinEdit::OnImeComposition(WPARAM wParam, LPARAM lParam)
{
    if (!IsEditable() || m_focusPane != PANE_TEXT)
        return DefWindowProcW(m_hwnd, WM_IME_COMPOSITION, wParam, lParam);

    if (lParam & GCS_RESULTSTR)
    {
        HIMC hImc = ImmGetContext(m_hwnd);
        if (hImc)
        {
            LONG nCount = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, nullptr, 0);
            if (nCount > 0)
            {
                std::vector<WCHAR> buf(nCount / sizeof(WCHAR) + 1, 0);
                ImmGetCompositionStringW(hImc, GCS_RESULTSTR, buf.data(), nCount);

                InsertUnicodeText(buf.data(), static_cast<int>(nCount / sizeof(WCHAR)));
                m_suppressImeChar = true;
            }
            ImmReleaseContext(m_hwnd, hImc);
        }
    }
    return DefWindowProcW(m_hwnd, WM_IME_COMPOSITION, wParam, lParam);
}

LRESULT BinEdit::OnImeEndComposition()
{
    return DefWindowProcW(m_hwnd, WM_IME_ENDCOMPOSITION, 0, 0);
}

LRESULT BinEdit::OnImeSetContext(WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(m_hwnd, WM_IME_SETCONTEXT, wParam, lParam);
}

LRESULT BinEdit::OnImeNotify(WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(m_hwnd, WM_IME_NOTIFY, wParam, lParam);
}

LRESULT BinEdit::OnImeChar(WPARAM wParam, LPARAM lParam)
{
    if (m_suppressImeChar)
    {
        m_suppressImeChar = false;
        return 0;
    }
    return DefWindowProcW(m_hwnd, WM_IME_CHAR, wParam, lParam);
}

// ===========================================================================
// 選択範囲 & クリップボード操作
// ===========================================================================

bool BinEdit::HasSelection() const
{
    return m_anchorOffset != m_caretOffset;
}

void BinEdit::GetSelection(size_t& start, size_t& end) const
{
    start = __min(m_anchorOffset, m_caretOffset);
    end   = __max(m_anchorOffset, m_caretOffset);
}

void BinEdit::SetSelection(size_t start, size_t end)
{
    const size_t dataSize = size();
    m_anchorOffset = __min(start, dataSize);
    m_caretOffset  = __min(end, dataSize);
    m_caretHiNibble = true;
    EnsureCaretVisible();
    InvalidateAll();
}

void BinEdit::ClearSelection()
{
    m_anchorOffset = m_caretOffset;
    InvalidateAll();
}

void BinEdit::ApplySelectionAfterMove(bool extend)
{
    if (!extend)
        m_anchorOffset = m_caretOffset;
}

bool BinEdit::DeleteSelection()
{
    if (!IsEditable() || !HasSelection()) return false;

    size_t start = 0, end = 0;
    GetSelection(start, end);

    DeleteRange(start, end - start);
    m_caretOffset = start;
    m_anchorOffset = start;
    m_caretHiNibble = true;

    // DeleteRange() が既にデコードキャッシュとスクロール情報を更新済みなので二重処理はしない
    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
    NotifyChanged();
    return true;
}

UINT BinEdit::GetBinEditBytesFormat()
{
    static UINT s_fmt = RegisterClipboardFormatW(L"BinEdit_BinaryData");
    return s_fmt;
}

std::wstring BinEdit::BytesToHexString(const BYTE* data, size_t count)
{
    std::wstring str;
    str.reserve(count * 3);
    for (size_t i = 0; i < count; ++i)
    {
        BYTE b = data[i];
        str.push_back(HEX_DIGITS[b >> 4]);
        str.push_back(HEX_DIGITS[b & 0x0F]);
        if (i + 1 < count) str.push_back(L' ');
    }
    return str;
}

bool BinEdit::ParseHexString(const WCHAR* text, data_type& out)
{
    out.clear();
    if (!text) return false;

    std::vector<BYTE> temp;
    int highVal = -1;

    for (const WCHAR* p = text; *p; ++p)
    {
        WCHAR ch = *p;
        int v = HexValue(ch);
        if (v >= 0)
        {
            if (highVal < 0)
            {
                highVal = v;
            }
            else
            {
                temp.push_back(static_cast<BYTE>((highVal << 4) | v));
                highVal = -1;
            }
        }
        else if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n' || ch == L',' || ch == L'-')
        {
            if (highVal >= 0)
            {
                temp.push_back(static_cast<BYTE>(highVal));
                highVal = -1;
            }
        }
    }
    if (highVal >= 0)
        temp.push_back(static_cast<BYTE>(highVal));

    if (temp.empty()) return false;

    out = std::move(temp);
    return true;
}

std::wstring BinEdit::SelectionAsText(size_t start, size_t end) const
{
    std::wstring result;
    if (start >= end || end > size()) return result;

    const BYTE* pData = m_data_src->data();

    if (m_textMode == BinEditTextMode::UTF16)
    {
        size_t count = (end - start) / sizeof(WCHAR);
        if (count > 0)
            result.assign(reinterpret_cast<const WCHAR*>(pData + start), count);
    }
    else
    {
        UINT cp = CP_UTF8;
        if (m_textMode == BinEditTextMode::ANSI) cp = CP_ACP;
        else if (m_textMode == BinEditTextMode::SJIS) cp = CP_SJIS;

        int len = MultiByteToWideChar(cp, 0, reinterpret_cast<const char*>(pData + start), static_cast<int>(end - start), nullptr, 0);
        if (len > 0)
        {
            result.resize(len);
            MultiByteToWideChar(cp, 0, reinterpret_cast<const char*>(pData + start), static_cast<int>(end - start), &result[0], len);
        }
    }
    return result;
}

bool BinEdit::SetClipboardBytes(const BYTE* data, size_t count, bool alsoHexText)
{
    if (!OpenClipboard(m_hwnd)) return false;
    EmptyClipboard();

    UINT customFmt = GetBinEditBytesFormat();
    if (HGLOBAL hBin = GlobalAlloc(GMEM_MOVEABLE, count))
    {
        if (void* ptr = GlobalLock(hBin))
        {
            memcpy(ptr, data, count);
            GlobalUnlock(hBin);
            SetClipboardData(customFmt, hBin);
        }
    }

    std::wstring hexText = BytesToHexString(data, count);
    if (HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE, (hexText.size() + 1) * sizeof(WCHAR)))
    {
        if (auto* ptr = static_cast<WCHAR*>(GlobalLock(hText)))
        {
            StringCchCopyW(ptr, hexText.size() + 1, hexText.c_str());
            GlobalUnlock(hText);
            SetClipboardData(CF_UNICODETEXT, hText);
        }
    }

    CloseClipboard();
    return true;
}

bool BinEdit::SetClipboardUnicodeText(const std::wstring& text)
{
    if (!OpenClipboard(m_hwnd)) return false;
    EmptyClipboard();

    if (HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(WCHAR)))
    {
        if (auto* ptr = static_cast<WCHAR*>(GlobalLock(hText)))
        {
            StringCchCopyW(ptr, text.size() + 1, text.c_str());
            GlobalUnlock(hText);
            SetClipboardData(CF_UNICODETEXT, hText);
        }
    }

    CloseClipboard();
    return true;
}

bool BinEdit::Copy()
{
    if (!HasSelection()) return false;

    size_t start = 0, end = 0;
    GetSelection(start, end);
    size_t count = end - start;

    if (m_focusPane == PANE_TEXT)
    {
        std::wstring text = SelectionAsText(start, end);
        return SetClipboardUnicodeText(text);
    }
    else
    {
        return SetClipboardBytes(m_data_src->data() + start, count, true);
    }
}

bool BinEdit::Cut()
{
    if (!IsEditable() || !HasSelection()) return false;
    if (Copy())
    {
        DeleteSelection();
        return true;
    }
    return false;
}

bool BinEdit::Paste()
{
    if (!IsEditable()) return false;

    if (!OpenClipboard(m_hwnd)) return false;

    data_type pasteData;
    bool handled = false;

    UINT customFmt = GetBinEditBytesFormat();
    if (IsClipboardFormatAvailable(customFmt))
    {
        if (HANDLE hBin = GetClipboardData(customFmt))
        {
            if (const BYTE* ptr = static_cast<const BYTE*>(GlobalLock(hBin)))
            {
                SIZE_T sz = GlobalSize(hBin);
                pasteData.assign(ptr, ptr + sz);
                GlobalUnlock(hBin);
                handled = true;
            }
        }
    }

    if (!handled && IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        if (HANDLE hText = GetClipboardData(CF_UNICODETEXT))
        {
            if (const WCHAR* ptr = static_cast<const WCHAR*>(GlobalLock(hText)))
            {
                if (m_focusPane == PANE_HEX)
                {
                    handled = ParseHexString(ptr, pasteData);
                }
                if (!handled)
                {
                    EncodeTextToBytes(ptr, static_cast<int>(wcslen(ptr)), pasteData);
                    handled = true;
                }
                GlobalUnlock(hText);
            }
        }
    }

    CloseClipboard();

    if (!handled || pasteData.empty()) return false;

    if (HasSelection()) DeleteSelection();

    ReplaceRange(m_caretOffset, 0, pasteData.data(), static_cast<size_t>(pasteData.size()));
    m_caretOffset += static_cast<size_t>(pasteData.size());
    m_anchorOffset = m_caretOffset;
    m_caretHiNibble = true;

    RebuildDecodeCache();
    UpdateScrollInfo();
    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
    NotifyChanged();
    return true;
}

// ===========================================================================
// バイト配列編集・挿入/削除＆テキストエンコード
// ===========================================================================

void BinEdit::InsertByteAt(size_t pos, BYTE value)
{
    if (size() >= m_maxLen) return; // 上限に達しているので挿入しない
    pos = __min(pos, size());
    m_data_src->insert(begin() + pos, value);
    UpdateDecodeCacheAfterEdit(pos, 0, 1);
    UpdateScrollInfo();
}

void BinEdit::DeleteByteAt(size_t pos)
{
    if (pos >= size()) return;
    if (size() <= m_minLen) return; // 下限を下回る削除は行わない
    m_data_src->erase(begin() + pos);
    UpdateDecodeCacheAfterEdit(pos, 1, 0);
    UpdateScrollInfo();
}

void BinEdit::DeleteRange(size_t pos, size_t count)
{
    if (pos >= size() || count == 0) return;
    size_t realCount = __min(count, size() - pos);

    // 下限を下回らないように削除量を調整する
    if (size() - realCount < m_minLen)
        realCount = (size() > m_minLen) ? (size() - m_minLen) : 0;
    if (realCount == 0) return;

    m_data_src->erase(begin() + pos, begin() + pos + realCount);
    UpdateDecodeCacheAfterEdit(pos, realCount, 0);
    UpdateScrollInfo();
}

void BinEdit::ReplaceRange(size_t pos, size_t oldCount, const BYTE* newBytes, size_t newCount)
{
    pos = __min(pos, size());
    size_t realOld = __min(oldCount, size() - pos);

    // 置換後のサイズが上限を超える場合は挿入するバイト数を減らす
    size_t resultSize = size() - realOld + newCount;
    if (resultSize > m_maxLen)
    {
        size_t over = resultSize - m_maxLen;
        newCount = (newCount > over) ? (newCount - over) : 0;
        resultSize = size() - realOld + newCount;
    }
    // 置換後のサイズが下限を下回る場合は削除するバイト数を減らす
    if (resultSize < m_minLen)
    {
        size_t shortage = m_minLen - resultSize;
        realOld = (realOld > shortage) ? (realOld - shortage) : 0;
    }

    m_data_src->erase(begin() + pos, begin() + pos + realOld);
    if (newBytes && newCount > 0)
    {
        m_data_src->insert(begin() + pos, newBytes, newBytes + newCount);
    }
    UpdateDecodeCacheAfterEdit(pos, realOld, newCount);
    UpdateScrollInfo();
}

void BinEdit::EncodeCharToBytes(WCHAR ch, data_type& outBytes) const
{
    EncodeTextToBytes(&ch, 1, outBytes);
}

void BinEdit::EncodeTextToBytes(const WCHAR* text, int cch, data_type& out) const
{
    out.clear();
    if (!text || cch <= 0) return;

    if (m_textMode == BinEditTextMode::UTF16)
    {
        const BYTE* p = reinterpret_cast<const BYTE*>(text);
        out.assign(p, p + cch * sizeof(WCHAR));
        return;
    }

    UINT cp = CP_UTF8;
    if (m_textMode == BinEditTextMode::ANSI) cp = CP_ACP;
    else if (m_textMode == BinEditTextMode::SJIS) cp = CP_SJIS;

    int len = WideCharToMultiByte(cp, 0, text, cch, nullptr, 0, nullptr, nullptr);
    if (len > 0)
    {
        out.resize(len);
        WideCharToMultiByte(cp, 0, text, cch, reinterpret_cast<char*>(out.data()), len, nullptr, nullptr);
    }
}

bool BinEdit::InsertUnicodeText(const WCHAR* text, int cch)
{
    if (!IsEditable() || !text || cch <= 0) return false;

    if (HasSelection()) DeleteSelection();

    data_type bytes;
    EncodeTextToBytes(text, cch, bytes);
    if (bytes.empty()) return false;

    ReplaceRange(m_caretOffset, 0, bytes.data(), static_cast<size_t>(bytes.size()));
    m_caretOffset += static_cast<size_t>(bytes.size());
    m_anchorOffset = m_caretOffset;
    m_caretHiNibble = true;

    // ReplaceRange() が既にデコードキャッシュとスクロール情報を更新済みなので二重処理はしない
    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
    NotifyChanged();
    return true;
}

// ===========================================================================
// キーボード移動 & 編集操作 (OnKey, OnChar)
// ===========================================================================

void BinEdit::MoveCaretBy(ptrdiff_t deltaBytes)
{
    ptrdiff_t newOff = static_cast<ptrdiff_t>(m_caretOffset) + deltaBytes;
    newOff = __max(static_cast<ptrdiff_t>(0), __min(newOff, static_cast<ptrdiff_t>(size())));
    m_caretOffset = static_cast<size_t>(newOff);
    m_caretHiNibble = true;
}

void BinEdit::MoveCaretHome(bool wholeBuffer)
{
    m_caretOffset = wholeBuffer ? 0 : (m_caretOffset - m_caretOffset % BYTES_PER_LINE);
    m_caretHiNibble = true;
}

void BinEdit::MoveCaretEnd(bool wholeBuffer)
{
    if (wholeBuffer)
    {
        m_caretOffset = size();
    }
    else
    {
        size_t lineStart = m_caretOffset - m_caretOffset % BYTES_PER_LINE;
        m_caretOffset = __min(lineStart + BYTES_PER_LINE, size());
    }
    m_caretHiNibble = true;
}

void BinEdit::OnKey(HWND hwnd, UINT vk, BOOL fDown, int /*cRepeat*/, UINT /*flags*/)
{
    const bool ctrl  = (GetKeyState(VK_CONTROL) < 0);
    const bool shift = (GetKeyState(VK_SHIFT) < 0);
    bool changed = false;
    bool moved = false;

    switch (vk)
    {
    case VK_LEFT:
        if (shift)
        {
            MoveCaretBy(-1);
        }
        else if (HasSelection())
        {
            size_t start, end;
            GetSelection(start, end);
            m_caretOffset = start;
            m_caretHiNibble = true;
        }
        else if (m_focusPane == PANE_HEX)
        {
            if (m_caretHiNibble)
            {
                MoveCaretBy(-1);
                m_caretHiNibble = false;
            }
            else
            {
                m_caretHiNibble = true;
            }
        }
        else
        {
            MoveCaretBy(-1);
        }
        moved = true;
        break;

    case VK_RIGHT:
        if (shift)
        {
            MoveCaretBy(1);
        }
        else if (HasSelection())
        {
            size_t start, end;
            GetSelection(start, end);
            m_caretOffset = end;
            m_caretHiNibble = true;
        }
        else if (m_focusPane == PANE_HEX)
        {
            if (m_caretHiNibble)
            {
                m_caretHiNibble = false;
            }
            else
            {
                MoveCaretBy(1);
                m_caretHiNibble = true;
            }
        }
        else
        {
            MoveCaretBy(static_cast<ptrdiff_t>(DecodedLengthAt(m_caretOffset)));
        }
        moved = true;
        break;

    case VK_UP:    MoveCaretBy(-BYTES_PER_LINE); moved = true; break;
    case VK_DOWN:  MoveCaretBy(BYTES_PER_LINE);  moved = true; break;
    case VK_PRIOR: MoveCaretBy(-BYTES_PER_LINE * m_visibleLines); moved = true; break;
    case VK_NEXT:  MoveCaretBy(BYTES_PER_LINE * m_visibleLines);  moved = true; break;
    case VK_HOME:  MoveCaretHome(ctrl); moved = true; break;
    case VK_END:   MoveCaretEnd(ctrl);  moved = true; break;

    case VK_TAB:
        m_focusPane = (m_focusPane == PANE_HEX) ? PANE_TEXT : PANE_HEX;
        m_caretHiNibble = true;
        break;

    case 'A':
        if (ctrl)
        {
            m_anchorOffset = 0;
            m_caretOffset = size();
            m_caretHiNibble = true;
            EnsureCaretVisible();
            UpdateCaretShape();
            InvalidateAll();
            return;
        }
        return;

    case 'C':
        if (ctrl) { Copy(); return; }
        return;

    case 'X':
        if (ctrl) { if (Cut()) NotifyChanged(); return; }
        return;

    case 'V':
        if (ctrl) { if (Paste()) NotifyChanged(); return; }
        return;

    case 'G':
        if (ctrl) { GoToOffsetDialog(); return; }
        return;

    case VK_INSERT:
        if (ctrl)  { Copy(); return; }
        if (shift) { if (Paste()) NotifyChanged(); return; }
        m_insertMode = !m_insertMode;
        RecreateCaret();
        break;

    case VK_DELETE:
        if (shift) { if (Cut()) NotifyChanged(); return; }
        if (DeleteSelection())
        {
            changed = true;
        }
        else if (IsEditable() && m_caretOffset < size())
        {
            size_t len = (m_focusPane == PANE_TEXT) ? DecodedLengthAt(m_caretOffset) : 1;
            DeleteRange(m_caretOffset, len);
            m_caretHiNibble = true;
            m_anchorOffset = m_caretOffset;
            changed = true;
        }
        break;

    case VK_BACK:
        if (DeleteSelection())
        {
            changed = true;
        }
        else if (IsEditable() && m_caretOffset > 0)
        {
            DeleteByteAt(m_caretOffset - 1);
            m_caretOffset -= 1;
            m_caretHiNibble = true;
            m_anchorOffset = m_caretOffset;
            changed = true;
        }
        break;

    default:
        return;
    }

    if (moved) ApplySelectionAfterMove(shift);

    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
    if (changed) NotifyChanged();
}

void BinEdit::OnChar(HWND hwnd, TCHAR ch, int /*cRepeat*/)
{
    if (m_suppressImeChar)
    {
        m_suppressImeChar = false;
        return;
    }

    if (ch < 0x20 || ch == 27 || GetKeyState(VK_CONTROL) < 0 || GetKeyState(VK_MENU) < 0)
        return;
    if (!IsEditable())
        return;

    if (HasSelection())
    {
        DeleteSelection();
    }

    if (m_focusPane == PANE_HEX)
    {
        int v = HexValue(ch);
        if (v < 0) return;

        const size_t editOffset = m_caretOffset;
        if (m_caretOffset >= size() || (m_insertMode && m_caretHiNibble))
        {
            InsertByteAt(m_caretOffset, 0);
        }

        BYTE& b = (*m_data_src)[editOffset];
        if (m_caretHiNibble)
        {
            b = static_cast<BYTE>((b & 0x0F) | (v << 4));
            m_caretHiNibble = false;
        }
        else
        {
            b = static_cast<BYTE>((b & 0xF0) | v);
            m_caretHiNibble = true;
            MoveCaretBy(1);
        }
        m_anchorOffset = m_caretOffset;
        // ニブルの書き換えはバイト長を変えない in-place 編集なので、
        // バッファ全体ではなく該当バイト周辺だけを再デコードする。
        UpdateDecodeCacheAfterEdit(editOffset, 1, 1);
    }
    else
    {
        WCHAR wch = static_cast<WCHAR>(ch);
        InsertUnicodeText(&wch, 1);
        return;
    }

    EnsureCaretVisible();
    UpdateCaretShape();
    InvalidateAll();
    NotifyChanged();
}
