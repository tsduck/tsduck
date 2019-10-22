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
#  Get configuration for DTAPI on current Linux system.
#  Options: --header --object --support
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE); pwd)
DTAPIDIR="$ROOTDIR/LinuxSDK/DTAPI"

error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Check if DTAPI is supported on the current system.
dtapi-support()
{
    # DTAPI is supported in Linux only.
    [[ $(uname -s) == Linux ]] || return -1

    # DTAPI is supported on Intel CPU only.
    arch=$(uname -m)
    [[ $arch == x86_64 || $arch == i?86 ]] || return -1

    # DTAPI is compiled with the GNU libc and is not supported on systems not using it.
    # Alpine Linux uses musl libc => not supported (undefined reference to __isnan).
    [[ -e /etc/alpine-release ]] && return -1

    # Seems to be a supported distro.
    return 0
}

# Compute an integer version from a x.y.z version string.
int-version()
{
    local -i v=0
    for f in $(cut -f 1-3 -d . <<<"$1.0.0.0" | tr . ' '); do
        v=$((($v * 100) + $f))
    done
    echo $v
}

# Get DTAPI header file.
get-header()
{
    # Get DTAPI support on this system.
    dtapi-support || return 0

    local HEADER="$DTAPIDIR/Include/DTAPI.h"
    [[ -e "$HEADER" ]] && echo "$HEADER"
}

# Get DTAPI object file.
get-object()
{
    # Get DTAPI support on this system.
    dtapi-support || return 0

    # Get GCC version as an integer.
    if [[ -z "$GCCVERSION" ]]; then
        local GCC=$(which gcc 2>/dev/null)
        [[ -z "$GCC" ]] && return
        GCCVERSION=$("$GCC" -dumpversion)
    fi
    local GCCVERS=$(int-version $GCCVERSION)
    local DIRVERS=

    # Get object file from platform name.
    if [[ $(uname -m) == x86_64 ]]; then
        OBJNAME=DTAPI64.o
    else
        OBJNAME=DTAPI.o
    fi

    # Find the DTAPI object with highest version, lower than or equal to GCC version.
    local OBJFILE=
    local OBJVERS=0
    for obj in $(find "$DTAPIDIR/Lib" -path "*/GCC*/$OBJNAME"); do
        DIRVERS=$(basename $(dirname "$obj"))
        DIRVERS=${DIRVERS#GCC}
        DIRVERS=${DIRVERS%%_*}
        DIRVERS=$(int-version $DIRVERS)
        if [[ ($DIRVERS -le $GCCVERS) && ($DIRVERS -gt $OBJVERS) ]]; then
            OBJFILE="$obj"
            OBJVERS=$DIRVERS
        fi
    done
    [[ -n "$OBJFILE" ]] && echo "$OBJFILE"
}

# Main command
case "$1" in
    --header)
        get-header
        ;;
    --object)
        get-object
        ;;
    --support)
        dtapi-support && echo supported        
        ;;
    -*)
        error "invalid option: $1"
        ;;
    *)
        echo "DTAPI_HEADER=$(get-header)"
        echo "DTAPI_OBJECT=$(get-object)"
        ;;
esac
exit 0
