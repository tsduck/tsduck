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

  Build TSDuck from the command line, an alternative to Visual Studio.

 .PARAMETER GitPull

  Perform a git pull command before building.

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

 .PARAMETER NoTeletext

  Build without Teletext support. The plugin "teletext" is not provided.

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

  Generate the 32-bit version of the binaries. If neither -Win32 nor -Win64
  is specified, both versions are built by default.

 .PARAMETER Win64

  Generate the 64-bit version of the binaries. If neither -Win32 nor -Win64
  is specified, both versions are built by default.
#>
param(
    [int]$Parallel = 0,
    [switch]$GitPull = $false,
    [switch]$Installer = $false,
    [switch]$NoLowPriority = $false,
    [switch]$Debug = $false,
    [switch]$Release = $false,
    [switch]$Win32 = $false,
    [switch]$Win64 = $false,
    [switch]$NoStatic = $false,
    [switch]$NoTeletext = $false,
    [switch]$NoPause = $false
)

# PowerShell execution policy.
Set-StrictMode -Version 3
if (((Get-ExecutionPolicy) -ne "Unrestricted") -and ((Get-ExecutionPolicy) -ne "RemoteSigned")) {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force -ErrorAction:SilentlyContinue
}
Import-Module -Force -Name (Join-Path $PSScriptRoot build-common.psm1)

# Apply defaults.
if (-not $Debug -and -not $Release -and -not $Installer) {
    $Release = $true
}
if (-not $Win32 -and -not $Win64) {
    $Win32 = $true
    $Win64 = $true
}

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$ProjDir = (Join-Path $PSScriptRoot "msvc")
$SolutionFileName = (Join-Path $ProjDir "tsduck.sln")

# Make sure that Git hooks are installed.
& (Join-Path $PSScriptRoot git-hook-update.ps1) -NoPause

# Lower process priority so that the build does not eat up all CPU.
if (-not $NoLowPriority) {
    (Get-Process -Id $PID).PriorityClass = "BelowNormal"
}

# Find MSBuild
$MSBuild = (Find-MSBuild)

# Update git repository if requested.
if ($GitPull) {
    # Search git command.
    $git = (Search-File "git.exe" @($env:Path, 'C:\Program Files\Git\cmd', 'C:\Program Files (x86)\Git\cmd'))
    if (-not $git) {
        Exit-Script -NoPause:$NoPause "Git not found"
    }
    Push-Location $RootDir
    & $git fetch origin
    & $git checkout master
    & $git pull origin master
    Pop-Location
}

# A function to invoke MSBuild.
function Call-MSBuild ([string] $configuration, [string] $platform, [string] $target = "")
{
    if ($NoTeletext) {
        $OptTeletext = "/property:NoTeletext=true"
    }
    else {
        $OptTeletext =""
    }
    if ($Parallel -gt 0) {
        $OptCPU = "/maxcpucount:$Parallel"
    }
    else {
        $OptCPU = "/maxcpucount"
    }
    & $MSBuild $SolutionFileName /nologo $OptCPU /property:Configuration=$configuration /property:Platform=$platform $OptTeletext $target
    if ($LastExitCode -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building $platform $configuration"
    }
}

# Build targets
$AllTargets = @(Select-String -Path (Join-Path $ProjDir "*.vcxproj") -Pattern '<RootNameSpace>' |
                ForEach-Object { $_ -replace '.*<RootNameSpace> *','' -replace ' *</RootNameSpace>.*','' })
$plugins = ($AllTargets | Select-String "tsplugin_*") -join ';'
$commands = ($AllTargets | Select-String -NotMatch @("tsduck*", "tsplugin_*", "tsp_static", "setpath", "utest*")) -join ';'

# Rebuild TSDuck.
if ($Installer) {
    # We build everything except test programs for the "Release" configuration.
    # Then, we need the DLL for all configurations (development environment).
    $targets = "tsduckdll;tsducklib;$commands;$plugins;setpath"

    if ($Win32 -and $Win64) {
        Call-MSBuild Release x64   /target:$targets
        Call-MSBuild Release Win32 /target:$targets
    }
    elseif ($Win32) {
        Call-MSBuild Release x64   /target:tsduckdll
        Call-MSBuild Release Win32 /target:$targets
    }
    elseif ($Win64) {
        Call-MSBuild Release x64   /target:$targets
        Call-MSBuild Release Win32 /target:tsduckdll
    }
    Call-MSBuild Debug x64   /target:tsduckdll
    Call-MSBuild Debug Win32 /target:tsduckdll
}
else {
    # Not for an installer, build everything.
    if ($NoStatic) {
        $targets = "/target:tsduckdll;$commands;$plugins;utests-tsduckdll"
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
    if ($Debug -and $Win64) {
        Call-MSBuild Debug x64 $targets
    }
    if ($Debug -and $Win32) {
        Call-MSBuild Debug Win32 $targets
    }
}

Exit-Script -NoPause:$NoPause
