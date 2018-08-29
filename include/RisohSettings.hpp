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

#include "MWindowBase.hpp"
#include <vector>
#include <map>

//////////////////////////////////////////////////////////////////////////////

#define MAX_MRU     5       // the number of most recently used (MRU) files

typedef std::vector<MString>            mru_type;           // MRU list type
typedef std::map<MString, MString>      assoc_map_type;     // association type
typedef std::map<MStringA, MStringA>    id_map_type;        // ID mapping type
typedef std::map<MString, MString>      macro_map_type;     // macros type
typedef std::vector<MString>            include_dirs_type;  // includes type
typedef std::vector<MString>            captions_type;      // captions type

//////////////////////////////////////////////////////////////////////////////

struct RisohSettings
{
    BOOL        bShowBinEdit;           // show the binary EDIT control?
    BOOL        bAlwaysControl;         // always show CONTROL statements?
    BOOL        bShowStatusBar;         // show the status bar?
    INT         nTreeViewWidth;         // the treeview width
    INT         nBmpViewWidth;          // the MBmpview width
    INT         nBinEditHeight;         // the binary EDIT control height
    BOOL        bGuiByDblClick;         // start GUI edit by double click?
    BOOL        bResumeWindowPos;       // resume the window position?
    BOOL        bAutoLoadNearbyResH;    // auto loading resource.h?
    BOOL        bAutoShowIDList;        // auto show ID list window?
    BOOL        bHideID;                // don't show ID macros?
    BOOL        bUseIDC_STATIC;         // use IDC_STATIC?
    BOOL        bShowDotsOnDialog;      // show dots on the dialog?
    INT         nComboHeight;           // the combobox height
    mru_type    vecRecentlyUsed;        // the MRU files
    INT         nWindowLeft;            // the main window X coordinate
    INT         nWindowTop;             // the main window Y coordinate
    INT         nWindowWidth;           // the main window width
    INT         nWindowHeight;          // the main window height
    BOOL        bMaximized;             // is the main window maximized?
    INT         nIDListLeft;            // the ID list X coordinate
    INT         nIDListTop;             // the ID list Y coordinate
    INT         nIDListWidth;           // the ID list width
    INT         nIDListHeight;          // the ID list height
    INT         nRadLeft;               // the RAD window X coordinate
    INT         nRadTop;                // the RAD window Y coordinate
    BOOL        bAskUpdateResH;         // ask update the resource.h file?
    BOOL        bCompressByUPX;         // compress the file by UPX?
    MString     strSrcFont;             // the source view font name
    INT         nSrcFontSize;           // the source view font size in points
    MString     strBinFont;             // the binary view font name
    INT         nBinFontSize;           // the binary view font size in points
    assoc_map_type      assoc_map;      // the ID association
    id_map_type         id_map;         // the macro ID mapping
    id_map_type         added_ids;      // the added macro IDs
    id_map_type         removed_ids;    // the removed macro IDs
    macro_map_type      macros;         // the predefined macros
    include_dirs_type   includes;       // the #include's directories
    MString             strWindResExe;  // the windres.exe location
    MString             strCppExe;      // the cpp.exe location
    MString             strPrevVersion;     // the previous RisohEditor version
    BOOL                bSepFilesByLang;    // use the "lang" folder to export and/or save?
    BOOL                bStoreToResFolder;  // store to the "res" folder?
    BOOL                bSelectableByMacro; // choose languages by macros?
    captions_type       captions;           // the remembered captions
    BOOL                bShowToolBar;       // show the toolbar?
    BOOL                bHasIDC_STATIC;     // 
    MString             strAtlAxWin;        // ATL OLE control name
    INT                 nSaveFilterIndex;   // the filter index on save
    BOOL                bWordWrap;          // do word wrap?
    BOOL                bBackup;            // do backup?
    MString             strBackupSuffix;    // backup suffix
    BOOL                bRedundantComments; // output redundant comments?

    RisohSettings()
    {
        bHasIDC_STATIC = FALSE;
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

    void AddIDC_STATIC()
    {
        id_map["IDC_STATIC"] = "-1";
        added_ids["IDC_STATIC"] = "-1";
        removed_ids["IDC_STATIC"] = "-1";
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

#define DEF_VALUE(x, val)         macros.insert(std::make_pair(TEXT(#x), TEXT(#val)))
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
    DEF_LANG("LANGUAGE_AR_SA");
    DEF_LANG("LANGUAGE_BG_BG");
    DEF_LANG("LANGUAGE_ZH_CN");
    DEF_LANG("LANGUAGE_ZH_TW");
    DEF_LANG("LANGUAGE_HR_HR");
    DEF_LANG("LANGUAGE_CS_CZ");
    DEF_LANG("LANGUAGE_DA_DK");
    DEF_LANG("LANGUAGE_NL_NL");
    DEF_LANG("LANGUAGE_EN_US");
    DEF_LANG("LANGUAGE_EN_GB");
    DEF_LANG("LANGUAGE_ET_EE");
    DEF_LANG("LANGUAGE_FI_FI");
    DEF_LANG("LANGUAGE_FR_CA");
    DEF_LANG("LANGUAGE_FR_FR");
    DEF_LANG("LANGUAGE_DE_DE");
    DEF_LANG("LANGUAGE_EL_GR");
    DEF_LANG("LANGUAGE_HE_IL");
    DEF_LANG("LANGUAGE_HU_HU");
    DEF_LANG("LANGUAGE_IT_IT");
    DEF_LANG("LANGUAGE_JA_JP");
    DEF_LANG("LANGUAGE_KO_KR");
    DEF_LANG("LANGUAGE_LV_LV");
    DEF_LANG("LANGUAGE_LT_LT");
    DEF_LANG("LANGUAGE_NB_NO");
    DEF_LANG("LANGUAGE_PL_PL");
    DEF_LANG("LANGUAGE_PT_BR");
    DEF_LANG("LANGUAGE_PT_PT");
    DEF_LANG("LANGUAGE_RO_RO");
    DEF_LANG("LANGUAGE_RU_RU");
    DEF_LANG("LANGUAGE_SK_SK");
    DEF_LANG("LANGUAGE_SL_SI");
    DEF_LANG("LANGUAGE_ES_MX");
    DEF_LANG("LANGUAGE_ES_ES");
    DEF_LANG("LANGUAGE_SV_SE");
    DEF_LANG("LANGUAGE_TH_TH");
    DEF_LANG("LANGUAGE_TR_TR");
    DEF_LANG("LANGUAGE_UK_UA");
#undef DEF_LANG
    }
};

#ifdef USE_GLOBALS
    extern RisohSettings g_settings;
#else
    inline RisohSettings&
    RisohSettings_GetMaster(void)
    {
        static RisohSettings s_settings;
        return s_settings;
    }
    #define g_settings RisohSettings_GetMaster()
#endif

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RISOHSETTINGS_HPP
