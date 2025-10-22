;-----------------------------------------------------------------------------
;
;  TSDuck - The MPEG Transport Stream Toolkit
;  Copyright (c) 2005-2025, Thierry Lelegard
;  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
;
;  NSIS script to build the TSDuck binary installer for Windows.
;  Do not invoke NSIS directly, use build-installer.ps1.
;
;  Required command-line definitions:
;  - Version : Product version.
;  - VersionInfo : Product version info in Windows format.
;  - BinDir : Directory of built binaries (.exe and .dll).
;  - Win32 : If defined, generate a 32-bit Intel installer.
;  - Win64 : If defined, generate a 64-bit Intel installer.
;  - Arm64 : If defined, generate a 64-bit Arm installer.
;  - DevWin32 : If defined, add development libraries for Win32.
;  - DevWin64 : If defined, add development libraries for Win64.
;  - DevArm64 : If defined, add development libraries for Arm64.
;  - HeadersDir : Directory containing all header files (development options).
;  - VCRedist : Full path of the MSVC redistributable installer.
;  - VCRedistName : Base name of the MSVC redistributable installer.
;  - JarFile : Optional name of the JAR file for Java bindings.
;
;  Note: Win32, Win64, Arm64 are mutually exclusive. This is the architecture
;  of the binaries. DevWin32, DevWin64, DevArm64 can be combined. Several
;  development environments can be included in the same installer.
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
!define RootDir "..\.."
!define InstallerDir "${RootDir}\pkg\installers"
!define PythonDir "${RootDir}\src\libtsduck\python"
!define BinRoot "${BinDir}\.."

VIProductVersion ${VersionInfo}
VIAddVersionKey ProductName "TSDuck"
VIAddVersionKey ProductVersion "${Version}"
VIAddVersionKey Comments "TSDuck - The MPEG Transport Stream Toolkit"
VIAddVersionKey CompanyName "Thierry Lelegard"
VIAddVersionKey LegalCopyright "Copyright (c) 2005-2025, Thierry Lelegard"
VIAddVersionKey FileVersion "${VersionInfo}"
VIAddVersionKey FileDescription "TSDuck - The MPEG Transport Stream Toolkit"

; Name of binary installer file and default installation folder.
!ifdef Win64
    OutFile "${InstallerDir}\TSDuck-Win64-${Version}.exe"
    InstallDir "$PROGRAMFILES64\TSDuck"
!else ifdef Win32
    OutFile "${InstallerDir}\TSDuck-Win32-${Version}.exe"
    InstallDir "$PROGRAMFILES\TSDuck"
!else ifdef Arm64
    OutFile "${InstallerDir}\TSDuck-Arm64-${Version}.exe"
    InstallDir "$PROGRAMFILES64\TSDuck"
!else
    Error "Undefined target platform"
!endif

; Generate a Unicode installer (default is ANSI).
Unicode true

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

; Get installation folder from registry if available from a previous installation.
InstallDirRegKey HKLM "${ProductKey}" "InstallDir"

; Installer pages.
!insertmacro MUI_PAGE_LICENSE "${RootDir}\LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Uninstaller pages.
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages.
!insertmacro MUI_LANGUAGE "English"

;-----------------------------------------------------------------------------
; Installation sections
;-----------------------------------------------------------------------------

; Installation of command-line tools and plugins
; ----------------------------------------------
Section "Tools & Plugins" SectionTools

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Delete obsolete configuration files from previous versions.
    Delete "$INSTDIR\bin\tsgentab.exe"
    Delete "$INSTDIR\bin\tsgentab*.dll"
    Delete "$INSTDIR\bin\tsduck.xml"
    Delete "$INSTDIR\bin\tsduck.channels.xml"
    Delete "$INSTDIR\bin\tsduck.dvb.names"
    Delete "$INSTDIR\bin\tsduck.names"

    ; Delete test programs which may have been erroneously included in previous version.
    Delete "$INSTDIR\bin\tsprofiling.exe"
    Delete "$INSTDIR\bin\tsmux.exe"
    Delete "$INSTDIR\bin\tsnet.exe"
    Delete "$INSTDIR\bin\tszlib.exe"

    ; Delete configuration files which were moved from libtsduck to libtscore.
    Delete "$INSTDIR\bin\tsduck.ip.names"
    Delete "$INSTDIR\bin\tsduck.keytable.model.xml"
    Delete "$INSTDIR\bin\tsduck.monitor.model.xml"
    Delete "$INSTDIR\bin\tsduck.monitor.xml"
    Delete "$INSTDIR\bin\tsduck.time.model.xml"
    Delete "$INSTDIR\bin\tsduck.time.xml"

    ; Delete obsolete plugins from previous versions.
    ; Maintenance: also update scripts/make-config.sh
    Delete "$INSTDIR\bin\tsplugin_drop.dll"
    Delete "$INSTDIR\bin\tsplugin_file.dll"
    Delete "$INSTDIR\bin\tsplugin_fork.dll"
    Delete "$INSTDIR\bin\tsplugin_hls.dll"
    Delete "$INSTDIR\bin\tsplugin_http.dll"
    Delete "$INSTDIR\bin\tsplugin_ip.dll"
    Delete "$INSTDIR\bin\tsplugin_null.dll"
    Delete "$INSTDIR\bin\tsplugin_psi.dll"
    Delete "$INSTDIR\bin\tsplugin_rist.dll"
    Delete "$INSTDIR\bin\tsplugin_srt.dll"
    Delete "$INSTDIR\bin\tsplugin_tables.dll"

    ; Create folder for binaries
    CreateDirectory "$INSTDIR\bin"
    SetOutPath "$INSTDIR\bin"
    File /x *_static.exe /x tsprofiling.exe /x tsmux.exe /x tsnet.exe /x tszlib.exe "${BinDir}\ts*.exe"
    File "${BinDir}\ts*.dll"
    File "${BinDir}\ts*.xml"
    File "${BinDir}\ts*.names"

SectionEnd

; Installation of documentation files
; -----------------------------------
Section "Documentation" SectionDocumentation

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Delete obsolete files from previous versions.
    Delete "$INSTDIR\doc\tsduck.pdf"
    Delete "$INSTDIR\doc\tsduck-dev.pdf"

    ; Documentation files.
    CreateDirectory "$INSTDIR\doc"
    SetOutPath "$INSTDIR\doc"
    File "${RootDir}\bin\doc\tsduck.html"
    File "${RootDir}\bin\doc\tsduck-dev.html"
    File "${RootDir}\CHANGELOG.txt"

    ; Create shortcuts in start menu.
    CreateDirectory "$SMPROGRAMS\TSDuck"
    CreateShortCut "$SMPROGRAMS\TSDuck\TSDuck User Guide.lnk" "$INSTDIR\doc\tsduck.html"
    CreateShortCut "$SMPROGRAMS\TSDuck\TSDuck Developer Guide.lnk" "$INSTDIR\doc\tsduck-dev.html"
    CreateShortCut "$SMPROGRAMS\TSDuck\TSDuck Release Notes.lnk" "$INSTDIR\doc\CHANGELOG.txt"

SectionEnd

; Installation of Python bindings
; -------------------------------
Section /o "Python Bindings" SectionPython
    ; Unselected by default (/o).

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Delete obsolete files from previous versions.
    RMDir /r "$INSTDIR\python\ts"

    ; Python files.
    CreateDirectory "$INSTDIR\python"
    SetOutPath "$INSTDIR\python"
    File "${PythonDir}\tsduck.py"
    File "${PythonDir}\ts.py"

SectionEnd

; Installation of Java bindings
; -----------------------------
!ifdef JarFile
Section /o "Java Bindings" SectionJava
    ; Unselected by default (/o).

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Java files.
    CreateDirectory "$INSTDIR\java"
    SetOutPath "$INSTDIR\java"
    File "${JarFile}"

SectionEnd
!endif

; Installation of development environment for third-party applications
; --------------------------------------------------------------------
Section /o "C++ Development" SectionDevelopment
    ; Unselected by default (/o).

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Delete obsolete files from previous versions (the list of files may change).
    RMDir /r "$INSTDIR\include"
    RMDir /r "$INSTDIR\lib"

    ; TSDuck header files.
    CreateDirectory "$INSTDIR\include"
    CreateDirectory "$INSTDIR\include\tscore"
    SetOutPath "$INSTDIR\include\tscore"
    File "${HeadersDir}\tscore\*.h"
    CreateDirectory "$INSTDIR\include\tsduck"
    SetOutPath "$INSTDIR\include\tsduck"
    File "${HeadersDir}\tsduck\*.h"

    ; TSDuck libraries.
    CreateDirectory "$INSTDIR\lib"

!ifdef DevWin32
    CreateDirectory "$INSTDIR\lib\Release-Win32"
    SetOutPath "$INSTDIR\lib\Release-Win32"
    File "${BinRoot}\Release-Win32\tscore.lib"
    File "${BinRoot}\Release-Win32\tscore.dll"
    File "${BinRoot}\Release-Win32\tsduck.lib"
    File "${BinRoot}\Release-Win32\tsduck.dll"

    CreateDirectory "$INSTDIR\lib\Debug-Win32"
    SetOutPath "$INSTDIR\lib\Debug-Win32"
    File "${BinRoot}\Debug-Win32\tscore.lib"
    File "${BinRoot}\Debug-Win32\tscore.dll"
    File "${BinRoot}\Debug-Win32\tsduck.lib"
    File "${BinRoot}\Debug-Win32\tsduck.dll"
!endif

!ifdef DevWin64
    CreateDirectory "$INSTDIR\lib\Release-Win64"
    SetOutPath "$INSTDIR\lib\Release-Win64"
    File "${BinRoot}\Release-x64\tscore.lib"
    File "${BinRoot}\Release-x64\tscore.dll"
    File "${BinRoot}\Release-x64\tsduck.lib"
    File "${BinRoot}\Release-x64\tsduck.dll"

    CreateDirectory "$INSTDIR\lib\Debug-Win64"
    SetOutPath "$INSTDIR\lib\Debug-Win64"
    File "${BinRoot}\Debug-x64\tscore.lib"
    File "${BinRoot}\Debug-x64\tscore.dll"
    File "${BinRoot}\Debug-x64\tsduck.lib"
    File "${BinRoot}\Debug-x64\tsduck.dll"
!endif

!ifdef DevArm64
    CreateDirectory "$INSTDIR\lib\Release-Arm64"
    SetOutPath "$INSTDIR\lib\Release-Arm64"
    File "${BinRoot}\Release-ARM64\tscore.lib"
    File "${BinRoot}\Release-ARM64\tscore.dll"
    File "${BinRoot}\Release-ARM64\tsduck.lib"
    File "${BinRoot}\Release-ARM64\tsduck.dll"

    CreateDirectory "$INSTDIR\lib\Debug-Arm64"
    SetOutPath "$INSTDIR\lib\Debug-Arm64"
    File "${BinRoot}\Debug-ARM64\tscore.lib"
    File "${BinRoot}\Debug-ARM64\tscore.dll"
    File "${BinRoot}\Debug-ARM64\tsduck.lib"
    File "${BinRoot}\Debug-ARM64\tsduck.dll"
!endif

    ; Visual Studio property files.
    SetOutPath "$INSTDIR"
    File "${RootDir}\pkg\nsis\tscore.props"
    File "${RootDir}\pkg\nsis\tsduck.props"

SectionEnd

; Common final mandatory section
; ------------------------------
Section "-Common" SectionCommon
    ; Not selectable, not displayed (because of leading '-' in name).

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Delete obsolete files from previous versions.
    Delete "$INSTDIR\setup\vc*redist*.exe"

    ; License files.
    SetOutPath "$INSTDIR"
    File "${RootDir}\LICENSE.txt"
    File "${RootDir}\OTHERS.txt"

    ; Setup tools.
    CreateDirectory "$INSTDIR\setup"
    SetOutPath "$INSTDIR\setup"
    File "${BinDir}\setpath.exe"
    File "${VCRedist}"
    File "${RootDir}\src\tstools\tsduck-completion.bash"

    ; Add an environment variable to TSDuck root.
    WriteRegStr HKLM ${EnvironmentKey} "TSDUCK" "$INSTDIR"

    ; Store installation folder in registry.
    WriteRegStr HKLM "${ProductKey}" "InstallDir" $INSTDIR

    ; Install or reinstall the Visual C++ redistributable library.
    nsExec::Exec '"$INSTDIR\setup\${VCRedistName}" /q /norestart'
    Pop $0

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\TSDuckUninstall.exe"

    ; Declare uninstaller in "Add/Remove Software" control panel
    WriteRegStr HKLM "${UninstallKey}" "DisplayName" "TSDuck"
    WriteRegStr HKLM "${UninstallKey}" "Publisher" "Thierry Lelegard"
    WriteRegStr HKLM "${UninstallKey}" "URLInfoAbout" "https://tsduck.io/"
    WriteRegStr HKLM "${UninstallKey}" "DisplayVersion" "${Version}"
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
        nsExec::Exec '"$INSTDIR\setup\setpath.exe" --prepend "$INSTDIR\bin"'
        Pop $0
    ${Else}
        WriteRegStr HKLM "${ProductKey}" "InstallTools" "false"
        ; Remove previous installation of tools & plugins.
        RMDir /r "$INSTDIR\bin"
        ; Remove binaries folder from system path
        nsExec::Exec '"$INSTDIR\setup\setpath.exe" --remove "$INSTDIR\bin"'
        Pop $0
    ${EndIf}
    ${If} ${SectionIsSelected} ${SectionDocumentation}
        WriteRegStr HKLM "${ProductKey}" "InstallDocumentation" "true"
    ${Else}
        WriteRegStr HKLM "${ProductKey}" "InstallDocumentation" "false"
        ; Remove previous installation of documentation.
        RMDir /r "$INSTDIR\doc"
        RMDir /r "$SMPROGRAMS\TSDuck"
    ${EndIf}
    ${If} ${SectionIsSelected} ${SectionPython}
        WriteRegStr HKLM "${ProductKey}" "InstallPython" "true"
        ; Add Python folder to Python path.
        nsExec::Exec '"$INSTDIR\setup\setpath.exe" --prepend "$INSTDIR\python" --environment PYTHONPATH'
        Pop $0
    ${Else}
        WriteRegStr HKLM "${ProductKey}" "InstallPython" "false"
        ; Remove previous installation of Python bindings.
        RMDir /r "$INSTDIR\python"
        ; Remove Python folder from Python path.
        nsExec::Exec '"$INSTDIR\setup\setpath.exe" --remove "$INSTDIR\python" --environment PYTHONPATH'
        Pop $0
    ${EndIf}
!ifdef JarFile
    ${If} ${SectionIsSelected} ${SectionJava}
        WriteRegStr HKLM "${ProductKey}" "InstallJava" "true"
        ; Add JAR file to Java class path
        nsExec::Exec '"$INSTDIR\setup\setpath.exe" --prepend "$INSTDIR\java\tsduck.jar" --environment CLASSPATH --final-separator'
        Pop $0
    ${Else}
!endif
        WriteRegStr HKLM "${ProductKey}" "InstallJava" "false"
        ; Remove previous installation of Java bindings.
        RMDir /r "$INSTDIR\java"
        ; Remove JAR file to Java class path
        nsExec::Exec '"$INSTDIR\setup\setpath.exe" --remove "$INSTDIR\java\tsduck.jar" --environment CLASSPATH --final-separator'
        Pop $0
!ifdef JarFile
    ${EndIf}
!endif
    ${If} ${SectionIsSelected} ${SectionDevelopment}
        WriteRegStr HKLM "${ProductKey}" "InstallDevelopment" "true"
    ${Else}
        WriteRegStr HKLM "${ProductKey}" "InstallDevelopment" "false"
        ; Remove previous installation of development environment.
        RMDir /r "$INSTDIR\include"
        RMDir /r "$INSTDIR\lib"
        Delete "$INSTDIR\tscore.props"
        Delete "$INSTDIR\tsduck.props"
    ${EndIf}

SectionEnd

;-----------------------------------------------------------------------------
; Description of installation sections (displayed by mouse hover)
;-----------------------------------------------------------------------------

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SectionTools} \
        "TSDuck command-line tools and plugins."
    !insertmacro MUI_DESCRIPTION_TEXT ${SectionDocumentation} \
        "TSDuck user guide."
    !insertmacro MUI_DESCRIPTION_TEXT ${SectionPython} \
        "TSDuck Python bindings."
!ifdef JarFile
    !insertmacro MUI_DESCRIPTION_TEXT ${SectionJava} \
        "TSDuck Java bindings."
!endif
    !insertmacro MUI_DESCRIPTION_TEXT ${SectionDevelopment} \
        "TSDuck development environment, for use from third-party applications manipulating MPEG transport streams."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

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
    nsExec::Exec '"$0\setup\setpath.exe" --remove "$0\bin"'
    Pop $1
    nsExec::Exec '"$0\setup\setpath.exe" --remove "$0\python" --environment PYTHONPATH'
    Pop $1
    nsExec::Exec '"$0\setup\setpath.exe" --remove "$0\java\tsduck.jar" --environment CLASSPATH --final-separator'
    Pop $1

    ; Delete product files.
    RMDir /r "$0\bin"
    RMDir /r "$0\doc"
    RMDir /r "$0\python"
    RMDir /r "$0\java"
    RMDir /r "$0\include"
    RMDir /r "$0\lib"
    Delete "$0\tscore.props"
    Delete "$0\tsduck.props"
    Delete "$0\LICENSE.txt"
    Delete "$0\OTHERS.txt"
    Delete "$0\setup\setpath.exe"
    Delete "$0\setup\${VCRedistName}"
    Delete "$0\setup\tsduck-completion.bash"
    RMDir "$0\setup"
    Delete "$0\TSDuckUninstall.exe"
    RMDir "$0"

SectionEnd

;-----------------------------------------------------------------------------
; Initialization functions
;-----------------------------------------------------------------------------

; Installation initialization.
function .onInit

    ; In 64-bit installers, don't use registry redirection. Also prevent execution
    ; of 64-bit installers on 32-bit systems (the installer itself is 32-bit and
    ; can run on 32-bit systems but it contains 64-bit executables).
    ${If} ${RunningX64}
    ${OrIf} ${IsNativeARM64}
        SetRegView 64
    ${EndIf}

    !ifdef Win64
        ${IfNot} ${IsNativeAMD64}
            MessageBox MB_OK|MB_ICONSTOP \
                "This is a 64-bit Intel version of TSDuck.$\r$\n\
                Not suitable for this system."
            Quit
        ${EndIf}
    !else ifdef Arm64
        ${IfNot} ${IsNativeARM64}
            MessageBox MB_OK|MB_ICONSTOP \
                "This is an Arm64 version of TSDuck.$\r$\n\
                Not suitable for this system."
            Quit
        ${EndIf}
    !else
        ${If} ${IsNativeAMD64}
        ${OrIf} ${IsNativeARM64}
            MessageBox MB_OK|MB_ICONSTOP \
                "This is a 32-bit Intel version of TSDuck.$\r$\n\
                Not suitable for this system."
            Quit
        ${EndIf}
    !endif

    ; Preset sections to install.
    ; First, check command line for "/all=true".
    ${GetParameters} $0
    ${GetOptions} $0 "/all=" $1
    ${If} $1 == "true"
        ; "/all=true" is set, install all sections.
        !insertmacro SelectSection ${SectionTools}
        !insertmacro SelectSection ${SectionDocumentation}
        !insertmacro SelectSection ${SectionPython}
!ifdef JarFile
        !insertmacro SelectSection ${SectionJava}
!endif
        !insertmacro SelectSection ${SectionDevelopment}
    ${Else}
        ; Preset the selections identically to the previous installation.
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
        ReadRegStr $0 HKLM "${ProductKey}" "InstallPython"
        ${If} $0 == "true"
            !insertmacro SelectSection ${SectionPython}
        ${EndIf}
        ${If} $0 == "false"
            !insertmacro UnSelectSection ${SectionPython}
        ${EndIf}
!ifdef JarFile
        ReadRegStr $0 HKLM "${ProductKey}" "InstallJava"
        ${If} $0 == "true"
            !insertmacro SelectSection ${SectionJava}
        ${EndIf}
        ${If} $0 == "false"
            !insertmacro UnSelectSection ${SectionJava}
        ${EndIf}
!endif
        ReadRegStr $0 HKLM "${ProductKey}" "InstallDevelopment"
        ${If} $0 == "true"
            !insertmacro SelectSection ${SectionDevelopment}
        ${EndIf}
        ${If} $0 == "false"
            !insertmacro UnSelectSection ${SectionDevelopment}
        ${EndIf}
    ${EndIf}

functionEnd

; Uninstallation initialization.
function un.onInit

    ; In 64-bit installers, don't use registry redirection.
    ${If} ${RunningX64}
    ${OrIf} ${IsNativeARM64}
        SetRegView 64
    ${EndIf}

functionEnd
