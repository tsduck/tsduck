#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Python for Windows.
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

Write-Output "==== Python download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

Install-Standard-Exe `
    "https://www.python.org/downloads/windows/" `
    "*/python-*-amd64.exe" `
    "https://www.python.org/ftp/python/3.9.0/python-3.9.0-amd64.exe" `
    @("/quiet", "InstallAllUsers=1", "CompileAll=1", "PrependPath=1", "Include_test=0")

Propagate-Environment "Path"

Exit-Script
