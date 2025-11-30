(English)
/////////////////////////////////////////////////////
RisohEditor by katahiromz
/////////////////////////////////////////////////////

RisohEditor is a free resource editor for Win32 development.
It can edit/extract/clone/delete the resource data in
RC/RES/EXE/DLL files.

Download binary: https://katahiromz.web.fc2.com/re/en

It works on Windows XP/2003/Vista/7/8.1/10 and ReactOS.

See "LICENSE.txt" for details of copyrights and 
license agreement.

/////////////////////////////////////////////////////

Question 1. What is "Risoh"?

    Answer. The word "Risoh" means "ideal" in Japanese.

Question 2. What are edt1, edt2, cmb1?

    Answer. Those are standard control ID macros defined in <dlgs.h>.

Question 3. What is mcdx?

    Answer. It's a special message compiler I made.
            See mcdx/MESSAGETABLEDX.md for details.

Question 4. Why did I get garbled characters when compiling with Visual Studio?

    Answer. rc.exe correctly supports UTF-16, but prior to Visual Studio 2022,
            loading a UTF-8 file will result in garbage in the output data.
            Use UTF-16 (but UTF-16 is not supported in GNU windres).

Question 5. What is the difference between no installer and portable version?

    Answer. The portable version doesn't use registry but an ini file.

Question 6. Does it support 64-bit?

    Answer. Yes, it does.

/////////////////////////////////////////////////////////////////////
Katayama Hirofumi MZ (katahiromz) [A.N.T.]
Webpage (English):    https://katahiromz.web.fc2.com/re/en
Webpage (Chinese):    https://katahiromz.web.fc2.com/re/ch
Webpage (Japanese):   https://katahiromz.fc2.page/risoheditor/
Webpage (Italian):    https://katahiromz.web.fc2.com/re/it
Webpage (Russian):    https://katahiromz.web.fc2.com/re/ru
Webpage (Portuguese): https://katahiromz.web.fc2.com/re/pt
Email                 katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
