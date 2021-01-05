#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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

<#
 .SYNOPSIS

  Download and install Java for Windows (AdoptOpenJDK community project).

 .PARAMETER ForceDownload

  Force a download even if Java is already downloaded.

 .PARAMETER JRE

  Download and install the JRE. By default, the JDK is installed.

 .PARAMETER NoInstall

  Do not install the Java package. By default, it is installed.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [switch]$ForceDownload = $false,
    [switch]$JRE = $false,
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false
)

Write-Output "Java (AdoptOpenJDK) download and installation procedure"
$API = "https://api.adoptopenjdk.net/v3"

# Local file names.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$ExtDir = "$RootDir\bin\external"

# Create the directory for external products when necessary.
[void] (New-Item -Path $ExtDir -ItemType Directory -Force)

# Without this, Invoke-WebRequest is awfully slow.
$ProgressPreference = 'SilentlyContinue'

# A function to exit this script.
function Exit-Script([string]$Message = "")
{
    $Code = 0
    if ($Message -ne "") {
        Write-Host "ERROR: $Message"
        $Code = 1
    }
    if (-not $NoPause) {
        pause
    }
    exit $Code
}

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
$InstallerPath = "$ExtDir\$InstallerName"

if (-not $InstallerURL -or -not $InstallerName) {
    Exit-Script "Cannot find URL for installer"
}

# Download installer
if (-not $ForceDownload -and (Test-Path $InstallerPath)) {
    Write-Output "$InstallerName already downloaded, use -ForceDownload to download again"
}
else {
    Write-Output "Downloading $InstallerURL ..."
    Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $InstallerURL -OutFile $InstallerPath
    if (-not (Test-Path $InstallerPath)) {
        Exit-Script "$InstallerURL download failed"
    }
}

# Install product
if (-not $NoInstall) {
    Write-Output "Installing $InstallerName"
    # Note: /passive does not request any input from the user but still displays a progress window.
    # It is however required because msiexec /quiet fails to request UAC and immediately fails.
    Start-Process msiexec -ArgumentList @("/i", "$InstallerPath", "INSTALLLEVEL=3", "/quiet", "/passive") -Wait
}

Exit-Script
