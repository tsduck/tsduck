# Build the TSDuck extension from the command line, an alternative to Visual Studio.

param([switch]$NoPause = $false)

$RootDir = (Split-Path -Parent $PSScriptRoot)
$ProjDir = (Join-Path $RootDir "msvc")
$SolutionFileName = (Join-Path $ProjDir "tsduck-extension-foo.sln")

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

# Find MSBuild.exe.
$MSBuild = Get-ChildItem -Recurse -Path "C:\Program Files*\Microsoft Visual Studio" -Include MSBuild.exe -ErrorAction Ignore |
           ForEach-Object { (Get-Command $_).FileVersionInfo } |
           Sort-Object -Unique -Property FileVersion |
           ForEach-Object { $_.FileName} |
           Select-Object -Last 1
if (-not $MSBuild) {
    Exit-Script -NoPause:$NoPause "MSBuild not found"
}

# A function to invoke MSBuild.
function Call-MSBuild ([string] $configuration, [string] $platform, [string] $target = "")
{
    & $MSBuild $SolutionFileName /nologo /maxcpucount /property:Configuration=$configuration /property:Platform=$platform $target 
    if ($LastExitCode -ne 0) {
        Exit-Script -NoPause:$NoPause "Error building $platform $configuration"
    }
}

# Rebuild TSDuck extension.
Call-MSBuild Release x64
Call-MSBuild Release Win32

Exit-Script -NoPause:$NoPause
