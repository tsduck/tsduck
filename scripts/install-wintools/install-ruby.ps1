#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Ruby.
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

Write-Output "==== Ruby download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

$Arch = if ($env:PROCESSOR_ARCHITECTURE -like 'Arm64*') { "arm" } else { "x64" }

Install-GitHub-Exe 'oneclick/rubyinstaller2'('/rubyinstaller-.*-' + $Arch + '\.exe$') `
    @("/allusers", "/verysilent", "/suppressmsgboxes", "/norestart") -Latest

Propagate-Environment "Path"
Exit-Script
