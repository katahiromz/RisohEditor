// Png.hpp -- PNG Images
//////////////////////////////////////////////////////////////////////////////

#ifndef PNG_HPP_
#define PNG_HPP_

#include <png.h>

#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "zlib.lib")

//////////////////////////////////////////////////////////////////////////////

// Png.cpp
HBITMAP ii_png_load_common(FILE *inf, float *dpi);
HBITMAP ii_png_load_a(const char *pszFileName, float *dpi);
HBITMAP ii_png_load_w(const wchar_t *pszFileName, float *dpi);
HBITMAP ii_png_load_mem(const void *pv, png_uint_32 cb);
HBITMAP ii_png_load_res_a(HINSTANCE hInstance, const char *pszResName);
HBITMAP ii_png_load_res_w(HINSTANCE hInstance, const wchar_t *pszResName);
bool ii_png_save_a(const char *pszFileName, HBITMAP hbm, float dpi);
bool ii_png_save_w(const wchar_t *pszFileName, HBITMAP hbm, float dpi);
void ii_premultiply(HBITMAP hbm32bpp);
void ii_fill(HBITMAP hbm, HBRUSH hbr);
void ii_draw(HBITMAP hbm, HBITMAP hbmSrc, INT x, INT y);

#ifdef UNICODE
    #define ii_png_load ii_png_load_w
    #define ii_png_load_res ii_png_load_res_w
    #define ii_png_save ii_png_save_w
#else
    #define ii_png_load ii_png_load_a
    #define ii_png_load_res ii_png_load_res_a
    #define ii_png_save ii_png_save_a
#endif

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef PNG_HPP_

//////////////////////////////////////////////////////////////////////////////
