#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Install all pre-requisites packages to build TSDuck on Windows.

 .PARAMETER ForceDownload

  Force downloads even if packages are already downloaded.

 .PARAMETER GitHubActions

  When used in a GitHub Action workflow, make sure that the required
  environment variables are propagated to subsequent jobs.

 .PARAMETER NoDoxygen

  Do not install doxygen and its dependencies. Install everything else to
  build the project binaries and installers.

 .PARAMETER NoInstaller

  Do not install NSIS and its dependencies. Install everything else to
  build the project binaries.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [switch]$ForceDownload = $false,
    [switch]$GitHubActions = $false,
    [switch]$NoDoxygen = $false,
    [switch]$NoInstaller = $false,
    [switch]$NoPause = $false
)

$InsDir = "$PSScriptRoot\install-wintools"

if (-not $NoDoxygen) {
    & "$InsDir\install-graphviz.ps1" -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
    & "$InsDir\install-doxygen.ps1"  -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
}
if (-not $NoInstaller) {
    & "$InsDir\install-nsis.ps1" -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
}
& "$InsDir\install-git.ps1"         -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-python.ps1"      -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-asciidoctor.ps1" -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-libsrt.ps1"      -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-librist.ps1"     -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-java.ps1"        -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-vatek.ps1"       -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-dektec.ps1"      -NoPause:$NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
