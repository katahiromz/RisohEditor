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
typedef std::map<MString, MString>      macro_map_type;
typedef std::vector<MString>            include_dirs_type;
typedef std::vector<MString>            captions_type;

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
    BOOL        bAskUpdateResH;
    BOOL        bCompressByUPX;
    MString     strSrcFont;
    INT         nSrcFontSize;
    MString     strBinFont;
    INT         nBinFontSize;
    assoc_map_type      assoc_map;
    id_map_type         id_map;
    id_map_type         added_ids;
    id_map_type         removed_ids;
    macro_map_type      macros;
    include_dirs_type   includes;
    MString             strWindResExe;
    MString             strCppExe;
    BOOL                bOldStyle;
    MString             strPrevVersion;
    BOOL                bSepFilesByLang;
    BOOL                bStoreToResFolder;
    BOOL                bSelectableByMacro;
    captions_type       captions;
    BOOL                bShowToolBar;

    RisohSettings()
    {
    }

    void AddCaption(LPCTSTR pszCaption)
    {
        if (!pszCaption || !pszCaption[0])
            return;

        for (size_t i = 0; i < captions.size(); ++i)
        {
            if (captions[i] == pszCaption)
            {
                captions.erase(captions.begin() + i);
                break;
            }
        }

        captions.insert(captions.begin(), pszCaption);
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

    void ResetAssoc()
    {
        assoc_map[L"Cursor.ID"] = L"IDC_";
        assoc_map[L"Bitmap.ID"] = L"IDB_";
        assoc_map[L"Menu.ID"] = L"IDR_";
        assoc_map[L"Dialog.ID"] = L"IDD_";
        assoc_map[L"String.ID"] = L"IDS_";
        assoc_map[L"Accel.ID"] = L"IDR_";
        assoc_map[L"Icon.ID"] = L"IDI_";
        assoc_map[L"AniCursor.ID"] = L"IDR_";
        assoc_map[L"AniIcon.ID"] = L"IDR_";
        assoc_map[L"Html.ID"] = L"IDR_";
        assoc_map[L"Help.ID"] = L"HID_";
        assoc_map[L"Command.ID"] = L"IDM_";
        assoc_map[L"Control.ID"] = L"IDC_";
        assoc_map[L"Resource.ID"] = L"IDR_";
        assoc_map[L"Message.ID"] = L"MSGID_";
        assoc_map[L"Window.ID"] = L"IDW_";
        assoc_map[L"New.Command.ID"] = L"ID_";
        assoc_map[L"Prompt.ID"] = L"IDP_";
        assoc_map[L"RCData.ID"] = L"IDR_";
        assoc_map[L"Unknown.ID"] = L"";
    }

    void ResetMacros()
    {
        macros.clear();

#define DEF_VALUE(x,val)         macros.insert(std::make_pair(TEXT(#x), TEXT(#val)))
        // TODO: update these values
        DEF_VALUE(_WIN32, 1);
        DEF_VALUE(__GNUC__, 7);
        DEF_VALUE(__GNUC_MINOR__, 3);
        DEF_VALUE(__GNUC_PATCHLEVEL__, 0);
        //#ifdef _WIN64
        //    DEF_VALUE(_WIN64, 1);
        //    DEF_VALUE(__x86_64, 1);
        //    DEF_VALUE(__x86_64__, 1);
        //    DEF_VALUE(__amd64, 1);
        //    DEF_VALUE(__amd64__, 1);
        //#else
            DEF_VALUE(_X86_, 1);
            DEF_VALUE(__i386, 1);
            DEF_VALUE(__i386__, 1);
        //#endif
#undef DEF_VALUE

#define DEF_LANG(lang) macros.insert(std::make_pair(TEXT(lang), TEXT("")))
        DEF_LANG("LANGUAGE_BG_BG");
        DEF_LANG("LANGUAGE_CA_ES");
        DEF_LANG("LANGUAGE_CS_CZ");
        DEF_LANG("LANGUAGE_DA_DK");
        DEF_LANG("LANGUAGE_DE_DE");
        DEF_LANG("LANGUAGE_EL_GR");
        DEF_LANG("LANGUAGE_EN_US");
        DEF_LANG("LANGUAGE_EO_AA");
        DEF_LANG("LANGUAGE_ES_ES");
        DEF_LANG("LANGUAGE_ET_EE");
        DEF_LANG("LANGUAGE_FI_FI");
        DEF_LANG("LANGUAGE_FR_FR");
        DEF_LANG("LANGUAGE_HE_IL");
        DEF_LANG("LANGUAGE_HU_HU");
        DEF_LANG("LANGUAGE_ID_ID");
        DEF_LANG("LANGUAGE_IT_IT");
        DEF_LANG("LANGUAGE_JA_JP");
        DEF_LANG("LANGUAGE_KO_KR");
        DEF_LANG("LANGUAGE_LT_LT");
        DEF_LANG("LANGUAGE_NB_NO");
        DEF_LANG("LANGUAGE_NL_NL");
        DEF_LANG("LANGUAGE_NO_NO");
        DEF_LANG("LANGUAGE_PL_PL");
        DEF_LANG("LANGUAGE_PT_BR");
        DEF_LANG("LANGUAGE_PT_PT");
        DEF_LANG("LANGUAGE_RO_RO");
        DEF_LANG("LANGUAGE_RU_RU");
        DEF_LANG("LANGUAGE_SK_SK");
        DEF_LANG("LANGUAGE_SL_SI");
        DEF_LANG("LANGUAGE_SQ_AL");
        DEF_LANG("LANGUAGE_SR_SP");
        DEF_LANG("LANGUAGE_SV_SE");
        DEF_LANG("LANGUAGE_TH_TH");
        DEF_LANG("LANGUAGE_TR_TR");
        DEF_LANG("LANGUAGE_UK_UA");
        DEF_LANG("LANGUAGE_ZH_CN");
        DEF_LANG("LANGUAGE_ZH_TW");
#undef DEF_LANG
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RISOHSETTINGS_HPP
