#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Adjust the compilation performances on Windows with Visual Studio or MSBuild.

  For the compiler, linker, and msbuild executables:
  - Set the compilation priority to "below normal".
  - Disable Windows power throttling.

  About power throttling: On some recent Intel chips such as the Alder Lake
  family, there are two types of CPU cores: P-cores and E-cores. P-cores are
  designed for performance and E-cores for power efficiency. As a rule of
  thumb, "foreground" processes, typically manually run with a window on
  screen, run on P-core while "background" processes run on E-cores.

  When a compilation process is started from Visual Studio, it is considered
  as interactive and runs on P-cores. However, when the compilation is started
  from a script which invokes MSBuild, it is considered as background and the
  compiler runs on E-cores. The net result is that you have a powerful machine
  where all the performance cores do nothing and all compilations are
  struggling on slower efficiency cores.

  The solution: Disable "power throttling" on compiler and linker executables.
  Thus, all these executables won't be candidate for E-cores and will run
  on P-cores.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.

#>
param([switch]$NoPause = $false)

# Executables to alter:
$Execs = @("cl.exe", "link.exe", "msbuild.exe")

# Registry key for image options:
$RegExeOpts = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options"

# Do we run as administrator?
$IsAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $IsAdmin) {
    Write-Output "Must be administrator to continue, trying to restart as administrator ..."
    $cmd = "& '" + $PSCommandPath + "'" + $(if ($NoPause) {" -NoPause"} else {""})
    Start-Process -Verb runas -FilePath PowerShell.exe -ArgumentList @("-ExecutionPolicy", "RemoteSigned", "-Command", $cmd)
}
else {
    # Execution as administrator, we can proceed.

    # Set low priority on executables.
    # CpuPriorityClass: 5 ("Below Normal")
    # IoPriority: 1 ("Low")
    $Execs | ForEach-Object {
        if (-not (Test-Path "$RegExeOpts\$_")) {
            [void](New-Item "$RegExeOpts\$_")
        }
        if (-not (Test-Path "$RegExeOpts\$_\PerfOptions")) {
            [void](New-Item "$RegExeOpts\$_\PerfOptions")
        }
        Set-ItemProperty -Path "$RegExeOpts\$_\PerfOptions" -Name "CpuPriorityClass" -Value 5
        Set-ItemProperty -Path "$RegExeOpts\$_\PerfOptions" -Name "IoPriority" -Value 1
    }

    # Disable power throttling.
    Get-ChildItem -Path @("C:\Program Files*\MSBuild*", "C:\Program Files*\*Visual Studio*") -Recurse -Include $Execs | ForEach-Object {
        powercfg /powerthrottling disable /path $_.FullName
    }
    powercfg /powerthrottling list
}

if (-not $NoPause) {
    pause
}
