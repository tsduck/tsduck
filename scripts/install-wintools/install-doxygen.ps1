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

Install-Standard-Exe `
    "http://www.doxygen.nl/download.html" `
    "*/doxygen-*-setup.exe" `
    "https://sourceforge.net/projects/doxygen/files/rel-1.8.16/doxygen-1.8.16-setup.exe/download" `
    @("/verysilent", "/suppressmsgboxes", "/norestart")

Exit-Script
