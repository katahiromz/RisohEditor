@echo off
for /R %%f in (build\*.exe build\*.dll *-setup.exe) do (
	echo "%%~ff" | findstr /I /C:"\CMakeFiles\" /C:"\.git\" /C:"\.vs\" >NUL
	if errorlevel 1 (
		call ..\do_sign.bat "%%~ff"
	)
)
