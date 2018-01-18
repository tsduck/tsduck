#!/bin/bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2018, Thierry Lelegard
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
#-----------------------------------------------------------------------------

SCRIPT=$(basename ${BASH_SOURCE[0]} .sh)

error() { echo >&2 "$SCRIPT: $*"; exit 1; }

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

# Get all libtsduck files by type.
GetSources()
{
    local type="$1"; shift
    local subdir="$1"; shift
    local prefix="$1"; shift
    local suffix="$1"; shift

    for f in $(find "$SRCDIR/$subdir" -depth 1 -type f -name "*.$type" "$@" | sort --ignore-case); do
        echo "$prefix$(basename $f)$suffix"
    done
}

# Generate the MS project file.
GenerateMSProject()
{
    echo '<?xml version="1.0" encoding="utf-8"?>'
#    echo '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
#    echo '  <ItemGroup>'
#    local prefix="    <ClInclude Include=\"$MS_RELPATH"
#    local suffix="\" />"
#    GetSources h "" "$prefix" "$suffix" ! -name "tsStaticReferences*"
#    GetSources h private "${prefix}private\\" "$suffix"
#    GetSources h windows "${prefix}windows\\" "$suffix"
#    echo '  </ItemGroup>'
#    echo '  <ItemGroup>'
#    prefix="    <ClCompile Include=\"$MS_RELPATH"
#    suffix="\" />"
#    GetSources cpp "" "$prefix" "$suffix" ! -name "tsStaticReferences*"
#    GetSources cpp private "${prefix}private\\" "$suffix"
#    GetSources cpp windows "${prefix}windows\\" "$suffix"
#    echo '  </ItemGroup>'
#    echo '</Project>'
}

# Generate the MS filters file.
GenerateMSFilters()
{
    echo '<?xml version="1.0" encoding="utf-8"?>'
#    echo '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
#    echo '  <ItemGroup>'
#    local prefix="    <ClInclude Include=\"$MsSrcRelPath"
#    local suffix="\">\n      <Filter>Header Files</Filter>\n    </ClInclude>"
#    GetSources h "" $prefix $suffix ! -name "tsStaticReferences*"
#    GetSources h private "${prefix}private\\" "$suffix"
#    GetSources h windows "${prefix}windows\\" "$suffix"
#    echo '  </ItemGroup>'
#    echo '  <ItemGroup>'
#    prefix="    <ClCompile Include=\"$MsSrcRelPath"
#    suffix="\">\n      <Filter>Source Files</Filter>\n    </ClCompile>"
#    GetSources cpp "" $prefix $suffix ! -name "tsStaticReferences*"
#    GetSources cpp private "${prefix}private\\" "$suffix"
#    GetSources cpp windows "${prefix}windows\\" "$suffix"
#    echo '  </ItemGroup>'
#    echo '</Project>'
}

# Generate the Qt Creator project file.
GenerateQtProject()
{
    echo 'HEADERS += \\'
#    local prefix="    $QtSrcRelPath"
#    local suffix=" \\"
#    GetSources h "" "$prefix" "$suffix" ! -name "tsStaticReferences*"
#    GetSources h private "${prefix}private/" $suffix
#    echo ''
#    echo 'SOURCES += \\'
#    GetSources cpp "" $prefix $suffix ! -name "tsStaticReferences*"
#    GetSources cpp private "${prefix}private/" "$suffix
#    $prefix = "    $prefix"
#    echo ''
#    echo 'linux {'
#    echo '    HEADERS += \\'
#    GetSources h unix "${prefix}unix/" $suffix
#    GetSources h linux "${prefix}linux/" $suffix
#    echo ''
#    echo '    SOURCES += \\'
#    GetSources cpp unix "${prefix}unix/" $suffix
#    GetSources cpp linux "${prefix}linux/" $suffix
#    echo ''
#    echo '}'
#    echo ''
#    echo 'mac {'
#    echo '    HEADERS += \\'
#    GetSources h unix "${prefix}unix/" $suffix
#    GetSources h mac "${prefix}mac/" $suffix
#    echo ''
#    echo '    SOURCES += \\'
#    GetSources cpp unix "${prefix}unix/" $suffix
#    GetSources cpp mac "${prefix}mac/" $suffix
#    echo ''
#    echo '}'
#    echo ''
#    echo 'win32|win64 {'
#    echo '    HEADERS += \\'
#    GetSources h windows "${prefix}windows/" $suffix
#    echo ''
#    echo '    SOURCES += \\'
#    GetSources cpp windows "${prefix}windows/" $suffix
#    echo ''
#    echo '}'
}

# Generate the main TSDuck header file.
GenerateMainHeader()
{
    local prefix="#include \""
    local suffix="\""

#    cat "$SRCDIR/../HEADER.txt"
#    echo ''
#    echo '#pragma once'
#    GetSources h "" "$prefix" "$suffix" ! -name "tsStaticReferences*" ! -name "*Template.h"
#    echo ''
#    echo '#if defined(TS_LINUX)'
#    GetSources h unix "$prefix" "$suffix" ! -name "*Template.h"
#    GetSources h linux "$prefix" "$suffix" ! -name "*Template.h"
#    echo '#endif'
#    echo ''
#    echo '#if defined(TS_MAC)'
#    GetSources h unix "$prefix" "$suffix" ! -name "*Template.h"
#    GetSources h mac "$prefix" "$suffix" ! -name "*Template.h"
#    echo '#endif'
#    echo ''
#    echo '#if defined(TS_WINDOWS)'
#    GetSources h windows "$prefix" "$suffix" ! -name "*Template.h"
#    echo '#endif'
}


# Generate the files.
# GenerateMSProject  | Out-File -Encoding ascii $MsvcDir\libtsduck-files.props
# GenerateMSFilters  | Out-File -Encoding ascii $MsvcDir\libtsduck-filters.props
# GenerateQtProject  | Out-File -Encoding ascii $RootDir\build\qtcreator\libtsduck\libtsduck-files.pri
# GenerateMainHeader | Out-File -Encoding ascii $SrcDir\tsduck.h
