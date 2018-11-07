(English)
/////////////////////////////////////////////////////
RisohEditor by katahiromz
/////////////////////////////////////////////////////

RisohEditor is a free resource editor for Win32 
development, created by Katayama Hirofumi MZ.

It works on Windows XP/2003/Vista/7/8.1/10.

See "Standardize.md" for standardization of resource IDs.

See "LICENSE.txt" for details of copyrights and 
license agreement.

NOTICE: Please avoid installing in "C:\Program Files" or 
"C:\Program Files (x86)". Because its resource 
compiler (windres) doesn't accept space character in 
the file path. 

/////////////////////////////////////////////////////

Question 1. What is "Risoh"?

    Answer. The word "Risoh" means "ideal" in Japanese.

Question 2. What are edt1, edt2, cmb1?

    Answer. Those are standard control ID macros defined in <dlgs.h>.

Question 3. I can't find my resource IDs in [List of Resource IDs] 
            Window.

    Answer. At first, please configure [ID Association] settings from
            [File] Menu --> [ID Association]. You have to load 
            the "resource.h" file to get the resource ID names 
            correctly.

Question 4. What is mcdx?

    Answer. It's a special message compiler I made.
            See mcdx/MESSAGETABLEDX.md for details.

/////////////////////////////////////////////////////
// HISTORY

 * 2017.08.31 ver.0.6
    - First release.
 * 2018.09.12 ver.5.0.8
    - Improve tool bar icons.
    - Fix the file types on extraction.
    - Write dagger to source file to help UTF-8 detection.
    - Fix the treeview icons.
    - Split the RT_DIALOG and RT_MENU resource templates in languages.
 * 2018.09.27 ver.5.0.9
    - Improve resource.h output.
    - Improve treeview file icon.
    - Improve importing.
 * 2018.10.08 ver.5.1.0
    - Improve "String Entry" dialogs (multiline).
    - Fix Ctrl+A behaviour.
    - Check file lock on saving a file.
    - Make resource IDs case sensitive.
    - Support encoding of resource items.
    - Fix the abnormal termination at "Languages" dialog.
 * 2018.XX.YY ver.5.1.1
    - Don't trim the caption text. Accept space in the caption.

/////////////////////////////////////////////////////
// Katayama Hirofumi MZ (katahiromz) [A.N.T.]
// Homepage     http://katahiromz.web.fc2.com
// BBS          http://katahiromz.bbs.fc2.com
// E-Mail       katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////
