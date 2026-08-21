cl /LD /EHsc dllmain.cpp /Fetiny64.dll
dumpbin /headers tiny64.dll | findstr machine
