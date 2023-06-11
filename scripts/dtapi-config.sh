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
#  Get configuration for DTAPI on current Linux system.
#
#-----------------------------------------------------------------------------

URL_BASE=https://www.dektec.com
HTML_URL=$URL_BASE/downloads/SDK/
GENERIC_URL=$URL_BASE/products/SDK/DTAPI/Downloads/LatestLinuxSDK
CACERT_URL=https://curl.se/ca/cacert.pem

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE)/..; pwd)
BINDIR="$ROOTDIR/bin"
SYSTEM=$(uname -s | tr A-Z a-z)

info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Get the root directory of the DTAPI.
get-dtapi()
{
    local prefix=
    case "$SYSTEM" in
        linux)
            header=$(find 2>/dev/null "$BINDIR/LinuxSDK/DTAPI" -path "*/DTAPI/Include/DTAPI.h" | head -1)
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

# Get DTAPI include directory.
get-include()
{
    # Get DTAPI support on this system.
    dtapi-support || return 0

    local INCLUDE="$(get-dtapi)/Include"
    [[ -e "$INCLUDE" ]] && echo "$INCLUDE"
}

# Get DTAPI object file.
get-object()
{
    # Get DTAPI support on this system.
    dtapi-support || return 0

    # Check that DTAPI binaries are present.
    [[ -d "$(get-dtapi)/Lib" ]] || return 0

    # Get gcc executable from external $GCC or default.
    GCC=${GCC:-$(which gcc 2>/dev/null)}
    [[ -z "$GCC" ]] && return 0

    # Get gcc version from external $GCC_VERSION or $GCCVERSION.
    GCCVERSION=${GCCVERSION:-$GCC_VERSION}
    GCCVERSION=${GCCVERSION:-$("$GCC" -dumpversion 2>/dev/null)}

    # Get GCC version as an integer.
    local GCCVERS=$(int-version $GCCVERSION)
    local DIRVERS=

    # Get object file from platform name.
    if ${OPT_M32:-false}; then
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
    local DIRNAME=
    if [[ -n "$DTAPIDIR" ]]; then
        for obj in $(find "$DTAPIDIR/Lib" -path "*/GCC*/$OBJNAME"); do
            DIRNAME=$(basename $(dirname "$obj"))
            DIRVERS=${DIRNAME#GCC}
            DIRVERS=${DIRVERS%%_*}
            DIRVERS=$(int-version $DIRVERS)
            if [[ ($DIRVERS -le $GCCVERS) && ($DIRVERS -gt $OBJVERS) ]]; then
                OBJFILE="$obj"
                OBJVERS=$DIRVERS
                # If directory ends in _ABI0 and the same file exists with _ABI1, use _ABI1.
                # This implements the default C++11 ABI after GCC 5.1.
                [[ $DIRNAME == *_ABI0 && -f "${obj/_ABI0/_ABI1}" ]] && OBJFILE="${obj/_ABI0/_ABI1}"
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

# Check SSL/TLS connectivity, try with latest certificates from curl.se.
# This can be necessary when the Let's Encrypt root of trust is unknown
# to curl (see "DST Root CA X3" issue). Can be called any number of times,
# the actual check is done only once.
check-tls()
{
    if [[ -z "$CHECKTLS_DONE" ]]; then
        export CHECKTLS_DONE=true
        export CURLOPT=
        # Try to access the site, check errors (possibly TLS).
        if ! curl --silent --head "$URL_BASE" >/dev/null; then
            # Error, try with CA certificates from curl.se.
            CACERT_NAME=$(basename "$CACERT_URL")
            CACERT_FILE="$BINDIR/$CACERT_NAME"
            mkdir -p "$BINDIR"
            if [[ ! -e "$CACERT_FILE" ]]; then
                info "curl error, getting CA certs from $CACERT_URL"
                curl --silent --show-error --location "$CACERT_URL" -o "$CACERT_FILE"
            fi
            if [[ ! -e "$CACERT_FILE" ]]; then
                info "error getting $CACERT_URL, using default certs"
            elif curl --silent --head "$URL_BASE" --cacert "$CACERT_FILE" >/dev/null; then
                # Success with downloaded cacert
                export CURLOPT="--cacert $CACERT_FILE"
            elif [[ -n $(find "$CACERT_FILE" -mtime +1) ]]; then
                # Current cacert file is older than 1 day, try to refresh it.
                info "error with existing CA certs, older than 1 day, getting fresh ones from $CACERT_URL"
                curl --silent --show-error --location "$CACERT_URL" -o "$CACERT_FILE"
                # No need to test, just use them, we have no better option.
                export CURLOPT="--cacert $CACERT_FILE"
            fi
        fi
    fi
}

# Retrieve the URL using the redirection from a fixed generic URL.
# This should be the preferred method but Dektec may forget to update
# the redirection in the generic URL.
get-url-from-redirection()
{
    check-tls
    curl $CURLOPT --silent --show-error --dump-header /dev/stdout "$GENERIC_URL" | \
        grep -i 'Location:' | \
        sed -e 's/.*: *//' -e 's/\r//g' | \
        merge-url "$GENERIC_URL"
}

# Retrieve the URL by parsing the HTML from the Dektec download web page.
get-url-from-html-page()
{
    check-tls
    curl $CURLOPT --silent --show-error --location "$HTML_URL" | \
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

# Get local tarball file name.
get-tarball()
{
    [[ $SYSTEM != linux ]] && return
    local link=$(readlink "$BINDIR/dektec-tarball")
    local tarball=
    [[ -n "$link" ]] && tarball="$BINDIR/$link"
    [[ -e "$tarball" ]] && echo "$tarball"
}

# Download and expand the Dektec LinuxSDK.
download-dtapi()
{
    # Do not download if DTAPI is not supported.
    dtapi-support || return 0

    # Cleanup first if --force is specified.
    $OPT_FORCE && rm -rf "$BINDIR"/LinuxSDK* "$BINDIR/dektec-tarball"

    # If DTAPI_ORIGIN is defined, use it as tarball.
    local tarball="$DTAPI_ORIGIN"
    [[ -z "$tarball" ]] && tarball=$(get-tarball)

    # Download the tarball if not present.
    if [[ ! -e "$tarball" ]]; then
        local url=$(get-url)
        local name=$(basename "$url")
        tarball="$BINDIR/$name"
        info "downloading $url ..."
        mkdir -p "$BINDIR"
        check-tls
        curl $CURLOPT --silent --show-error --location "$url" -o "$BINDIR/$name"
        ln -sf "$name" "$BINDIR/dektec-tarball"
        rm -rf "$BINDIR/LinuxSDK"
    fi

    # Expand tarball.
    if [[ ! -d "$BINDIR/LinuxSDK" ]]; then
        info "expanding $tarball ..."
        mkdir -p "$BINDIR"
        tar -C "$BINDIR" -xzf "$tarball"
        # Make sure that files are more recent than already compiled binaries.
        find "$BINDIR/LinuxSDK" -print0 | xargs -0 touch
    fi
}

# Get options.
CMD_ALL=true
CMD_DOWNLOAD=false
CMD_TARBALL=false
CMD_DTAPI=false
CMD_HEADER=false
CMD_INCLUDE=false
CMD_OBJECT=false
CMD_URL=false
CMD_SUPPORT=false
OPT_FORCE=false
OPT_M32=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --download)
            CMD_DOWNLOAD=true
            CMD_ALL=false
            ;;
        --tarball)
            CMD_TARBALL=true
            CMD_ALL=false
            ;;
        --dtapi)
            CMD_DTAPI=true
            CMD_ALL=false
            ;;
        --header)
            CMD_HEADER=true
            CMD_ALL=false
            ;;
        --include)
            CMD_INCLUDE=true
            CMD_ALL=false
            ;;
        --object)
            CMD_OBJECT=true
            CMD_ALL=false
            ;;
        --url)
            CMD_URL=true
            CMD_ALL=false
            ;;
        --support)
            CMD_SUPPORT=true
            CMD_ALL=false
            ;;
        --force)
            OPT_FORCE=true
            ;;
        --m32)
            OPT_M32=true
            ;;
        *)
            error "invalid option $1 (use --dtapi --header --object --include --url --support --tarball --download --force --m32)"
            ;;
    esac
    shift
done

# Execute commands. Download first if requested.
$CMD_DOWNLOAD && download-dtapi
$CMD_TARBALL && get-tarball
$CMD_DTAPI && get-dtapi
$CMD_HEADER && get-header
$CMD_INCLUDE && get-include
$CMD_OBJECT && get-object
$CMD_URL && get-url
$CMD_SUPPORT && dtapi-support && echo supported

if $CMD_ALL; then
    echo "DTAPI_ROOT=$(get-dtapi)"
    echo "DTAPI_HEADER=$(get-header)"
    echo "DTAPI_INCLUDE=$(get-include)"
    echo "DTAPI_OBJECT=$(get-object)"
    echo "DTAPI_TARBALL=$(get-tarball)"
    echo "DTAPI_URL=$(get-url)"
fi
exit 0
