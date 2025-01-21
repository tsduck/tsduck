#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Java for Windows (AdoptOpenJDK community project).
#  See parameters documentation in install-common.ps1.
#
#  Additional parameters:
#
#  -JRE
#     Download and install the JRE. By default, the JDK is installed.
#
#-----------------------------------------------------------------------------


[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [string]$Destination = "",
    [switch]$ForceDownload = $false,
    [switch]$GitHubActions = $false,
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false,
    # Addition parameters:
    [switch]$JRE = $false
)

Write-Output "==== Java (AdoptOpenJDK) download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

# REST API for the latest releases of Eclipse Temurin.
$API = "https://api.adoptium.net/v3"

$ImageType = if ($JRE) { "jre" } else { "jdk" }
$Arch = if ($env:PROCESSOR_ARCHITECTURE -like 'Arm64*') { "aarch64" } else { "x64" }

# Get latest LTS versions, in reverse order.
$all_lts = (Invoke-RestMethod $API/info/available_releases).available_lts_releases
[array]::Reverse($all_lts)

# Get a list of assets for LTS until one has 64-bit Windows.
foreach ($lts in $all_lts) {

    $bin = (Invoke-RestMethod $API/assets/latest/$lts/hotspot).binary | `
        Where-Object os -eq windows | `
        Where-Object architecture -eq $Arch | `
        Where-Object jvm_impl -eq hotspot | `
        Where-Object heap_size -eq normal | `
        Where-Object image_type -eq $ImageType | `
        Where-Object project -eq jdk | `
        Select-Object -First 1 -Property installer

    $InstallerURL = $bin.installer.link
    $InstallerName = $bin.installer.name
    $InstallerPath = "$Destination\$InstallerName"

    if (-not -not $InstallerURL -and -not -not $InstallerName) {
        break
    }
}

if (-not $InstallerURL -or -not $InstallerName) {
    Exit-Script "Cannot find URL for installer"
}

Download-Package $InstallerURL $InstallerPath

if (-not $NoInstall) {
    Write-Output "Installing $InstallerName"
    Start-Process -Verb runas -FilePath msiexec.exe -ArgumentList @("/i", $InstallerPath, "INSTALLLEVEL=3", "/quiet", "/qn", "/norestart") -Wait
}

Propagate-Environment "JAVA_HOME"
Exit-Script
