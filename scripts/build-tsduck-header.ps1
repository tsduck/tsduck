# Windows PowerShell wrapper to build-tsduck-header.py.

param([switch]$NoPause = $false)
$RootDir = (Split-Path -Parent $PSScriptRoot)
. $PSScriptRoot\build-tsduck-header.py "$RootDir\src\libtsduck" | Out-File -Encoding ascii "$RootDir\src\libtsduck\tsduck.h"
if (-not $NoPause) {
    pause
}
