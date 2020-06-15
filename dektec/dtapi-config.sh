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
#  Get configuration for DTAPI on current Linux system.
#  Options: --dtapi --header --object --url --support --m32
#
#-----------------------------------------------------------------------------

URL_BASE=https://www.dektec.com
HTML_URL=$URL_BASE/downloads/SDK/
GENERIC_URL=$URL_BASE/products/SDK/DTAPI/Downloads/LatestLinuxSDK

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE); pwd)
SYSTEM=$(uname -s | tr A-Z a-z)

error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Get the root directory of the DTAPI.
get-dtapi()
{
    local prefix=
    case "$SYSTEM" in
        linux)
            header=$(find 2>/dev/null "$ROOTDIR/LinuxSDK/DTAPI" -path "*/DTAPI/Include/DTAPI.h" | head -1)
            ;;
        cygwin*)
            header=$(find 2>/dev/null /cygdrive/c/Program\ Files*/Dektec -path "*/DTAPI/Include/DTAPI.h" | head -1)
            ;;
        mingw*|msys*)
            header=$(find 2>/dev/null /c/Program\ Files*/Dektec -path "*/DTAPI/Include/DTAPI.h" | head -1)
            ;;
        *)
            header=
    esac
    if [[ -n "$header" ]]; then
        d=$(dirname "$header")
        dirname "$d"
    fi
}

# Check if DTAPI is supported on the current system.
dtapi-support()
{
    # Environment variable NODTAPI disables the usage of DTAPI.
    [[ -n "$NODTAPI" ]] && return -1

    # DTAPI is supported on Linux and Windows only.
    case "$SYSTEM" in
        linux|cygwin*|mingw*|msys*)
            ;;
        *)
            return -1
    esac

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

    local HEADER="$(get-dtapi)/Include/DTAPI.h"
    [[ -e "$HEADER" ]] && echo "$HEADER"
}

# Get DTAPI object file.
get-object()
{
    # Get DTAPI support on this system.
    dtapi-support || return 0

    # Check that DTAPI binaries are present.
    [[ -d "$(get-dtapi)/Lib" ]] || return 0

    # Get GCC version as an integer.
    if [[ -z "$GCCVERSION" ]]; then
        local GCC=$(which gcc 2>/dev/null)
        [[ -z "$GCC" ]] && return
        GCCVERSION=$("$GCC" -dumpversion)
    fi
    local GCCVERS=$(int-version $GCCVERSION)
    local DIRVERS=

    # Get object file from platform name.
    if ${M32:-false}; then
        OBJNAME=DTAPI.o
    elif [[ $(uname -m) == x86_64 ]]; then
        OBJNAME=DTAPI64.o
    else
        OBJNAME=DTAPI.o
    fi

    # Find the DTAPI object with highest version, lower than or equal to GCC version.
    local OBJFILE=
    local OBJVERS=0
    local DTAPIDIR=$(get-dtapi)
    if [[ -n "$DTAPIDIR" ]]; then
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
    fi
    [[ -n "$OBJFILE" ]] && echo "$OBJFILE"
}

# Merge an URL with its base.
# The base is the argument. The URL is read from stdin.
merge-url()
{
    local ref="$1"
    local url
    read url

    if [[ -n "$url" ]]; then
        if [[ $url == *:* ]]; then
            echo "$url"
        elif [[ $url == /* ]]; then
            echo "$URL_BASE$url"
        elif [[ $ref == */ ]]; then
            echo "$ref$url"
        else
            ref=$(dirname "$ref")
            echo "$ref/$url"
        fi
    fi
}

# Retrieve the URL using the redirection from a fixed generic URL.
# This should be the preferred method but Dektec may forget to update
# the redirection in the generic URL.
get-url-from-redirection()
{
    curl --silent --show-error --dump-header /dev/stdout "$GENERIC_URL" | \
        grep -i 'Location:' | \
        sed -e 's/.*: *//' -e 's/\r//g' | \
        merge-url "$GENERIC_URL"
}

# Retrieve the URL by parsing the HTML from the Dektec download web page.
get-url-from-html-page()
{
    curl --silent --show-error --location "$HTML_URL" | \
        grep 'href=".*LinuxSDK' | \
        sed -e 's/.*href="//' -e 's/".*//' | \
        merge-url "$HTML_URL"
}

# Get Dektec LinuxSDK URL.
get-url()
{
    # Try the HTML parsing first, then redirection.
    URL=$(get-url-from-html-page)
    [[ -z "$URL" ]] && URL=$(get-url-from-redirection)
    [[ -z "$URL" ]] && error "cannot locate LinuxSDK location from Dektec"
    echo "$URL"
}

# Get options.
CMD=""
M32=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        --m32)
            M32=true
            ;;
        *)
            CMD="$1"
            ;;
    esac
    shift
done

# Main command
case "$CMD" in
    --dtapi)
        get-dtapi
        ;;
    --header)
        get-header
        ;;
    --object)
        shift
        get-object
        ;;
    --url)
        get-url
        ;;
    --support)
        dtapi-support && echo supported
        ;;
    -*)
        error "invalid option $CMD (use --dtapi --header --object --url --support [--m32])"
        ;;
    *)
        shift
        echo "DTAPI_ROOT=$(get-dtapi)"
        echo "DTAPI_HEADER=$(get-header)"
        echo "DTAPI_OBJECT=$(get-object)"
        echo "DTAPI_URL=$(get-url)"
        ;;
esac
exit 0
