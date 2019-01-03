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

## HISTORY

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
 * 2018.11.08 ver.5.1.1
    - Accept space in the caption text box.
    - Accept space in the language text box.
 * 2018.12.08 ver.5.1.2
    - Accept space in the window class name text box.
    - Accept C string literal in the window class name text box.
    - Improve template system.
    - Enable manifest template.
    - Fix toolbar UI update.
    - Correctly display ID types of "List of Resource IDs".
    - Correctly sort the languages upon output.
 * 2019.01.01 ver.5.1.3
    - Fix "List of Resource IDs".
 * 2019.XX.YY ver.5.1.4
    - Add English installer.

## Contact Us

katayama.hirofumi.mz@gmail.com
