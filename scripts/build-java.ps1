#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see the LICENSE.txt file
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
