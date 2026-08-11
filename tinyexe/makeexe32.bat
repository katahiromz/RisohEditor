cl /O1 /EHsc exemain.cpp /Fetiny32.exe
dumpbin /headers tiny32.exe | findstr machine
