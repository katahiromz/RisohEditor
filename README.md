[![Build Status on Travis CI](https://travis-ci.org/katahiromz/RisohEditor.svg?branch=master)](https://travis-ci.org/katahiromz/RisohEditor)
[![Build status on AppVeyor](https://ci.appveyor.com/api/projects/status/4sdaed4vyakby61h?svg=true)](https://ci.appveyor.com/project/katahiromz/risoheditor)

# RisohEditor by katahiromz

RisohEditor is a free resource editor for Win32 development, created by Katayama Hirofumi MZ.

## Supported Platforms

It works on Windows 2003/Vista/7/8.1/10.

## License Agreement

See LICENSE.txt for details of copyrights and license agreement.

## NOTICE

Please avoid to install in "C:\Program Files" or "C:\Program Files (x86)". Because its resource compiler (windres) doesn't accept a space character in the file path. 

## FAQ

### Question 1. What is "Risoh"?

The word "risoh" means "ideal" in Japanese.

### Question 2. What are edt1, edt2, cmb1?

Those are control ID macros defined in <dlgs.h>.

### Question 3. I can't find my resource IDs in [List of Resource IDs] Window.

At first, please configure [ID Association] settings from [File] Menu --> [ID Association]. You have to load the "resource.h" file to get the resource ID names correctly.

##Contact Us

>Katayama Hirofumi MZ (katahiromz) [A.N.T.]
>Homepage     http://katahiromz.web.fc2.com
>BBS          http://katahiromz.bbs.fc2.com
>E-Mail       katayama.hirofumi.mz@gmail.com
