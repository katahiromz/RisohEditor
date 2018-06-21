; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; TODO: Update the version numbers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

AppId={{AF7494D1-406F-4D04-A8FE-8F9DAB97F611}
AppName=RisohEditor
AppVerName=RisohEditor 4.9.8
AppPublisher=Katayama Hirofumi MZ
AppPublisherURL=http://katahiromz.web.fc2.com/
AppSupportURL=http://katahiromz.web.fc2.com/
AppUpdatesURL=http://katahiromz.web.fc2.com/
DefaultDirName=C:\RisohEditor
DefaultGroupName=RisohEditor
AllowNoIcons=yes
LicenseFile=LICENSE.txt
OutputDir=.
OutputBaseFilename=risoheditor-4.9.8-setup
SetupIconFile=src\res\RisohEditor.ico
Compression=lzma
SolidCompression=yes
UninstallDisplayIcon={app}\RisohEditor.exe
VersionInfoCompany=Katayama Hirofumi MZ
VersionInfoCopyright=Copyright (C) 2017-2018 Katayama Hirofumi MZ.
VersionInfoDescription=Win32 Resource Editor
VersionInfoProductName=RisohEditor
VersionInfoProductTextVersion=4.9.8
VersionInfoProductVersion=4.9.8
VersionInfoVersion=4.9.8

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "README.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "READMEJP.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "Standardize.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "HYOJUNKA.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "mcdx\MESSAGETABLEDX.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\RisohEditor.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "src\resource.h"; DestDir: "{app}"; Flags: ignoreversion
Source: "data\Constants.txt"; DestDir: "{app}\data"; Flags: ignoreversion
Source: "data\bin\cpp.exe"; DestDir: "{app}\data\bin"; Flags: ignoreversion
Source: "build\mcdx.exe"; DestDir: "{app}\data\bin"; Flags: ignoreversion
Source: "src\MOleCtrl.hpp"; DestDir: "{app}\OLE"; Flags: ignoreversion
Source: "include\MWindowBase.hpp"; DestDir: "{app}\OLE"; Flags: ignoreversion
Source: "src\DlgInit.h"; DestDir: "{app}\DlgInit"; Flags: ignoreversion
Source: "MyWndCtrl\MWindowBase.hpp"; DestDir: "{app}\MyWndCtrl"; Flags: ignoreversion
Source: "MyWndCtrl\MyWndCtrl.cpp"; DestDir: "{app}\MyWndCtrl"; Flags: ignoreversion
Source: "MyWndCtrl\CMakeLists.txt"; DestDir: "{app}\MyWndCtrl"; Flags: ignoreversion
Source: "build\MyWndCtrl.dll"; DestDir: "{app}\MyWndCtrl"; Flags: ignoreversion
Source: "data\bin\libgcc_s_dw2-1.dll"; DestDir: "{app}\data\bin"; Flags: ignoreversion
Source: "data\bin\libgmp-10.dll"; DestDir: "{app}\data\bin"; Flags: ignoreversion
Source: "data\bin\libwinpthread-1.dll"; DestDir: "{app}\data\bin"; Flags: ignoreversion
Source: "data\bin\windres.exe"; DestDir: "{app}\data\bin"; Flags: ignoreversion
Source: "data\bin\zlib1.dll"; DestDir: "{app}\data\bin"; Flags: ignoreversion
Source: "data\bin\upx.exe"; DestDir: "{app}\data\bin"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\cc1.exe"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\libgcc_s_dw2-1.dll"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\libgmp-10.dll"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\libwinpthread-1.dll"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\zlib1.dll"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\commctrl.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\dlgs.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\windef.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\windows.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\winnt.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\winresrc.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\winuser.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\winver.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
Source: "data\lib\gcc\i686-w64-mingw32\7.3.0\include\afxres.h"; DestDir: "{app}\data\lib\gcc\i686-w64-mingw32\7.3.0\include"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\RisohEditor"; Filename: "{app}\RisohEditor.exe"
Name: "{group}\README.txt"; Filename: "{app}\README.txt"
Name: "{group}\READMEJP.txt"; Filename: "{app}\READMEJP.txt"
Name: "{group}\LICENSE.txt"; Filename: "{app}\LICENSE.txt"
Name: "{group}\{cm:ProgramOnTheWeb,RisohEditor}"; Filename: "http://katahiromz.web.fc2.com"
Name: "{group}\{cm:UninstallProgram,RisohEditor}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\RisohEditor"; Filename: "{app}\RisohEditor.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\RisohEditor.exe"; Description: "{cm:LaunchProgram,RisohEditor}"; Flags: nowait postinstall skipifsilent
