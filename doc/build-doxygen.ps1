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
#
#  Windows PowerShell script to build the documentation using Doxygen.
#
#  See http://www.doxygen.org/download.html for Doxygen binary installers
#  for Windows. The installer updates %Path% to include the Doxygen binary
#  directory. Thus, doxygen is available as a simple command.
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Build the project documentation using Doxygen.

 .DESCRIPTION

  Build the project documentation using Doxygen.

 .PARAMETER NoOpen

  Do not open the generated documentation. By default, run the default HTML
  browser on the generated documentation hom page.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.

 .PARAMETER Version

  Version of the product. The default is extracted from the source file
  tsVersion.h.
#>
param(
    [Parameter(Mandatory=$false)][string]$Version,
    [switch]$NoOpen = $false,
    [switch]$NoPause = $false
)

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$SrcDir = "$RootDir\src"
$DoxyFile = "$PSScriptRoot\Doxyfile"

# Get the product version.
if (-not $Version) {
    $Version = (python "$RootDir\scripts\get-version-from-sources.py")
}
$env:TS_FULL_VERSION = $Version

# List of include directories.
$env:DOXY_INCLUDE_PATH = (Get-ChildItem "$SrcDir\libtsduck" -Recurse -Directory | ForEach-Object { $_.FullName }) -join " "

# Check if Doxygen is installed.
$DoxyExe = (Get-ChildItem 'C:\Program Files*\Doxygen*\bin' -Include doxygen.exe -Recurse | Select-Object FullName -First 1)

# Check if Graphviz is installed.
$DotExe = (Get-ChildItem 'C:\Program Files*\Graphviz*\bin' -Include dot.exe -Recurse | Select-Object DirectoryName -First 1)
if ($DotExe) {
    $env:HAVE_DOT = "YES"
    $env:DOT_PATH = $DotExe.DirectoryName
}
else {
    $env:HAVE_DOT = "NO"
    $env:DOT_PATH = ""
}

# A function to remove empty directories, recursively.
function Remove-EmptyFolder($path)
{
    Get-ChildItem $path -Directory | ForEach-Object { Remove-EmptyFolder $_.FullName }
    if (@(Get-ChildItem $path).Count -eq 0) {
        Remove-Item $path -Force
    }
}

# Generate Doxygen documentation.
if ($DoxyExe) {
    Push-Location $PSScriptRoot

    # Get Doxygen output directory from Doxyfile.
    $DoxyDir = (Get-Content $DoxyFile | Select-String '^ *OUTPUT_DIRECTORY *=' | Select-Object -Last 1) -replace '^.*= *' -replace ' *$'
    if ($DoxyDir -eq "") {
        $DoxyDir = "."
    }

    # Create the Doxygen output directory, of specified in Doxyfile.
    # Doxygen does not create parent directories.
    if (-not (Test-Path $DoxyDir)) {
        [void](New-Item -Path $DoxyDir -ItemType Directory -Force)
    }
    $DoxyDir = (Resolve-Path $DoxyDir)

    # Generate a summary file of all signalization.
    python "$SrcDir\doc\signalization-gen.py"

    # Generate documentation.
    Write-Host "Running Doxygen..."
    & $DoxyExe.FullName
    Pop-Location

    # Delete empty subdirectories (many of them created for nothing in case of hierachical output).
    Remove-EmptyFolder $DoxyDir

    # Open the browser.
    if (-not $NoOpen) {
        $HtmlIndex = "$DoxyDir\html\index.html"
        if (Test-Path $HtmlIndex) {
            Invoke-Item $HtmlIndex
        }
    }
}
else {
    Write-Host "Error: Doxygen not found"
}

if (-not $NoPause) {
    pause
}
