// MPointSizeRect.hpp -- Win32API point, size and rectangle     -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MPOINTSIZERECT_HPP_
#define MZC4_MPOINTSIZERECT_HPP_    4   /* Version 4 */

class MPoint;
class MSize;
class MRect;
//VOID NormalizeRectDx(LPRECT prc);

////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif
#ifndef _INC_WINDOWSX
    #include <windowsx.h>   // macro API
#endif
#include <cassert>          // assert

////////////////////////////////////////////////////////////////////////////

inline VOID GetScreenRectDx(LPRECT prc)
{
#ifndef SM_XVIRTUALSCREEN
    #define SM_XVIRTUALSCREEN   76
    #define SM_YVIRTUALSCREEN   77
    #define SM_CXVIRTUALSCREEN  78
    #define SM_CYVIRTUALSCREEN  79
#endif
    INT x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    INT y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    INT cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    INT cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (cx)
    {
        SetRect(prc, x, y, x + cx, y + cy);
    }
    else
    {
        cx = GetSystemMetrics(SM_CXSCREEN);
        cy = GetSystemMetrics(SM_CYSCREEN);
        SetRect(prc, 0, 0, cx, cy);
    }
}

////////////////////////////////////////////////////////////////////////////

class MPoint : public POINT
{
public:
    MPoint();
    MPoint(INT x_, INT y_);
    MPoint(POINT pt);
    MPoint(SIZE siz);
    MPoint(DWORD dwPoint);
    VOID    Offset(INT dx, INT dy);
    VOID    Offset(POINT pt);
    VOID    Offset(SIZE siz);
    operator LPPOINT();
    operator const POINT *() const;
    BOOL    operator==(POINT pt) const;
    BOOL    operator!=(POINT pt) const;
    VOID    operator+=(SIZE siz);
    VOID    operator-=(SIZE siz);
    VOID    operator+=(POINT pt);
    VOID    operator-=(POINT pt);
    VOID    SetPoint(INT x_, INT y_);
    MPoint  operator+(SIZE siz) const;
    MPoint  operator-(SIZE siz) const;
    MPoint  operator-() const;
    MPoint  operator+(POINT pt) const;
    MSize   operator-(POINT pt) const;
    MRect   operator+(LPCRECT prc) const;
    MRect   operator-(LPCRECT prc) const;
};

////////////////////////////////////////////////////////////////////////////

class MSize : public SIZE
{
public:
    MSize();
    MSize(INT cx_, INT cy_);
    MSize(SIZE siz);
    MSize(POINT pt);
    MSize(DWORD dwSize);
    operator LPSIZE();
    operator const SIZE *() const;
    BOOL    operator==(SIZE siz) const;
    BOOL    operator!=(SIZE siz) const;
    VOID    operator+=(SIZE siz);
    VOID    operator-=(SIZE siz);
    VOID    SetSize(INT cx_, INT cy_);
    MSize   operator+(SIZE siz) const;
    MSize   operator-(SIZE siz) const;
    MSize   operator-() const;
    MPoint  operator+(POINT pt) const;
    MPoint  operator-(POINT pt) const;
    MRect   operator+(LPCRECT prc) const;
    MRect   operator-(LPCRECT prc) const;
};

////////////////////////////////////////////////////////////////////////////

class MRect : public RECT
{
public:
    MRect();
    MRect(INT l, INT t, INT r, INT b);
    MRect(const RECT& rcSrc);
    MRect(LPCRECT lpSrcRect);
    MRect(POINT pt, SIZE siz);
    MRect(POINT topLeft, POINT bottomRight);
    operator LPRECT();
    operator LPCRECT() const;

    INT   Width() const;
    INT   Height() const;
    MSize Size() const;

    MPoint&       TopLeft();
    const MPoint& TopLeft() const;
    MPoint&       BottomRight();
    const MPoint& BottomRight() const;
    MPoint        CenterPoint() const;

    BOOL IsRectEmpty() const;
    BOOL IsRectNull() const;
    BOOL PtInRect(POINT pt) const;
    VOID SetRect(INT x1, INT y1, INT x2, INT y2);
    VOID SetRect(POINT topLeft, POINT bottomRight);
    VOID SetRectEmpty();
    VOID CopyRect(LPCRECT lpSrcRect);
    BOOL EqualRect(LPCRECT prc) const;
    VOID InflateRect(INT x, INT y);
    VOID InflateRect(SIZE siz);
    VOID InflateRect(LPCRECT prc);
    VOID InflateRect(INT l, INT t, INT r, INT b);
    VOID DeflateRect(INT x, INT y);
    VOID DeflateRect(SIZE siz);
    VOID DeflateRect(LPCRECT prc);
    VOID DeflateRect(INT l, INT t, INT r, INT b);
    VOID OffsetRect(INT x, INT y);
    VOID OffsetRect(SIZE siz);
    VOID OffsetRect(POINT pt);
    VOID NormalizeRect();
    VOID MoveToX(INT x);
    VOID MoveToY(INT y);
    VOID MoveToXY(INT x, INT y);
    VOID MoveToXY(POINT pt);
    BOOL IntersectRect(LPCRECT prc1, LPCRECT prc2);
    BOOL UnionRect(LPCRECT prc1, LPCRECT prc2);
    BOOL SubtractRect(LPCRECT prcSrc1, LPCRECT prcSrc2);

    VOID operator=(const RECT& rcSrc);
    BOOL operator==(const RECT& rc) const;
    BOOL operator!=(const RECT& rc) const;
    VOID operator+=(POINT pt);
    VOID operator+=(SIZE siz);
    VOID operator+=(LPCRECT prc);
    VOID operator-=(POINT pt);
    VOID operator-=(SIZE siz);
    VOID operator-=(LPCRECT prc);
    VOID operator&=(const RECT& rc);
    VOID operator|=(const RECT& rc);
    MRect operator+(POINT pt) const;
    MRect operator-(POINT pt) const;
    MRect operator+(LPCRECT prc) const;
    MRect operator+(SIZE siz) const;
    MRect operator-(SIZE siz) const;
    MRect operator-(LPCRECT prc) const;
    MRect operator&(const RECT& rc2) const;
    MRect operator|(const RECT& rc2) const;
    MRect MulDiv(INT nMultiplier, INT nDivisor) const;
};

VOID NormalizeRectDx(LPRECT prc);

////////////////////////////////////////////////////////////////////////////

inline MPoint::MPoint()
    { x = y = 0; }

inline MPoint::MPoint(INT x_, INT y_)
    { x = x_; y = y_; }

inline MPoint::MPoint(POINT pt)
    { *reinterpret_cast<POINT *>(this) = pt; }

inline MPoint::MPoint(SIZE siz)
    { *reinterpret_cast<SIZE *>(this) = siz; }

inline MPoint::MPoint(DWORD dwPoint)
    { x = GET_X_LPARAM(dwPoint); y = GET_Y_LPARAM(dwPoint); }

inline VOID MPoint::Offset(INT dx, INT dy)
    { x += dx; y += dy; }

inline VOID MPoint::Offset(POINT pt)
    { x += pt.x; y += pt.y; }

inline VOID MPoint::Offset(SIZE siz)
    { x += siz.cx; y += siz.cy; }

inline MPoint::operator LPPOINT()
    { return reinterpret_cast<LPPOINT>(this); }

inline MPoint::operator const POINT *() const
    { return reinterpret_cast<const POINT *>(this); }

inline BOOL MPoint::operator==(POINT pt) const
    { return (x == pt.x && y == pt.y); }

inline BOOL MPoint::operator!=(POINT pt) const
    { return (x != pt.x || y != pt.y); }

inline VOID MPoint::operator+=(SIZE siz)
    { x += siz.cx; y += siz.cy; }

inline VOID MPoint::operator-=(SIZE siz)
    { x -= siz.cx; y -= siz.cy; }

inline VOID MPoint::operator+=(POINT pt)
    { x += pt.x; y += pt.y; }

inline VOID MPoint::operator-=(POINT pt)
    { x -= pt.x; y -= pt.y; }

inline VOID MPoint::SetPoint(INT x_, INT y_)
    { x = x_; y = y_; }

inline MPoint MPoint::operator+(SIZE siz) const
    { return MPoint(x + siz.cx, y + siz.cy); }

inline MPoint MPoint::operator-(SIZE siz) const
    { return MPoint(x - siz.cx, y - siz.cy); }

inline MPoint MPoint::operator-() const
    { return MPoint(-x, -y); }

inline MPoint MPoint::operator+(POINT pt) const
    { return MPoint(x + pt.x, y + pt.y); }

inline MSize MPoint::operator-(POINT pt) const
    { return MSize(x - pt.x, y - pt.y); }

inline MRect MPoint::operator+(LPCRECT prc) const
    { return MRect(prc) + *this; }

inline MRect MPoint::operator-(LPCRECT prc) const
    { return MRect(prc) - *this; }

////////////////////////////////////////////////////////////////////////////

inline MSize::MSize()
    { cx = cy = 0; }

inline MSize::MSize(INT cx_, INT cy_)
    { cx = cx_; cy = cy_; }

inline MSize::MSize(SIZE siz)
    { *reinterpret_cast<SIZE *>(this) = siz; }

inline MSize::MSize(POINT pt)
    { *reinterpret_cast<POINT *>(this) = pt; }

inline MSize::MSize(DWORD dwSize)
    { cx = GET_X_LPARAM(dwSize); cy = GET_Y_LPARAM(dwSize); }

inline MSize::operator LPSIZE()
    { return reinterpret_cast<LPSIZE>(this); }

inline MSize::operator const SIZE *() const
    { return reinterpret_cast<const SIZE *>(this); }

inline BOOL MSize::operator==(SIZE siz) const
    { return (cx == siz.cx && cy == siz.cy); }

inline BOOL MSize::operator!=(SIZE siz) const
    { return (cx != siz.cx || cy != siz.cy); }

inline VOID MSize::operator+=(SIZE siz)
    { cx += siz.cx; cy += siz.cy; }

inline VOID MSize::operator-=(SIZE siz)
    { cx -= siz.cx; cy -= siz.cy; }

inline VOID MSize::SetSize(INT cx_, INT cy_)
    { cx = cx_; cy = cy_; }

inline MSize MSize::operator+(SIZE siz) const
    { return MSize(cx + siz.cx, cy + siz.cy); }

inline MSize MSize::operator-(SIZE siz) const
    { return MSize(cx - siz.cx, cy - siz.cy); }

inline MSize MSize::operator-() const
    { return MSize(-cx, -cy); }

inline MPoint MSize::operator+(POINT pt) const
    { return MPoint(cx + pt.x, cy + pt.y); }

inline MPoint MSize::operator-(POINT pt) const
    { return MPoint(cx - pt.x, cy - pt.y); }

inline MRect MSize::operator+(LPCRECT prc) const
    { return MRect(prc) + *this; }

inline MRect MSize::operator-(LPCRECT prc) const
    { return MRect(prc) - *this; }

template <class Number>
inline MSize operator*(SIZE s, Number n)
    { return MSize((INT)(s.cx * n), (INT)(s.cy * n)); }

template <class Number>
inline VOID operator*=(SIZE & s, Number n)
    { s = s * n; }

template <class Number>
inline MSize operator/(SIZE s, Number n)
    { return MSize((INT)(s.cx / n), (INT)(s.cy / n)); }

template <class Number>
inline VOID operator/=(SIZE & s, Number n)
    { s = s / n; }

////////////////////////////////////////////////////////////////////////////

inline MRect::MRect()
    { left = top = right = bottom = 0; }

inline MRect::MRect(INT l, INT t, INT r, INT b)
    { left = l; top = t; right = r; bottom = b; }

inline MRect::MRect(const RECT& rcSrc)
    { ::CopyRect(this, &rcSrc); }

inline MRect::MRect(LPCRECT lpSrcRect)
    { ::CopyRect(this, lpSrcRect); }

inline MRect::MRect(POINT pt, SIZE siz)
{
    right = (left = pt.x) + siz.cx;
    bottom = (top = pt.y) + siz.cy;
}

inline MRect::MRect(POINT topLeft, POINT bottomRight)
{
    left = topLeft.x;
    top = topLeft.y;
    right = bottomRight.x;
    bottom = bottomRight.y;
}

inline VOID MRect::InflateRect(LPCRECT prc)
{
    left -= prc->left;
    top -= prc->top;
    right += prc->right;
    bottom += prc->bottom;
}

inline VOID MRect::InflateRect(INT l, INT t, INT r, INT b)
{
    left -= l;
    top -= t;
    right += r;
    bottom += b;
}

inline VOID MRect::DeflateRect(LPCRECT prc)
{
    left += prc->left;
    top += prc->top;
    right -= prc->right;
    bottom -= prc->bottom;
}

inline VOID MRect::DeflateRect(INT l, INT t, INT r, INT b)
{
    left += l;
    top += t;
    right -= r;
    bottom -= b;
}

inline VOID MRect::NormalizeRect()
{
    INT nTemp;
    if (left > right)
    {
        nTemp = left;
        left = right;
        right = nTemp;
    }
    if (top > bottom)
    {
        nTemp = top;
        top = bottom;
        bottom = nTemp;
    }
}

inline MRect MRect::operator+(POINT pt) const
{
    MRect rc(*this);
    ::OffsetRect(&rc, pt.x, pt.y);
    return rc;
}

inline MRect MRect::operator-(POINT pt) const
{
    MRect rc(*this);
    ::OffsetRect(&rc, -pt.x, -pt.y);
    return rc;
}

inline MRect MRect::operator+(LPCRECT prc) const
{
    MRect rc(this);
    rc.InflateRect(prc);
    return rc;
}

inline MRect MRect::operator+(SIZE siz) const
{
    MRect rc(*this);
    ::OffsetRect(&rc, siz.cx, siz.cy);
    return rc;
}

inline MRect MRect::operator-(SIZE siz) const
{
    MRect rc(*this);
    ::OffsetRect(&rc, -siz.cx, -siz.cy);
    return rc;
}

inline MRect MRect::operator-(LPCRECT prc) const
{
    MRect rc(this);
    rc.DeflateRect(prc);
    return rc;
}

inline MRect MRect::operator&(const RECT& rc2) const
{
    MRect rc;
    ::IntersectRect(&rc, this, &rc2);
    return rc;
}

inline MRect MRect::operator|(const RECT& rc2) const
{
    MRect rc;
    ::UnionRect(&rc, this, &rc2);
    return rc;
}

inline MRect MRect::MulDiv(INT nMultiplier, INT nDivisor) const
{
    return MRect(
        ::MulDiv(left, nMultiplier, nDivisor),
        ::MulDiv(top, nMultiplier, nDivisor),
        ::MulDiv(right, nMultiplier, nDivisor),
        ::MulDiv(bottom, nMultiplier, nDivisor));
}

inline INT MRect::Width() const
    { return right - left; }

inline INT MRect::Height() const
    { return bottom - top; }

inline MSize MRect::Size() const
    { return MSize(right - left, bottom - top); }

inline MPoint& MRect::TopLeft()
    { return *((MPoint*) this); }

inline MPoint& MRect::BottomRight()
    { return *((MPoint*) this + 1); }

inline const MPoint& MRect::TopLeft() const
    { return *((MPoint*) this); }

inline const MPoint& MRect::BottomRight() const
    { return *((MPoint*) this + 1); }

inline MPoint MRect::CenterPoint() const
    { return MPoint((left + right) / 2, (top + bottom) / 2); }

inline MRect::operator LPRECT()
    { return this; }

inline MRect::operator LPCRECT() const
    { return this; }

inline BOOL MRect::IsRectEmpty() const
    { return ::IsRectEmpty(this); }

inline BOOL MRect::IsRectNull() const
    { return (left == 0 && right == 0 && top == 0 && bottom == 0); }

inline BOOL MRect::PtInRect(POINT pt) const
    { return ::PtInRect(this, pt); }

inline VOID MRect::SetRect(INT x1, INT y1, INT x2, INT y2)
    { ::SetRect(this, x1, y1, x2, y2); }

inline VOID MRect::SetRect(POINT topLeft, POINT bottomRight)
{
    ::SetRect(this, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
}

inline VOID MRect::SetRectEmpty()
    { ::SetRectEmpty(this); }

inline VOID MRect::CopyRect(LPCRECT lpSrcRect)
    { ::CopyRect(this, lpSrcRect); }

inline BOOL MRect::EqualRect(LPCRECT prc) const
    { return ::EqualRect(this, prc); }

inline VOID MRect::InflateRect(INT x, INT y)
    { ::InflateRect(this, x, y); }

inline VOID MRect::InflateRect(SIZE siz)
    { ::InflateRect(this, siz.cx, siz.cy); }

inline VOID MRect::DeflateRect(INT x, INT y)
    { ::InflateRect(this, -x, -y); }

inline VOID MRect::DeflateRect(SIZE siz)
    { ::InflateRect(this, -siz.cx, -siz.cy); }

inline VOID MRect::OffsetRect(INT x, INT y)
    { ::OffsetRect(this, x, y); }

inline VOID MRect::OffsetRect(SIZE siz)
    { ::OffsetRect(this, siz.cx, siz.cy); }

inline VOID MRect::OffsetRect(POINT pt)
    { ::OffsetRect(this, pt.x, pt.y); }

inline VOID MRect::MoveToX(INT x)
    { right = Width() + x; left = x; }

inline VOID MRect::MoveToY(INT y)
    { bottom = Height() + y; top = y; }

inline VOID MRect::MoveToXY(INT x, INT y)
    { MoveToX(x); MoveToY(y); }

inline VOID MRect::MoveToXY(POINT pt)
    { MoveToX(pt.x); MoveToY(pt.y); }

inline BOOL MRect::IntersectRect(LPCRECT prc1, LPCRECT prc2)
    { return ::IntersectRect(this, prc1, prc2); }

inline BOOL MRect::UnionRect(LPCRECT prc1, LPCRECT prc2)
    { return ::UnionRect(this, prc1, prc2); }

inline BOOL MRect::SubtractRect(LPCRECT prcSrc1, LPCRECT prcSrc2)
    { return ::SubtractRect(this, prcSrc1, prcSrc2); }

inline VOID MRect::operator=(const RECT& rcSrc)
    { ::CopyRect(this, &rcSrc); }

inline BOOL MRect::operator==(const RECT& rc) const
    { return ::EqualRect(this, &rc); }

inline BOOL MRect::operator!=(const RECT& rc) const
    { return !::EqualRect(this, &rc); }

inline VOID MRect::operator+=(POINT pt)
    { ::OffsetRect(this, pt.x, pt.y); }

inline VOID MRect::operator+=(SIZE siz)
    { ::OffsetRect(this, siz.cx, siz.cy); }

inline VOID MRect::operator+=(LPCRECT prc)
    { InflateRect(prc); }

inline VOID MRect::operator-=(POINT pt)
    { ::OffsetRect(this, -pt.x, -pt.y); }

inline VOID MRect::operator-=(SIZE siz)
    { ::OffsetRect(this, -siz.cx, -siz.cy); }

inline VOID MRect::operator-=(LPCRECT prc)
    { DeflateRect(prc); }

inline VOID MRect::operator&=(const RECT& rc)
    { ::IntersectRect(this, this, &rc); }

inline VOID MRect::operator|=(const RECT& rc)
    { ::UnionRect(this, this, &rc); }

inline VOID NormalizeRectDx(LPRECT prc)
{
    INT nTemp;
    if (prc->left > prc->right)
    {
        nTemp = prc->left;
        prc->left = prc->right;
        prc->right = nTemp;
    }
    if (prc->top > prc->bottom)
    {
        nTemp = prc->top;
        prc->top = prc->bottom;
        prc->bottom = nTemp;
    }
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MPOINTSIZERECT_HPP_
