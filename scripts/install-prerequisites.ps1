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
& "$InsDir\install-git.ps1"     -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-git-lfs.ps1" -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-python.ps1"  -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-libsrt.ps1"  -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-librist.ps1" -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-java.ps1"    -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-vatek.ps1"   -NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
& "$InsDir\install-dektec.ps1"  -NoPause:$NoPause -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
