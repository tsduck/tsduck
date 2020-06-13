# Build the TSDuck extension binary installers for Windows.

param([switch]$NoPause = $false)

$RootDir = (Split-Path -Parent $PSScriptRoot)
$MsvcDir = (Join-Path $RootDir "msvc")
$SrcDir = (Join-Path $RootDir "src")
$InstallerDir = (Join-Path $RootDir "installers")
$Script = (Join-Path $PSScriptRoot "tsduck-extension-foo.nsi")

# Lower process priority so that the build does not eat up all CPU.
(Get-Process -Id $PID).PriorityClass = "BelowNormal"

# Exit this script. If optional variable $NoPause is $false, pause.
function Exit-Script
{
    param([Parameter(Mandatory=$false)][String] $Message = "", [switch]$NoPause = $false)
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

# Locate NSIS, the Nullsoft Scriptable Installation System.
$NSIS = Get-Item "C:\Program Files*\NSIS\makensis.exe" | ForEach-Object { $_.FullName} | Select-Object -Last 1
if (-not $NSIS) {
    Exit-Script -NoPause:$NoPause "NSIS not found"
}

# Build the project.
Push-Location
& (Join-Path $PSScriptRoot build.ps1) -NoPause
$Code = $LastExitCode
Pop-Location
if ($Code -ne 0) {
    Exit-Script -NoPause:$NoPause "Error building the project"
}

# Build binary installers.
& $NSIS /V2 /DWin64 $Script
& $NSIS /V2 /DWin32 $Script

Exit-Script -NoPause:$NoPause
