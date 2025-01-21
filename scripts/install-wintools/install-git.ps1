#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Git for Windows.
#  See parameters documentation in install-common.ps1.
#
#-----------------------------------------------------------------------------

[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [string]$Destination = "",
    [switch]$ForceDownload = $false,
    [switch]$GitHubActions = $false,
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false
)

Write-Output "==== Git download and installation procedure"
. "$PSScriptRoot\install-common.ps1"

winget install Git.Git --accept-source-agreements

Propagate-Environment "Path"
Exit-Script
