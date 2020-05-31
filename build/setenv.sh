#!/usr/bin/env bash
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
#  This script builds the name of the directory which contains binaries.
#  The typical usage is to 'source' it: it adds the binary directory to
#  the path. Other options:
#
#     --display : only display the binary directory, don't set PATH
#     --debug : use debug build
#
#-----------------------------------------------------------------------------

# Default options.
TARGET=release
DISPLAY=false

# Decode command line options.
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)
            TARGET=debug
            ;;
        --display)
            DISPLAY=true
            ;;
    esac
    shift
done

# Build binary directory.
ROOTDIR=$(cd $(dirname "${BASH_SOURCE[0]}")/..; pwd)
ARCH=$(uname -m | sed -e 's/i.86/i386/' -e 's/^arm.*$/arm/')
HOST=$(hostname | sed -e 's/\..*//')
BINDIR="$ROOTDIR/bin/$TARGET-$ARCH-$HOST"

# Display or set path.
if $DISPLAY; then
    echo "$BINDIR"
elif [[ ":$PATH:" != *:$BINDIR:* ]]; then
    export PATH="$BINDIR:$PATH"
else
    # Make sure to exit with success status
    true
fi
