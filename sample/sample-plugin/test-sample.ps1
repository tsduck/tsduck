# PowerShell script to test the sample plugin on Windows.

# Directory of this script.
$dir = $PSScriptRoot

# We assume that the plugin DLL has been copied into the same
# directory as the script (see build.ps1 for instance).
$env:TSPLUGINS_PATH = "${dir};${env:TSPLUGINS_PATH}"

# Test the sample plugin.
tsp -v -I null -P until --packet 200 -P sample --count -O drop

# WARNING: tsp writes its log on stderr which makes PowerShell interpret
# this log as error. So, you have to ignore the extra error messages
# from PowerShell, claiming that tsp generated errors.
