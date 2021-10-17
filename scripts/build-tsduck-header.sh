#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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
#  Rebuilt tsduck.h, the global header for the TSDuck library.
#  This script is useful when source files are added to or removed from the
#  directory src/libtsduck.
#
#  See the PowerShell script build-tsduck-header.ps1 for a Windows equivalent.
#
#-----------------------------------------------------------------------------

# Execute in the libtsduck root directory.
cd $(dirname $0)/../src/libtsduck

# Enforce LANG to get the same sort order everywhere.
export LANG=C
export LC_ALL=$LANG

# Get all libtsduck files by type.
get-headers()
{
    find . -type f -name '*.h' ! -name tsduck.h ! -name "*Template.h" "$@" |
        sed -e 's|.*/||' -e 's|^|#include "|' -e 's|$|"|' |
        sort --ignore-case
}

# Generate the main TSDuck header file.
(
    cat ../HEADER.txt
    echo ''
    echo '#pragma once'
    get-headers ! -path '*/linux/*' ! -path '*/mac/*' ! -path '*/unix/*' ! -path '*/windows/*' ! -path '*/private/*'
    echo ''
    echo '#if defined(TS_LINUX)'
    get-headers \( -path '*/unix/*' -o -path '*/linux/*' \)
    echo '#endif'
    echo ''
    echo '#if defined(TS_MAC)'
    get-headers \( -path '*/unix/*' -o -path '*/mac/*' \)
    echo '#endif'
    echo ''
    echo '#if defined(TS_WINDOWS)'
    get-headers -path '*/windows/*'
    echo '#endif'
) >tsduck.h

exit 0
