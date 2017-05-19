;-----------------------------------------------------------------------------
;
;  TSDuck - The MPEG Transport Stream Toolkit
;  Copyright (c) 2005-2017, Thierry Lelegard
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

!verbose push
!verbose 0
!include "MUI2.nsh"
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

; Name of binary installer file.
!ifdef Win64
    OutFile "${InstallerDir}\TSDuck-Win64-${tsduckVersion}.exe"
!else
    OutFile "${InstallerDir}\TSDuck-Win32-${tsduckVersion}.exe"
!endif

; Registry entry for product info and uninstallation info.
!define ProductKey "Software\TSDuck"
!define UninstallKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\TSDuck"

; Use XP manifest.
XPStyle on

; Request administrator privileges for Windows Vista and higher.
RequestExecutionLevel admin

; "Modern User Interface" (MUI) settings
!define MUI_ABORTWARNING
!define MUI_ICON "${RootDir}\images\tsduck.ico"
!define MUI_UNICON "${RootDir}\images\tsduck.ico"

; Default installation folder
!ifdef Win64
    InstallDir "$PROGRAMFILES64\TSDuck"
!else
    InstallDir "$PROGRAMFILES\TSDuck"
!endif

; Get installation folder from registry if available from a previous installation.
InstallDirRegKey HKLM "${ProductKey}" "InstallDir"

; Installer pages
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
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
; Installation section
;-----------------------------------------------------------------------------

Section "Install"

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Create folder for binaries
    CreateDirectory "$INSTDIR\bin"
    SetOutPath "$INSTDIR\bin"
    File "${BinDir}\ts*.exe"
    File "${BinDir}\ts*.dll"

    ; Setup tools
    CreateDirectory "$INSTDIR\setup"
    SetOutPath "$INSTDIR\setup"
    File "${BinDir}\setpath.exe"
    File "${ProjectDir}\redist\${MsvcRedistExe}"

    ; Store installation folder in registry.
    WriteRegStr HKLM "${ProductKey}" "InstallDir" $INSTDIR

    ; Install or reinstall the Visual C++ redistributable library.
    ExecWait '"$INSTDIR\setup\${MsvcRedistExe}" /q /norestart'

    ; Add binaries folder to system path
    ExecWait '"$INSTDIR\setup\setpath.exe" --prepend "$INSTDIR\bin"'

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\TSDuckUninstall.exe"

    ; Declare uninstaller in "Add/Remove Software" control panel
    WriteRegStr HKLM "${UninstallKey}" "DisplayName" "TSDuck"
    WriteRegStr HKLM "${UninstallKey}" "DisplayVersion" "${tsduckVersion}"
    WriteRegStr HKLM "${UninstallKey}" "DisplayIcon" "$INSTDIR\TSDuckUninstall.exe"
    WriteRegStr HKLM "${UninstallKey}" "UninstallString" "$INSTDIR\TSDuckUninstall.exe"

SectionEnd

;-----------------------------------------------------------------------------
; Uninstallation section
;-----------------------------------------------------------------------------

Section "Uninstall"

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Get installation folder from registry
    ReadRegStr $0 HKLM "${ProductKey}" "InstallDir"

    ; Remove binaries folder from system path
    ExecWait '"$0\setup\setpath.exe" --remove "$0\bin"'

    ; Delete product files.
    RMDir /r "$0\bin"
    Delete "$0\setup\setpath.exe"
    Delete "$0\setup\${MsvcRedistExe}"
    RMDir "$0\setup"
    Delete "$0\TSDuckUninstall.exe"
    RMDir "$0"

    ; Delete registry entries
    DeleteRegKey HKCU "${ProductKey}"
    DeleteRegKey HKLM "${ProductKey}"
    DeleteRegKey HKLM "${UninstallKey}"

SectionEnd
