# PowerShell script to build the sample plugin on Windows.

# Directory of this script.
$dir = $PSScriptRoot

# MSBuild from Visual Studio 2017. Adapt to your environment.
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\amd64\MSBuild.exe"

# Build the project for Release x64.
& $msbuild $dir\sample-plugin.sln /nologo /maxcpucount /property:Configuration=Release /property:Platform=x64

# Copy the plugin DLL to the project directory.
# Not always a good idea, this is just for test.
Copy-Item $dir\x64\Release\tsplugin_sample.dll $dir
