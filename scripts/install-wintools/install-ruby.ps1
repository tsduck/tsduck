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

if (-not $IsAdmin) {
    # Execution for non-admin user, recurse for admin part.
    Recurse-Admin
}
else {
    # Ruby is unconventional here. Instead of a constant package name and different
    # versions, the package name contains the version. Each version has a new package
    # name. Get latest version.
    $RubyPackage = (winget search --id RubyInstallerTeam.Ruby. |
                    Select-String RubyInstallerTeam.Ruby |
                    Select-Object -First 1) -replace '.* (RubyInstallerTeam\.Ruby\.[\.0-9]*) .*','$1'
    if (-not $RubyPackage) {
        Exit-Script "Latest Ruby package not found"
    }

    # Install Ruby.
    Write-Output "Installing $RubyPackage ..."
    winget install $RubyPackage --disable-interactivity --accept-package-agreements --accept-source-agreements

    # Get installation directory and add it to the system path.
    $RubyExe = Get-ChildItem "${env:SystemDrive}\Ruby*\bin\ruby.exe" | Select-Object -Last 1
    if (-not $RubyExe) {
        Exit-Script "ruby.exe not found after installation"
    }
    $RubyBin = $RubyExe.Directory.FullName
    Add-Directory-To-Path $RubyBin

    # Also add it locally
    if (";${env:Path};" -notlike "*;$RubyBin;*") {
        $env:Path = "$RubyBin;${env:Path}"
    }
}

Propagate-Environment "Path"
Exit-Script
