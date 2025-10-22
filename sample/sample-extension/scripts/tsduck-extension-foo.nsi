; NSIS script to build the TSDuck extension binary installer for Windows.
; Do not invoke NSIS directly, use build-installer.ps1.

!verbose push
!verbose 0
!include "MUI2.nsh"
!include "Sections.nsh"
!include "TextFunc.nsh"
!include "FileFunc.nsh"
!include "WinMessages.nsh"
!include "x64.nsh"
!verbose pop

; Get TSDuck version.
!system 'tsversion --version=nsis 2>version-tmp.nsh'
!include version-tmp.nsh
!delfile version-tmp.nsh

; Product identification.
!define FullName "TSDuck Foo Extension"
!define ShortName "TSDuck-Extension-Foo"
!define AuthorName "Foo Company"
!define DocLinkName "TSDuck Foo Extension User Guide"

Name "${ShortName}"
Caption "${FullName} Installer"

VIProductVersion ${tsduckVersionInfo}
VIAddVersionKey ProductName "${FullName}"
VIAddVersionKey ProductVersion "${tsduckVersion}"
VIAddVersionKey Comments "${FullName}"
VIAddVersionKey CompanyName "${AuthorName}"
VIAddVersionKey LegalCopyright "Copyright (c) ${AuthorName}"
VIAddVersionKey FileVersion "${tsduckVersionInfo}"
VIAddVersionKey FileDescription "${FullName}"

; Directories.
!define RootDir ".."
!define SrcDir "${RootDir}\src"
!define DocDir "${RootDir}\doc"
!ifdef Win64
    !define BinDir "${RootDir}\msvc\Release-x64"
    OutFile "${RootDir}\installers\${ShortName}-Win64-${tsduckVersion}.exe"
!else
    !define BinDir "${RootDir}\msvc\Release-Win32"
    OutFile "${RootDir}\installers\${ShortName}-Win32-${tsduckVersion}.exe"
!endif

; Generate a Unicode installer (default is ANSI).
Unicode true

; Registry entry for product info and uninstallation info.
!define ProductKey "Software\${ShortName}"
!define UninstallKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ShortName}"

; Use XP manifest.
XPStyle on

; Request administrator privileges for Windows Vista and higher.
RequestExecutionLevel admin

; "Modern User Interface" (MUI) settings
!define MUI_ABORTWARNING
!define MUI_ICON "tsduck-extension.ico"
!define MUI_UNICON "tsduck-extension.ico"

; The installation folder is the same as TSDuck.
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
                "This is a 64-bit version of ${ShortName}.$\r$\n\
                You have a 32-bit version of Windows.$\r$\n\
                Please use a 32-bit version of ${ShortName} on this system."
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

    ; Create folder for binaries (inside TSDuck installation).
    CreateDirectory "$INSTDIR\bin"
    SetOutPath "$INSTDIR\bin"
    File "${BinDir}\footool.exe"
    File "${BinDir}\tsplugin_foot.dll"
    File "${BinDir}\tslibext_foo.dll"
    File "${SrcDir}\tslibext_foo.xml"
    File "${SrcDir}\tslibext_foo.names"

    ; Documentation
    CreateDirectory "$INSTDIR\doc"
    SetOutPath "$INSTDIR\doc"
    File "${DocDir}\tsduck-extension-foo.pdf"

    ; Create shortcuts in start menu (documentation only, same as TSDuck).
    CreateDirectory "$SMPROGRAMS\TSDuck"
    CreateShortCut "$SMPROGRAMS\TSDuck\${DocLinkName}.lnk" "$INSTDIR\doc\tsduck-extension-foo.pdf"

    ; Store installation folder in registry.
    WriteRegStr HKLM "${ProductKey}" "InstallDir" $INSTDIR

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\${ShortName}-Uninstall.exe"

    ; Declare uninstaller in "Add/Remove Software" control panel
    WriteRegStr HKLM "${UninstallKey}" "DisplayName" "${FullName}"
    WriteRegStr HKLM "${UninstallKey}" "Publisher" "${AuthorName}"
    WriteRegStr HKLM "${UninstallKey}" "DisplayVersion" "${tsduckVersion}"
    WriteRegStr HKLM "${UninstallKey}" "DisplayIcon" "$INSTDIR\${ShortName}-Uninstall.exe"
    WriteRegStr HKLM "${UninstallKey}" "UninstallString" "$INSTDIR\${ShortName}-Uninstall.exe"

SectionEnd

;-----------------------------------------------------------------------------
; Uninstallation section
;-----------------------------------------------------------------------------

Section "Uninstall"

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Get installation folder from registry
    ReadRegStr $0 HKLM "${ProductKey}" "InstallDir"

    ; Delete product files.
    Delete "$0\bin\footool.exe"
    Delete "$0\bin\tsplugin_foot.dll"
    Delete "$0\bin\tslibext_foo.dll"
    Delete "$0\bin\tslibext_foo.xml"
    Delete "$0\bin\tslibext_foo.names"
    Delete "$0\doc\tsduck-extension-foo.pdf"
    Delete "$0\${ShortName}-Uninstall.exe"

    ; Delete directories which are not empty.
    ; If TSDuck is still installed, this deletes nothing.
    RMDir "$0\bin"
    RMDir "$0\doc"
    RMDir "$0"

    ; Delete start menu entries
    Delete "$SMPROGRAMS\TSDuck\${DocLinkName}.lnk"

    ; Delete registry entries
    DeleteRegKey HKCU "${ProductKey}"
    DeleteRegKey HKLM "${ProductKey}"
    DeleteRegKey HKLM "${UninstallKey}"

SectionEnd
