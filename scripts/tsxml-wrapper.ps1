#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------
#
#  This PowerShell script is a wrapper which calls tsxml and passes all its
#  parameters. It is essentially called in the context of MSBuild or Visual
#  Studio to build tsduck.tables.model.xml.
#
#  Description of the bootstrap problem: The file tsduck.tables.model.xml is
#  built using tsxml. So, in the build process, tsxml shall be built before
#  tsduck.tables.model.xml. For a native build, this is sufficient. As a
#  worst case, the installed version in $TSDUCK is used.
#
#  However, there are several problems:
#
#  1. TSDuck is not already installed everywhere (CI systems for instance).
#
#  2. When building on x86/x64, for Arm64 target, the tsxml.exe which is
#     built cannot run on x86/x64 to build tsduck.tables.model.xml. So, we
#     cannot always use the tsxml that was just built and we need to look for
#     one that can run on the build-system.
#
#  3. In the MSBuild XML files, it is possible to test the availability of
#     various versions of tsxml, looking for one which can run on the build
#     system. However, the MSBuild XML files are evaluated at the beginning
#     of the build process. At that moment, no tsxml.exe can be found but
#     there will be one later to build tsduck.tables.model.xml.
#
#  4. Arm64 systems can natively run Arm64 executables or emulate Intel
#     executables. However, the opposite is not true. Intel systems can only
#     run native Intel executables but not emulate Arm64 executables. The
#     capabilities are:
#     - on x86 => run x86 only
#     - on x64 => run x64 or x86
#     - on Arm64 => run Arm64, emulate x64 or x86
#
#  Therefore, the solution is too dynamically look for a suitable versions of
#  tsxml at the time tsduck.tables.model.xml. This cannot be done with the
#  MSBuild syntax. Instead, MSBuild calls the present script and the script
#  searches for tsxml at that time.
#
#-----------------------------------------------------------------------------

# PowerShell execution policy.
Set-StrictMode -Version 3
if (((Get-ExecutionPolicy) -ne "Unrestricted") -and ((Get-ExecutionPolicy) -ne "RemoteSigned")) {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force -ErrorAction:SilentlyContinue
}
Import-Module -Force -Name "${PSScriptRoot}\tsbuild.psm1"

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)

# Get the native $(Platform) name as used in MSBuild files.
if ($env:PROCESSOR_ARCHITECTURE -like 'arm64*') {
    $Platform = "ARM64"
}
elseif ($env:PROCESSOR_ARCHITECTURE -like 'amd64*') {
    $Platform = "x64"
}
elseif ($env:PROCESSOR_ARCHITECTURE -like 'x86*') {
    $Platform = "Win32"
}
else {
    Write-Error "Unknown platform for $env:PROCESSOR_ARCHITECTURE"
    exit 1
}

# Platform execution compatibility matrix, in order of preference.
$Compat = @{
    "Win32" = @("Win32");
    "x64"   = @("x64", "Win32");
    "Arm64" = @("Arm64", "x64", "Win32")
}

# Look for a tsxml.exe we can run.
$TSXML = ""
foreach ($pf in $Compat[$Platform]) {
    if (Test-Path "$RootDir\bin\Release-$pf\tsxml.exe") {
        $TSXML = "$RootDir\bin\Release-$pf\tsxml.exe"
    }
    elseif (Test-Path "$RootDir\bin\Debug-$pf\tsxml.exe") {
        $TSXML = "$RootDir\bin\Debug-$pf\tsxml.exe"
    }
    if ($TSXML -ne "") {
        break
    }

}

# Look for an installed one.
if ($TSXML -eq "" -and $TSDUCK -ne "" -and (Test-Path "$TSDUCK\bin\tsxml.exe")) {
    $TSXML = "$TSDUCK\bin\tsxml.exe"
}

# If none found, rebuild for the native platform (will take time...)
if ($TSXML -eq "") {
    $MSBuild = (Find-MSBuild)
    $SolutionFileName = "${PSScriptRoot}\msvc\tsduck.sln"
    & $MSBuild $SolutionFileName /nologo /maxcpucount /property:Configuration=Release /property:Platform=$Platform /target:tsxml
    $TSXML = "$RootDir\bin\Release-$Platform\tsxml.exe"
}

# Finally, invoke the right tsxml with all parameters from this script.
Write-Output "Using tsxml: $TSXML"
& $TSXML @args
