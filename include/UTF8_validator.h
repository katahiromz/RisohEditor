/* UTF8_validator.h --- MZC4 UTF-8 Validator
 *
 * Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * Copyright (c) 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef MZC4_UTF8_VALIDATOR_H_
#define MZC4_UTF8_VALIDATOR_H_      2   /* Version 2 */

#if __cplusplus >= 201103L          /* C++11 */
    #include <cstdint>
#elif __STDC_VERSION__ >= 199901L   /* C99 */
    #include <stdint.h>
    #include <stdbool.h>
#else
    #include "pstdint.h"
    #include "pstdbool.h"
#endif

/****************************************************************************/

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

inline const uint8_t *
UTF8_table(void)
{
    static const uint8_t utf8d[] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 00..1f */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 20..3f */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 40..5f */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 60..7f */
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, /* 80..9f */
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, /* a0..bf */
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* c0..df */
        0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, /* e0..ef */
        0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, /* f0..ff */
        0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, /* s0..s0 */
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, /* s1..s2 */
        1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, /* s3..s4 */
        1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, /* s5..s6 */
        1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* s7..s8 */
    };
    return utf8d;
}

inline uint32_t
UTF8_decode(uint32_t* state, uint32_t* codep, uint32_t byte)
{
    uint32_t type = UTF8_table()[byte];

    *codep = (*state != UTF8_ACCEPT) ?
        (byte & 0x3fu) | (*codep << 6) :
        (0xff >> type) & (byte);

    *state = UTF8_table()[256 + *state * 16 + type];
    return *state;
}

inline bool
UTF8_count_code_points(const char *str, size_t *count)
{
    uint32_t codepoint, state = 0;

    for (*count = 0; *str; ++str)
    {
        if (!UTF8_decode(&state, &codepoint, *str))
        {
            ++(*count);
        }
    }

    return state != UTF8_ACCEPT;
}

inline uint32_t
UTF8_validate_state(uint32_t *state, const char *str, size_t len)
{
    size_t i;
    uint32_t type;

    for (i = 0; i < len; i++)
    {
        type = UTF8_table()[(uint8_t)str[i]];
        *state = UTF8_table()[256 + (*state) * 16 + type];

        if (*state == UTF8_REJECT)
            break;
    }

    return *state;
}

inline bool
UTF8_validate(const char *str, size_t len)
{
    uint32_t state = UTF8_ACCEPT;
    return UTF8_validate_state(&state, str, len) == UTF8_ACCEPT;
}

/****************************************************************************/

#endif  /* ndef MZC4_UTF8_VALIDATOR_H_ */
