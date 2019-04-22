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

  - build/msvc2017/libtsduck-files.props
  - build/msvc2017/libtsduck-filters.props
  - build/qtcreator/libtsduck/libtsduck-files.pri
  - src/libtsduck/tsduck.h
  - src/libtsduck/tsTables.h
  - src/libtsduck/private/tsRefType.h

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
if (((Get-ExecutionPolicy) -ne "Unrestricted") -and ((Get-ExecutionPolicy) -ne "RemoteSigned")) {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force -ErrorAction:SilentlyContinue
}
Import-Module -Force -Name (Join-Path $PSScriptRoot Build-Common.psm1)

# Get the project directories.
$RootDir = (Split-Path -Parent $PSScriptRoot)
$SrcDir = "$RootDir\src\libtsduck"

# Relative path from Visual Studio project files to libtsduck source directory.
$MsSrcRelPath = "..\..\src\libtsduck\"
$QtSrcRelPath = "../../../src/libtsduck/"

# Get location of Visual Studio and project files.
$VS = Search-VisualStudio
$MsvcDir = $VS.MsvcDir

# Get all libtsduck files by type.
function GetSources([string]$type, [string]$subdir = "", [string]$prefix = "", [string]$suffix, $exclude = "")
{
    Get-ChildItem "$SrcDir\$subdir\*" -Include "*.$type" -Exclude $exclude | `
        ForEach-Object {$prefix + $_.Name + $suffix} | `
        Sort-Object -Culture en-US
}

# Get files based on doxygen group.
function GetGroup([string]$group, [string]$subdir = "", [string]$prefix = "", [string]$suffix)
{
    Get-ChildItem "$SrcDir\$subdir\*.h" -Exclude @("tsAbstract*.h", "tsVCT.h") | `
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
    GetSources h "" $prefix $suffix @("tsStaticReferences*")
    GetSources h private "${prefix}private\" $suffix
    GetSources h windows "${prefix}windows\" $suffix
    echo '  </ItemGroup>'
    echo '  <ItemGroup>'
    $prefix = "    <ClCompile Include=`"$MsSrcRelPath"
    $suffix = "`" />"
    GetSources cpp "" $prefix $suffix @("tsStaticReferences*")
    GetSources cpp private "${prefix}private\" $suffix
    GetSources cpp windows "${prefix}windows\" $suffix
    echo '  </ItemGroup>'
    echo '</Project>'
}

# Generate the MS filters file.
function GenerateMSFilters()
{
    echo '<?xml version="1.0" encoding="utf-8"?>'
    echo '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
    echo '  <ItemGroup>'
    $prefix = "    <ClInclude Include=`"$MsSrcRelPath"
    $suffix = "`">`n      <Filter>Header Files</Filter>`n    </ClInclude>"
    GetSources h "" $prefix $suffix @("tsStaticReferences*")
    GetSources h private "${prefix}private\" $suffix
    GetSources h windows "${prefix}windows\" $suffix
    echo '  </ItemGroup>'
    echo '  <ItemGroup>'
    $prefix = "    <ClCompile Include=`"$MsSrcRelPath"
    $suffix = "`">`n      <Filter>Source Files</Filter>`n    </ClCompile>"
    GetSources cpp "" $prefix $suffix @("tsStaticReferences*")
    GetSources cpp private "${prefix}private\" $suffix
    GetSources cpp windows "${prefix}windows\" $suffix
    echo '  </ItemGroup>'
    echo '</Project>'
}

# Generate the Qt Creator project file.
function GenerateQtProject()
{
    echo 'HEADERS += \'
    $prefix = "    $QtSrcRelPath"
    $suffix = " \"
    GetSources h "" $prefix $suffix @("tsStaticReferences*")
    GetSources h private "${prefix}private/" $suffix
    echo ''
    echo 'SOURCES += \'
    GetSources cpp "" $prefix $suffix @("tsStaticReferences*")
    GetSources cpp private "${prefix}private/" $suffix
    $prefix = "    $prefix"
    echo ''
    echo 'linux {'
    echo '    HEADERS += \'
    GetSources h unix "${prefix}unix/" $suffix
    GetSources h linux "${prefix}linux/" $suffix
    echo ''
    echo '    SOURCES += \'
    GetSources cpp unix "${prefix}unix/" $suffix
    GetSources cpp linux "${prefix}linux/" $suffix
    echo ''
    echo '}'
    echo ''
    echo 'mac {'
    echo '    HEADERS += \'
    GetSources h unix "${prefix}unix/" $suffix
    GetSources h mac "${prefix}mac/" $suffix
    echo ''
    echo '    SOURCES += \'
    GetSources cpp unix "${prefix}unix/" $suffix
    GetSources cpp mac "${prefix}mac/" $suffix
    echo ''
    echo '}'
    echo ''
    echo 'win32|win64 {'
    echo '    HEADERS += \'
    GetSources h windows "${prefix}windows/" $suffix
    echo ''
    echo '    SOURCES += \'
    GetSources cpp windows "${prefix}windows/" $suffix
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
    GetSources h "" $prefix $suffix @("tsStaticReferences*", "*Template.h", "tsduck.h")
    echo ''
    echo '#if defined(TS_LINUX)'
    GetSources h unix $prefix $suffix @("*Template.h")
    GetSources h linux $prefix $suffix @("*Template.h")
    echo '#endif'
    echo ''
    echo '#if defined(TS_MAC)'
    GetSources h unix $prefix $suffix $suffix @("*Template.h")
    GetSources h mac $prefix $suffix @("*Template.h")
    echo '#endif'
    echo ''
    echo '#if defined(TS_WINDOWS)'
    GetSources h windows $prefix $suffix @("*Template.h")
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
    GetGroup table "" $prefix $suffix
    echo ''
    GetGroup descriptor "" $prefix $suffix
}

# Generate the header file containing static references to all tables and descriptors.
function GenerateRefType()
{
    $prefix = "    REF_TYPE("
    $suffix = ");"

    GetGroup table "" $prefix $suffix
    echo ''
    GetGroup descriptor "" $prefix $suffix
}

# Generate the files.
GenerateMSProject    | Out-File -Encoding ascii $MsvcDir\libtsduck-files.props
GenerateMSFilters    | Out-File -Encoding ascii $MsvcDir\libtsduck-filters.props
GenerateQtProject    | Out-File -Encoding ascii $RootDir\build\qtcreator\libtsduck\libtsduck-files.pri
GenerateMainHeader   | Out-File -Encoding ascii $SrcDir\tsduck.h
GenerateTablesHeader | Out-File -Encoding ascii $SrcDir\tsTables.h
GenerateRefType      | Out-File -Encoding ascii $SrcDir\private\tsRefType.h

Exit-Script -NoPause:$NoPause
