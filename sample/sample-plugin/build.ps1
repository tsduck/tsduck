# PowerShell script to build the sample plugin on Windows.

# Directory of this script.
$dir = $PSScriptRoot

# Add various locations for MSBuild.exe, depending on your version of Visual Studio
$env:Path = $env:Path + `
    ';C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64' + `
    ';C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin' + `
    ';C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\amd64' + `
    ';C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin' + `
    ';C:\Program Files (x86)\MSBuild\14.0\Bin\amd64' + `
    ';C:\Program Files (x86)\MSBuild\14.0\Bin'

# Build the project for Release x64.
MSBuild $dir\sample-plugin.sln /nologo /maxcpucount /property:Configuration=Release /property:Platform=x64

# Copy the plugin DLL to the project directory.
# Not always a good idea, this is just for test.
Copy-Item $dir\x64\Release\tsplugin_sample.dll $dir
