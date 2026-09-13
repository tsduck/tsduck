#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Graphviz for Windows (Graphviz is used by Doxygen).
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

Write-Output "==== Graphviz download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

Install-Standard-Exe "http://graphviz.org/download/" "*Release_graphviz-install-*-win64.exe*" "" @("/S")

Exit-Script
