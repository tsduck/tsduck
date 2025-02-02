#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

Import-Module -Force -Name "${PSScriptRoot}\tsbuild.psm1"
$MSBuild = (Find-MSBuild)

# Build configuration files for all configurations.
foreach ($configuration in @("Release", "Debug")) {
    foreach ($platform in @("x64", "Win32", "ARM64")) {
        Write-Output "======== Building for $configuration-$platform ..."
        & $MSBuild ${PSScriptRoot}\msvc\config.vcxproj /nologo /property:Configuration=$configuration /property:Platform=$platform /target:BuildAllConfig
    }
}

Exit-Script -NoPause:$NoPause
