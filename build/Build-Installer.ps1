#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2018, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Build the TSDuck binary installers for Windows.

 .DESCRIPTION

  Build the binary installers for Windows. The release version of the project
  is automatically rebuilt before building the installer.

  By default, installers are built for 32-bit and 64-bit systems, full
  executable binary installers, standalone binaries (without admin rights),
  source code and wintools archives.

 .PARAMETER GitPull

  Perform a git pull command before building.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.

 .PARAMETER NoBuild

  Do not rebuild the product, assumed to be still valid.

 .PARAMETER NoInstaller

  Do not build the binary installers.

 .PARAMETER NoLowPriority

  Do not lower the process priority.

 .PARAMETER NoPortable

  Do not build the portable packages.

 .PARAMETER NoSource

  Do not build the source archive.

 .PARAMETER NoTeletext

  Build without Teletext support. The plugin "teletext" is not provided.

 .PARAMETER Win32

  Generate the 32-bit installer. If neither -Win32 nor -Win64 is specified,
  both versions are built by default.

 .PARAMETER Win64

  Generate the 64-bit installer. If neither -Win32 nor -Win64 is specified,
  both versions are built by default.
#>
[CmdletBinding()]
param(
    [switch]$GitPull = $false,
    [switch]$NoPause = $false,
    [switch]$NoBuild = $false,
    [switch]$NoInstaller = $false,
    [switch]$NoLowPriority = $false,
    [switch]$NoPortable = $false,
    [switch]$NoTeletext = $false,
    [switch]$NoSource = $false,
    [switch]$Win32 = $false,
    [switch]$Win64 = $false
)

# PowerShell execution policy.
Set-StrictMode -Version 3
if (((Get-ExecutionPolicy) -ne "Unrestricted") -and ((Get-ExecutionPolicy) -ne "RemoteSigned")) {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force -ErrorAction:SilentlyContinue
}
Import-Module -Force -Name (Join-Path $PSScriptRoot Build-Common.psm1)

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$SrcDir = (Join-Path $RootDir "src")
$InstallerDir = (Join-Path $RootDir "installers")

# Get location of Visual Studio and project files.
$VS = Search-VisualStudio
$MsvcDir = $VS.MsvcDir

# Apply defaults.
if (-not $Win32 -and -not $Win64) {
    $Win32 = $true
    $Win64 = $true
}

# Requires .NET 4.5 to build zip files (need compression methods).
if (-not $NoSource) {
    $DotNetVersion = Get-DotNetVersion
    if ($DotNetVersion -lt 405) {
        $DotNetString = "v" + [int]($DotNetVersion / 100) + "." + ($DotNetVersion % 100)
        Exit-Script -NoPause:$NoPause "Require .NET Framework v4.5, running $DotNetString"
    }
}

# Locate NSIS, the Nullsoft Scriptable Installation System.
if (-not $NoInstaller) {
    $NsisExe = Get-FileInPath makensis.exe "$env:Path;C:\Program Files\NSIS;C:\Program Files (x86)\NSIS"
    $NsisScript = Join-Path $PSScriptRoot "tsduck.nsi"
}

# Get version name.
$Major = ((Get-Content $SrcDir\libtsduck\tsVersion.h | Select-String -Pattern "#define TS_VERSION_MAJOR ").ToString() -replace "#define TS_VERSION_MAJOR *","")
$Minor = ((Get-Content $SrcDir\libtsduck\tsVersion.h | Select-String -Pattern "#define TS_VERSION_MINOR ").ToString() -replace "#define TS_VERSION_MINOR *","")
$Commit = ((Get-Content $SrcDir\libtsduck\tsVersion.h | Select-String -Pattern "#define TS_COMMIT ").ToString() -replace "#define TS_COMMIT *","")
$Version = "${Major}.${Minor}-${Commit}"

# Lower process priority so that the build does not eat up all CPU.
if (-not $NoLowPriority) {
    (Get-Process -Id $PID).PriorityClass = "BelowNormal"
}

# Specific options to build without Teletext support.
if ($NoTeletext) {
    $NsisOptTeletext = "/DNoTeletext"
}
else {
    $NsisOptTeletext =""
}

# Build the project.
if (-not $NoBuild) {
    Push-Location
    & (Join-Path $PSScriptRoot Build.ps1) -Installer -NoPause -Win32:$Win32 -Win64:$Win64 -GitPull:$GitPull -NoLowPriority:$NoLowPriority -NoTeletext:$NoTeletext
    $Code = $LastExitCode
    Pop-Location
    if ($Code -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building the product"
    }
}

# Download redistributable libraries, if not already downloaded.
if (-not $NoInstaller) {
    & (Join-MultiPath @($MsvcDir, "redist", "Download-Vcredist.ps1")) -NoPause
}

# Build binary installers.
if (-not $NoInstaller -and $Win32) {
    & $NsisExe "/DProjectDir=$MsvcDir" $NsisOptTeletext $NsisScript
}
if (-not $NoInstaller -and $Win64) {
    & $NsisExe "/DProjectDir=$MsvcDir" $NsisOptTeletext /DWin64 $NsisScript
}

# A function to build a portable package.
function Build-Portable([string]$BinSuffix, [string]$InstallerSuffix, [string]$VcRedist)
{
    # Full bin directory.
    $BinDir = (Join-Path $MsvcDir "Release-${BinSuffix}")

    # Package name.
    $ZipFile = (Join-Path $InstallerDir "TSDuck-${InstallerSuffix}-${Version}-Portable.zip")

    # Create a temporary directory.
    $TempDir = New-TempDirectory

    Push-Location $TempDir
    try {
        # Build directory tree and copy files.
        $TempRoot = (New-Directory @($TempDir, "TSDuck"))

        $TempBin = (New-Directory @($TempRoot, "bin"))
        Copy-Item (Join-Path $BinDir "ts*.exe") -Exclude "*_static.exe" -Destination $TempBin
        Copy-Item (Join-Path $BinDir "ts*.dll") -Destination $TempBin
        Copy-Item (Join-Multipath @($SrcDir, "libtsduck", "tsduck.xml")) -Destination $TempBin
        Copy-Item (Join-Multipath @($SrcDir, "libtsduck", "tsduck.*.names")) -Destination $TempBin

        $TempDoc = (New-Directory @($TempRoot, "doc"))
        Copy-Item (Join-Multipath @($RootDir, "doc", "tsduck.pdf")) -Destination $TempDoc
        Copy-Item (Join-Path $RootDir "CHANGELOG.txt") -Destination $TempDoc
        Copy-Item (Join-Path $RootDir "LICENSE.txt") -Destination $TempDoc

        $TempSetup = (New-Directory @($TempRoot, "setup"))
        Copy-Item (Join-Path $BinDir "setpath.exe") -Destination $TempSetup
        Copy-Item (Join-Multipath @($MsvcDir, "redist", $VcRedist)) -Destination $TempSetup

        if ($NoTeletext) {
            Get-ChildItem $TempBin -Recurse -Include "tsplugin_teletext.*" | Remove-Item -Force -ErrorAction Ignore
        }

        # Create the zip file.
        Get-ChildItem -Recurse (Split-Path $TempRoot) | New-ZipFile $ZipFile -Force -Root $TempDir
    }
    finally {
        # Delete the temporary directory.
        Pop-Location
        if (Test-Path $TempDir) {
            Remove-Item $TempDir -Recurse -Force
        }
    }
}

# Build portable packages.
if (-not $NoPortable -and $Win32) {
    Build-Portable "Win32" "Win32" "vcredist32.exe"
}
if (-not $NoPortable -and $Win64) {
    Build-Portable "x64" "Win64" "vcredist64.exe"
}

# Build the source archives.
if (-not $NoSource) {

    # Source archive name.
    $SrcArchive = (Join-Path $InstallerDir "TSDduck-${Version}-src.zip")

    # Create a temporary directory.
    $TempDir = New-TempDirectory

    Push-Location $TempDir
    try {
        # Copy project tree into temporary directory.
        $TempRoot = (Join-Path $TempDir "TSDuck-${Version}")
        Copy-Item $RootDir $TempRoot -Recurse

        # Cleanup the temporary tree.
        & (Join-MultiPath @($TempRoot, "build", "Cleanup.ps1")) -Deep -NoPause -Silent

        if ($NoTeletext) {
            Get-ChildItem $TempRoot -Recurse -Include @("tsTeletextDemux.*", "tsTeletextCharset.*", "tsplugin_teletext.*") | Remove-Item -Force -ErrorAction Ignore
        }

        # Create the source zip file.
        Get-ChildItem -Recurse (Split-Path $TempRoot) | New-ZipFile $SrcArchive -Force -Root $TempDir
    }
    finally {
        # Delete the temporary directory.
        Pop-Location
        if (Test-Path $TempDir) {
            Remove-Item $TempDir -Recurse -Force
        }
    }
}

Exit-Script -NoPause:$NoPause
