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

  Build the .names and .xml configuration files for TSDuck.

  This script calls MSBuild with the corresponding targets. It is useful to
  quickly rebuild the configuration files when a .names or .xml file was
  modified but, since these files are MSBuild targets, they are automatically
  rebuilt by the project and there is no need to explicitly run this script.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
param([switch]$NoPause = $false)

# Fast search of MSBuild.
$Search = @("C:\Program Files\Microsoft Visual Studio\*\Community\MSBuild\Current\Bin\amd64\MSBuild.exe",
            "C:\Program Files\Microsoft Visual Studio\*\Community\MSBuild\Current\Bin\MSBuild.exe")
$MSBuild = Get-Item $Search | Select-Object -First 1 | ForEach-Object { $_.FullName}
if (-not $MSBuid) {
	# If not immediately found, use a full search.
    Import-Module -Force -Name "${PSScriptRoot}\tsbuild.psm1"
    $MSBuild = (Find-MSBuild)
}

# Build configuration files for all configurations.
foreach ($configuration in @("Release", "Debug")) {
    foreach ($platform in @("x64", "Win32")) {
        Write-Output "======== Building for $configuration-$platform ..."
        & $MSBuild ${PSScriptRoot}\msvc\config.vcxproj /nologo /property:Configuration=$configuration /property:Platform=$platform /target:BuildAllConfig
    }
}

Exit-Script -NoPause:$NoPause
