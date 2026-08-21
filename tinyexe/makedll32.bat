cl /LD /EHsc dllmain.cpp /Fetiny32.dll
dumpbin /headers tiny32.dll | findstr machine
