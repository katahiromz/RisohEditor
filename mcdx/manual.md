MESSAGETABLEDX Resource
=======================

The *MESSAGETABLEDX* resource is a dirty extension of the *MESSAGETABLE* resource.
It can be compiled by the [mcdx](https://github.com/katahiromz/RisohEditor/tree/master/mcdx) program, created by Katayama Hirofumi MZ.

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

See Also
--------

[mcdx](https://github.com/katahiromz/RisohEditor/tree/master/mcdx)
