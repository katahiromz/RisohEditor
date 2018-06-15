[![Build Status on Travis CI](https://travis-ci.org/katahiromz/RisohEditor.svg?branch=master)](https://travis-ci.org/katahiromz/RisohEditor)
[![Build status on AppVeyor](https://ci.appveyor.com/api/projects/status/4sdaed4vyakby61h?svg=true)](https://ci.appveyor.com/project/katahiromz/risoheditor)

# RisohEditor by katahiromz

RisohEditor is a free resource editor for Win32 development, created by Katayama Hirofumi MZ.

## Supported Platforms

It works on Windows XP/2003/Vista/7/8.1/10.

## License Agreement

See "LICENSE.txt" for details of copyrights and license agreement.

## NOTICE

Please avoid installing in "C:\Program Files" or "C:\Program Files (x86)". Because its resource compiler (windres) doesn't accept space character in the file path. 

## Standardization

See "Standardize.md" for standardization of resource IDs.

## FAQ

### Question 1. What is "Risoh"?

The word "risoh" means "ideal" in Japanese.

### Question 2. What are edt1, edt2, cmb1?

Those are standard control ID macros defined in <dlgs.h>.

### Question 3. Why can't I add a control data to my dialog item?

Make the dialog box extended.

### Question 4. What is mcdx?

It's a special message compiler I made. See mcdx/MESSAGETABLEDX.md for details.

## Contact Us

katayama.hirofumi.mz@gmail.com
