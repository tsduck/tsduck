#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2017, Thierry Lelegard
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

  is specified, both versions are built by default.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
param(
    [switch]$Debug = $false,
    [switch]$Release = $false,
    [switch]$Win32 = $false,
    [switch]$Win64 = $false,
    [switch]$NoPause = $false
)

# PowerShell execution policy.
Set-StrictMode -Version 3
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force -ErrorAction:SilentlyContinue
Import-Module -Force -Name (Join-Path $PSScriptRoot Build-Common.psm1)

# Apply defaults.
if (-not $Debug -and -not $Release) {
    $Release = $true
}
if (-not $Win32 -and -not $Win64) {
    $Win32 = $true
    $Win64 = $true
}

# We need to work in the directory the project files.
Set-Location (Join-Path (Split-Path -Parent $PSScriptRoot) $MsvcDir)

# Rebuild TSDuck.
if ($Release -and $Win64) {
    & $MSBuild tsduck.sln /nologo /maxcpucount /property:Configuration=Release /property:Platform=x64
    if ($LastExitCode -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building 64-bit Release"
    }
}
if ($Release -and $Win32) {
    & $MSBuild tsduck.sln /nologo /maxcpucount /property:Configuration=Release /property:Platform=Win32
    if ($LastExitCode -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building 32-bit Release"
    }
}
if ($Debug -and $Win64) {
    & $MSBuild tsduck.sln /nologo /maxcpucount /property:Configuration=Debug /property:Platform=x64
    if ($LastExitCode -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building 64-bit Debug"
    }
}
if ($Debug -and $Win32) {
    & $MSBuild tsduck.sln /nologo /maxcpucount /property:Configuration=Release /property:Platform=Win32
    if ($LastExitCode -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building 32-bit Debug"
    }
}

Exit-Script -NoPause:$NoPause
