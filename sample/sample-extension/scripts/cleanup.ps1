# Windows PowerShell script to clean up the project directory tree.
# Back to a clean state of source files.

[CmdletBinding(SupportsShouldProcess=$true)]
param([switch]$NoPause = $false)

$RootDir = (Split-Path -Parent $PSScriptRoot)

# Note that we cannot pipe Get-ChildItem directly into Remove-Item since Get-ChildItem -Recurse
# return directories first, followed by their content. So, we first get the list of files and
# directories to remove and then we delete them, if not yet deleted by a previous recursion.

$files = @(Get-ChildItem -Recurse -Force $RootDir | Where-Object {
    ($_.FullName -notlike '*\.git\*') -and
    ($_.FullName -notlike '*\installers\*') -and
    ($_.Name -like "doxy" -or
    $_.Name -like "debug" -or
    $_.Name -like "debug-*" -or
    $_.Name -like "release" -or
    $_.Name -like "release-*" -or
    $_.Name -like "ipch" -or
    $_.Name -like ".vs" -or
    $_.Name -like "*.user" -or
    $_.Name -like "*.user.*" -or
    $_.Name -like "*.VC.db" -or
    $_.Name -like "*.VC.opendb" -or
    $_.Name -like "*.sdf" -or
    $_.Name -like "*.suo" -or
    $_.Name -like "*.opensdf" -or
    $_.Name -like "*~" -or
    $_.Name -like "*.exe" -or
    $_.Name -like "*.obj" -or
    $_.Name -like "*.o" -or
    $_.Name -like "*.so" -or
    $_.Name -like "*.dylib" -or
    $_.Name -like "core" -or
    $_.Name -like "core.*" -or
    $_.Name -like "*.bak" -or
    $_.Name -like "*.tmp" -or
    $_.Name -like "*.lib" -or
    $_.Name -like "*.dll" -or
    $_.Name -like "*.exe" -or
    $_.Name -like "*.autosave")
}).FullName

foreach ($file in $files) {
    if ((Test-Path $file) -and $PSCmdlet.ShouldProcess($file,"Delete")) {
        "Deleting $file"
        Remove-Item $file -Recurse -Force
    }
}

if (-not $NoPause) {
    pause
}
