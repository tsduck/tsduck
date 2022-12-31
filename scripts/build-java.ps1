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

  Build TSDuck Java bindings from the command line.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
param(
    [switch]$NoPause = $false
)

# PowerShell execution policy.
Set-StrictMode -Version 3
if (((Get-ExecutionPolicy) -ne "Unrestricted") -and ((Get-ExecutionPolicy) -ne "RemoteSigned")) {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force -ErrorAction:SilentlyContinue
}
Import-Module -Force -Name "${PSScriptRoot}\tsbuild.psm1"

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$BinDir = "${RootDir}\bin\java"
$SrcRoot = "${RootDir}\src\libtsduck\java"
$JavaBin = (Find-Java)

if (-not $JavaBin) {
    Exit-Script -NoPause:$NoPause "No Java compiler found, skipped Java bindings"
}

# Build output directory for Java bindings.
[void] (New-Directory $BinDir)

# Build the manifest for the TSDuck JAR.
$Version = (python "${PSScriptRoot}\get-version-from-sources.py")
Get-Content "${SrcRoot}\Manifest.txt" |
    ForEach-Object {
        $_ -replace "{{VERSION}}","$Version"
    } |
    Out-File "${BinDir}\Manifest.txt" -Encoding ascii

# Compile all Java source files.
# Generate classes to make sure they are compatible with Java 8.
Push-Location $SrcRoot\src
Get-ChildItem -Recurse . -Name *.java |
    ForEach-Object {
        Write-Output "Compiling $_ ..."
        . "${JavaBin}\javac.exe" -source 1.8 -target 1.8 -Xlint:-options -d "$BinDir" $_
    }
Pop-Location

# Generating the TSDuck JAR.
Push-Location $BinDir
Write-Output "Generating tsduck.jar ..."
. "${JavaBin}\jar.exe" -c -f tsduck.jar -m Manifest.txt (Get-ChildItem -Recurse . -Name *.class)
Pop-Location

Exit-Script -NoPause:$NoPause
