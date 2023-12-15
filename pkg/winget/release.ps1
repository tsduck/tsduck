#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Create a new TSDuck release for winget package manager.

 .PARAMETER Token

  Specify a Github token with "public_repo" access rights. By default, use
  environment variable GITHUB_TOKEN.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
param(
    [string]$Token = "",
    [switch]$NoPause = $false
)

# Without this, Invoke-WebRequest is awfully slow.
$ProgressPreference = 'SilentlyContinue'

# Force TLS 1.2 as default.
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# A function to exit the script.
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

# Search an executable in path.
function Find-Executable([string]$exe)
{
    foreach ($dir in $($env:Path -split ';')) {
        $dir = $dir.Trim()
        if ($dir -ne "") {
            $path = Join-Path $dir $exe
            if (Test-Path $path) {
                return $path
            }
        }
    }
    return $null
}

# Test a URL.
function Test-URL([string]$url)
{
    try {
        $response = Invoke-WebRequest -UseBasicParsing -Uri $url
        return [int][Math]::Floor($response.StatusCode / 100) -eq 2
    }
    catch {
        return $false
    }
}

# Check Github token.
if (-not $Token) {
    $Token = $env:GITHUB_TOKEN
    if (-not $Token) {
        Exit-Script "No Github token specified"
    }
}
$Cred = [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes(":$Token"))
$Headers = @{Authorization="Basic $Cred"}

# Search and install WingetCreate.
if (-not (Find-Executable 'wingetcreate.exe')) {
    if (-not (Find-Executable 'winget.exe')) {
        Exit-Script "Winget not installed"
    }
    Write-Output "WingetCreate not found, installing it..."
    winget install Microsoft.WingetCreate
    if (-not (Find-Executable 'wingetcreate.exe')) {
        Exit-Script "wingetcreate not found after installation"
    }
}

# Get TSDuck latest version.
$resp = Invoke-RestMethod -Headers $Headers https://api.github.com/repos/tsduck/tsduck/releases?per_page=1
$tag = $resp.tag_name
$version = $tag -replace '^v',''
$installer = "https://github.com/tsduck/tsduck/releases/download/$tag/TSDuck-Win64-$version.exe"
if (-not $version) {
    Exit-Script "Cannot find latest TSDuck release"
}

# Check if it already exists in winget.
Write-Output "Checking if TSDuck $version already exists in winget ..."
if (Test-URL "https://github.com/microsoft/winget-pkgs/tree/master/manifests/t/TSDuck/TSDuck/$version") {
    Exit-Script "TSDuck $version already exists in TSDuck"
}

# Check if the installer is available online.
Write-Output "Checking $installer ..."
if (-not (Test-URL "$installer")) {
    Exit-Script "TSDuck $version installer not found"
}

# Create the pull request for the new version.
Write-Output "Creating a pull request for TSDuck $version ..."
wingetcreate update --submit --token $Token --urls $installer --version $version TSDuck.TSDuck

Exit-Script
