#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install the librist library for Windows.
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

Write-Output "==== librist download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

Install-GitHub-Exe 'tsduck/rist-installer' '/librist-.*\.exe$' @("/S")

Propagate-Environment "LIBRIST"

Exit-Script
