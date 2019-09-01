# Build the TSDuck extension from the command line, an alternative to Visual Studio.

param([switch]$NoPause = $false)

$RootDir = (Split-Path -Parent $PSScriptRoot)
$ProjDir = (Join-Path $RootDir "msvc")
$SolutionFileName = (Join-Path $ProjDir "foo.sln")

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
$MSBuild = ""
$MSBuildPath = ($env:Path -split ";") + @(
    'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64',
    'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin',
    'C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\amd64',
    'C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin',
    'C:\Program Files (x86)\MSBuild\14.0\Bin\amd64',
    'C:\Program Files (x86)\MSBuild\14.0\Bin'
) | Where-Object {$_}
foreach ($dir in $MSBuildPath) {
    $path = (Join-Path $dir "MSBuild.exe")
    if (Test-Path $path) {
        $MSBuild = $path
        break
    }
}
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
