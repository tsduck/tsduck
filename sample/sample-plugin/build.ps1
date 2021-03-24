# PowerShell script to build the sample plugin on Windows.

# Directory of this script.
$dir = $PSScriptRoot

# Find MSBuild.exe.
$MSBuild = Get-ChildItem -Recurse -Path "C:\Program Files*\Microsoft Visual Studio" -Include MSBuild.exe -ErrorAction Ignore |
           ForEach-Object { (Get-Command $_).FileVersionInfo } |
           Sort-Object -Unique -Property FileVersion |
           ForEach-Object { $_.FileName} |
           Select-Object -Last 1
if (-not $MSBuild) {
    Write-Host "ERROR: MSBuild not found"
    exit
}

# Build the project for Release x64.
& $MSBuild $dir\sample-plugin.sln /nologo /maxcpucount /property:Configuration=Release /property:Platform=x64

# Copy the plugin DLL to the project directory.
# Not always a good idea, this is just for test.
Copy-Item $dir\x64\Release\tsplugin_sample.dll $dir
