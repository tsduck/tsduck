#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2019, Thierry Lelegard
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

  Build the various project files for the TSDuck library.

 .DESCRIPTION

  This script is useful when source files are added to or removed from the
  directory src\libtsduck.

  The following files are rebuilt:

  - build/msvc/libtsduck-files.props
  - build/qtcreator/libtsduck/libtsduck-files.pri
  - src/libtsduck/tsduck.h
  - src/libtsduck/dtv/tsTables.h
  - src/libtsduck/dtv/private/tsRefType.h

  See the shell script build-project-files.sh for a Unix equivalent.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
[CmdletBinding()]
param([switch]$NoPause = $false)

# PowerShell execution policy.
Set-StrictMode -Version 3

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$SrcDir = "$RootDir\src\libtsduck"
$MsvcDir = "$PSScriptRoot\msvc"

# Relative path from Visual Studio project files to libtsduck source directory.
$MsSrcRelPath = "..\..\src\libtsduck\"
$QtSrcRelPath = "../../../src/libtsduck/"

# Get all libtsduck files by type.
function GetSources()
{
    [CmdletBinding()]
    param([string]$Type, [switch]$Unix = $false, [switch]$Windows = $false, [string]$Prefix = "", [string]$Suffix, [string[]]$Include = @(), [string[]]$Exclude = @())

    Get-ChildItem "$SrcDir" -Recurse -Include "*.$Type" | `
        Where-Object {
            $tmpname = $_.FullName
            ($Include.Count -eq 0) -or (($Include | ForEach-Object { $tmpname -like $_ }) -contains $true)
        } | `
        Where-Object {
            $tmpname = $_.FullName
            ($Exclude.Count -eq 0) -or -not (($Exclude | ForEach-Object { $tmpname -like $_ }) -contains $true)
        } | `
        ForEach-Object {
            if ($Unix) {
                $name = $_.FullName.Replace("$SrcDir\","") -replace "\\","/"
            }
            elseif ($Windows) {
                $name = $_.FullName.Replace("$SrcDir\","")
            }
            else {
                $name = $_.Name
            }
            $Prefix + $name + $Suffix
        } | `
        Sort-Object -Culture en-US
}

# Get files based on doxygen group.
function GetGroup()
{
    [CmdletBinding()]
    param([string]$Group, [string]$Prefix = "", [string]$Suffix)

    Get-ChildItem "$SrcDir" -Recurse -Include "*.h" -Exclude @("tsAbstract*.h", "tsVCT.h") | `
        Select-String -Pattern "@ingroup *$group" | `
        Select-Object -Unique Filename | `
        ForEach-Object {$prefix + ($_.Filename -replace '^ts','' -replace '\.h$','') + $suffix} | `
        Sort-Object -Culture en-US
}

# Generate the MS project file.
function GenerateMSProject()
{
    echo '<?xml version="1.0" encoding="utf-8"?>'
    echo '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
    echo '  <ItemGroup>'
    $prefix = "    <ClInclude Include=`"$MsSrcRelPath"
    $suffix = "`" />"
    GetSources -Type h -Windows -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*", "*\unix\*", "*\linux\*", "*\mac\*")
    echo '  </ItemGroup>'
    echo '  <ItemGroup>'
    $prefix = "    <ClCompile Include=`"$MsSrcRelPath"
    $suffix = "`" />"
    GetSources -Type cpp -Windows -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*", "*\unix\*", "*\linux\*", "*\mac\*")
    echo '  </ItemGroup>'
    echo '</Project>'
}

# Generate the Qt Creator project file.
function GenerateQtProject()
{
    echo 'HEADERS += \'
    $prefix = "    $QtSrcRelPath"
    $suffix = " \"
    GetSources -Type h -Unix -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*", "*\unix\*", "*\linux\*", "*\mac\*", "*\windows\*")
    echo ''
    echo 'SOURCES += \'
    GetSources -Type cpp -Unix -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*", "*\unix\*", "*\linux\*", "*\mac\*", "*\windows\*")
    $prefix = "    $prefix"
    echo ''
    echo 'linux {'
    echo '    HEADERS += \'
    GetSources -Type h -Unix -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*") -Include @("*\unix\*", "*\linux\*")
    echo ''
    echo '    SOURCES += \'
    GetSources -Type cpp -Unix -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*") -Include @("*\unix\*", "*\linux\*")
    echo ''
    echo '}'
    echo ''
    echo 'mac {'
    echo '    HEADERS += \'
    GetSources -Type h -Unix -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*") -Include @("*\unix\*", "*\mac\*")
    echo ''
    echo '    SOURCES += \'
    GetSources -Type cpp -Unix -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*") -Include @("*\unix\*", "*\mac\*")
    echo ''
    echo '}'
    echo ''
    echo 'win32|win64 {'
    echo '    HEADERS += \'
    GetSources -Type h -Unix -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*") -Include @("*\windows\*")
    echo ''
    echo '    SOURCES += \'
    GetSources -Type cpp -Unix -Prefix $prefix -Suffix $suffix -Exclude @("*\tsStaticReferences*") -Include @("*\windows\*")
    echo ''
    echo '}'
}

# Generate the main TSDuck header file.
function GenerateMainHeader()
{
    $prefix = "#include `""
    $suffix = "`""

    Get-Content $SrcDir\..\HEADER.txt
    echo ''
    echo '#pragma once'
    GetSources -Type h -Prefix $prefix -Suffix $suffix `
        -Exclude @("*\tsStaticReferences*", "*Template.h", "*\tsduck.h", "*\unix\*", "*\linux\*", "*\mac\*", "*\windows\*", "*\private\*")
    echo ''
    echo '#if defined(TS_LINUX)'
    GetSources -Type h -Prefix $prefix -Suffix $suffix -Exclude @("*Template.h") -Include @("*\unix\*", "*\linux\*")
    echo '#endif'
    echo ''
    echo '#if defined(TS_MAC)'
    GetSources -Type h -Prefix $prefix -Suffix $suffix -Exclude @("*Template.h") -Include @("*\unix\*", "*\mac\*")
    echo '#endif'
    echo ''
    echo '#if defined(TS_WINDOWS)'
    GetSources -Type h -Prefix $prefix -Suffix $suffix -Exclude @("*Template.h") -Include @("*\windows\*")
    echo '#endif'
}


# Generate the header file for PSI/SI tables and descriptors.
function GenerateTablesHeader()
{
    $prefix = "#include `"ts"
    $suffix = ".h`""

    Get-Content $SrcDir\..\HEADER.txt
    echo ''
    echo '//! @file'
    echo '//! All headers for MPEG/DVB tables and descriptors.'
    echo ''
    echo '#pragma once'
    echo ''
    GetGroup -Group table -Prefix $prefix -Suffix $suffix
    echo ''
    GetGroup -Group descriptor -Prefix $prefix -Suffix $suffix
}

# Generate the header file containing static references to all tables and descriptors.
function GenerateRefType()
{
    $prefix = "    REF_TYPE("
    $suffix = ");"

    GetGroup -Group table -Prefix $prefix -Suffix $suffix
    echo ''
    GetGroup -Group descriptor -Prefix $prefix -Suffix $suffix
}

# Generate the files.
GenerateMSProject    | Out-File -Encoding ascii $MsvcDir\libtsduck-files.props
GenerateQtProject    | Out-File -Encoding ascii $RootDir\build\qtcreator\libtsduck\libtsduck-files.pri
GenerateMainHeader   | Out-File -Encoding ascii $SrcDir\tsduck.h
GenerateTablesHeader | Out-File -Encoding ascii $SrcDir\dtv\tsTables.h
GenerateRefType      | Out-File -Encoding ascii $SrcDir\dtv\private\tsRefType.h

if (-not $NoPause) {
    pause
}
