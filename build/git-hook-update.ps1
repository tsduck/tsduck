#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Update the Git hooks when required.

 .DESCRIPTION

  A few standard Git hooks are established to manage the "commit count"
  which is used to identify the product version.

  The actual code to execute is in build\git-hook.sh. This script is executed
  on Windows using the so-called "Git Bash" (in practice, a trimmed version
  of Msys). The hook name is passed as parameter.

  The template for a hook script is:

    #!/bin/bash
    exec $GIT_DIR/../build/git-hook.sh <hook-name>

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
param([switch]$NoPause = $false)

# List of Git hooks to set.
$GitLooksList = @("pre-commit", "post-merge")

# Git hooks are in .git/hooks
$GitHooksDir = (Join-Path (Join-Path (Split-Path -Parent $PSScriptRoot) .git) hooks)

# Check each hook for the line to execute.
$GitLooksList | ForEach-Object {
    $name = $_
    $file = (Join-Path $GitHooksDir $name)
    $line = "exec `$(dirname `$0)/../../build/git-hook.sh $name"
    $fileOK = (Test-Path $file)
    if (-not $fileOK -or -not (Select-String -Quiet -Path $file -SimpleMatch $line)) {
        Write-Output "  [GIT] updating $name hook"
        # We use IO.File methods to enforce LF as end of line.
        if (-not $fileOK) {
            [IO.File]::WriteAllText($file, "#!/bin/bash`n")
        }
        [IO.File]::AppendAllText($file, "$line`n")
    }
}

# Exit script.
if (-not $NoPause) {
    pause
}
