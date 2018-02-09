// RisohSettings.hpp --- RisohEditor settings
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
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

#ifndef RISOHSETTINGS_HPP
#define RISOHSETTINGS_HPP

//////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include <vector>
#include <map>

#define MAX_MRU         5       // the number of most recently used files

typedef std::vector<MString>            mru_type;
typedef std::map<MString, MString>      assoc_map_type;
typedef std::map<MStringA, MStringA>    id_map_type;

//////////////////////////////////////////////////////////////////////////////

struct RisohSettings
{
    BOOL        bShowBinEdit;
    BOOL        bAlwaysControl;
    BOOL        bShowStatusBar;
    INT         nTreeViewWidth;
    INT         nBmpViewWidth;
    INT         nBinEditHeight;
    BOOL        bGuiByDblClick;
    BOOL        bResumeWindowPos;
    BOOL        bAutoLoadNearbyResH;
    BOOL        bAutoShowIDList;
    BOOL        bHideID;
    BOOL        bShowDotsOnDialog;
    INT         nComboHeight;
    mru_type    vecRecentlyUsed;
    INT         nWindowLeft;
    INT         nWindowTop;
    INT         nWindowWidth;
    INT         nWindowHeight;
    BOOL        bMaximized;
    INT         nIDListLeft;
    INT         nIDListTop;
    INT         nIDListWidth;
    INT         nIDListHeight;
    INT         nRadLeft;
    INT         nRadTop;
    BOOL        bUpdateResH;
    BOOL        bCompressByUPX;
    assoc_map_type      assoc_map;
    id_map_type         id_map;
    id_map_type         added_ids;
    id_map_type         removed_ids;

    RisohSettings()
    {
    }

    void AddFile(LPCTSTR pszFile)
    {
        for (size_t i = 0; i < vecRecentlyUsed.size(); ++i)
        {
            if (vecRecentlyUsed[i] == pszFile)
            {
                vecRecentlyUsed.erase(vecRecentlyUsed.begin() + i);
                break;
            }
        }
        vecRecentlyUsed.insert(vecRecentlyUsed.begin(), pszFile);
        if (vecRecentlyUsed.size() > MAX_MRU)
            vecRecentlyUsed.resize(MAX_MRU);
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RISOHSETTINGS_HPP
