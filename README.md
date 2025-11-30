![CMake](https://github.com/katahiromz/RisohEditor/workflows/CMake/badge.svg)

# ![](img/re-icon.png "") RisohEditor by katahiromz

RisohEditor is a free resource editor for Win32 development, created by Katayama Hirofumi MZ.

It can read/write resource data in RC/RES/EXE/DLL files. UTF-16 resource files are also supported.

- Web page (English): https://katahiromz.web.fc2.com/re/en
- Web page (Chinese): https://katahiromz.web.fc2.com/re/ch
- Web page (Italian): https://katahiromz.web.fc2.com/re/it
- Web page (Japanese): https://katahiromz.fc2.page/risoheditor/
- Web page (Korean): https://katahiromz.web.fc2.com/re/ko
- Web page (Russian): https://katahiromz.web.fc2.com/re/ru
- Web page (Portuguese): https://katahiromz.web.fc2.com/re/pt

More information can be found at https://github.com/katahiromz/risoheditor-doc .

## Supported Platforms

It works on Windows XP/2003/Vista/7/8.1/10 and ReactOS.

## License Agreement

See [LICENSE.txt](https://github.com/katahiromz/RisohEditor/blob/master/LICENSE.txt) for details of copyrights and license agreement.

## Standardization

See "Standardize.md" for our standardization of resource IDs.

## FAQ

### Question 1. What is "Risoh"?

The word "risoh" means "ideal" in Japanese.

### Question 2. What are edt1, edt2, cmb1?

Those are standard control ID macros defined in `<dlgs.h>`.

### Question 3. What is mcdx?

It's a special message compiler I made. See [mcdx/MESSAGETABLEDX.md](https://github.com/katahiromz/RisohEditor/blob/master/mcdx/MESSAGETABLEDX.md) for details.

### Question 4. Why did I get garbled characters when compiling with Visual Studio?

rc.exe correctly supports UTF-16, but prior to Visual Studio 2022, loading a UTF-8 file will result in garbage in the output data.

Use UTF-16 (but UTF-16 is not supported in GNU windres).

### Question 5. What is the difference between no installer and portable version?

The portable version doesn't use registry but an ini file.

### Question 6. Does it support 64-bit?

Yes, it does.

## Contact Us

katayama.hirofumi.mz@gmail.com
