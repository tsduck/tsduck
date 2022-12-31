#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
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

  Builds the name of the directory which contains binaries and add it to the paths.

 .PARAMETER Debug

  Use the debug binaries instead of the release ones.

 .PARAMETER Display

  Only displays the directory, don't set the paths.

 .PARAMETER Win32

  Use the 32-bit version of the binaries. The default is the native platform.

 .PARAMETER Win64

  Use the 64-bit version of the binaries. The default is the native platform.
#>
param(
    [switch]$Debug = $false,
    [switch]$Display = $false,
    [switch]$Win32 = $false,
    [switch]$Win64 = $false
)

if ((-not $Win32 -and -not $Win64) -and [System.Environment]::Is64BitOperatingSystem) {
    $Win64 = $true
}

$Arch = ("Win32", "x64")[[bool]$Win64]
$Target = ("Release", "Debug")[[bool]$Debug]

$RootDir = (Split-Path -Parent $PSScriptRoot)
$BinDir = "$RootDir\bin\$Target-$Arch"
$PyDir = "$RootDir\src\libtsduck\python"
$Jar = "$RootDir\bin\java\tsduck.jar"

# A function to add a directory at the beginning of a search path.
function Prepend([string]$value, [string]$path)
{
    return (@($value) + @($path -split ';' | where {($_ -ne $value) -and ($_ -ne "")})) -join ';'
}

if ($Display) {
    Get-Item $BinDir
}
else {
    $env:Path=(Prepend $BinDir $env:Path)
    $env:TSPLUGINS_PATH=(Prepend $BinDir $env:TSPLUGINS_PATH)
    $env:PYTHONPATH=(Prepend $PyDir $env:PYTHONPATH)
    $env:CLASSPATH=(Prepend $Jar $env:CLASSPATH) + ";"
}
