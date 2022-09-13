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
# Common part for all install scripts.
# Not a PowerShell module for easier integration of variables.
#
# Must be included as follow by all scripts:
# . "$PSScriptRoot\install-common.ps1"
#
# Assume that the calling script defined standard installation parameters:
#
# -Destination directory
#    Specify a local directory where the package will be downloaded.
#    By default, use the downloads folder for the current user.
#    Sometimes unused if there is no external package to download
#    (Windows buildin features for instance).
#
# -ForceDownload
#    Force a download even if the package is already downloaded.
#    Sometimes unused if there is no external package to download.
#
# -GitHubActions
#    When used in a GitHub Action workflow, make sure that the required
#    environment variables are propagated to subsequent jobs. Ignored when
#    the installation of the package does not need specific variables.
#
# -NoInstall
#    Do not install the package. Download it only. By default, the package
#    is installed.
#
# -NoPause
#    Do not wait for the user to press <enter> at end of execution.
#    By default, execute a "pause" instruction at the end of execution,
#    which is useful when the script was run from Windows Explorer.
#
#-----------------------------------------------------------------------------

# Current user environment.
$IsAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
$CurrentUserName = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
$AdminUserName = (Get-CimInstance -ClassName Win32_UserAccount -Filter "LocalAccount = TRUE and SID like 'S-1-5-%-500'").name

# Without this, Invoke-WebRequest is awfully slow.
$ProgressPreference = 'SilentlyContinue'

# Create the directory for external products or use default.
if (-not $Destination) {
    $Destination = (New-Object -ComObject Shell.Application).NameSpace('shell:Downloads').Self.Path
}
[void](New-Item -Path $Destination -ItemType Directory -Force)

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

# Recurse the current script in administrator mode.
function Recurse-Admin([string]$CmdArgs = "")
{
    Write-Output "Must be administrator to continue, trying to restart as administrator ..."
    $cmd = "& '" + $MyInvocation.PSCommandPath + "' " + $CmdArgs
    if ($NoPause) {
        $cmd += " -NoPause"
    }
    Start-Process -Wait -Verb runas -FilePath PowerShell.exe -ArgumentList @("-ExecutionPolicy", "RemoteSigned", "-Command", $cmd)
}

# Install a Windows Capability.
function Install-Windows-Capability([string]$Name)
{
    $product = (Get-WindowsCapability -Online | Where-Object Name -like "${Name}*" | Select-Object -First 1)
    if ($product -eq $null) {
        Write-Output "$Name not found"
    }
    elseif ($product.State -like "Installed") {
        Write-Output "$($product.Name) already installed"
    }
    else {
        Write-Output "Installing $($product.Name) ..."
        [void](Add-WindowsCapability -Online -Name $product.Name)
    }
}

# Create a file when non-existent. Restrict its protection to a list of owners.
function Create-File-Set-Owner([string]$Path, [string[]]$Owners = @())
{
    # Create parent directory if non-existent.
    $dir = (Split-Path $Path -Parent)
    if (-not (Test-Path $dir -PathType Container)) {
        Write-Output "Creating directory $dir ..."
        [void](New-Item $dir -ItemType Directory -Force)
    }

    # Create file if non-existent.
    if (-not (Test-Path $Path -PathType Leaf)) {
        Write-Output "Creating file $Path ..."
        [void](New-Item $Path -ItemType File -Force)
    }

    # Reset ownership.
    Write-Output "Adjusting security of $Path ..."
    $acl = Get-Acl $Path
    $acl.SetAccessRuleProtection($true, $false)
    foreach ($user in $Owners) {
        $rule = New-Object system.security.accesscontrol.filesystemaccessrule($user, "FullControl", "Allow")
        $acl.SetAccessRule($rule)
    }
    $acl | Set-Acl
}

# Get the local file name part of a URL.
function Get-URL-Local([string]$Url)
{
    return (Split-Path -Leaf ([System.Uri]$Url).LocalPath)
}

# Get an HTML page. Return $null if not found.
function Get-HTML([string]$Url, [switch]$Fatal = $false)
{
    $status = 0
    $message = ""
    try {
        $response = Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $Url
        $status = [int][Math]::Floor($response.StatusCode / 100)
    }
    catch {
        $message = $_.Exception.Message
    }
    if ($status -ne 1 -and $status -ne 2) {
        # Error fetching download page.
        if ($message -eq "" -and (Test-Path variable:response)) {
            $message = "Status code $($response.StatusCode), $($response.StatusDescription)"
        }
        else {
            $message = "#### Error accessing ${Url}: $message"
        }
        if ($Fatal) {
            Exit-Script $message
        }
        Write-Output $message
        $reponse = $null
    }
    return $response
}

# Get an URL matching a pattern in a web page.
function Get-URL-In-HTML([string]$Url, [string]$Pattern, [string]$FallbackURL = "")
{
    $Fatal = -not $FallbackURL
    $response = Get-HTML $Url -Fatal:$Fatal
    if ($response -eq $null) {
        $ref = $null
    }
    else {
        $ref = $response.Links.href | Where-Object { $_ -like $Pattern } | Select-Object -First 1
    }
    if (-not -not $ref) {
        # Build the absolute URL's from base URL (the download page) and href links.
        return [string](New-Object -TypeName 'System.Uri' -ArgumentList ([System.Uri]$Url, $ref))
    }
    elseif ($Fatal) {
        Exit-Script "No package found in $Url"
    }
    else {
        return $FallbackURL
    }
}

# Get an URL matching a pattern in a GitHub project release.
function Get-URL-In-GitHub([string]$Repo, [string]$Pattern)
{
    $Url = (Invoke-RestMethod "https://api.github.com/repos/$Repo/releases?per_page=20" |
            ForEach-Object { $_.assets } |
            ForEach-Object { $_.browser_download_url } |
            Select-String $Pattern |
            Select-Object -First 1)

    if (-not $Url) {
        Exit-Script "No package matching '$Pattern' in GitHub repo $Repo"
    }
    else {
        return [string]$Url
    }
}

# Download a package.
function Download-Package([string]$Url, [string]$InstallerPath)
{
    if (-not $ForceDownload -and (Test-Path $InstallerPath)) {
        $InstallerName = (Split-Path -Leaf $InstallerPath)
        Write-Output "$InstallerName already downloaded, use -ForceDownload to download again"
    }
    else {
        Write-Output "Downloading $Url ..."
        Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $Url -OutFile $InstallerPath
        if (-not (Test-Path $InstallerPath)) {
            Exit-Script "$Url download failed"
        }
    }
}

# Standard installation procedure for an executable installer.
function Install-Standard-Exe([string]$ReleasePage, [string]$Pattern, [string]$FallbackURL = "", [string[]]$InstallerParams = @())
{
    $Url = Get-URL-In-HTML $ReleasePage $Pattern $FallbackURL
    $InstallerName = Get-URL-Local $Url
    $InstallerPath = "$Destination\$InstallerName"
    Download-Package $Url $InstallerPath
    if (-not $NoInstall) {
        Write-Output "Installing $InstallerName"
        Start-Process -Wait -FilePath $InstallerPath -ArgumentList $InstallerParams
    }
}

# Standard installation procedure for an executable installer from GitHub release assets.
function Install-GitHub-Exe([string]$Repo, [string]$Pattern, [string[]]$InstallerParams = @())
{
    $Url = Get-URL-In-GitHub $Repo $Pattern
    $InstallerName = Get-URL-Local $Url
    $InstallerPath = "$Destination\$InstallerName"
    Download-Package $Url $InstallerPath
    if (-not $NoInstall) {
        Write-Output "Installing $InstallerName"
        Start-Process -Wait -FilePath $InstallerPath -ArgumentList $InstallerParams
    }
}

# Standard installation procedure for an MSI installer.
function Install-Standard-Msi([string]$ReleasePage, [string]$Pattern, [string]$FallbackURL = "")
{
    $Url = Get-URL-In-HTML $ReleasePage $Pattern $FallbackURL
    $InstallerName = Get-URL-Local $Url
    $InstallerPath = "$Destination\$InstallerName"
    Download-Package $Url $InstallerPath
    if (-not $NoInstall) {
        Write-Output "Installing $InstallerName"
        Start-Process -Wait -Verb runas -FilePath msiexec.exe -ArgumentList @("/i", $InstallerPath, "/quiet", "/qn", "/norestart")
    }
}

# Define system-wide environment variable.
function Define-Environment([string]$Name, [string]$Value)
{
    [System.Environment]::SetEnvironmentVariable($Name, $Value, [System.EnvironmentVariableTarget]::Machine)
}

# Propagate an environment variable in next jobs for GitHub Actions.
# If value is unspecified, get it from system environment.
function Propagate-Environment([string]$Name, [string]$Value = "")
{
    if ($GitHubActions) {
        if ($Value -eq "") {
            $Value = [System.Environment]::GetEnvironmentVariable($Name, "Machine")
        }
        Write-Output "${Name}=${Value}" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
    }
}
