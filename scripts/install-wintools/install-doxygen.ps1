#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Doxygen for Windows.
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

Write-Output "==== Doxygen download and installation procedure"
. "$PSScriptRoot\install-common.ps1"

winget install DimitriVanHeesch.Doxygen --accept-source-agreements

Exit-Script
