#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install the libsrt library for Windows.
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

Write-Output "==== libsrt download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

if ($env:PROCESSOR_ARCHITECTURE -like 'Arm64*') {
    Exit-Script "libsrt is not available on Arm64"
}

# Get the URL of the latest installer.
$URL = (Get-URL-In-GitHub "Haivision/srt" @("/libsrt-.*\.exe$", "/libsrt-.*-win-installer\.zip$"))

if (-not ($URL -match "\.zip$") -and -not ($URL -match "\.exe$")) {
    Exit-Script "Unexpected URL, not .exe, not .zip: $URL"
}

$InstallerName = Get-URL-Local $URL
$InstallerPath = "$Destination\$InstallerName"
Download-Package $URL $InstallerPath

# If installer is an archive, expect an exe with same name inside.
if ($InstallerName -match "\.zip$") {

    # Expected installer name in archive.
    $ZipName = $InstallerName
    $ZipPath = $InstallerPath
    $InstallerName = $ZipName -replace '-win-installer.zip','.exe'
    $InstallerPath = "$Destination\$InstallerName"

    # Extract the installer.
    Remove-Item -Force $InstallerPath -ErrorAction SilentlyContinue
    Write-Output "Expanding $ZipName ..."
    Expand-Archive $ZipPath -DestinationPath $Destination
    if (-not (Test-Path $InstallerPath)) {
        Exit-Script "$InstallerName not found in $ZipName"
    }
}

# Install package
if (-not $NoInstall) {
    Write-Output "Installing $InstallerName"
    Start-Process -FilePath $InstallerPath -ArgumentList @("/S") -Wait
}

# Propagate LIBSRT in next jobs for GitHub Actions.
Propagate-Environment "LIBSRT"

Exit-Script
