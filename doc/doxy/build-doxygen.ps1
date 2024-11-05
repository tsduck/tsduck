# Windows PowerShell wrapper to build-doxygen.py.

param([switch]$NoPause = $false)
python $PSScriptRoot\build-doxygen.py
if (-not $NoPause) {
    pause
}
