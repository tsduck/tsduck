# Windows PowerShell wrapper to cleanup.py.

param([switch]$NoPause = $false)
. $PSScriptRoot\cleanup.py
if (-not $NoPause) {
    pause
}
