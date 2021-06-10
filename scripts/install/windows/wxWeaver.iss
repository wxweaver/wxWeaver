
; File:            wxWeaver.iss
; Original Author: Ryan Pusztai & Ryan Mulder (as wxFormBuilder)
; Author:          Andrea Zanellato
; Date:            07/05/2021
; Copyright:       (c) 2007 Ryan Pusztai & Ryan Mulder (as wxFormBuilder)
;                  (c) 2021 Andrea Zanellato <redtid3@gmail.com>
; License:         wxWindows license

; MyAppVer gets parsed from source file
#define MyAppName "wxWeaver"
#define MyAppPublisher "wxWeaver Team"
#define MyAppURL "https://wxweaver.github.io"
#define MyAppExeName "wxWeaver.exe"

#define protected FileHandle
#define protected FileLine

#sub ProcessVersionLine
  #define private FileLine = FileRead(FileHandle)
  #if Pos("VERSION = """, FileLine) > 0
    #define private temp Copy(FileLine, Pos("""", FileLine) + 1)
    #define public MyAppVer Copy(temp, 1, RPos("""", temp) - 1)
  #endif
#endsub

#for {FileHandle = FileOpen("..\..\src\gui\appdata.cpp"); FileHandle && !FileEof(FileHandle) && !Defined(MyAppVer); ""} ProcessVersionLine
#if FileHandle
  #expr FileClose(FileHandle)
#endif

[Setup]
AppName={#MyAppName}
AppVerName={#MyAppName} {#MyAppVer}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf}\{#MyAppName}
DisableDirPage=false
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=false
OutputBaseFilename={#MyAppName}-{#MyAppVer}
Compression=lzma/ultra
SolidCompression=true
InternalCompressLevel=ultra
OutputDir=.
ShowLanguageDialog=yes
AppVersion={#MyAppVer}
AppendDefaultGroupName=false
AllowNoIcons=true
WizardImageFile=compiler:WizModernImage-IS.bmp
WizardSmallImageFile=compiler:WizModernSmallImage-IS.bmp
SetupIconFile=support\wxWeaver.ico
UninstallDisplayIcon={app}\wxWeaver.exe
ChangesAssociations=true
VersionInfoVersion={#MyAppVer}
VersionInfoDescription={#MyAppName}
;InfoAfterFile=..\..\output\Changelog.txt
LicenseFile=..\..\COPYING
MinVersion=0,6.0

[Messages]
BeveledLabel={#MyAppName} {#MyAppVer}

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: main; Description: wxWeaver (required); Types: full custom; Flags: fixed checkablealone
Name: runtime; Description: Compiler Runtime (required); Types: full custom; Flags: fixed checkablealone

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked

[Files]
#define protected RootDir "..\..\output"
#define protected PluginsBaseDir "plugins"

#define protected FindHandle
#define protected FindResult

#sub ProcessFoundPlugin
  #define private FileName FindGetFileName(FindHandle)
  #if FileName != "." && FileName != ".."
    #define private PluginsSourceDir AddBackslash(RootDir) + AddBackslash(PluginsBaseDir) + FileName
    #define private PluginsDestDir AddBackslash(PluginsBaseDir) + FileName
    Source: {#PluginsSourceDir}\lib{#FileName}.dll; DestDir: {app}\{#PluginsDestDir}; Flags: ignoreversion; Components: main
    Source: {#PluginsSourceDir}\icons\*; DestDir: {app}\{#PluginsDestDir}\icons; Flags: recursesubdirs createallsubdirs; Components: main
    Source: {#PluginsSourceDir}\xml\*; DestDir: {app}\{#PluginsDestDir}\xml; Flags: recursesubdirs createallsubdirs; Components: main
  #endif
#endsub

Source: {#RootDir}\Changelog.txt; DestDir: {app}; Components: main
Source: {#RootDir}\license.txt; DestDir: {app}; Components: main
Source: {#RootDir}\wxWeaver.exe; DestDir: {app}; Flags: ignoreversion; Components: main
Source: {#RootDir}\resources\*; DestDir: {app}\resources; Flags: recursesubdirs createallsubdirs; Components: main
Source: {#RootDir}\xml\*; DestDir: {app}\xml; Flags: recursesubdirs createallsubdirs; Components: main
#for {FindHandle = FindResult = FindFirst(AddBackslash(RootDir) + AddBackslash(PluginsBaseDir) + "*", faDirectory); FindResult; FindResult = FindNext(FindHandle)} ProcessFoundPlugin
#if FindHandle
  #expr FindClose(FindHandle)
#endif

Source: C:\msys64\mingw32\bin\wx*.dll; DestDir: {app}; Components: main

Source: C:\msys64\mingw32\bin\libstdc++*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libgcc*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libintl*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libexpat*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libjpeg*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libpng*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libtiff*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\zlib*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libwinpthread*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libiconv*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\liblzma*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libzstd*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libdeflate*.dll; DestDir: {app}; Components: runtime
Source: C:\msys64\mingw32\bin\libwebp*.dll; DestDir: {app}; Components: runtime

[Icons]
Name: {group}\{#MyAppName}; Filename: {app}\{#MyAppExeName}
;Name: {group}\{#MyAppName} Help; Filename: {app}\{#MyAppName}.hlp
Name: {group}\{cm:UninstallProgram,{#MyAppName}}; Filename: {uninstallexe}
Name: {autodesktop}\{#MyAppName}; Filename: {app}\{#MyAppExeName}; Tasks: desktopicon

[Registry]
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\{#MyAppExeName}; ValueType: string; ValueData: {app}\{#MyAppExeName}; Flags: uninsdeletekey
Root: HKCR; SubKey: .fbp; ValueType: string; ValueData: {#MyAppName}.Project; Flags: uninsdeletekey
Root: HKCR; SubKey: {#MyAppName}.Project\DefaultIcon; ValueType: string; ValueName: ; ValueData: {app}\{#MyAppExeName}; Flags: uninsdeletekey
Root: HKCR; SubKey: {#MyAppName}.Project; ValueType: string; ValueData: {#MyAppName} Project File; Flags: uninsdeletekey
Root: HKCR; SubKey: {#MyAppName}.Project\Shell\Open\Command; ValueType: string; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletevalue

[Run]
Filename: {app}\{#MyAppExeName}; Description: {cm:LaunchProgram,{#MyAppName}}; Flags: nowait postinstall skipifsilent

; Debug output of preprocessor up to current line, at end == complete file
;#expr SaveToFile("preprocessed.iss")
