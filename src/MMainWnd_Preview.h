// MMainWnd_Preview.h --- Preview function declarations for MMainWnd
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

// This header documents the code organization for MMainWnd preview functions.
//
// The MMainWnd class is defined in RisohEditor.cpp.
// The preview function implementations are in MMainWnd_Preview.cpp.
// MMainWnd_Preview.cpp is #include'd from RisohEditor.cpp after the class definition.
//
// This approach is used because the MMainWnd class is defined in the .cpp file,
// not in a header, so MMainWnd_Preview.cpp cannot be compiled as a separate
// translation unit.
