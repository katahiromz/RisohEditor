# Standardize Win32 Resource

New RisohEditor resource information should follow the following rules.
If you are using old RisohEditor resource data, then you have to apply the following rules.

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
- The "user ID" is the ID that the user's project directly provides, and that is not provided by another project or framework.
- The "icon ID" is the resource ID of RT_GROUP_ICON type.
- The "cursor ID" is the resource ID of RT_GROUP_CURSOR type.
- The "string ID" is the ID of one string data in the string table.
- The "message ID" is the ID of one message data in the message table.

## RULES

The contents of file "resource.h" should begin with the C++ comments as follows:

```c
//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ Compatible
// TheProject.rc
```

Here, "TheProject.rc" must be replaced with the actual resource file name for this "resource.h".

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

| ID Type                  | ID Prefix | Bounded Range | Preferable Range |
|--------------------------|-----------|---------------|------------------|
| String ID                | IDS_      | 1 to 0x7FFF   | 100 to 0x7FFF    |
| Message ID               | MSGID_    | 0 to 0x7FFF   | 0 to 0x7FFF      |
| Command ID               | ID_       | 1 to 0x7FFF   | 100 to 0x7FFF    |
| Command ID (Old Type)    | IDM_      | 1 to 0x7FFF   | 100 to 0x7FFF    |
| Control ID               | IDC_      | 8 to 0xDFFF   | 1000 to 0x7FFF   |
| Cursor ID                | IDC_      | 1 to 0x7FFF   | 100 to 999       |
| Icon ID                  | IDI_      | 1 to 0x7FFF   | 100 to 999       |
| Dialog ID                | IDD_      | 1 to 0x7FFF   | 100 to 0x7FFF    |
| Bitmap ID                | IDB_      | 1 to 0x7FFF   | 100 to 0x7FFF    |
| Other Entity Resource ID | IDR_      | 1 to 0x7FFF   | 100 to 0x7FFF    |
| Window ID                | IDW_      | 1 to 0x7FFF   | 1 to 0x7FFF      |

Two different resource IDs of the same ID prefix should have a different value from each other.
The resource data of ID prefix "IDC_" is either a control ID or a cursor ID.
If we follow the preferable ranges of ID prefixes, then control IDs and a cursor IDs don't collide.

Don't use the "IDP_" prefix.

## TEXTINCLUDE

The resource file should have three TEXTINCLUDE data as follows:

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

## SEE ALSO

[https://msdn.microsoft.com/en-us/library/t2zechd4.aspx](https://msdn.microsoft.com/en-us/library/t2zechd4.aspx)
