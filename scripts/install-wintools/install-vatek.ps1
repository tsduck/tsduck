#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install the VATek (Vision Advance Technologies) SDK for Windows.
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

Write-Output "==== VATek SDK download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

if ($env:PROCESSOR_ARCHITECTURE -like 'Arm64*') {
    Exit-Script "VATek SDK is not available on Arm64"
}

Install-GitHub-Exe 'VisionAdvanceTechnologyInc/vatek_sdk_2' '/VATek-Win64-.*\.exe$' @("/verysilent", "/suppressmsgboxes", "/norestart") -Latest

Exit-Script
