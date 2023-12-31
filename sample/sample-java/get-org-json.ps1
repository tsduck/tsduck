#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Download and compile the org.json Java package. Required for sample Java
  applications using JSON.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding(SupportsShouldProcess=$true)]
param([switch]$NoPause = $false)

$root = $PSScriptRoot
Write-Output "Get org.json Java package"
$API = "https://api.github.com/repos/stleary/JSON-java"

# Without this, Invoke-WebRequest is awfully slow.
$ProgressPreference = 'SilentlyContinue'

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

# Get URL of source archive and download it.
$zip_url = (Invoke-RestMethod "$API/releases/latest").zipball_url
$zip_file = "$root\org.json.zip"
Write-Output "Downloading org.json source code from $zip_url ..."
Invoke-WebRequest -UseBasicParsing -UserAgent Download -Uri $zip_url -OutFile $zip_file
if (-not (Test-Path $zip_file)) {
    Exit-Script "$zip_url download failed"
}

# Add the DLL that helps with file compression.
# This requires .NET 4.5 (FileNotFoundException on previous releases).
Add-Type -Assembly System.IO.Compression.FileSystem

# Extract to this temporary directory.
$temp = "$root\temp"
Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
[void] (New-Item -Path $temp -ItemType Directory -Force)
try {
    Write-Output "Expanding $zip_file ..."
    $archive = [System.IO.Compression.ZipFile]::Open($zip_file, "Read")
    [System.IO.Compression.ZipFileExtensions]::ExtractToDirectory($archive, $temp)
}
finally {
    $archive.Dispose()
    $archive = $null
}

# Find the final source directory (all classes are in org.json).
$deep_src_json = (Get-ChildItem $temp -Recurse | Where-Object { $_.FullName -like '*\src\main\java\org\json' } | Select-Object -First 1)
$deep_src_json = $deep_src_json.FullName
Write-Output "Found source code in $deep_src_json"
$deep_src_org = (Split-Path -Parent $deep_src_json)
$root_src_org = "$root\org"

# Move it directly under current directory (org.json).
Write-Output "Moving $deep_src_org to $root_src_org ..."
Remove-Item $root_src_org -Recurse -Force -ErrorAction SilentlyContinue
Move-Item $deep_src_org $root_src_org -Force

# Compile org.json classes.
$jar_file = "$root\org.json.jar"
Write-Output "Compiling org.json classes ..."
Push-Location $root
javac org\json\*.java
jar cf $jar_file org\json\*.class
Pop-Location
Write-Output "Final jar: $jar_file"

# Cleanup.
Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $root_src_org -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $zip_file -Recurse -Force -ErrorAction SilentlyContinue

Exit-Script
