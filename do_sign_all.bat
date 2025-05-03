@echo off
for /R %%f in (*.exe *.dll) do (
    call ..\do_sign.bat "%%~ff"
)
