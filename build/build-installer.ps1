#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
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
  source code archive.

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
Import-Module -Force -Name (Join-Path $PSScriptRoot build-common.psm1)

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$MsvcDir = (Join-Path $PSScriptRoot "msvc")
$SrcDir = (Join-Path $RootDir "src")
$BinRoot = (Join-Path $RootDir "bin")
$InstallerDir = (Join-Path $RootDir "installers")

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

# Collect files for the installers.
if (-not $NoInstaller) {
    # Locate NSIS, the Nullsoft Scriptable Installation System.
    $NSIS = Get-Item "C:\Program Files*\NSIS\makensis.exe" | ForEach-Object { $_.FullName} | Select-Object -Last 1
    if (-not $NSIS) {
        Exit-Script -NoPause:$NoPause "NSIS not found"
    }
}

# MSVC redistributable installer.
if (-not $NoInstaller -or -not $NoPortable) {
    Write-Output "Searching MSVC Redistributable Libraries Installers..."
    $VCRedist32 = Get-ChildItem -Recurse -Path "C:\Program Files*\Microsoft Visual Studio" -Include "vc*redist*86.exe" -ErrorAction Ignore |
                  ForEach-Object { (Get-Command $_).FileVersionInfo } |
                  Sort-Object -Unique -Property FileVersion  |
                  ForEach-Object { $_.FileName} | Select-Object -Last 1
    # Use "*x64" instead of "*64" since some VS installations may include an arm64 version.
    $VCRedist64 = Get-ChildItem -Recurse -Path "C:\Program Files*\Microsoft Visual Studio" -Include "vc*redist*x64.exe" -ErrorAction Ignore |
                  ForEach-Object { (Get-Command $_).FileVersionInfo } |
                  Sort-Object -Unique -Property FileVersion  |
                  ForEach-Object { $_.FileName} | Select-Object -Last 1
    if (-not $VCRedist32 -or -not $VCRedist64) {
        Exit-Script -NoPause:$NoPause "MSVC Redistributable Libraries Installers not found"
    }
}

# Get version name.
$GetVersion = (Join-Path $PSScriptRoot get-version-from-sources.ps1)
$Version = (& $GetVersion)
$VersionInfo = (& $GetVersion -Windows)

# Lower process priority so that the build does not eat up all CPU.
if (-not $NoLowPriority) {
    (Get-Process -Id $PID).PriorityClass = "BelowNormal"
}

# Build the project.
if (-not $NoBuild) {
    Write-Output "Compiling..."
    Push-Location
    & (Join-Path $PSScriptRoot build.ps1) -Installer -NoPause -Win32:$Win32 -Win64:$Win64 -GitPull:$GitPull -NoLowPriority:$NoLowPriority -NoTeletext:$NoTeletext
    $Code = $LastExitCode
    Pop-Location
    if ($Code -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building the product"
    }
}

# A function to build a binary installer.
function Build-Binary([string]$BinSuffix, [string]$Arch, [string]$VCRedist, [string]$HeadersDir)
{
    Write-Output "Building installer for $Arch..."

    # Full bin directory.
    $BinDir = (Join-Path $BinRoot "Release-${BinSuffix}")

    # NSIS script for this project.
    $NsisScript = Join-Path $PSScriptRoot "tsduck.nsi"

    # Base name of the MSVC redistributable.
    $VCRedistName = (Get-Item $VCRedist).Name

    # Specific options to build without Teletext support.
    if ($NoTeletext) {
        $NsisOptTeletext = "/DNoTeletext"
    }
    else {
        $NsisOptTeletext = ""
    }

    # Build the binary installer.
    & $NSIS /V2 $NsisOptTeletext /D$Arch /DBinDir=$BinDir /DVCRedist=$VCRedist /DVCRedistName=$VCRedistName /DHeadersDir=$HeadersDir /DVersion=$Version /DVersionInfo=$VersionInfo $NsisScript
}

# Build binary installers.
if (-not $NoInstaller) {

    # Create a temporary directory for header files (development options).
    $TempDir = New-TempDirectory
    $Exclude = @("*\unix\*", "*\linux\*", "*\mac\*", "*\private\*")
    if ($NoTeletext) {
        $Exclude += "*\tsduck.h"
        $Exclude += "*\tsTeletextDemux.h"
        $Exclude += "*\tsTeletextCharset.h"
        Get-Content (Join-Multipath @($SrcDir, "libtsduck", "tsduck.h")) | `            Where-Object { ($_ -notmatch 'tsTeletextDemux.h') -and ($_ -notmatch 'tsTeletextCharset.h') } | `            Out-File -Encoding ascii (Join-Path $TempDir tsduck.h)
    }
    Get-ChildItem (Join-Path $SrcDir libtsduck) -Recurse -Include "*.h" | `
        Where-Object {
            $fname = $_.FullName
            ($Exclude | ForEach-Object { $fname -like $_ }) -notcontains $true
        } | `
        ForEach-Object {
            Copy-Item $_ $TempDir
        }

    Push-Location $TempDir
    try {
        if ($Win32) {
            Build-Binary "Win32" "Win32" $VCRedist32 $TempDir
        }
        if ($Win64) {
            Build-Binary "x64" "Win64" $VCRedist64 $TempDir
        }
    }
    finally {
        # Delete the temporary directory.
        Pop-Location
        if (Test-Path $TempDir) {
            Remove-Item $TempDir -Recurse -Force
        }
    }
}

# A function to build a portable package.
function Build-Portable([string]$BinSuffix, [string]$InstallerSuffix, [string]$VCRedist)
{
    Write-Output "Building portable installer for $InstallerSuffix..."

    # Full bin directory.
    $BinDir = (Join-Path $BinRoot "Release-${BinSuffix}")

    # Package name.
    $ZipFile = (Join-Path $InstallerDir "TSDuck-${InstallerSuffix}-${Version}-Portable.zip")

    # Create a temporary directory.
    $TempDir = New-TempDirectory

    Push-Location $TempDir
    try {
        # Build directory tree and copy files.
        $TempRoot = (New-Directory @($TempDir, "TSDuck"))
        Copy-Item (Join-Path $RootDir "LICENSE.txt") -Destination $TempRoot
        Copy-Item (Join-Path $RootDir "OTHERS.txt") -Destination $TempRoot

        $TempBin = (New-Directory @($TempRoot, "bin"))
        Copy-Item (Join-Path $BinDir "ts*.exe") -Exclude "*_static.exe" -Destination $TempBin
        Copy-Item (Join-Path $BinDir "ts*.dll") -Destination $TempBin
        Copy-Item (Join-Multipath @($SrcDir, "libtsduck", "dtv", "tsduck*.xml")) -Destination $TempBin
        Copy-Item (Join-Multipath @($SrcDir, "libtsduck", "dtv", "tsduck*.names")) -Destination $TempBin

        $TempDoc = (New-Directory @($TempRoot, "doc"))
        Copy-Item (Join-Multipath @($RootDir, "doc", "tsduck.pdf")) -Destination $TempDoc
        Copy-Item (Join-Path $RootDir "CHANGELOG.txt") -Destination $TempDoc

        $TempSetup = (New-Directory @($TempRoot, "setup"))
        Copy-Item (Join-Path $BinDir "setpath.exe") -Destination $TempSetup
        Copy-Item $VCRedist -Destination $TempSetup

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
    Build-Portable "Win32" "Win32" $VCRedist32
}
if (-not $NoPortable -and $Win64) {
    Build-Portable "x64" "Win64" $VCRedist64
}

# Build the source archives.
if (-not $NoSource) {
    Write-Output "Building source archive..."

    # Source archive name.
    $SrcArchive = (Join-Path $InstallerDir "TSDuck-${Version}-src.zip")

    # Create a temporary directory.
    $TempDir = New-TempDirectory

    Push-Location $TempDir
    try {
        # Copy project tree into temporary directory.
        $TempRoot = (Join-Path $TempDir "TSDuck-${Version}")
        Copy-Item $RootDir $TempRoot -Recurse

        # Cleanup the temporary tree.
        & (Join-MultiPath @($TempRoot, "build", "cleanup.ps1")) -Deep -NoPause -Silent

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
