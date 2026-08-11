cl /O1 /EHsc exemain.cpp /Fetiny64.exe
dumpbin /headers tiny64.exe | findstr machine
