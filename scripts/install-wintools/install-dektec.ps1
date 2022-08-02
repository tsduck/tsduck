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
#  Download and install the Dektec DTAPI for Windows.
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

Write-Output "==== Dektec WinSDK download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

$ReleasePage = "http://www.dektec.com/downloads/SDK/"
$DtapiInstaller = "DekTec SDK - Windows Setup.exe"

$Url = Get-URL-In-HTML $ReleasePage "*/WinSDK*.zip"

$DtapiZipName = Get-URL-Local $Url
$DtapiZipFile = (Join-Path $Destination $DtapiZipName)
$DtapiDir = (Join-Path $Destination ([io.fileinfo] $DtapiZipName).BaseName)
$DtapiSetup = (Join-Path $DtapiDir $DtapiInstaller)

Download-Package $Url $DtapiZipFile

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
