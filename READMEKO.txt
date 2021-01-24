(한국어)
/////////////////////////////////////////////////////
katahiromz가 개발한 RisohEditor
/////////////////////////////////////////////////////

RisohEditor는 Win32 개발을 위한 무료 리소스 편집기입니다.
RC/RES/EXE/DLL 파일의 리소스 데이터를 편집/추출/복제/삭제
할 수 있습니다.

바이너리 다운로드: https://katahiromz.web.fc2.com/re/en

Windows XP/2003/Vista/7/8.1/10 및 ReactOS에서 작동합니다.

저작권 및 라이센스 계약에 대한 자세한 내용은 
"LICENSE.txt"를 참조하십시오.

주의 사항: "C:\Program Files" 또는 "C:\Program Files (x86)"
에 설치하지 마십시오. 리소스 컴파일러 (windres)는 파일 경로
에 공백 문자를 허용하지 않기 때문입니다.

/////////////////////////////////////////////////////

질문 1. "Risoh"가 무엇입니까?

    대답. "Risoh"라는 단어는 일본어로 "이상적인"을 의미합니다.

질문 2. edt1, edt2, cmb1은 무엇입니까?

    대답. 이들은 <dlgs.h>에 정의된 표준 제어 ID 매크로입니다.

질문 3. mcdx란 무엇입니까?

    대답. 제가 만든 특별한 메시지 컴파일러입니다.
            자세한 내용은 mcdx/MESSAGETABLEDX.md를 참조하십시오.

질문 4. Visual Studio로 컴파일할 때 잘못된 문자가 표시되는 이유는 무엇입니까?

    대답. MSVC의 리소스 컴파일러는 UTF-8 리소스 파일을 처리하는데 
            버그가 있습니다.

            UTF-16을 사용합니다 (그러나 UTF-16은 GNU windres에서 지원되지 않습니다).

질문 5. 설치 프로그램 없음과 휴대용 버전의 차이점은 무엇입니까?

    대답. 휴대용 버전은 레지스트리를 사용하지 않고 ini 파일을 사용합니다.

질문 6. 64비트 파일이 지원됩니까?

    대답. 예. 64비트 Windows에서는 가능합니다. 그러나 WoW64 에뮬레이션 레이어는 
           "C:\Program Files" 또는 C:\Windows\system32"에서 로드되는 것을 방지합니다.
            로드하기 전에 64비트 파일을 다른 위치에 복사해야 합니다.

/////////////////////////////////////////////////////
// 업데이트 내역

- v.0.6 (2017.08.31)
    - First release. 
- v.5.0.8 (2018.09.12)
    - Improve tool bar icons.
    - Fix the file types on extraction.
    - Write dagger to source file to help UTF-8 detection.
    - Fix the treeview icons.
    - Split the RT_DIALOG and RT_MENU resource templates in languages.
- v.5.0.9 (2018.09.27)
    - Improve resource.h output.
    - Improve treeview file icon.
    - Improve importing.
- v.5.1.0 (2018.10.08)
    - Improve "String Entry" dialogs (multiline).
    - Fix Ctrl+A behaviour.
    - Check file lock on saving a file.
    - Make resource IDs case sensitive.
    - Support encoding of resource items.
    - Fix the abnormal termination at "Languages" dialog.
- v.5.1.1 (2018.11.08)
    - Accept space in the caption text box.
    - Accept space in the language text box.
- v.5.1.2 (2018.12.08)
    - Accept space in the window class name text box.
    - Accept C string literal in the window class name text box.
    - Improve template system.
    - Enable manifest template.
    - Fix toolbar UI update.
    - Correctly display ID types of "List of Resource IDs".
    - Correctly sort the languages upon output.
- v.5.1.3 (2019.01.01)
    - Fix "List of Resource IDs".
- v.5.1.4 (2019.01.13)
    - Add English installer.
    - Add Italian installer.
    - Add Italian translation.
    - Add some language installer.
- v.5.1.5 (2019.01.27)
    - Don't use WS_EX_MDICHILD extended style.
    - Support ReactOS.
- v.5.1.6 (2019.02.24)
    - Improve Italian translation.
    - GUI adjustment.
    - Fix "Clone In New Language".
- v.5.1.7 (2019.03.20)
    - Add PBS_MARQUEE and PBS_SMOOTHREVERSE styles.
    - Fix the process of compilation error.
- v.5.1.8 (2019.05.14)
    - Add check of recompilation upon cloning.
    - Fix the selection after cloning.
    - Correctly fail upon compilation error of string table and message table.
- v.5.1.9 (2019.07.14)
    - Supported UTF-16 source input/output.
- v.5.2.0 (2019.07.26)
    - Correctly treat DIALOG STYLE values (WS_CAPTION is default value of DIALOG STYLE).
    - Fixed a bug that the application unexpectedly deletes the file when it opened a compressed EXE file without expanding.
- v.5.2.1 (2019.08.04)
    - "English (United States)" will be selected if you entered "En" for language name.
    - Add "Query Constant" feature.
    - Fix "Collapse All".
    - WS_POPUPWINDOW | WS_BORDER must be WS_POPUPWINDOW | WS_CAPTION.
    - RT_FONT support.
    - Improved "Add Resource" dialog.
- v.5.2.2 (2019.08.14)
    - Update Italian translation.
    - Support XML, XSLT, SCHEMA and REGISTRY resource types.
    - Improved interpretation when language mismatch of RT_ICON/RT_CURSOR and RT_GROUP_ICON/RT_GROUP_CURSOR.
    - Improved interpretation when language mismatch of RT_DLGINIT and RT_DIALOG.
- v.5.2.3 (2019.09.14)
    - Fix AUTORADIOBUTTON STYLE.
- v.5.2.4 (2019.09.15)
    - XP support.
- v.5.2.5 (2019.09.19)
    - Fixed a WS_CHILDWINDOW bug in dialog style listbox.
    - Changed the release filename (RisohEditor-X.X.X.exe and RisohEditor-X.X.X.zip).
    - Added TRANSLATORS.txt.
    - Made owner-drawn controls visible.
- v.5.2.6 (2019.09.23)
    - Improve Languages dialog.
    - Add Russian translation.
    - Show error message if the installed location has space characters.
    - Add --use-temp-file option in invoking windres to fix the popen problem.
- v.5.2.7 (2019.10.20)
    - Improved Languages dialog again.
    - Fix the positions of the dialog item marks.
- v.5.2.8 (2020.01.30)
    - Fix Russian translation.
    - Able to display undefined controls.
    - Added "Use BEGIN/END" option.
    - Fixed a bug that DLL could not be saved as another name.
- v.5.2.9 (2020.02.01)
    - XP support of 5.2.8 is forgotten. Now enabled.
    - Fixed processing of file saving.
    - Able to save EXE/DLL files without user-owned executable.
    - Fixed how to backup.
- v.5.3.0 (2020.02.06)
    - Added ES_AUTOHSCROLL to some textboxes.
    - Fixed the logical error upon overwriting file.
- v.5.3.1 (2020.02.23)
    - Fix and improve menu resource reading/displaying.
    - Improved IDC_STATIC handling.
    - Improved "List of Resource IDs" window.
- v.5.3.2 (2020.03.02)
    - Able to save with Ctrl+S without querying the location.
    - Made the "Output RC files as UTF-16" option non-volatile.
    - Bring "List of Resource IDs" window to top.
- v.5.3.3 (2020.03.03)
    - Reduced file size from 9MB to 3MB.
- v.5.3.4 (2020.03.19)
    - Changed toolbar save button behavior.
    - Introduced automation by using programming language EGA.
    - Added confirmation of saving changes of file.
- v.5.3.5 (2020.03.26)
    - Improve Italian translation.
    - Fix needless save confirmation.
- v.5.3.6 (2020.04.15)
    - The infinite loop is avoided by changing the loop variables to 32-bits.
    - Delphi DFM data support.
    - Improved context menu.
- v.5.3.7 (2020.04.28)
    - Updated file change flag when file drop.
    - Improved the filename when extracting.
    - Strengthened EGA.
    - Improved Russian translation.
- v.5.3.8 (2020.05.22)
    - Added "Open EGA Manual" item to "Automation" menu.
    - Strictly controlled the file change flag.
- v.5.3.9 (2020.06.01)
    - Added "replacing-dialog-fonts" feature.
    - Moved some menu items to "Edit" menu.
    - Added value-zero-check of resource name and resource type.
    - Added DS_CENTER style to the sample resource dialog.
    - Enabled F1, F3, F5 and F6 function keys.
    - Added Portable version.
    - Improved search feature.
- v.5.4.0 (2020.06.13)
    - Fixed RT_ACCELERATOR output (unsigned 16-bit).
    - Removed "Store into res folder" option.
    - Fixed crash upon GUI edit.
    - Strengthened extraction feature.
    - Fixed replacing-dialog-font feature.
    - Used the full path for title bar.
    - Added drop-down language arrow.
    - Fixed "Query Constant" dialog.
    - Fixed "Encoding of Resource Item" dialog.
    - Improved "ID Association" dialog.
    - Improved "Predefined Macros" dialog.
    - Improved "Configuration" dialog.
    - Fixed "Search" dialog.
    - Improved status message.
    - Improved modified flag handling.
- v.5.4.1 (2020.06.14)
    - Fixed the bug that the second overwrite save fails.
    - Fixed title bar text.
    - Fixed status bar message.
    - Added German translation.
    - Added French translation.
- v.5.4.2 (2020.06.18)
    - Added tab control to choose "Code Editor" or "Hex Viewer".
    - Improved German and French translations.
    - Fixed language drop-down arrow.
    - Fixed Unicode encoding processing.
    - Improved exporting and extracting.
    - Improved EGA dialog.
- v.5.4.3 (2020.07.03)
    - Improved extraction filename.
    - Sorted the treeview items upon change of resource name/language.
    - Improved icon/cursor extraction.
    - Improved newline codes of encoded text.
    - Fixed "Add Resource" dialog.
    - Made HTML/Manifest importable.
    - Added update check feature in "Help" menu.
    - Fixed title of message box.
    - Relaxed the character limit and supported large data.
    - Reset checksum to zero when saving file.
- v.5.4.4 (2020.07.09)
    - Flushed file contents before using it.
    - Supported international Delphi DFM data.
    - Inserted "Delphi DFM Settings" menu item into "Edit" menu.
    - Correctly reset checksum to zero when saving file.
    - Fixed a bug in which a control is shifted by 1 pixel.
    - Added some waits upon file saving, for virus checker.
    - Added "A Guide to RisohEditor" link to "Help" menu.
- v.5.4.5 (2020.08.03)
    - Added Indonesian translation.
    - Added auto complete for language combo boxes.
- v.5.4.6 (2020.10.03)
    - Deleted cache in version check.
    - Fixed test dialog position.
    - Fixed a bug that RT_DLGINIT disappears without permission.
- v.5.4.7 (2020.10.18)
    - Downgraded Inno Setup to 5.6.1 for XP support.
    - Initial support of ActiveX window class "AtlAxWin140" on test dialog.
- v.5.4.8 (2020.11.12)
    - Added Finnish translation.
    - Added *.rc and *.res file association.
    - Initial support of OLE controls.
    - Deleted MOleCtrl and added MOleHost.
    - Supported REGINST data.
- v.5.4.9 (2021.01.21)
    - Refresh ID List upon renaming treeview item.
- v.5.5.0 (2021.XX.YY)
    - Add Korean translation.

/////////////////////////////////////////////////////////////////////
Katayama Hirofumi MZ (katahiromz) [A.N.T.]
Webpage (English):  https://katahiromz.web.fc2.com/re/en
Webpage (Chinese):  https://katahiromz.web.fc2.com/re/ch
Webpage (Japanese): https://katahiromz.web.fc2.com/re/ja
Webpage (Italian):  https://katahiromz.web.fc2.com/re/it
Webpage (Russian):  https://katahiromz.web.fc2.com/re/ru
Email               katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
