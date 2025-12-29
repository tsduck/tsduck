#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
