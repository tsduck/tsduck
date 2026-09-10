#-----------------------------------------------------------------------------
#
#  Copyright (c) 2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install the Robotweax SRT SDK for Windows.
#  Two version of the SDK exist, one with OpenSSL cryptographic backend and
#  one with Microsoft BCrypt (CNG) backend. We install the BCrypt one.
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

Write-Output "==== Robotweax SRT SDK download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

Install-GitHub-Exe 'Robotweax/srt' '/robotweax-srt-.*-windows-sdk-bcrypt\.exe$' @("/VERYSILENT /SUPPRESSMSGBOXES /NORESTART")

Propagate-Environment "ROBOTWEAX_SRT_BCRYPT"

Exit-Script
