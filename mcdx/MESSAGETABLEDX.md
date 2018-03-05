MESSAGETABLEDX Resource
=======================

The *MESSAGETABLEDX* resource is a dirty extension of Win32 *MESSAGETABLE* resource.
It can be compiled by the [mcdx](https://github.com/katahiromz/RisohEditor/tree/master/mcdx) program, created by Katayama Hirofumi MZ.
The compiled binary data is compatible to Win32 *MESSAGETABLE* resource.

Syntax
------

```rc
#ifdef MCDX_INVOKED
MESSAGETABLEDX
{
    message-statement
    ...
}
#endif
```

*MESSAGETABLEDX* contains one or more *message-statement*'s.

message-statement
-----------------

An *message-statement* is a statement to specify a resource message and has the following syntax:

```rc
message-id, "text"
```

A *message-id* is a 32-bit integer value.

Why do we use MESSAGETABLEDX resources?
---------------------------------------

Win32 *MESSAGETABLE* resource has complicated syntax.
Our *MESSAGETABLEDX* resource is simple.
You can embed the *MESSAGETABLEDX* resources in your resource file (.rc).
To build the resource file with the *MESSAGETABLEDX* resources,
please see [RisohEditor's CMakeList.txt file](https://github.com/katahiromz/RisohEditor/blob/master/src/CMakeLists.txt).

See Also
--------

[mcdx](https://github.com/katahiromz/RisohEditor/tree/master/mcdx)
