@rem midlwrap.bat --- Execute MIDL Compiler
@rem %1 --- x86 or amd64
@rem %2 --- the input file (*.idl)
@rem %3 --- tht output file (*.tlb)
@echo off
set VCBAT=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=C:\Program Files\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=C:\Program Files\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=C:\Program Files\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=%VS140COMNTOOLS%..\..\VC\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=%VS120COMNTOOLS%..\..\VC\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=%VS110COMNTOOLS%..\..\VC\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=%VS100COMNTOOLS%..\..\VC\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
set VCBAT=%VS90COMNTOOLS%..\..\VC\vcvarsall.bat
if EXIST "%VCBAT%" goto vc_ok
echo error: Visual Studio vcvarsall.bat not found. 1>&2
exit 1000
:vc_ok
call "%VCBAT%" %1
midl /nologo /no_warn %2 /tlb %3
exit
