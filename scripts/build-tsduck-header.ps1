# Windows PowerShell wrapper to build-tsduck-header.py.

param([switch]$NoPause = $false)
$RootDir = (Split-Path -Parent $PSScriptRoot)
python "${PSScriptRoot}\build-tsduck-header.py" "${RootDir}\src\libtsduck\tsduck.h"
if (-not $NoPause) {
    pause
}
