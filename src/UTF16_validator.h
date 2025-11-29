/* UTF16_validator.h --- MZC4 UTF-16 Validator
 */

#ifndef MZC4_UTF16_VALIDATOR_H_
#define MZC4_UTF16_VALIDATOR_H_      3   /* Version 3 */

#include "UTF8_validator.h"

#ifdef __cplusplus
	#include <cstring>
	#include <cassert>
#else
	#include <string.h>
	#include <assert.h>
#endif

bool UTF16_validate(const void *pv, size_t cb);

/****************************************************************************/

inline bool
UTF16_validate(const void *pv, size_t cb)
{
#if defined(_WIN32) && !defined(WONVER)
	return !!IsTextUnicode(pv, (int)cb, NULL);
#else
	const WCHAR *wide_str;
	int cch, k;
	bool bASCII;

	assert(sizeof(WCHAR) == 2);

	/* check zero length */
	if (cb == 0)
		return true;

	/* check odd length */
	if ((cb & 1) != 0)
		return false;

	wide_str = (const WCHAR *)pv;
	cch = cb / 2;

	/* check BOM */
	if (memcmp(pv, "\xFE\xFF", 2) == 0)   /* UTF-16BE */
	{
		return false;
	}
	if (memcmp(pv, "\xFF\xFE", 2) == 0)   /* UTF-16LE */
	{
		return true;
	}
	if (cb >= 3)
	{
		if (memcmp(pv, "\xEF\xBB\xBF", 3) == 0)   /* UTF-8 */
		{
			return false;
		}
		if (memcmp(pv, "\x2B\x2F\x76", 3) == 0)   /* UTF-7 */
		{
			return false;
		}
	}
	if (cb >= 4)
	{
		if (memcmp(pv, "\0\0\xFE\xFF", 4) == 0)   /* UTF-32BE */
		{
			return false;
		}
		if (memcmp(pv, "\xFF\xFE\0\0", 4) == 0)   /* UTF-32LE */
		{
			return false;
		}
	}

	/* check illegal */
	for (k = 0; k < cch; ++k)
	{
		WCHAR wch = wide_str[k];
		if (wch == 0 || wch == 0xFFFF ||
			memcmp(&wch, "\xFE\xFF", 2) == 0 ||
			memcmp(&wch, "\r\n", 2) == 0)
		{
			return false;
		}
	}

	/* check ASCII */
	bASCII = true;
	for (k = 0; k < cch; ++k)
	{
		if (wide_str[k] & 0xFF00)
		{
			bASCII = false;
			break;
		}
	}
	if (bASCII)
	{
		return true;
	}

	/* check UTF-8 */
	if (UTF8_validate(pv, cb))
		return false;

	return true;
#endif
} /* UTF16_validate */

/****************************************************************************/

#endif  /* ndef MZC4_UTF16_VALIDATOR_H_ */
