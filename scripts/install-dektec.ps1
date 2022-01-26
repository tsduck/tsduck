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

  Download and install the Dektec DTAPI for Windows.

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

Write-Output "==== Dektec WinSDK download and installation procedure"

# Web page for the latest releases.
$DektecUrl = "http://www.dektec.com/downloads/SDK/"
$DtapiInstaller = "DekTec SDK - Windows Setup.exe"

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

# Get the HTML page for downloads.
$status = 0
$message = ""
$Ref = $null
try {
    $response = Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $DektecUrl
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
    $Ref = $response.Links.href | Where-Object { $_ -like "*/WinSDK*.zip" } | Select-Object -First 1
}

if (-not $Ref) {
    # Could not find a reference to installer.
    Exit-Script "Could not find a reference to librist installer in ${ReleasePage}"
}
else {
    # Build the absolute URL's from base URL (the download page) and href links.
    $DtapiUrl = (New-Object -TypeName 'System.Uri' -ArgumentList ([System.Uri]$DektecUrl),$Ref)
}

# Create the directory for external products or use default.
if (-not $Destination) {
    $Destination = (New-Object -ComObject Shell.Application).NameSpace('shell:Downloads').Self.Path
}
else {
    [void](New-Item -Path $Destination -ItemType Directory -Force)
}

# Local installer files.
$DtapiZipName = (Split-Path -Leaf $DtapiUrl.toString())
$DtapiZipFile = (Join-Path $Destination $DtapiZipName)
$DtapiDir = (Join-Path $Destination ([io.fileinfo] $DtapiZipName).BaseName)
$DtapiSetup = (Join-Path $DtapiDir $DtapiInstaller)

# Download WinSDK.zip
if (-not $ForceDownload -and (Test-Path $DtapiZipFile)) {
    Write-Output "$DtapiZipName already downloaded, use -ForceDownload to download again"
}
else {
    Write-Output "Downloading $DtapiUrl ..."
    Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $DtapiUrl -OutFile $DtapiZipFile
    if (-not (Test-Path $DtapiZipFile)) {
        Exit-Script "$DtapiUrl download failed"
    }
}

# Extract archive.
if (Test-Path $DtapiDir) {
    Write-Output "Cleaning up previous $DtapiDir"
    Remove-Item $DtapiDir -Recurse -Force
}
Write-Output "Expanding DTAPI to $DtapiDir ..."
Expand-Archive $DtapiZipFile -DestinationPath $DtapiDir
if (-not (Test-Path $DtapiSetup)) {
    Exit-Script "$DtapiInstaller not found in $DtapiZipFile"
}

# Install the DTAPI.
if (-not $NoInstall) {
    # The Dektec WinSDK refuses to upgrade, you need to uninstall first.
    # Registry roots for uninstallation procedures:
    $RegUninstall = @(
        "Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
        "Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
    )

    # Loop on all uninstallation entries, looking for *dektec* names.
    foreach ($reg in $RegUninstall) {
        if (Test-Path $reg) {
            # We must get the list of registered products first, then uninstall.
            # If we uninstall during Get-ChildItem, we break the iteration.
            $Products = (Get-ChildItem -Recurse -Path $reg)
            $Products | ForEach-Object {
                $name = (Split-Path -Leaf $_.Name)
                $entries = ($_ | Get-ItemProperty)
                if ($entries.DisplayName -like "*dektec*") {
                    # Found a Dektec product, uninstall it.
                    $cmd = $entries.UninstallString
                    Write-Output "Uninstalling $($entries.DisplayName) version $($entries.DisplayVersion)"
                    if ($cmd -like "msiexec*") {
                        # Run msiexec with silent options.
                        Start-Process -FilePath msiexec.exe -ArgumentList @("/uninstall", "$name", "/passive", "/norestart") -Wait
                    }
                    else {
                        # Run the uninstall command
                        Write-Output "Executing $cmd"
                        Start-Process -FilePath powershell.exe -ArgumentList $cmd -Wait
                    }
                }
            }
        }
    }

    # Install new version in silent mode.
    Write-Output "Executing $DtapiSetup"
    Start-Process -FilePath $DtapiSetup -ArgumentList @("/S", "/v/qn") -Wait
}

Exit-Script
