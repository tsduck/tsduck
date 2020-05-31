#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
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

  Download and install the libsrt library for Windows.

 .PARAMETER ForceDownload

  Force a download even if the package is already downloaded.

 .PARAMETER GitHubActions

  When used in a GitHub Action workflow, make sure that the LIBSRT
  environment variable is propagated to subsequent jobs.

 .PARAMETER NoInstall

  Do not install the package. By default, libsrt is installed.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [switch]$ForceDownload = $false,
    [switch]$GitHubActions = $false,
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false
)

Write-Output "libsrt download and installation procedure"
$ReleasePage = "https://github.com/tsduck/srt-win-installer/releases/latest"

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

# Local file names.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$ExtDir = "$RootDir\bin\external"

# Create the directory for external products when necessary.
[void] (New-Item -Path $ExtDir -ItemType Directory -Force)

# Without this, Invoke-WebRequest is awfully slow.
$ProgressPreference = 'SilentlyContinue'

# Get the HTML page for latest libsrt release.
$status = 0
$message = ""
try {
    $response = Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $ReleasePage
    $status = [int] [Math]::Floor($response.StatusCode / 100)
}
catch {
    $message = $_.Exception.Message
}

if ($status -ne 1 -and $status -ne 2) {
    # Error fetch NSIS download page.
    if ($message -eq "" -and (Test-Path variable:response)) {
        Exit-Script "Status code $($response.StatusCode), $($response.StatusDescription)"
    }
    else {
        Exit-Script "#### Error accessing ${ReleasePage}: $message"
    }
}

# Parse HTML page to locate the latest installer.
$Ref = $response.Links.href | Where-Object { $_ -like "*/libsrt-*.exe" } | Select-Object -First 1

if (-not $Ref) {
    Exit-Script "Could not find a reference to libsrt installer in ${ReleasePage}"
}

# Build the absolute URL's from base URL (the download page) and href links.
$Url = New-Object -TypeName 'System.Uri' -ArgumentList ([System.Uri]$ReleasePage, $Ref)
$InstallerName = (Split-Path -Leaf $Url.LocalPath)
$InstallerPath = "$ExtDir\$InstallerName"

# Download installer
if (-not $ForceDownload -and (Test-Path $InstallerPath)) {
    Write-Output "$InstallerName already downloaded, use -ForceDownload to download again"
}
else {
    Write-Output "Downloading $Url ..."
    Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $Url -OutFile $InstallerPath
    if (-not (Test-Path $InstallerPath)) {
        Exit-Script "$Url download failed"
    }
}

# Install libsrt
if (-not $NoInstall) {
    Write-Output "Installing $InstallerName"
    Start-Process -FilePath $InstallerPath -ArgumentList @("/S") -Wait
}

# Propagate LIBSRT in next jobs for GitHub Actions.
if ($GitHubActions) {
    $libsrt = [System.Environment]::GetEnvironmentVariable("LIBSRT","Machine")
    Write-Output "::set-env name=LIBSRT::$libsrt"
}

Exit-Script
