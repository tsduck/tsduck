#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Build the TSDuck binary installers for Windows.

 .DESCRIPTION

  Build the binary installers for Windows. The release version of the project
  is automatically rebuilt before building the installer.

  By default, installers are built for 64-bit systems only, full executable
  binary installer and portable archive (for install without admin rights).
  The development environments are provided for 64-bit applications only.

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

 .PARAMETER Win32

  Generate the 32-bit installers in addition to the 64-bit installers.
  Also build development environments for 32-bit and 64-bit applications
  in the two installers.
#>
[CmdletBinding()]
param(
    [switch]$GitPull = $false,
    [switch]$NoPause = $false,
    [switch]$NoBuild = $false,
    [switch]$NoInstaller = $false,
    [switch]$NoLowPriority = $false,
    [switch]$NoPortable = $false,
    [switch]$Win32 = $false
)

# Get the project directories.
$RootDir    = (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$ScriptsDir = "${RootDir}\scripts"
$MsvcDir    = "${ScriptsDir}\msvc"
$SrcDir     = "${RootDir}\src"
$BinRoot    = "${RootDir}\bin"
$BinInclude = "${BinRoot}\include"
$JarFile    = "${BinRoot}\java\tsduck.jar"
$InstallerDir = "${RootDir}\pkg\installers"

# PowerShell execution policy.
Set-StrictMode -Version 3
if (((Get-ExecutionPolicy) -ne "Unrestricted") -and ((Get-ExecutionPolicy) -ne "RemoteSigned")) {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force -ErrorAction:SilentlyContinue
}
Import-Module -Force -Name "${ScriptsDir}\tsbuild.psm1"

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
    # Use "*x64" instead of "*64" since some VS installations may include an arm64 version.
    $VCRedist64 = Get-ChildItem -Recurse -Path "C:\Program Files*\Microsoft Visual Studio" -Include "vc*redist*x64.exe" -ErrorAction Ignore |
                  ForEach-Object { (Get-Command $_).FileVersionInfo } |
                  Sort-Object -Unique -Property FileVersion  |
                  ForEach-Object { $_.FileName} | Select-Object -Last 1
    if (-not $VCRedist64) {
        Exit-Script -NoPause:$NoPause "MSVC Redistributable Libraries 64-bit  Installer not found"
    }
    if ($Win32) {
        $VCRedist32 = Get-ChildItem -Recurse -Path "C:\Program Files*\Microsoft Visual Studio" -Include "vc*redist*86.exe" -ErrorAction Ignore |
                      ForEach-Object { (Get-Command $_).FileVersionInfo } |
                      Sort-Object -Unique -Property FileVersion  |
                      ForEach-Object { $_.FileName} | Select-Object -Last 1
        if (-not $VCRedist32) {
            Exit-Script -NoPause:$NoPause "MSVC Redistributable Libraries 32-bit Installer not found"
        }
    }
}

# Lower process priority so that the build does not eat up all CPU.
if (-not $NoLowPriority) {
    (Get-Process -Id $PID).PriorityClass = "BelowNormal"
}

# Build the project.
if (-not $NoBuild) {
    Write-Output "Compiling..."
    Push-Location
    & "${ScriptsDir}\build.ps1" -Installer -NoPause -Win32:$Win32 -Win64 -GitPull:$GitPull -NoLowPriority:$NoLowPriority
    $Code = $LastExitCode
    Pop-Location
    if ($Code -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building the product"
    }
}

# Get version name.
$Version = (python "${ScriptsDir}\get-version-from-sources.py")
$VersionInfo = (python "${ScriptsDir}\get-version-from-sources.py" --windows)

# A function to build a binary installer.
function Build-Binary([string]$BinSuffix, [string]$Arch, [string]$OtherDevArch, [string]$VCRedist, [string]$HeadersDir)
{
    Write-Output "Building installer for $Arch..."

    # Full bin directory.
    $BinDir = "${BinRoot}\Release-${BinSuffix}"

    # NSIS script for this project.
    $NsisScript = "${PSScriptRoot}\tsduck.nsi"

    # Base name of the MSVC redistributable.
    $VCRedistName = (Get-Item $VCRedist).Name

    # Specify JAR file option if it exists.
    if (Test-Path $JarFile) {
        $NsisOptJar = "/DJarFile=$JarFile"
    }
    else {
        $NsisOptJar = ""
    }

    # Build the binary installer.
    & $NSIS /V2 $NsisOptJar /D$Arch /DBinDir=$BinDir /DDev$Arch /DDev$OtherDevArch `
        /DVCRedist=$VCRedist /DVCRedistName=$VCRedistName /DHeadersDir=$HeadersDir `
        /DVersion=$Version /DVersionInfo=$VersionInfo $NsisScript
}

# Build binary installers.
if (-not $NoInstaller) {

    # Create a temporary directory for header files (development options).
    $TempDir = New-TempDirectory
    $Exclude = @("*\unix\*", "*\linux\*", "*\mac\*", "*\bsd\*", "*\private\*")
    Copy-Item "${BinInclude}\tsduck.h" "${TempDir}\tsduck.h"
    Get-ChildItem "${SrcDir}\libtsduck" -Recurse -Include "*.h" | `
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
            Build-Binary "x64" "Win64" "Win32" $VCRedist64 $TempDir
            Build-Binary "Win32" "Win32" "Win64" $VCRedist32 $TempDir
        }
        else {
            Build-Binary "x64" "Win64" "" $VCRedist64 $TempDir
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
    $BinDir = "${BinRoot}\Release-${BinSuffix}"

    # Package name.
    $ZipFile = "${InstallerDir}\TSDuck-${InstallerSuffix}-${Version}-Portable.zip"

    # Create a temporary directory.
    $TempDir = New-TempDirectory

    Push-Location $TempDir
    try {
        # Build directory tree and copy files.
        $TempRoot = (New-Directory "${TempDir}\TSDuck")
        Copy-Item "${RootDir}\LICENSE.txt" -Destination $TempRoot
        Copy-Item "${RootDir}\OTHERS.txt" -Destination $TempRoot

        $TempBin = (New-Directory "${TempRoot}\bin")
        Copy-Item "${BinDir}\ts*.exe" -Exclude @("*_static.exe", "tsprofiling.exe", "tsmux.exe") -Destination $TempBin
        Copy-Item "${BinDir}\ts*.dll" -Destination $TempBin
        Copy-Item "${BinDir}\ts*.xml" -Destination $TempBin
        Copy-Item "${BinDir}\ts*.names" -Destination $TempBin

        $TempDoc = (New-Directory "${TempRoot}\doc")
        Copy-Item "${RootDir}\doc\tsduck.pdf" -Destination $TempDoc
        Copy-Item "${RootDir}\CHANGELOG.txt" -Destination $TempDoc

        $TempPython = (New-Directory "${TempRoot}\python")
        Copy-Item "${SrcDir}\libtsduck\python\*.py" -Destination $TempPython

        $TempJava = (New-Directory "${TempRoot}\java")
        Copy-Item $JarFile -Destination $TempJava

        $TempSetup = (New-Directory "${TempRoot}\setup")
        Copy-Item "${BinDir}\setpath.exe" -Destination $TempSetup
        Copy-Item $VCRedist -Destination $TempSetup

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
if (-not $NoPortable) {
    Build-Portable "x64" "Win64" $VCRedist64
}

Exit-Script -NoPause:$NoPause
