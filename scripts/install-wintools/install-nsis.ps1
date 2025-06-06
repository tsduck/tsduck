#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install NSIS, the NullSoft Installer Scripting.
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

Write-Output "==== NSIS download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

Install-Standard-Exe `
    "https://nsis.sourceforge.io/Download" `
    "*/nsis-*-setup.exe?download" `
    "http://prdownloads.sourceforge.net/nsis/nsis-3.05-setup.exe?download" `
    @("/S")

Exit-Script
