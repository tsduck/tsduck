#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Asciidoctor.
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

Write-Output "==== Asciidoctor download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

# Need ruby gem
if (-not (Search-Path "gem") -and -not (Search-Path "gem.cmd") -and -not (Search-Path "gem.exe")) {
    & "$PSScriptRoot\install-ruby.ps1" -NoPause -Destination:$Destination -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
    $Path = Get-Environment "Path"
    $env:Path = "${env:Path};$Path"
}

gem install asciidoctor asciidoctor-pdf

Exit-Script
