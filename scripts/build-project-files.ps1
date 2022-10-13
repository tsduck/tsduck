# Windows PowerShell wrapper to build-project-files.py.

param([switch]$NoPause = $false)
python $PSScriptRoot\build-project-files.py
if (-not $NoPause) {
    pause
}
