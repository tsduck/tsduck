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

  Download and install the librsrt library for Windows.

 .PARAMETER Destination

  Specify a local directory where the package will be downloaded.
  By default, use the downloads folder for the current user.

 .PARAMETER ForceDownload

  Force a download even if the package is already downloaded.

 .PARAMETER GitHubActions

  When used in a GitHub Action workflow, make sure that the required
  environment variables are propagated to subsequent jobs.

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
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false
)

Write-Output "==== libsrt download and installation procedure"

# Web page for the latest releases of rist-installer.
$ReleasePage = "https://github.com/tsduck/rist-installer/releases/latest"

# A function to exit this script.
function Exit-Script([string]$Message = "")
{
    $Code = 0
    if ($Message -ne "") {
        Write-Output "ERROR: $Message"
        $Code = 1
    }
    if (-not $NoPause) {
        pause
    }
    exit $Code
}

# Without this, Invoke-WebRequest is awfully slow.
$ProgressPreference = 'SilentlyContinue'

# Get the URL of the latest installer.
$URL = (Invoke-RestMethod "https://api.github.com/repos/Haivision/srt/releases?per_page=20" |
        ForEach-Object { $_.assets } |
        ForEach-Object { $_.browser_download_url } |
        Select-String @("/libsrt-.*\.exe$", "/libsrt-.*-win-installer\.zip$") |
        Select-Object -First 1)

if (-not $URL) {
    Exit-Script "Could not find a libsrt installer on GitHub"
}
if (-not ($URL -match "\.zip$") -and -not ($URL -match "\.exe$")) {
    Exit-Script "Unexpected URL, not .exe, not .zip: $URL"
}

# Create the directory for external products or use default.
if (-not $Destination) {
    $Destination = (New-Object -ComObject Shell.Application).NameSpace('shell:Downloads').Self.Path
}
else {
    [void](New-Item -Path $Destination -ItemType Directory -Force)
}

# Local installer file.
$InstallerName = (Split-Path -Leaf $URL)
$InstallerPath = "$Destination\$InstallerName"

# Download installer
if (-not $ForceDownload -and (Test-Path $InstallerPath)) {
    Write-Output "$InstallerName already downloaded, use -ForceDownload to download again"
}
else {
    Write-Output "Downloading $URL ..."
    Invoke-WebRequest $URL.ToString() -UseBasicParsing -UserAgent Download -OutFile $InstallerPath
    if (-not (Test-Path $InstallerPath)) {
        Exit-Script "$URL download failed"
    }
}

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
if ($GitHubActions) {
    $libsrt = [System.Environment]::GetEnvironmentVariable("LIBSRT","Machine")
    Write-Output "LIBSRT=$libsrt" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
}

Exit-Script
