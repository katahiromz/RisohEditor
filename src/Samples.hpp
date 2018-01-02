// Samples.hpp --- Sample Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#ifndef SAMPLES_HPP_
#define SAMPLES_HPP_

#include <windows.h>

//////////////////////////////////////////////////////////////////////////////

const BYTE *GetAccelSample(DWORD& Size);
const BYTE *GetDialogSample(DWORD& Size);
const BYTE *GetMenuSample(DWORD& Size);
const BYTE *GetStringSample(DWORD& Size);
const BYTE *GetVersionSample(DWORD& Size);
const BYTE *GetHtmlSample(DWORD& Size);
const BYTE *GetManifestSample(DWORD& Size);

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef SAMPLES_HPP_
