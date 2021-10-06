# This PowerShell script shall be invoked to make the extension visible to TSDuck directly after compilation.

$root = (Split-Path -Parent $PSScriptRoot)
$bin = (Join-Path (Join-Path $root "msvc") "Release-x64")
$src = (Join-Path $root "src")

if ("$env:TSPLUGINS_PATH" -eq "") {
    $env:TSPLUGINS_PATH = "$bin;$src"
}
if (";$env:TSPLUGINS_PATH;" -notlike "*;$src;*") {
    $env:TSPLUGINS_PATH = "$src;$env:TSPLUGINS_PATH"
}
if (";$env:TSPLUGINS_PATH;" -notlike "*;$bin;*") {
    $env:TSPLUGINS_PATH = "$bin;$env:TSPLUGINS_PATH"
}
if (";$env:Path;" -notlike "*;$bin;*") {
    $env:Path = "$bin;$env:Path"
}
