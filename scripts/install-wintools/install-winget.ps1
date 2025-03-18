#-----------------------------------------------------------------------------
#
#  Copyright (c) 2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Install WinGet package manager, if not already installed.
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

Write-Output "==== WinGet download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

# A function to locate WinGet.
function Find-WinGet()
{
    foreach ($dir in $env:Path.Split(';')) {
        if (Test-Path "$dir\WinGet.exe") {
            return "$dir\WinGet.exe"
        }
    }
    return $null
}

# Install WinGet only when not found.
if (-not (Find-WinGet)) {
    if (-not $IsAdmin) {
        # Execution for non-admin user, recurse for admin part.
        Recurse-Admin
    }
    else {
        # See https://learn.microsoft.com/en-us/windows/package-manager/winget/
        # We noticed intermittent failures, retry up to 3 times.
        $retries = 3
        Write-Output "PowerShell version: $($PSVersionTable.PSVersion.ToString())"
        while (-not (Find-WinGet) -and $retries -gt 0) {
            $retries--
            if ($PSVersionTable.PSVersion.Major -lt 7) {
                Write-Output "Installing NuGet ..."
                Install-PackageProvider -Name "NuGet" -Force -ForceBootstrap -ErrorAction Continue
            }
            if (-not (Get-Module -Name Microsoft.WinGet.Client -ListAvailable)) {
                Write-Output "Installing Microsoft.WinGet.Client PowerShell module ..."
                Install-Module -Name Microsoft.WinGet.Client -AcceptLicense -Force -AllowClobber -Repository PSGallery -ErrorAction Continue
            }
            Import-Module -Name Microsoft.WinGet.Client
            Write-Output "Installing WinGet ..."
            try { Repair-WinGetPackageManager -AllUsers } catch {}
            # We noticed some spurious asynchronous activity after installation.
            # Let this background activity complete.
            $timeout = 5
            while (-not (Find-WinGet) -and $timeout -gt 0) {
                $timeout--
                Write-Output "WinGet not found, retrying in two seconds..."
                Start-Sleep -Seconds 2
            }
        }
    }
}

Write-Output "WinGet: $(Find-WinGet)"
Exit-Script
