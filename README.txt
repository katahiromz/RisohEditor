(English)
/////////////////////////////////////////////////////
RisohEditor by katahiromz
/////////////////////////////////////////////////////

RisohEditor is a free resource editor for Win32 
development, created by Katayama Hirofumi MZ.

It works on Windows 2003/Vista/7/8.1/10.

See "Standardize.md" for standardization of resource IDs.

See "LICENSE.txt" for details of copyrights and 
license agreement.

NOTICE: Please avoid to install in "C:\Program Files" or 
"C:\Program Files (x86)". Because its resource 
compiler (windres) doesn't accept a space character in 
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

Question 4. Why can't I add a control data to my dialog item?

    Answer. Make the dialog box extended.

Question 5. What is mcdx?

    Answer. It's a special message compiler I made.
            See mcdx/MESSAGETABLEDX.md for details.

/////////////////////////////////////////////////////
// Katayama Hirofumi MZ (katahiromz) [A.N.T.]
// Homepage     http://katahiromz.web.fc2.com
// BBS          http://katahiromz.bbs.fc2.com
// E-Mail       katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////
