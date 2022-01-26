#-----------------------------------------------------------------------------
#
#  Copyright (c) 2021, Thierry Lelegard
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

 .PARAMETER Destination

  Specify a local directory where the package will be downloaded.
  By default, use the downloads folder for the current user.

 .PARAMETER ForceDownload

  Force a download even if the package is already downloaded.

 .PARAMETER GitHubActions

  When used in a GitHub Action workflow, make sure that the required
  environment variables are propagated to subsequent jobs.

 .PARAMETER JRE

  Download and install the JRE. By default, the JDK is installed.

 .PARAMETER NoInstall

  Do not install the package. By default, the package is installed.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [string]$Destination = "",
    [switch]$ForceDownload = $false,
    [switch]$GitHubActions = $false,
    [switch]$JRE = $false,
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false
)

Write-Output "==== Java (AdoptOpenJDK) download and installation procedure"

# REST API for the latest releases of AdoptOpenJDK.
$API = "https://api.adoptopenjdk.net/v3"

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

# Without this, Invoke-WebRequest is awfully slow.
$ProgressPreference = 'SilentlyContinue'

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

if (-not $InstallerURL -or -not $InstallerName) {
    Exit-Script "Cannot find URL for installer"
}

# Create the directory for external products or use default.
if (-not $Destination) {
    $Destination = (New-Object -ComObject Shell.Application).NameSpace('shell:Downloads').Self.Path
}
else {
    [void](New-Item -Path $Destination -ItemType Directory -Force)
}

# Local installer file.
$InstallerPath = "$Destination\$InstallerName"

# Download installer.
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

# Install package.
if (-not $NoInstall) {
    Write-Output "Installing $InstallerName"
    # Note: /passive does not request any input from the user but still displays a progress window.
    # It is however required because msiexec /quiet fails to request UAC and immediately fails.
    Start-Process -Verb runas -FilePath msiexec.exe -ArgumentList @("/i", $InstallerPath, "INSTALLLEVEL=3", "/quiet", "/qn", "/norestart") -Wait
}

# Propagate JAVA_HOME in next jobs for GitHub Actions.
if ($GitHubActions) {
    $jhome = [System.Environment]::GetEnvironmentVariable("JAVA_HOME","Machine")
    Write-Output "JAVA_HOME=$jhome" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
}

Exit-Script
