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
#  This script extracts the TSDuck version from the source files.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE)/..; pwd)
VERSFILE="$ROOTDIR/src/libtsduck/tsVersion.h"

major()  { grep '\#define *TS_VERSION_MAJOR ' "$VERSFILE" | sed -e 's/.* //'; }
minor()  { grep '\#define *TS_VERSION_MINOR ' "$VERSFILE" | sed -e 's/.* //'; }
commit() { grep '\#define *TS_COMMIT ' "$VERSFILE" | sed -e 's/.* //'; }

distro()
{
    if [[ -n "$(which lsb_release 2>/dev/null)" ]]; then
        dis=$(lsb_release -si 2>/dev/null | tr A-Z a-z)$(lsb_release -sr 2>/dev/null | sed 's/\..*//')
        [[ -n "$dis" ]] && dis=".$dis"
    elif [[ -e /etc/fedora-release ]]; then
        dis=$(grep " release " /etc/fedora-release 2>/dev/null | sed -e 's/^.* release \([0-9]*\).*$/\1/')
        [[ -n "$dis" ]] && dis=".fc$dis"
    elif [[ -e /etc/redhat-release ]]; then
        dis=$(grep " release " /etc/redhat-release 2>/dev/null | sed -e 's/^.* release \([0-9]*\).*$/\1/')
        [[ -n "$dis" ]] && dis=".el$dis"
    elif [[ -e /etc/alpine-release ]]; then
        dis=$(head -1 /etc/alpine-release | sed -e 's/\.[^\.]*$//' -e 's/\.//g')
        [[ -n "$dis" ]] && dis=".alpine$dis"
    elif [[ $(uname -s) == Darwin ]]; then
        dis=$(sw_vers -productVersion 2>/dev/null | sed -e 's/\.[^\.]*$//' -e 's/\.//g')
        [[ -n "$dis" ]] && dis=".macos$dis"
    else
        dis=
    fi
    echo "$dis"
}

case "$1" in
    --major)  major ;;
    --minor)  minor ;;
    --commit) commit ;;
    --distro) distro ;;
    --full)   echo "$(major).$(minor)-$(commit)$(distro)" ;;
    --main)   echo "$(major).$(minor)" ;;
    *)        echo "$(major).$(minor)-$(commit)" ;;
esac
