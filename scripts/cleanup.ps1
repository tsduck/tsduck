# Windows PowerShell wrapper to cleanup.py.

param([switch]$NoPause = $false)
python $PSScriptRoot\cleanup.py (Split-Path -Parent $PSScriptRoot)
if (-not $NoPause) {
    pause
}
