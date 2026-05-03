#pragma once

#define ES_NOOLEDRAGDROP      0x00000008
#define ES_DISABLENOSCROLL    0x00002000
#define ES_SUNKEN             0x00004000
#define ES_SAVESEL            0x00008000
#define ES_SELFIME            0x00040000
#define ES_NOIME              0x00080000
#define ES_VERTICAL           0x00400000
#define ES_SELECTIONBAR       0x01000000
#define ES_EX_NOCALLOLEINIT   0x01000000

#ifndef ES_LEFT
#define ES_LEFT                0x00000000
#define ES_CENTER              0x00000001
#define ES_RIGHT               0x00000002
#define ES_MULTILINE           0x00000004
#define ES_AUTOVSCROLL         0x00000040
#define ES_AUTOHSCROLL         0x00000080
#define ES_NOHIDESEL           0x00000100
#define ES_READONLY            0x00000800
#define ES_WANTRETURN          0x00001000
#define ES_NUMBER              0x00002000
#endif
