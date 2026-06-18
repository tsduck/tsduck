#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#-----------------------------------------------------------------------------


<#
 .SYNOPSIS

  Cleanup temporary keys for TLS servers in TSDuck applications.

 .DESCRIPTION

  Some TSDuck commands create secured TCP servers using SSL/TLS. A server
  needs a private key and a certificate. When using ephemeral certificates
  (using option --ephemeral-rsa-bits for instance), the private key cannot
  simply stay in memory. For some obscure reason, Windows SChannel requires
  that the key is persistent. In that case, when the server terminates, the
  temporary keys are automatically deleted. However, if the application is
  prematurely killed or interrupted, the automatic cleanup cannot be
  performed and a spurious file remains on disk. This script finds and
  deletes all leftover key files.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
param([switch]$NoPause = $false)

$Pattern = "ts-temp-key-"
$KeyDir = "$($env:AppData)\Microsoft\Crypto\Keys"

certutil -user -key -csp "Microsoft Software Key Storage Provider" |
    Select-String "^ *$Pattern" -CaseSensitive -Context 0,1 |
    Out-String -Stream |
    ForEach-Object { $_ -replace ' ','' }  |
    Select-String -NotMatch "$Pattern",'^$' |
    ForEach-Object {
        $KeyFile = "$KeyDir\$_"
        if (Test-Path $KeyFile) {
            Write-Output "Deleting $KeyFile"
            Remove-Item -Path $KeyFile -Force
        }
        else {
            Write-Output "File not found: $KeyFile"
        }
    }

if (-not $NoPause) {
    pause
}
