#!/bin/bash
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
#
#  This script builds the various project files for the TSDuck library.
#  This script is useful when source files are added to or removed from the
#  directory src\libtsduck.
#
#  The following files are rebuilt:
#
#  - build/msvc2017/libtsduck-files.props
#  - build/msvc2017/libtsduck-filters.props
#  - build/qtcreator/libtsduck/libtsduck-files.pri
#  - src/libtsduck/tsduck.h
#  - src/libtsduck/tsTables.h
#  - src/libtsduck/private/tsRefType.h
#
#  See the PowerShell script Build-Project-Files.ps1 for a Windows equivalent.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename ${BASH_SOURCE[0]} .sh)

error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Optional file to build.
TARGET=$(basename "$1")

# Get the project directories.
BUILDDIR=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)
ROOTDIR=$(cd "$BUILDDIR/.."; pwd)
SRCDIR="$ROOTDIR/src/libtsduck"

# Relative path from Visual Studio project files to libtsduck source directory.
MS_RELPATH="..\\..\\src\\libtsduck\\"
QT_RELPATH="../../../src/libtsduck/"

# Get location of Visual Studio and project files.
MSVCDIR=$(ls -d "$BUILDDIR"/msvc* | sort | tail -1)
[[ -z "$MSVCDIR" ]] && error "MSVC project directory not found"

# find(1) option to limit the search to one level.
[[ $(uname -s) == Linux ]] && FIND1="-maxdepth 1" || FIND1="-depth 1"

# Enforce LANG to get the same sort order as "Sort-Object -Culture en-US" in PowerShell
export LANG=C
export LC_ALL=$LANG

# Embedded newline character for variables.
NL=$'\n'

# Get all libtsduck files by type.
GetSources()
{
    local type="$1"; shift
    local subdir="$1"; shift

    for f in $(find "$SRCDIR/$subdir" $FIND1 -type f -name "*.$type" "$@" | sort --ignore-case); do
        echo "${PREFIX}$(basename $f)${SUFFIX}"
    done
}

# Generate includes based on doxygen group.
GetGroup()
{
    local group="$1"; shift
    local subdir="$1"; shift
    local name=""

    for f in $(grep -l "@ingroup *$group" "$SRCDIR/$subdir/ts"*.h | grep -v -e /tsAbstract -e /tsVCT | sort --ignore-case); do
        name=$(basename $f .h | sed -e 's/^ts//')
        echo "${PREFIX}${name}${SUFFIX}"
    done
}

# Generate the MS project file.
GenerateMSProject()
{
    echo '<?xml version="1.0" encoding="utf-8"?>'
    echo '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
    echo '  <ItemGroup>'
    PREFIX="    <ClInclude Include=\"${MS_RELPATH}"
    SUFFIX="\" />"
    GetSources h "" ! -name "tsStaticReferences*"
    PREFIX="    <ClInclude Include=\"${MS_RELPATH}private\\"
    GetSources h private
    PREFIX="    <ClInclude Include=\"${MS_RELPATH}windows\\"
    GetSources h windows
    echo '  </ItemGroup>'
    echo '  <ItemGroup>'
    PREFIX="    <ClCompile Include=\"${MS_RELPATH}"
    GetSources cpp "" ! -name "tsStaticReferences*"
    PREFIX="    <ClCompile Include=\"${MS_RELPATH}private\\"
    GetSources cpp private
    PREFIX="    <ClCompile Include=\"${MS_RELPATH}windows\\"
    GetSources cpp windows
    echo '  </ItemGroup>'
    echo '</Project>'
}

# Generate the MS filters file.
GenerateMSFilters()
{
    echo '<?xml version="1.0" encoding="utf-8"?>'
    echo '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
    echo '  <ItemGroup>'
    PREFIX="    <ClInclude Include=\"${MS_RELPATH}"
    SUFFIX="\">${NL}      <Filter>Header Files</Filter>${NL}    </ClInclude>"
    GetSources h "" ! -name "tsStaticReferences*"
    PREFIX="    <ClInclude Include=\"${MS_RELPATH}private\\"
    GetSources h private
    PREFIX="    <ClInclude Include=\"${MS_RELPATH}windows\\"
    GetSources h windows
    echo '  </ItemGroup>'
    echo '  <ItemGroup>'
    PREFIX="    <ClCompile Include=\"${MS_RELPATH}"
    SUFFIX="\">${NL}      <Filter>Source Files</Filter>${NL}    </ClCompile>"
    GetSources cpp "" ! -name "tsStaticReferences*"
    PREFIX="    <ClCompile Include=\"${MS_RELPATH}private\\"
    GetSources cpp private
    PREFIX="    <ClCompile Include=\"${MS_RELPATH}windows\\"
    GetSources cpp windows
    echo '  </ItemGroup>'
    echo '</Project>'
}

# Generate the Qt Creator project file.
GenerateQtProject()
{
    echo 'HEADERS += \'
    PREFIX="    ${QT_RELPATH}"
    SUFFIX=" \\"
    GetSources h "" ! -name "tsStaticReferences*"
    PREFIX="    ${QT_RELPATH}private/"
    GetSources h private
    echo ''
    echo 'SOURCES += \'
    PREFIX="    ${QT_RELPATH}"
    GetSources cpp "" ! -name "tsStaticReferences*"
    PREFIX="    ${QT_RELPATH}private/"
    GetSources cpp private
    PREFIX="        ${QT_RELPATH}"
    echo ''
    echo 'linux {'
    echo '    HEADERS += \'
    PREFIX="        ${QT_RELPATH}unix/"
    GetSources h unix
    PREFIX="        ${QT_RELPATH}linux/"
    GetSources h linux
    echo ''
    echo '    SOURCES += \'
    PREFIX="        ${QT_RELPATH}unix/"
    GetSources cpp unix
    PREFIX="        ${QT_RELPATH}linux/"
    GetSources cpp linux
    echo ''
    echo '}'
    echo ''
    echo 'mac {'
    echo '    HEADERS += \'
    PREFIX="        ${QT_RELPATH}unix/"
    GetSources h unix
    PREFIX="        ${QT_RELPATH}mac/"
    GetSources h mac
    echo ''
    echo '    SOURCES += \'
    PREFIX="        ${QT_RELPATH}unix/"
    GetSources cpp unix
    PREFIX="        ${QT_RELPATH}mac/"
    GetSources cpp mac
    echo ''
    echo '}'
    echo ''
    echo 'win32|win64 {'
    echo '    HEADERS += \'
    PREFIX="        ${QT_RELPATH}windows/"
    GetSources h windows
    echo ''
    echo '    SOURCES += \'
    GetSources cpp windows
    echo ''
    echo '}'
}

# Generate the main TSDuck header file.
GenerateMainHeader()
{
    PREFIX="#include \""
    SUFFIX="\""

    cat "$ROOTDIR/src/HEADER.txt"
    echo ''
    echo '#pragma once'
    GetSources h "" ! -name "tsStaticReferences*" ! -name "*Template.h" ! -name "tsduck.h"
    echo ''
    echo '#if defined(TS_LINUX)'
    GetSources h unix ! -name "*Template.h"
    GetSources h linux ! -name "*Template.h"
    echo '#endif'
    echo ''
    echo '#if defined(TS_MAC)'
    GetSources h unix ! -name "*Template.h"
    GetSources h mac ! -name "*Template.h"
    echo '#endif'
    echo ''
    echo '#if defined(TS_WINDOWS)'
    GetSources h windows ! -name "*Template.h"
    echo '#endif'
}

# Generate the header file for PSI/SI tables and descriptors.
GenerateTablesHeader()
{
    PREFIX="#include \"ts"
    SUFFIX=".h\""

    cat "$ROOTDIR/src/HEADER.txt"
    echo ''
    echo '//! @file'
    echo '//! All headers for MPEG/DVB tables and descriptors.'
    echo ''
    echo '#pragma once'
    echo ''
    GetGroup table
    echo ''
    GetGroup descriptor
}

# Generate the header file containing static references to all tables and descriptors.
GenerateRefType()
{
    PREFIX="    REF_TYPE("
    SUFFIX=");"

    GetGroup table
    echo ''
    GetGroup descriptor
}

# Generate the files.
[[ -z "$TARGET" || "$TARGET" == "libtsduck-files.props"   ]] && GenerateMSProject | unix2dos >"$MSVCDIR/libtsduck-files.props"
[[ -z "$TARGET" || "$TARGET" == "libtsduck-filters.props" ]] && GenerateMSFilters | unix2dos >"$MSVCDIR/libtsduck-filters.props"
[[ -z "$TARGET" || "$TARGET" == "libtsduck-files.pri"     ]] && GenerateQtProject  >"$ROOTDIR/build/qtcreator/libtsduck/libtsduck-files.pri"
[[ -z "$TARGET" || "$TARGET" == "tsduck.h"                ]] && GenerateMainHeader >"$SRCDIR/tsduck.h"
[[ -z "$TARGET" || "$TARGET" == "tsTables.h"              ]] && GenerateTablesHeader >"$SRCDIR/tsTables.h"
[[ -z "$TARGET" || "$TARGET" == "tsRefType.h"             ]] && GenerateRefType >"$SRCDIR/private/tsRefType.h"

exit 0
