#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
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
#  Shell script to build the documentation using Doxygen.
#
#-----------------------------------------------------------------------------

# Get the project directories.
ROOTDIR=$(cd $(dirname ${BASH_SOURCE[0]})/..; pwd)
DOXYDIR="$ROOTDIR/bin/doxy"
DOCDIR="$ROOTDIR/doc"
SRCDIR="$ROOTDIR/src/libtsduck"

# Make sure that the output directory is created (doxygen does not create parent directories).
mkdir -p "$DOXYDIR"

# Environment variables, used in Doxyfile.
export TS_FULL_VERSION=$("$ROOTDIR/scripts/get-version-from-sources.py")
export DOT_PATH=$(which dot 2>/dev/null)
[[ -n "DOT_PATH" ]] && export HAVE_DOT=YES || export HAVE_DOT=NO
export DOXY_INCLUDE_PATH=$(find "$SRCDIR" -type d | tr '\n' ' ')

# Generate a summary file of all signalization.
"$ROOTDIR/src/doc/signalization-gen.py"

# Doxygen version, used to filter various doxygen bugs.
DOXVERSION=$(doxygen --version)

# Run doxygen. Filter known false errors (bugs) based on doxygen version.
cd "$DOCDIR"
if [[ $DOXVERSION == *1.8.17* ]]; then
    doxygen 2>&1 | grep -v 'warning: return type of member .* is not documented'
elif [[ $DOXVERSION == *1.9.6* ]]; then
    doxygen 2>&1 | grep -v \
        -e 'python/tsduck.py:[0-9]*: warning: Member _.* is not documented' \
        -e 'tsTunerDevice.h:[0-9]*: warning: Detected potential recursive class relation between class ts::TunerDevice and base class ts::TunerBase'
else
    doxygen
fi

# Delete empty subdirectories (many of them created for nothing in case of hierachical output).
find "$DOXYDIR" -type d -empty -delete
