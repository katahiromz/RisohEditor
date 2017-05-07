#include "stdafx.hpp"

#define II_WIDTHBYTES(i) (((i) + 31) / 32 * 4)

typedef struct II_MEMORY
{
    const png_byte *    m_pb;           /* pointer to memory */
    png_uint_32         m_i;            /* reading position */
    png_uint_32         m_size;         /* memory size */
    void *              p_user;         /* user data pointer */
    size_t              i_user;         /* user data integer */
    size_t              i_user_2;       /* user data integer 2nd */
} II_MEMORY;

HBITMAP
ii_png_load_common(FILE *inf, float *dpi)
{
    HBITMAP         hbm;
    png_structp     png;
    png_infop       info;
    png_uint_32     y, width, height, rowbytes;
    int             color_type, depth, widthbytes;
    double          gamma;
    BITMAPINFO      bi;
    png_byte      *pbBits;
    png_uint_32     res_x, res_y;
    int             unit_type;
    png_bytepp      rows;
    HDC             hdc;

    assert(inf);
    if (inf == NULL)
        return NULL;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info = png_create_info_struct(png);
    if (png == NULL || info == NULL || setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(inf);
        return NULL;
    }

    png_init_io(png, inf);
    png_read_info(png, info);

    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);
    png_set_strip_16(png);
    png_set_gray_to_rgb(png);
    png_set_palette_to_rgb(png);
    png_set_bgr(png);
    png_set_packing(png);
    png_set_interlace_handling(png);
    if (png_get_gAMA(png, info, &gamma))
        png_set_gamma(png, 2.2, gamma);
    else
        png_set_gamma(png, 2.2, 0.45455);

    png_read_update_info(png, info);
    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);

    if (dpi)
    {
        *dpi = 0.0;
        if (png_get_pHYs(png, info, &res_x, &res_y, &unit_type))
        {
            if (unit_type == PNG_RESOLUTION_METER)
                *dpi = (float)(res_x * 2.54 / 100.0);
        }
    }

    rowbytes = (png_uint_32)png_get_rowbytes(png, info);
    rows = (png_bytepp)malloc(height * sizeof(png_bytep));
    for (y = 0; y < height; y++)
    {
        rows[y] = (png_bytep)malloc(rowbytes);
    }

    png_read_image(png, rows);
    png_read_end(png, NULL);
    fclose(inf);

    ZeroMemory(&bi.bmiHeader, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = width;
    bi.bmiHeader.biHeight      = height;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = depth * png_get_channels(png, info);

    hdc = CreateCompatibleDC(NULL);
    hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (void **)&pbBits,
                           NULL, 0);
    DeleteDC(hdc);
    if (hbm == NULL)
    {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    widthbytes = II_WIDTHBYTES(width * bi.bmiHeader.biBitCount);
    for (y = 0; y < height; y++)
    {
        memcpy(pbBits + y * widthbytes, rows[height - 1 - y], rowbytes);
    }

    png_destroy_read_struct(&png, &info, NULL);
    free(rows);
    return hbm;
}

HBITMAP
ii_png_load_a(const char *pszFileName, float *dpi)
{
    using namespace std;
    FILE            *inf;
    inf = fopen(pszFileName, "rb");
    if (inf)
        return ii_png_load_common(inf, dpi);
    return NULL;
}

HBITMAP
ii_png_load_w(const wchar_t *pszFileName, float *dpi)
{
    using namespace std;
    FILE            *inf;
    inf = _wfopen(pszFileName, L"rb");
    if (inf)
        return ii_png_load_common(inf, dpi);
    return NULL;
}

static void
ii_png_mem_read(png_structp png, png_bytep data, png_size_t length)
{
    II_MEMORY *memory;
    assert(png);
    memory = (II_MEMORY *)png_get_io_ptr(png);
    assert(memory);
    if (memory->m_i + length <= memory->m_size)
    {
        memcpy(data, memory->m_pb + memory->m_i, length);
        memory->m_i += (png_uint_32)length;
    }
}

HBITMAP
ii_png_load_mem(const void *pv, png_uint_32 cb)
{
    HBITMAP         hbm;
    png_structp     png;
    png_infop       info;
    png_uint_32     y, width, height, rowbytes;
    int             color_type, depth, widthbytes;
    double          gamma;
    BITMAPINFO      bi;
    LPBYTE          pbBits;
    II_MEMORY       memory;
    png_bytepp      rows;
    HDC             hdc;

    memory.m_pb = (const png_byte *)pv;
    memory.m_i = 0;
    memory.m_size = cb;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info = png_create_info_struct(png);
    if (png == NULL || info == NULL || setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    png_set_read_fn(png, &memory, ii_png_mem_read);
    png_read_info(png, info);

    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);
    png_set_expand(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);
    png_set_strip_16(png);
    png_set_gray_to_rgb(png);
    png_set_palette_to_rgb(png);
    png_set_bgr(png);
    png_set_packing(png);
    png_set_interlace_handling(png);
    if (png_get_gAMA(png, info, &gamma))
        png_set_gamma(png, 2.2, gamma);
    else
        png_set_gamma(png, 2.2, 0.45455);

    png_read_update_info(png, info);
    png_get_IHDR(png, info, &width, &height, &depth, &color_type,
                 NULL, NULL, NULL);

    rowbytes = (png_uint_32)png_get_rowbytes(png, info);
    rows = (png_bytepp)malloc(height * sizeof(png_bytep));
    for (y = 0; y < height; y++)
    {
        rows[y] = (png_bytep)malloc(rowbytes);
    }

    png_read_image(png, rows);
    png_read_end(png, NULL);

    ZeroMemory(&bi.bmiHeader, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = width;
    bi.bmiHeader.biHeight      = height;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = (WORD)(depth * png_get_channels(png, info));

    hdc = CreateCompatibleDC(NULL);
    hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (VOID **)&pbBits,
                           NULL, 0);
    DeleteDC(hdc);
    if (hbm == NULL)
    {
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    widthbytes = II_WIDTHBYTES(width * bi.bmiHeader.biBitCount);
    for (y = 0; y < height; y++)
    {
        memcpy(pbBits + y * widthbytes, rows[height - 1 - y], rowbytes);
    }

    png_destroy_read_struct(&png, &info, NULL);
    free(rows);
    return hbm;
}

HBITMAP
ii_png_load_res_a(HINSTANCE hInstance, const char *pszResName)
{
    HGLOBAL hGlobal;
    png_uint_32 dwSize;
    HBITMAP hbm;
    LPVOID lpData;
    HRSRC hRsrc;

    assert(pszResName);
    hRsrc = FindResourceA(hInstance, pszResName, "PNG");
    if (hRsrc == NULL)
        return NULL;

    dwSize = SizeofResource(hInstance, hRsrc);
    hGlobal = LoadResource(hInstance, hRsrc);
    if (hGlobal == NULL)
        return NULL;

    lpData = LockResource(hGlobal);
    hbm = ii_png_load_mem(lpData, dwSize);

#ifdef WIN16
    UnlockResource(hGlobal);
    FreeResource(hGlobal);
#endif

    return hbm;
}

HBITMAP
ii_png_load_res_w(HINSTANCE hInstance, const wchar_t *pszResName)
{
    HGLOBAL hGlobal;
    png_uint_32 dwSize;
    HBITMAP hbm;
    LPVOID lpData;
    HRSRC hRsrc;

    assert(pszResName);
    hRsrc = FindResourceW(hInstance, pszResName, L"PNG");
    if (hRsrc == NULL)
        return NULL;

    dwSize = SizeofResource(hInstance, hRsrc);
    hGlobal = LoadResource(hInstance, hRsrc);
    if (hGlobal == NULL)
        return NULL;

    lpData = LockResource(hGlobal);
    hbm = ii_png_load_mem(lpData, dwSize);

#ifdef WIN16
    UnlockResource(hGlobal);
    FreeResource(hGlobal);
#endif

    return hbm;
}

bool
ii_png_save_common(FILE *outf, HBITMAP hbm, float dpi)
{
    png_structp png;
    png_infop info;
    png_color_8 sBIT;
    HDC hMemDC;
    BITMAPINFO bi;
    BITMAP bm;
    png_uint_32 rowbytes, cbBits;
    LPBYTE pbBits;
    int y, nDepth;
    png_bytep *lines = NULL;
    bool ok = false;

    assert(outf);
    if (outf == NULL)
        return false;

    if (!GetObject(hbm, sizeof(bm), &bm))
    {
        fclose(outf);
        return false;
    }

    nDepth = (bm.bmBitsPixel == 32 ? 32 : 24);
    rowbytes = II_WIDTHBYTES(bm.bmWidth * nDepth);
    cbBits = rowbytes * bm.bmHeight;

    do
    {
        pbBits = (LPBYTE)malloc(cbBits);
        if (pbBits == NULL)
            break;

        ok = false;
        hMemDC = CreateCompatibleDC(NULL);
        if (hMemDC != NULL)
        {
            ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
            bi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth    = bm.bmWidth;
            bi.bmiHeader.biHeight   = bm.bmHeight;
            bi.bmiHeader.biPlanes   = 1;
            bi.bmiHeader.biBitCount = (WORD)nDepth;
            ok = !!GetDIBits(hMemDC, hbm, 0, bm.bmHeight, pbBits, &bi,
                             DIB_RGB_COLORS);
            DeleteDC(hMemDC);
        }
        if (!ok)
            break;
        ok = false;

        png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        info = png_create_info_struct(png);
        if (png == NULL || info == NULL)
            break;

        if (setjmp(png_jmpbuf(png)))
            break;

        png_init_io(png, outf);
        png_set_IHDR(png, info, bm.bmWidth, bm.bmHeight, 8,
            (nDepth == 32 ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB),
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_BASE);

        sBIT.red = 8;
        sBIT.green = 8;
        sBIT.blue = 8;
        sBIT.alpha = (png_byte)(nDepth == 32 ? 8 : 0);
        png_set_sBIT(png, info, &sBIT);

        if (dpi != 0.0)
        {
            png_uint_32 res = (png_uint_32)(dpi * 100 / 2.54 + 0.5);
            png_set_pHYs(png, info, res, res, PNG_RESOLUTION_METER);
        }

        png_write_info(png, info);
        png_set_bgr(png);

        lines = (png_bytep *)malloc(sizeof(png_bytep *) * bm.bmHeight);
        if (lines == NULL)
            break;
        for (y = 0; y < bm.bmHeight; y++)
        {
            lines[y] = (png_bytep)&pbBits[rowbytes * (bm.bmHeight - y - 1)];
        }

        png_write_image(png, lines);
        png_write_end(png, info);
        ok = true;
    } while (0);

    png_destroy_write_struct(&png, &info);

    free(lines);
    free(pbBits);
    fclose(outf);

    return ok;
}

bool ii_png_save_a(const char *pszFileName, HBITMAP hbm, float dpi)
{
    using namespace std;
    FILE *outf;
    outf = fopen(pszFileName, "wb");
    if (outf)
    {
        if (ii_png_save_common(outf, hbm, dpi))
            return true;
        DeleteFileA(pszFileName);
    }
    return false;
}

bool
ii_png_save_w(const wchar_t *pszFileName, HBITMAP hbm, float dpi)
{
    using namespace std;
    FILE *outf;
    outf = _wfopen(pszFileName, L"wb");
    if (outf)
    {
        if (ii_png_save_common(outf, hbm, dpi))
            return true;
        DeleteFileW(pszFileName);
    }
    return false;
}

void
ii_premultiply(HBITMAP hbm32bpp)
{
    BITMAP bm;
    png_uint_32 cdw;
    LPBYTE pb;
    png_byte alpha;
    GetObject(hbm32bpp, sizeof(bm), &bm);
    if (bm.bmBitsPixel == 32)
    {
        cdw = bm.bmWidth * bm.bmHeight;
        pb = (LPBYTE) bm.bmBits;
        while (cdw--)
        {
            alpha = pb[3];
            pb[0] = (png_byte) ((png_uint_32) pb[0] * alpha / 255);
            pb[1] = (png_byte) ((png_uint_32) pb[1] * alpha / 255);
            pb[2] = (png_byte) ((png_uint_32) pb[2] * alpha / 255);
            pb += 4;
        }
    }
}

void ii_fill(HBITMAP hbm, HBRUSH hbr)
{
    BITMAP bm;
    if (!GetObject(hbm, sizeof(bm), &bm))
        return;

    HDC hDC = CreateCompatibleDC(NULL);
    {
        HGDIOBJ hbmOld = SelectObject(hDC, hbm);
        {
            RECT rc;
            SetRect(&rc, 0, 0, bm.bmWidth, bm.bmHeight);
            FillRect(hDC, &rc, hbr);
            DeleteObject(hbr);
        }
        SelectObject(hDC, hbmOld);
    }
    DeleteDC(hDC);
}

void ii_draw(HBITMAP hbm, HBITMAP hbmSrc, INT x, INT y)
{
    BITMAP bmSrc;
    GetObject(hbmSrc, sizeof(bmSrc), &bmSrc);

    HDC hDC = CreateCompatibleDC(NULL);
    HDC hDC2 = CreateCompatibleDC(NULL);
    {
        HGDIOBJ hbmOld = SelectObject(hDC, hbm);
        HGDIOBJ hbm2Old = SelectObject(hDC2, hbmSrc);
        if (bmSrc.bmBitsPixel == 32)
        {
            BLENDFUNCTION bf;
            bf.BlendOp = AC_SRC_OVER;
            bf.BlendFlags = 0;
            bf.SourceConstantAlpha = 0xFF;
            bf.AlphaFormat = AC_SRC_ALPHA;
            AlphaBlend(hDC, x, y, bmSrc.bmWidth, bmSrc.bmHeight,
                       hDC2, 0, 0, bmSrc.bmWidth, bmSrc.bmHeight, bf);
        }
        else
        {
            BitBlt(hDC, x, y, bmSrc.bmWidth, bmSrc.bmHeight,
                   hDC2, 0, 0, SRCCOPY);
        }
        SelectObject(hDC, hbm2Old);
        SelectObject(hDC, hbmOld);
    }
    DeleteDC(hDC2);
    DeleteDC(hDC);
}

