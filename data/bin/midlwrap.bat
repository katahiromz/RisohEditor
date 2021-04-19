@rem midlwrap.bat --- Execute MIDL Compiler
@rem %1 --- Visual Studio's vcvarsall.bat
@rem %2 --- x86 or amd64
@rem %3 --- the input file (*.idl)
@rem %4 --- tht output file (*.tlb)
@echo off
call %1 %2
midl /nologo /no_warn %3 /tlb %4
exit
