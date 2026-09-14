#-----------------------------------------------------------------------------
#
#  Copyright (c) 2022, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file
#
#  Download and install Asciidoctor.
#  See parameters documentation in install-common.ps1.
#
#-----------------------------------------------------------------------------

[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [string]$Destination = "",
    [switch]$ForceDownload = $false,
    [switch]$GitHubActions = $false,
    [switch]$NoInstall = $false,
    [switch]$NoPause = $false
)

Write-Output "==== Asciidoctor download and installation procedure"

. "$PSScriptRoot\install-common.ps1"

# Need Ruby 4 at least. The installation of "rouge" requires module strscan 3.1.8.
# With Ruby 3, an older version is embedded and the installation of "rouge" needs
# to compile strscan-3.1.8. Since the Ruby development kit is likely not installed,
# the compilation of strscan fails. We also need the Ruby gem command.

if (-not (Search-Command "ruby") -or -not (Search-Command "gem")) {
    $InstallRuby = $true
}
else {
    $InstallRuby = [int](ruby -e "print RUBY_VERSION.to_i") -lt 4
}
if ($InstallRuby) {
    & "$PSScriptRoot\install-ruby.ps1" -NoPause -Destination:$Destination -ForceDownload:$ForceDownload -GitHubActions:$GitHubActions
    # Get ruby installation path and make it visible in the Path.
    $rubydir = (get-environment "Path") -split ';' | `
        Select-String "ruby" | `
        Select-String -NotMatch "ruby1", "ruby2", "ruby3" | `
        Select-Object -First 1
    if ($rubydir -ne $null) {
        $env:Path = "$rubydir;${env:Path}"
    }
}

ruby --version
gem install asciidoctor asciidoctor-pdf rouge

Exit-Script
