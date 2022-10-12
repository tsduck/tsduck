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

  Download and install TSDuck for Windows.

  This is a sample script which can be used by other projects which need TSDuck.

 .PARAMETER All

  Install all options. By default, only the tools, plugins and documentation
  are installed. In case of upgrade over an existing installation, the default
  is to upgrade the same options as in the previous installation.

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
    [switch]$All = $false,
    [string]$Destination = "",
    [switch]$ForceDownload = $false,
    [switch]$GitHubActions = $false,
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false
)

Write-Output "==== TSDuck download and installation procedure"

# Web page for the latest releases.
$ReleasePage = "https://github.com/tsduck/tsduck/releases/latest"

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

# Get the HTML page for latest package release.
$status = 0
$message = ""
$Ref = $null
try {
    $response = Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $ReleasePage
    $status = [int] [Math]::Floor($response.StatusCode / 100)
}
catch {
    $message = $_.Exception.Message
}

if ($status -ne 1 -and $status -ne 2) {
    # Error fetching download page.
    if ($message -eq "" -and (Test-Path variable:response)) {
        Write-Output "Status code $($response.StatusCode), $($response.StatusDescription)"
    }
    else {
        Write-Output "#### Error accessing ${ReleasePage}: $message"
    }
}
else {
    # Parse HTML page to locate the latest installer.
    $Ref = $response.Links.href | Where-Object { $_ -like "*/TSDuck-Win64-*.exe" } | Select-Object -First 1
}

if (-not $Ref) {
    # Could not find a reference to installer.
    Exit-Script "Could not find a reference to installer in ${ReleasePage}"
}
else {
    # Build the absolute URL's from base URL (the download page) and href links.
    $Url = New-Object -TypeName 'System.Uri' -ArgumentList ([System.Uri]$ReleasePage, $Ref)
}

# Create the directory for external products or use default.
if (-not $Destination) {
    $Destination = (New-Object -ComObject Shell.Application).NameSpace('shell:Downloads').Self.Path
}
else {
    [void](New-Item -Path $Destination -ItemType Directory -Force)
}

# Local installer file.
$InstallerName = (Split-Path -Leaf $Url.LocalPath)
$InstallerPath = "$Destination\$InstallerName"

# Download installer.
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

# Install package.
if (-not $NoInstall) {
    Write-Output "Installing $InstallerName"
    $ArgList = @("/S")
    if ($All) {
        $ArgList += "/all=true"
    }
    Start-Process -FilePath $InstallerPath -ArgumentList $ArgList -Wait
}

# Propagate Path in next jobs for GitHub Actions.
if ($GitHubActions) {
    $tsduck     = [System.Environment]::GetEnvironmentVariable("TSDUCK",     [System.EnvironmentVariableTarget]::Machine)
    $path       = [System.Environment]::GetEnvironmentVariable("Path",       [System.EnvironmentVariableTarget]::Machine)
    $pythonpath = [System.Environment]::GetEnvironmentVariable("PYTHONPATH", [System.EnvironmentVariableTarget]::Machine)
    $classpath  = [System.Environment]::GetEnvironmentVariable("CLASSPATH",  [System.EnvironmentVariableTarget]::Machine)
    Write-Output "TSDUCK=$tsduck"         | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
    Write-Output "Path=$path"             | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
    Write-Output "PYTHONPATH=$pythonpath" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
    Write-Output "CLASSPATH=$classpath"   | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
}

Exit-Script
