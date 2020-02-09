#!/bin/bash
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
#
#  This script builds the various project files for the TSDuck library.
#  This script is useful when source files are added to or removed from the
#  directory src\libtsduck.
#
#  The following files are rebuilt:
#
#  - build/qtcreator/libtsduck/libtsduck-files.pri
#  - src/libtsduck/tsduck.h
#  - src/libtsduck/dtv/tsTables.h
#  - src/libtsduck/dtv/private/tsRefType.h
#
#  See the PowerShell script build-project-files.ps1 for a Windows equivalent.
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
MSVCDIR="$BUILDDIR/msvc"

# Relative path from Visual Studio project files to libtsduck source directory.
MS_RELPATH="..\\..\\src\\libtsduck\\"
QT_RELPATH="../../../src/libtsduck/"

# On macOS, make sure that commands which were installed by Homebrew packages are in the path.
[[ $(uname -s) == Darwin ]] && export PATH="$PATH:/usr/local/bin"

# Enforce LANG to get the same sort order as "Sort-Object -Culture en-US" in PowerShell
export LANG=C
export LC_ALL=$LANG

# Embedded newline character for variables.
NL=$'\n'

# Get all libtsduck files by type.
# Syntax: GetSources type unix|windows|none [additional-find-arguments]
GetSources()
{
    local type="$1"; shift
    local mode="$1"; shift

    find "$SRCDIR" -type f -name "*.$type" "$@" |
        (
            case $mode in
                unix)    sed -e "s|$SRCDIR/||" ;;
                windows) sed -e "s|$SRCDIR/||" -e 's|/|\\|g' ;;
                none)    sed -e "s|$SRCDIR/||" -e 's|.*/||' ;;
                *)       cat ;;
            esac
        ) |
        sort --ignore-case |
        sed -e "s|^|${PREFIX}|" -e "s|\$|${SUFFIX}|"
}

# Generate includes based on doxygen group name (as in "@ingroup name").
# Syntax: GetGroup name
GetGroup()
{
    local group="$1"; shift
    local name=""

    find "$SRCDIR" -name 'ts*.h' ! -path '*/tsAbstract*.h' ! -name tsVCT.h |
        xargs grep -l "@ingroup *$group" |
        sed -e 's|^.*/ts\(.*\)\.h|\1|' |
        sort --ignore-case |
        sed -e "s|^|${PREFIX}|" -e "s|\$|${SUFFIX}|"
}

# Generate the main TSDuck header file.
GenerateMainHeader()
{
    PREFIX="#include \""
    SUFFIX="\""

    cat "$ROOTDIR/src/HEADER.txt"
    echo ''
    echo '#pragma once'
    GetSources h none \
        ! -path '*/linux/*' ! -path '*/mac/*' ! -path '*/unix/*' ! -path '*/windows/*' ! -path '*/private/*' \
        ! -name "tsStaticReferences*" ! -name "*Template.h" ! -name "tsduck.h"
    echo ''
    echo '#if defined(TS_LINUX)'
    GetSources h none \( -path '*/unix/*' -o -path '*/linux/*' \) ! -name "*Template.h"
    echo '#endif'
    echo ''
    echo '#if defined(TS_MAC)'
    GetSources h none \( -path '*/unix/*' -o -path '*/mac/*' \) ! -name "*Template.h"
    echo '#endif'
    echo ''
    echo '#if defined(TS_WINDOWS)'
    GetSources h none -path '*/windows/*' ! -name "*Template.h"
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
[[ -z "$TARGET" || "$TARGET" == "tsduck.h"    ]] && GenerateMainHeader >"$SRCDIR/tsduck.h"
[[ -z "$TARGET" || "$TARGET" == "tsTables.h"  ]] && GenerateTablesHeader >"$SRCDIR/dtv/tsTables.h"
[[ -z "$TARGET" || "$TARGET" == "tsRefType.h" ]] && GenerateRefType >"$SRCDIR/dtv/private/tsRefType.h"

exit 0
