# Standardize Win32 Resource

New RisohEditor resource information should follow the following guidelines.
If you are using old RisohEditor resource data, then you have to apply the following guidelines.

## DEFINITIONS

- A "resource ID" is the ID, that is used for Win32 resource data.
- An "ID prefix" is the prefix of the resource ID.
- An "entity resource" is a resource data except RT_ICON, RT_CURSOR, RT_STRING, RT_MANIFEST, RT_VERSION and RT_MESSAGETABLE.
- A "resource name" of an entity resource is either a 16-bit integer value, a wide string, or a macro of 16-bit integer value.
- The resource data of RT_ICON type is referred by the resource data of RT_GROUP_ICON type.
- The resource data of RT_CURSOR type is referred by the resource data of RT_GROUP_CURSOR type.
- The RT_STRING or RT_MESSAGETABLE resource can have multiple resource IDs.
- The resource ID of RT_MANIFEST or RT_VERSION has special meanings.
- The "user resource" is a Win32 resource data that the user's project directly provides, and that is not provided by another project or framework.
- The "user ID" is the resource ID that the user's project directly provides, and that is not provided by another project or framework.
- The "icon ID" is the resource ID of RT_GROUP_ICON type.
- The "cursor ID" is the resource ID of RT_GROUP_CURSOR type.
- The "string ID" is the ID of one string data in the string table.
- The "message ID" is the ID of one message data in the message table.

## GUIDELINES

For Visual C++ compatibility, the contents of file "resource.h" should begin with the C++ comments as follows:

```c
//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ Compatible
// TheProject.rc
```

Here, "TheProject.rc" must be replaced with the actual resource file name for this "resource.h".

File "resource.h" should not use include guard.

In "resource.h", please don't define string macros.
Please don't use resource IDs of string value.

If the rc file has #include's of the system headers, then the includes should be wrapped as follows:

```c
#define APSTUDIO_HIDDEN_SYMBOLS
#include <windows.h>
#include <commctrl.h>
#undef APSTUDIO_HIDDEN_SYMBOLS
```

The contents of file "resource.h" should end with the text like:

```c
#ifdef APSTUDIO_INVOKED
    #ifndef APSTUDIO_READONLY_SYMBOLS
        #define _APS_NO_MFC                 1
        #define _APS_NEXT_RESOURCE_VALUE    1000
        #define _APS_NEXT_COMMAND_VALUE     220
        #define _APS_NEXT_CONTROL_VALUE     1002
        #define _APS_NEXT_SYMED_VALUE       300
    #endif
#endif
```

Here, the value of _APS_NO_MFC macro should be one.
Here, the value of _APS_NEXT_RESOURCE_VALUE macro must be the maximum integer value + 1 of all the user entity resources.
Here, the value of _APS_NEXT_COMMAND_VALUE macro must be the maximum integer value + 1 of all the user command IDs.
Here, the value of _APS_NEXT_CONTROL_VALUE macro must be the maximum integer value + 1 of all the user control IDs.
Here, we should ignore the value of _APS_NEXT_SYMED_VALUE macro.

## ID PREFIXES

The user ID prefixes defined in file "resource.h" should apply the following table:

| ID Type                  | ID Prefix |
|--------------------------|-----------|
| String ID                | IDS_      |
| Message ID               | MSGID_    |
| Command ID               | ID_       |
| Command ID (Old Type)    | IDM_      |
| Control ID               | IDC_      |
| Cursor ID                | IDC_      |
| Icon ID                  | IDI_      |
| Dialog ID                | IDD_      |
| Bitmap ID                | IDB_      |
| Other Entity Resource ID | IDR_      |
| Window ID                | IDW_      |
| Help ID                  | HID_      |

Don't use the "IDP_" prefix.

## VALUES AND RANGES

The user ID defined in file "resource.h" should be inside of the following ranges:

| ID Type                  | Bounded Range    | Preferable Range |
|--------------------------|------------------|------------------|
| String ID                | 0 to 0x7FFF      | 100 to 0x7FFF    |
| Message ID               | 0 to 0xFFFFFFFF  | 1 to 0x7FFFFFFF  |
| Command ID               | 0 to 0x7FFF      | 100 to 0x7FFF    |
| Command ID (Old Type)    | 0 to 0x7FFF      | 100 to 0x7FFF    |
| Control ID               | 8 to 0xDFFF      | 1000 to 0x7FFF   |
| Cursor ID                | 0 to 0x7FFF      | 100 to 999       |
| Icon ID                  | 0 to 0x7FFF      | 100 to 999       |
| Dialog ID                | 0 to 0x7FFF      | 100 to 0x7FFF    |
| Bitmap ID                | 0 to 0x7FFF      | 100 to 0x7FFF    |
| Other Entity Resource ID | 0 to 0x7FFF      | 100 to 0x7FFF    |
| Window ID                | 0 to 0x7FFF      | 1 to 0x7FFF      |
| Help ID                  | 0 to 0xFFFFFFFF  | 1 to 0x7FFFFFFF  |

Two different resource IDs of the same ID prefix should have a different value from each other.
The resource ID of ID prefix "IDC_" is either a control ID or a cursor ID.
If we follow the preferable ranges of IDs, then control IDs and a cursor IDs don't collide.

## TEXTINCLUDE

For Visual C++ compatibility, the resource file should have three TEXTINCLUDE data as follows:

```rc
#ifdef APSTUDIO_INVOKED

1 TEXTINCLUDE
BEGIN
    "resource.h\0"
END

2 TEXTINCLUDE 
BEGIN
    "#define APSTUDIO_HIDDEN_SYMBOLS\r\n"
    "#include <windows.h>\r\n"
    "#include <commctrl.h>\r\n"
    "#undef APSTUDIO_HIDDEN_SYMBOLS\r\n"
    "\0"
END

3 TEXTINCLUDE 
BEGIN
    "\r\n"
    "\0"
END

#endif    // APSTUDIO_INVOKED
```

### TEXTINCLUDE 1

TEXTINCLUDE 1 specifies the local header (custom resource header) to be included by this resource file.

However, RisohEditor writes resource IDs on `resource.h` even if you specify a custom resource header.
Please specify that `resource.h` is to be included from the custom resource header.
If you write the RC file to a different location, the custom resource header will be copied to the destination.

When you add `"< "` as a prefix to TEXTINCLUDE 1, the resource file will be considered read-only.
RisohEditor will warn you and ask you to confirm the write if you try to overwrite a read-only resource file.

It is recommended that you use forward slashes (`/`), not backslashes (`\`), to separate paths.

### TEXTINCLUDE 2

TEXTINCLUDE 2 specifies the system headers to be included by this resource file.
You can specify system headers to load in addition to `<windows.h>` and `<commctrl.h>`.

### TEXTINCLUDE 3

TEXTINCLUDE 3 specifies the code to embed read-only resource data in this resource file.
With RisohEditor, you can choose whether or not to import TEXTINCLUDE 3 when loading from the app.

## NOTE

- The resource file and "resource.h" file should use C++ comments rather than C comments.
- `MAKEINTRESOURCE(0)` results in a null pointer, which is why you should not use zero as a resource name.

## SEE ALSO

[https://learn.microsoft.com/en-us/cpp/mfc/tn020-id-naming-and-numbering-conventions](https://learn.microsoft.com/en-us/cpp/mfc/tn020-id-naming-and-numbering-conventions)
