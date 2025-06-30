#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Build TSDuck from the command line, an alternative to Visual Studio.

 .PARAMETER GitPull

  Perform a git pull command before building.

 .PARAMETER Prerequisites

  Install prerequistes packages before compiling. Requires administrator access.

 .PARAMETER Installer

  Generate everything which is needed for installer.

 .PARAMETER NoLowPriority

  Do not lower the process priority.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.

 .PARAMETER NoStatic

  Do not build static library and corresponding tests. "setpath" is not built.

 .PARAMETER Parallel

  Specify the number of compilation processes to run in parallel. By default,
  use all CPU's.

 .PARAMETER Debug

  Generate the debug version of the binaries. If neither -Release nor -Debug
  is specified, the release version is built by default.

 .PARAMETER Release

  Generate the release version of the binaries. If neither -Release nor -Debug
  is specified, the release version is built by default.

 .PARAMETER Win32

  Generate the 32-bit Intel version of the binaries. If neither -Win32 nor -Win64
  nor -Arm64 is specified, the version for the native system is built by default.

 .PARAMETER Win64

  Generate the 64-bit Intel version of the binaries. If neither -Win32 nor -Win64
  nor -Arm64 is specified, the version for the native system is built by default.

 .PARAMETER Arm64

  Generate the 64-bit Arm version of the binaries. If neither -Win32 nor -Win64
  nor -Arm64 is specified, the version for the native system is built by default.
#>
param(
    [int]$Parallel = 0,
    [switch]$GitPull = $false,
    [switch]$Prerequisites = $false,
    [switch]$Installer = $false,
    [switch]$NoLowPriority = $false,
    [switch]$Debug = $false,
    [switch]$Release = $false,
    [switch]$Win32 = $false,
    [switch]$Win64 = $false,
    [switch]$Arm64 = $false,
    [switch]$NoStatic = $false,
    [switch]$NoPause = $false
)

# PowerShell execution policy.
Set-StrictMode -Version 3
if (((Get-ExecutionPolicy) -ne "Unrestricted") -and ((Get-ExecutionPolicy) -ne "RemoteSigned")) {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force -ErrorAction:SilentlyContinue
}
Import-Module -Force -Name "${PSScriptRoot}\tsbuild.psm1"

# Apply defaults.
if (-not $Debug -and -not $Release -and -not $Installer) {
    $Release = $true
}
if (-not $Win32 -and -not $Win64 -and -not $Arm64) {
    if ($env:PROCESSOR_ARCHITECTURE -like 'arm64*') {
        $Arm64 = $true
    }
    elseif ($env:PROCESSOR_ARCHITECTURE -like 'amd64*') {
        $Win64 = $true
    }
    elseif ($env:PROCESSOR_ARCHITECTURE -like 'x86*') {
        $Win32 = $true
    }
}

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$ProjDir = "${PSScriptRoot}\msvc"
$SolutionFileName = "${ProjDir}\tsduck.sln"

# Search git command.
$git = (Search-File "git.exe" @($env:Path, 'C:\Program Files\Git\cmd', 'C:\Program Files (x86)\Git\cmd'))

# Make sure that Git hooks are installed.
if ($git -ne $null) {
    python "${PSScriptRoot}\git-hook-update.py"
}

# Lower process priority so that the build does not eat up all CPU.
if (-not $NoLowPriority) {
    (Get-Process -Id $PID).PriorityClass = "BelowNormal"
}

# Find MSBuild
$MSBuild = (Find-MSBuild)

# Compute MSBuild version on 6 digits (e.g. 161001 for version 16.10.1)
$MSBuildVersionString = (& $MSBuild -version | Select-String -Pattern '^\d*\.\d*').ToString()
$VFields = ($MSBuildVersionString -split "[-\. ]") + @("0", "0", "0") | Select-String -Pattern '^\d*$'
$MSBuildVersion = (10000 * [int]$VFields[0].ToString()) + (100 * [int]$VFields[1].ToString()) + [int]$VFields[2].ToString()
Write-Output "MSBuild version $MSBuildVersionString ($MSBuildVersion)"

# Visual Studio installs compilers for 3 architectures, each one in 3 native architectures.
# A strange behaviour is observed on x64 build systems. When building on x64 for x64, the
# x64 version of the x64 compiler is used. However, when building on x64 for arm64, the x86
# version of the arm64 compiler is used, even though the x64 version is installed. Therefore,
# we force the use of the native versions of the cross-compilers.
if ($env:PROCESSOR_ARCHITECTURE -like 'arm64*') {
    $env:PreferredToolArchitecture = "ARM64"
}
elseif ($env:PROCESSOR_ARCHITECTURE -like 'amd64*') {
    $env:PreferredToolArchitecture = "x64"
}

# Update git repository if requested.
if ($GitPull) {
    if ($git -eq $null) {
        Exit-Script -NoPause:$NoPause "Git not found"
    }
    Push-Location $RootDir
    & $git fetch origin
    & $git checkout master
    & $git pull origin master
    Pop-Location
}

# Update prerequisites if requested.
if ($Prerequisites) {
    & "${PSScriptRoot}\install-prerequisites.ps1" -NoPause
}

# A function to invoke MSBuild.
function Call-MSBuild ([string] $configuration, [string] $platform, [string] $target = "")
{
    Write-Output "==== Building $configuration for $platform"
    if ($Parallel -gt 0) {
        $OptCPU = "/maxcpucount:$Parallel"
    }
    else {
        $OptCPU = "/maxcpucount"
    }
    if (-not -not $target) {
        if ($MSBuildVersion -eq 161000) {
            # There is some kind of bug in MSBuild 16.10.0. Rebuilding with /target:projname fails.
            # Adding :Rebuild to each project name works. Supposed to be fixed in 16.10.1.
            Write-Output "Bug in MSBuild version $MSBuildVersionString ($MSBuildVersion), forcing rebuild"
            $target = ($target -replace ';',':Rebuild;') + ':Rebuild'
        }
        $target="/target:$target"
    }
    & $MSBuild $SolutionFileName /nologo $OptCPU /property:Configuration=$configuration /property:Platform=$platform $target
    if ($LastExitCode -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building $platform $configuration"
    }
}

# Build targets
$AllTargets = @(Select-String -Path "${ProjDir}\*.vcxproj" -Pattern '<RootNameSpace>' |
                ForEach-Object { $_ -replace '.*<RootNameSpace> *','' -replace ' *</RootNameSpace>.*','' })
$plugins = ($AllTargets | Select-String "^tsplugin_") -join ';'
$commands = ($AllTargets | Select-String "^ts" | Select-String -NotMatch @("dll$", "lib$", "^tsplugin_", "^tsp_static$", "^tsmux$", "^tsprofiling$")) -join ';'

# Rebuild TSDuck.
if ($Installer) {
    # We build everything except test programs for the "Release" configuration.
    # Then, we need the DLL for "Debug" configurations (development environment).
    $targets = "tsduckdll;tsducklib;$commands;$plugins;setpath"
    if ($Win64) {
        Call-MSBuild Release x64 $targets
        Call-MSBuild Debug x64 tsduckdll
    }
    if ($Win32) {
        Call-MSBuild Release Win32 $targets
        Call-MSBuild Debug Win32 tsduckdll
    }
    if ($Arm64) {
        Call-MSBuild Release ARM64 $targets
        Call-MSBuild Debug ARM64 tsduckdll
    }
}
else {
    # Not for an installer, build everything.
    if ($NoStatic) {
        $targets = "tsduckdll;$commands;$plugins;utests-tsduckdll"
    }
    else {
        $targets = ""
    }
    if ($Release -and $Win64) {
        Call-MSBuild Release x64 $targets
    }
    if ($Release -and $Win32) {
        Call-MSBuild Release Win32 $targets
    }
    if ($Release -and $Arm64) {
        Call-MSBuild Release ARM64 $targets
    }
    if ($Debug -and $Win64) {
        Call-MSBuild Debug x64 $targets
    }
    if ($Debug -and $Win32) {
        Call-MSBuild Debug Win32 $targets
    }
    if ($Debug -and $Arm64) {
        Call-MSBuild Debug ARM64 $targets
    }
}

Exit-Script -NoPause:$NoPause
