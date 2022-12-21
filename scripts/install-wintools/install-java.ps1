#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
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

# Get JDK or JRE.
if ($JRE) {
    $ImageType = "jre"
}
else {
    $ImageType = "jdk"
}

# Get latest LTS version.
$lts = (Invoke-RestMethod $API/info/available_releases).most_recent_lts

# Get a list of assets for this LTS. Extract the one for 64-bit Windows.
$bin = (Invoke-RestMethod $API/assets/latest/$lts/hotspot).binary | `
    Where-Object os -eq windows | `
    Where-Object architecture -eq x64 | `
    Where-Object jvm_impl -eq hotspot | `
    Where-Object heap_size -eq normal | `
    Where-Object image_type -eq $ImageType | `
    Where-Object project -eq jdk | `
    Select-Object -First 1 -Property installer

$InstallerURL = $bin.installer.link
$InstallerName = $bin.installer.name
$InstallerPath = "$Destination\$InstallerName"

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
