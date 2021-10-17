#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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

  Rebuilt tsduck.h, the global header for the TSDuck library.

 .DESCRIPTION

  This script is useful when source files are added to or removed from the
  directory src\libtsduck.

  See the shell script build-tsduck-header.sh for a Unix equivalent.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding()]
param([switch]$NoPause = $false)

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$SrcDir = "$RootDir\src\libtsduck"

# Get all libtsduck files by type.
function Get-Headers([string[]] $Include = @(), [string[]] $Exclude = @())
{
    Get-ChildItem "$SrcDir" -Recurse -Include "*.h" | `
        Where-Object {
            $tmpname = $_.FullName
            ($Include.Count -eq 0) -or (($Include | ForEach-Object { $tmpname -like $_ }) -contains $true)
        } | `
        Where-Object {
            $tmpname = $_.FullName
            ($Exclude.Count -eq 0) -or -not (($Exclude | ForEach-Object { $tmpname -like $_ }) -contains $true)
        } | `
        ForEach-Object {
            "#include `"" + $_.Name + "`""
        } | `
        Sort-Object -Culture en-US
}

# Generate the main TSDuck header file.
& {
    Get-Content $SrcDir\..\HEADER.txt
    echo ''
    echo '#pragma once'
    Get-Headers -Exclude @("*Template.h", "*\tsduck.h", "*\unix\*", "*\linux\*", "*\mac\*", "*\windows\*", "*\private\*")
    echo ''
    echo '#if defined(TS_LINUX)'
    Get-Headers -Exclude @("*Template.h") -Include @("*\unix\*", "*\linux\*")
    echo '#endif'
    echo ''
    echo '#if defined(TS_MAC)'
    Get-Headers -Exclude @("*Template.h") -Include @("*\unix\*", "*\mac\*")
    echo '#endif'
    echo ''
    echo '#if defined(TS_WINDOWS)'
    Get-Headers -Exclude @("*Template.h") -Include @("*\windows\*")
    echo '#endif'
} | Out-File -Encoding ascii $SrcDir\tsduck.h

if (-not $NoPause) {
    pause
}
