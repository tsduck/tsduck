;-----------------------------------------------------------------------------
;
;  TSDuck - The MPEG Transport Stream Toolkit
;  Copyright (c) 2005-2018, Thierry Lelegard
;  All rights reserved.
;
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions are met:
;
;  1. Redistributions of source code must retain the above copyright notice,
;     this list of conditions and the following disclaimer.
;  2. Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
;  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
;  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
;  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
;  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
;  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
;  THE POSSIBILITY OF SUCH DAMAGE.
;
;-----------------------------------------------------------------------------
;
;  NSIS script to build the TSDuck binary installer for Windows.
;
;  Do not invoke NSIS directly, use Build-Installer.ps1.
;  If Win64 is defined, generate a 64-bit installer (default: 32-bit).
;  If ProjectDir is defined, it designates the subdirectory containing the
;  Visual Studio files. By default, use "msvc2017".
;
;-----------------------------------------------------------------------------

Name "TSDuck"
Caption "TSDuck Installer"

!verbose push
!verbose 0
!include "MUI2.nsh"
!include "Sections.nsh"
!include "TextFunc.nsh"
!include "FileFunc.nsh"
!include "WinMessages.nsh"
!include "x64.nsh"
!verbose pop

; Directories.
!define RootDir ".."
!define InstallerDir "${RootDir}\installers"
!ifndef ProjectDir
    !define ProjectDir "${RootDir}\build\msvc2017"
!endif
!ifdef Win64
    !define BinDir "${ProjectDir}\Release-x64"
    !define MsvcRedistExe "vcredist64.exe"
!else
    !define BinDir "${ProjectDir}\Release-Win32"
    !define MsvcRedistExe "vcredist32.exe"
!endif

; Get TSDuck version
!system '"${BinDir}\tsp.exe" --version=nsis 2>tsduck-tmp.nsh'
!include tsduck-tmp.nsh
!delfile tsduck-tmp.nsh

VIProductVersion ${tsduckVersionInfo}
VIAddVersionKey ProductName "TSDuck"
VIAddVersionKey ProductVersion "${tsduckVersion}"
VIAddVersionKey Comments "TSDuck - The MPEG Transport Stream Toolkit"
VIAddVersionKey CompanyName "Thierry Lelegard"
VIAddVersionKey LegalCopyright "Copyright (c) 2005-2018, Thierry Lelegard"
VIAddVersionKey FileVersion "${tsduckVersionInfo}"
VIAddVersionKey FileDescription "TSDuck - The MPEG Transport Stream Toolkit"

; Name of binary installer file.
!ifdef Win64
    OutFile "${InstallerDir}\TSDuck-Win64-${tsduckVersion}.exe"
!else
    OutFile "${InstallerDir}\TSDuck-Win32-${tsduckVersion}.exe"
!endif

; Registry key for environment variables
!define EnvironmentKey '"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'

; Registry entry for product info and uninstallation info.
!define ProductKey "Software\TSDuck"
!define UninstallKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\TSDuck"

; Use XP manifest.
XPStyle on

; Request administrator privileges for Windows Vista and higher.
RequestExecutionLevel admin

; "Modern User Interface" (MUI) settings.
!define MUI_ABORTWARNING
!define MUI_ICON "${RootDir}\images\tsduck.ico"
!define MUI_UNICON "${RootDir}\images\tsduck.ico"

; Default installation folder.
!ifdef Win64
    InstallDir "$PROGRAMFILES64\TSDuck"
!else
    InstallDir "$PROGRAMFILES\TSDuck"
!endif

; Get installation folder from registry if available from a previous installation.
InstallDirRegKey HKLM "${ProductKey}" "InstallDir"

; Installer pages. The function ComponentsPre is invoked before the components page.
!insertmacro MUI_PAGE_LICENSE "${RootDir}\LICENSE.txt"
!define MUI_PAGE_CUSTOMFUNCTION_PRE ComponentsPre
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Uninstaller pages.
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages.
!insertmacro MUI_LANGUAGE "English"

;-----------------------------------------------------------------------------
; Initialization functions
;-----------------------------------------------------------------------------

; Installation initialization.
function .onInit
    ; In 64-bit installers, don't use registry redirection. Also prevent execution
    ; of 64-bit installers on 32-bit systems (the installer itself is 32-bit and
    ; can run on 32-bit systems but it contains 64-bit executables).
    !ifdef Win64
        ${If} ${RunningX64}
            SetRegView 64
        ${Else}
            MessageBox MB_OK|MB_ICONSTOP \
                "This is a 64-bit version of TSDuck.$\r$\n\
                You have a 32-bit version of Windows.$\r$\n\
                Please use a 32-bit version of TSDuck on this system."
            Quit
        ${EndIf}
    !endif
functionEnd

; Uninstallation initialization.
function un.onInit
    ; In 64-bit installers, don't use registry redirection.
    !ifdef Win64
        ${If} ${RunningX64}
            SetRegView 64
        ${EndIf}
    !endif
functionEnd

;-----------------------------------------------------------------------------
; Installation sections
;-----------------------------------------------------------------------------

; Installation of command-line tools and plugins.
Section "Tools & Plugins" SectionTools

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Create folder for binaries
    CreateDirectory "$INSTDIR\bin"
    SetOutPath "$INSTDIR\bin"
    File /x *_static.exe "${BinDir}\ts*.exe"
    File "${BinDir}\ts*.dll"
    File "${RootDir}\src\libtsduck\tsduck.xml"
    File "${RootDir}\src\libtsduck\tsduck.*.names"

    ; Delete obsolete files from previous versions.
    Delete "$INSTDIR\bin\tsgentab.exe"
    Delete "$INSTDIR\bin\tsgentab*.dll"

SectionEnd

; Installation of documentation files.
Section "Documentation" SectionDocumentation

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Documentation files.
    CreateDirectory "$INSTDIR\doc"
    SetOutPath "$INSTDIR\doc"
    File "${RootDir}\doc\tsduck.pdf"
    File "${RootDir}\CHANGELOG.txt"
    File "${RootDir}\LICENSE.txt"

    ; Create shortcuts in start menu.
    CreateDirectory "$SMPROGRAMS\TSDuck"
    CreateShortCut "$SMPROGRAMS\TSDuck\TSDuck User's Guide.lnk" "$INSTDIR\doc\tsduck.pdf"
    CreateShortCut "$SMPROGRAMS\TSDuck\TSDuck Release Notes.lnk" "$INSTDIR\doc\CHANGELOG.txt"

SectionEnd

; Installation of development environment for third-party applications.
; Unselected by default (/o).
Section /o "Development" SectionDevelopment

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Delete obsolete files from previous versions.
    Delete "$INSTDIR\include\tinyxml*"

    ; TSDuck header files.
    CreateDirectory "$INSTDIR\include"
    SetOutPath "$INSTDIR\include"
    File "${RootDir}\src\libtsduck\*.h"
    File "${RootDir}\src\libtsduck\windows\*.h"

    ; TSDuck libraries.
    CreateDirectory "$INSTDIR\lib"
    CreateDirectory "$INSTDIR\lib\Release-Win32"
    SetOutPath "$INSTDIR\lib\Release-Win32"
    File "${ProjectDir}\Release-Win32\tsduck.lib"
    File "${ProjectDir}\Release-Win32\tsduck.dll"

    CreateDirectory "$INSTDIR\lib\Release-Win64"
    SetOutPath "$INSTDIR\lib\Release-Win64"
    File "${ProjectDir}\Release-x64\tsduck.lib"
    File "${ProjectDir}\Release-x64\tsduck.dll"

    CreateDirectory "$INSTDIR\lib\Debug-Win32"
    SetOutPath "$INSTDIR\lib\Debug-Win32"
    File "${ProjectDir}\Debug-Win32\tsduck.lib"
    File "${ProjectDir}\Debug-Win32\tsduck.dll"

    CreateDirectory "$INSTDIR\lib\Debug-Win64"
    SetOutPath "$INSTDIR\lib\Debug-Win64"
    File "${ProjectDir}\Debug-x64\tsduck.lib"
    File "${ProjectDir}\Debug-x64\tsduck.dll"

    ; Visual Studio property files.
    SetOutPath "$INSTDIR"
    File "${RootDir}\build\tsduck.props"

SectionEnd

; Common final mandatory section.
; Not selectable, not displayed (because of leading '-' in name).
Section "-Common" SectionCommon

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Setup tools.
    CreateDirectory "$INSTDIR\setup"
    SetOutPath "$INSTDIR\setup"
    File "${BinDir}\setpath.exe"
    File "${ProjectDir}\redist\${MsvcRedistExe}"

    ; Add an environment variable to TSDuck root.
    WriteRegStr HKLM ${EnvironmentKey} "TSDUCK" "$INSTDIR"

    ; Store installation folder in registry.
    WriteRegStr HKLM "${ProductKey}" "InstallDir" $INSTDIR

    ; Install or reinstall the Visual C++ redistributable library.
    ExecWait '"$INSTDIR\setup\${MsvcRedistExe}" /q /norestart'

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\TSDuckUninstall.exe"
 
    ; Declare uninstaller in "Add/Remove Software" control panel
    WriteRegStr HKLM "${UninstallKey}" "DisplayName" "TSDuck"
    WriteRegStr HKLM "${UninstallKey}" "Publisher" "Thierry Lelegard"
    WriteRegStr HKLM "${UninstallKey}" "URLInfoAbout" "https://tsduck.io/"
    WriteRegStr HKLM "${UninstallKey}" "DisplayVersion" "${tsduckVersion}"
    WriteRegStr HKLM "${UninstallKey}" "DisplayIcon" "$INSTDIR\TSDuckUninstall.exe"
    WriteRegStr HKLM "${UninstallKey}" "UninstallString" "$INSTDIR\TSDuckUninstall.exe"

    ; Get estimated size of installed files
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "${UninstallKey}" "EstimatedSize" "$0"

    ; Remember installation selections in registry.
    ; Cleanup previously installed options which are now unselected.
    ${If} ${SectionIsSelected} ${SectionTools}
        WriteRegStr HKLM "${ProductKey}" "InstallTools" "true"
        ; Add binaries folder to system path
        ExecWait '"$INSTDIR\setup\setpath.exe" --prepend "$INSTDIR\bin"'
    ${Else}
        WriteRegStr HKLM "${ProductKey}" "InstallTools" "false"
        ; Remove previous installation of tools & plugins.
        RMDir /r "$INSTDIR\bin"
        ; Remove binaries folder from system path
        ExecWait '"$INSTDIR\setup\setpath.exe" --remove "$INSTDIR\bin"'
    ${EndIf}
    ${If} ${SectionIsSelected} ${SectionDocumentation}
        WriteRegStr HKLM "${ProductKey}" "InstallDocumentation" "true"
    ${Else}
        WriteRegStr HKLM "${ProductKey}" "InstallDocumentation" "false"
        ; Remove previous installation of documentation.
        RMDir /r "$INSTDIR\doc"
        RMDir /r "$SMPROGRAMS\TSDuck"
    ${EndIf}
    ${If} ${SectionIsSelected} ${SectionDevelopment}
        WriteRegStr HKLM "${ProductKey}" "InstallDevelopment" "true"
    ${Else}
        WriteRegStr HKLM "${ProductKey}" "InstallDevelopment" "false"
        ; Remove previous installation of development environment.
        RMDir /r "$INSTDIR\include"
        RMDir /r "$INSTDIR\lib"
        Delete "$INSTDIR\tsduck.props"
    ${EndIf}

SectionEnd

; Description of installation sections (displayed by mouse hover).
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SectionTools} \
        "TSDuck command-line tools and plugins."
    !insertmacro MUI_DESCRIPTION_TEXT ${SectionDocumentation} \
        "TSDuck user's guide."
    !insertmacro MUI_DESCRIPTION_TEXT ${SectionDevelopment} \
        "TSDuck development environment, for use from third-party applications manipulating MPEG transport streams."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; This function is called before the "components" page.
; It presets the selections identically to the previous installation.
function ComponentsPre
    ReadRegStr $0 HKLM "${ProductKey}" "InstallTools"
    ${If} $0 == "true"
        !insertmacro SelectSection ${SectionTools}
    ${EndIf}
    ${If} $0 == "false"
        !insertmacro UnSelectSection ${SectionTools}
    ${EndIf}
    ReadRegStr $0 HKLM "${ProductKey}" "InstallDocumentation"
    ${If} $0 == "true"
        !insertmacro SelectSection ${SectionDocumentation}
    ${EndIf}
    ${If} $0 == "false"
        !insertmacro UnSelectSection ${SectionDocumentation}
    ${EndIf}
    ReadRegStr $0 HKLM "${ProductKey}" "InstallDevelopment"
    ${If} $0 == "true"
        !insertmacro SelectSection ${SectionDevelopment}
    ${EndIf}
    ${If} $0 == "false"
        !insertmacro UnSelectSection ${SectionDevelopment}
    ${EndIf}
functionEnd

;-----------------------------------------------------------------------------
; Uninstallation section
;-----------------------------------------------------------------------------

Section "Uninstall"

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Get installation folder from registry
    ReadRegStr $0 HKLM "${ProductKey}" "InstallDir"

    ; Delete start menu entries  
    RMDir /r "$SMPROGRAMS\TSDuck"

    ; Delete product registry entries
    DeleteRegKey HKCU "${ProductKey}"
    DeleteRegKey HKLM "${ProductKey}"
    DeleteRegKey HKLM "${UninstallKey}"
    DeleteRegValue HKLM ${EnvironmentKey} "TSDUCK"

    ; Remove binaries folder from system path
    ExecWait '"$0\setup\setpath.exe" --remove "$0\bin"'

    ; Delete product files.
    RMDir /r "$0\bin"
    RMDir /r "$0\doc"
    RMDir /r "$0\include"
    RMDir /r "$0\lib"
    Delete "$0\tsduck.props"
    Delete "$0\setup\setpath.exe"
    Delete "$0\setup\${MsvcRedistExe}"
    RMDir "$0\setup"
    Delete "$0\TSDuckUninstall.exe"
    RMDir "$0"

SectionEnd
