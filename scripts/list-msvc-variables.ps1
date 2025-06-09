#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Invoke MSBuild to display all MSVC variables, in all configurations.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
param([switch]$NoPause = $false)

Import-Module -Force -Name "${PSScriptRoot}\tsbuild.psm1"
$MSBuild = (Find-MSBuild)

foreach ($configuration in @("Release", "Debug")) {
    foreach ($platform in @("x64", "Win32", "ARM64")) {
        & $MSBuild ${PSScriptRoot}\msvc\list-variables.vcxproj /nologo /property:Configuration=$configuration /property:Platform=$platform /target:'ListVariables'
    }
}

Exit-Script -NoPause:$NoPause
