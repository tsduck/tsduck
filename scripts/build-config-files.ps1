#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see the LICENSE.txt file
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
