#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Git LFS extension (Large File Storage) for Windows.
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

Write-Output "==== Git LFS download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

Install-Standard-Exe `
    "https://git-lfs.github.com" `
    "*/git-lfs-windows*.exe" `
    "https://github.com/git-lfs/git-lfs/releases/download/v3.2.0/git-lfs-windows-v3.2.0.exe" `
    @("/verysilent", "/suppressmsgboxes", "/norestart")

Exit-Script
