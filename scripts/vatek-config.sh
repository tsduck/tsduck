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
#  Get configuration for Vatek core API on current Linux or macOS system.
#  Vatek provides an open-source API on GitHub for their modulator chips.
#  On macOS, the Homebrew package libvatek provides a binary version.
#  On Linux, a binary tarball is downloaded by this script. When the
#  binary tarball is not appropriate for the current system, the source
#  code of the Vatek library is dowloaded and rebuilt.
#
#-----------------------------------------------------------------------------

# All possible options.
OPTIONS="--support --cflags --ldlibs --lib --include --src-url --bin-url --src-tarball --bin-tarball --download --force --rebuild"

# Error reporting.
SCRIPT=$(basename $BASH_SOURCE)
info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Structure of the local directory containing the Vatek API.
ROOTDIR=$(cd $(dirname $BASH_SOURCE)/..; pwd)
VATEK_ROOT="$ROOTDIR/bin/vatek"
VATEK_BUILD="$VATEK_ROOT/build"
VATEK_INSTALL="$VATEK_ROOT/install"
SYSTEM=$(uname -s)

# GitHub authentication for curl.
curl-auth()
{
    if [[ -n "$GITHUB_TOKEN" ]]; then
        curl -sL --header "authorization: Bearer $GITHUB_TOKEN" "$@"
    else
        curl -sL "$@"
    fi
}

# Check if Vatek is supported on the current system.
vatek-support()
{
    case "$SYSTEM" in
        Linux|Darwin)
            return 0
            ;;
        *)
            return -1
            ;;
    esac
}

 # Get Vatek API source tarball URL for the latest release.
get-src-url()
{
    # Use the GitHub REST API to get the source tarball of the latest release of the Vatek API.
    curl-auth https://api.github.com/repos/VisionAdvanceTechnologyInc/vatek_sdk_2/releases/latest |
        grep '"tarball_url"' |
        sed 's/.*"tarball_url"[ :"]*\([^"]*\)".*/\1/' |
        head -1
}

# Get Vatek API binary tarball URL for the latest release on Linux.
get-bin-url()
{
    # Use the GitHub REST API to get the URL if an asset named VATek-Linux-x86_64.*\.tgz in the latest release of the Vatek API.
    if [[ $SYSTEM == Linux ]]; then
        curl-auth https://api.github.com/repos/VisionAdvanceTechnologyInc/vatek_sdk_2/releases/latest |
            grep '"browser_download_url" *:.*/VATek-Linux-x86_64.*\.tgz"' |
            sed 's/.*"browser_download_url"[ :"]*\([^"]*\)".*/\1/' |
            head -1
    fi
}

# Check if the Vatek API binary tarball can be used on this platform or must be rebuilt.
need-rebuild()
{
    # Prebuilt binaries are available for x86_64 with GNU libc API only.
    $OPT_REBUILD || [[ -f /etc/alpine-release ]] || [[ $(uname -m) != x86_64 ]]
}

# Get suffix of shared libraries.
solib-suffix()
{
    [[ $SYSTEM == Darwin ]] && echo .dylib || echo .so
}

# Get the Homebrew root (empty if non existent).
homebrew-root()
{
    if [[ $SYSTEM == Linux ]]; then
        [[ -n "$LINUXBREW" && -n "$HOMEBREW_PREFIX" ]] && echo "$HOMEBREW_PREFIX"
    elif [[ $SYSTEM == Darwin ]]; then
        [[ -d /opt/homebrew/bin ]] && echo /opt/homebrew || echo /usr/local
    fi
}

# Get Vatek include directory (empty if non existent).
get-include()
{
    local brew=$(homebrew-root)
    if [[ -n "$brew" ]]; then
        [[ -d "$brew/include/vatek" ]] && echo "$brew/include/vatek"
    elif [[ -d "$VATEK_INSTALL/include/vatek" ]]; then
        echo "$VATEK_INSTALL/include/vatek"
    fi
}

# Get Vatek static or dynamic library path (empty if non existent).
get-lib()
{
    # Always use the shared library with Homebrew. Use static library on Linux.
    local brew=$(homebrew-root)
    if [[ -n "$brew" ]]; then
        local suffix=$(solib-suffix)
        [[ -e "$brew/lib/libvatek_core$suffix" ]] && echo "$brew/lib/libvatek_core$suffix"
    elif [[ $SYSTEM == Linux && -e "$VATEK_INSTALL/lib/libvatek_core.a" ]]; then
        echo "$VATEK_INSTALL/lib/libvatek_core.a"
    fi
}

# Get compilation flags for Vatek library.
get-cflags()
{
    # Use -isystem instead of -I to avoid all warnings in Vatek headers.
    local inc=$(get-include)
    [[ -n "$inc" ]] && echo -isystem "$inc"
}

# Get library flags for Vatek library.
get-ldlibs()
{
    local lib=$(get-lib)
    local suffix=$(solib-suffix)
    if [[ $lib == *$suffix ]]; then
        lib=$(basename "$lib" $suffix)
        echo -l${lib/#lib/}
    elif [[ $SYSTEM == Linux ]]; then
        echo "$lib -lusb-1.0"
    fi
}

# Get local source tarball file name.
get-src-tarball()
{
    [[ $SYSTEM != Linux ]] && return
    local link=$(readlink "$VATEK_ROOT/src-tarball")
    local tarball=
    [[ -n "$link" ]] && tarball="$VATEK_ROOT/$link"
    [[ -e "$tarball" ]] && echo "$tarball"
}

# Get local binary tarball file name.
get-bin-tarball()
{
    [[ $SYSTEM != Linux ]] && return
    local link=$(readlink "$VATEK_ROOT/bin-tarball")
    local tarball=
    [[ -n "$link" ]] && tarball="$VATEK_ROOT/$link"
    [[ -e "$tarball" ]] && echo "$tarball"
}

# Download and rebuild (when necessary) the Vatek API.
download-vatek()
{
    # Only if platform is supported.
    vatek-support || return

    # With Homebrew, use installed binary Vatek library, do not download.
    [[ -n "$(homebrew-root)" ]] && return

    # Cleanup first if --force is specified.
    $OPT_FORCE && rm -rf "$VATEK_ROOT"

    # Download source and rebuild or download binary.
    if need-rebuild; then

        # If VATEK_SRC_ORIGIN is defined, use it as tarball.
        local tarball="$VATEK_SRC_ORIGIN"
        [[ -z "$tarball" ]] && tarball=$(get-src-tarball)

        # Download the source tarball if not present.
        if [[ ! -e "$tarball" ]]; then
            local url=$(get-src-url)
            local name=$(basename "$url")
            tarball="$VATEK_ROOT/$name.tgz"
            info "downloading $url ..."
            mkdir -p "$VATEK_ROOT"
            curl --silent --show-error --location "$url" -o "$tarball"
            [[ ! -e "$tarball" ]] && error "error downloading $url"
            ln -sf "$name.tgz" "$VATEK_ROOT/src-tarball"
            rm -rf "$VATEK_ROOT/$name"
        fi
        local srcdir="$VATEK_ROOT"/$(basename "$tarball" .tgz)

        # Expand tarball.
        if [[ ! -d "$srcdir" ]]; then
            info "expanding $tarball ..."
            mkdir -p "$srcdir"
            tar -C "$srcdir" -xzf "$tarball" --strip-components=1
        fi

        # Build the Vatek API.
        if [[ -z "$(get-lib)" ]]; then
            info "rebuilding the vatek library ..."
            (
                rm -rf "$VATEK_BUILD"
                # Remain compatible with old versions of cmake.
                cmake -S "$srcdir" -B "$VATEK_BUILD" -DCMAKE_INSTALL_PREFIX:PATH="$VATEK_INSTALL" \
                      -DSDK2_EN_QT=OFF -DSDK2_EN_APP=OFF -DSDK2_EN_SAMPLE=OFF -DSDK2_EN_STATIC_ONLY=ON &&
                    cmake --build "$VATEK_BUILD" &&
                    cmake --install "$VATEK_BUILD"
            ) &>"$VATEK_BUILD.log"
            [[ -z "$(get-lib)" ]] && error "error building vatek library, see $VATEK_BUILD.log"
        fi

    else

        # If VATEK_BIN_ORIGIN is defined, use it as tarball.
        local tarball="$VATEK_BIN_ORIGIN"
        [[ -z "$tarball" ]] && tarball=$(get-bin-tarball)

        # Download the binary tarball if not present.
        if [[ ! -e "$tarball" ]]; then
            local url=$(get-bin-url)
            local name=$(basename "$url")
            tarball="$VATEK_ROOT/$name"
            info "downloading $url ..."
            mkdir -p "$VATEK_ROOT"
            curl --silent --show-error --location "$url" -o "$tarball"
            [[ ! -e "$tarball" ]] && error "error downloading $url"
            ln -sf "$name" "$VATEK_ROOT/bin-tarball"
            rm -rf "$VATEK_INSTALL"
        fi

        # Expand tarball.
        if [[ ! -d "$VATEK_INSTALL" ]]; then
            info "expanding $tarball ..."
            mkdir -p "$VATEK_INSTALL"
            tar -C "$VATEK_INSTALL" -xzf "$tarball" --strip-components=1
        fi
    fi
}

# Decode command line options.
varname() { tr <<<${1/#--/OPT_} a-z- A-Z_; }
OPT_ALL=true
for opt in $OPTIONS; do
    eval $(varname $opt)=false
done
while [[ $# -gt 0 ]]; do
    if [[ \ $OPTIONS\  != *\ $1\ * ]]; then
        error "invalid option $1 (use $OPTIONS)"
    else
        eval $(varname $1)=true
        OPT_ALL=false
    fi
    shift
done

# Execute commands.
if $OPT_ALL; then
    # No parameter, display all values.
    echo "VATEK_INCLUDE=$(get-include)"
    echo "VATEK_LIB=$(get-lib)"
    echo "VATEK_CFLAGS=$(get-cflags)"
    echo "VATEK_LDLIBS=$(get-ldlibs)"
    echo "VATEK_SRC_TARBALL=$(get-src-tarball)"
    echo "VATEK_BIN_TARBALL=$(get-bin-tarball)"
    echo -n VATEK_REBUILD=; need-rebuild && echo true || echo false
    echo "VATEK_SRC_URL=$(get-src-url)"
    echo "VATEK_BIN_URL=$(get-bin-url)"
else
    # Execute specific options only. Download first if requested.
    $OPT_SUPPORT && vatek-support && echo supported
    $OPT_DOWNLOAD && download-vatek
    $OPT_SRC_URL && get-src-url
    $OPT_BIN_URL && get-bin-url
    $OPT_SRC_TARBALL && get-src-tarball
    $OPT_BIN_TARBALL && get-bin-tarball
    $OPT_INCLUDE && get-include
    $OPT_LIB && get-lib
    $OPT_CFLAGS && get-cflags
    $OPT_LDLIBS && get-ldlibs
fi
exit 0
