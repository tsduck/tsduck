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
#  This script installs all pre-requisites packages to build TSDuck,
#  depending on the system.
#
#  Supported options:
#
#  -m, --m32 : install libraries for 32-bit cross-compilation (when supported)
#  -s, --static : install static libraries for static build (when supported)
#
#  Supported operating systems:
#
#  - macOS
#  - Ubuntu
#  - Debian
#  - Raspbian
#  - Fedora
#  - Red Hat
#  - CentOS
#  - Arch Linux
#  - Alpine Linux
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
SCRIPTDIR=$(cd $(dirname $0); pwd)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Default options.
STATIC=false
M32=false

# Decode command line options.
while [[ $# -gt 0 ]]; do
    case "$1" in
        -m|--m32)
            M32=true
            ;;
        -s|--static)
            STATIC=true
            ;;
        *)
            error "unsupported option $1"
            ;;
    esac
    shift
done

#-----------------------------------------------------------------------------

DISTRO=$(lsb_release -i 2>/dev/null | sed -e 's/.*:[\t ]*//')
MAJOR=$(lsb_release -r 2>/dev/null | sed -e 's/.*:[\t ]*//' -e 's/\..*//')
MINOR=$(lsb_release -r 2>/dev/null | sed -e '/\./!d' -e 's/.*:[\t ]*//' -e 's/.*\.//')

if [[ $(uname -s) == Darwin ]]; then

    # macOS
    pkglist="gnu-sed grep dos2unix coreutils pcsc-lite srt"
    if [[ -z $(which clang 2>/dev/null) ]]; then
        # Build tools not installed
        xcode-select --install
    fi
    if [[ -z $(which brew 2>/dev/null) ]]; then
        # Homebrew not installed
        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    fi
    brew update
    for pkg in $pkglist; do
        # Install or upgrade package (cannot be done in command)
        brew ls --versions $pkg >/dev/null && cmd=upgrade || cmd=install
        HOMEBREW_NO_AUTO_UPDATE=1 brew $cmd $pkg
    done

elif [[ "$DISTRO" == "Ubuntu" ]]; then

    # Ubuntu
    pkglist="g++ dos2unix curl tar zip doxygen graphviz pcscd libpcsclite-dev dpkg-dev"
    if [[ "$MAJOR" -le 17 ]]; then
        pkglist="$pkglist libcurl3 libcurl3-dev"
    else
        pkglist="$pkglist libcurl4 libcurl4-openssl-dev"
    fi
    if $M32; then
        pkglist="$pkglist gcc-multilib"
    fi
    if [[ "$MAJOR" -ge 19 ]]; then
        pkglist="$pkglist libsrt-dev"
    fi
    sudo apt update
    sudo apt install -y $pkglist

elif [[ "$DISTRO" = "Debian" || "$DISTRO" = "Raspbian" ]]; then

    # Debian or Raspbian (Raspberry Pi)
    pkglist="g++ dos2unix curl tar zip doxygen graphviz pcscd libpcsclite-dev dpkg-dev"
    if $M32; then
        pkglist="$pkglist gcc-multilib"
    fi
    if [[ "$MAJOR" -le 9 ]]; then
        pkglist="$pkglist libcurl3 libcurl3-dev"
    else
        pkglist="$pkglist libcurl4 libcurl4-openssl-dev"
    fi
    sudo apt update
    sudo apt install -y $pkglist

elif [[ -f /etc/fedora-release ]]; then

    # Fedora
    FC=$(grep " release " /etc/fedora-release 2>/dev/null | sed -e 's/^.* release \([0-9\.]*\) .*$/\1/')
    pkglist="gcc-c++ dos2unix curl tar zip doxygen graphviz pcsc-tools pcsc-lite-devel libcurl libcurl-devel rpmdevtools"
    if $STATIC; then
        pkglist="$pkglist glibc-static libstdc++-static"
    fi
    if $M32; then
        pkglist="$pkglist glibc-devel.i686 libstdc++-devel.i686 pcsc-lite-devel.i686 libcurl-devel.i686"
    fi
    if [[ $FC -ge 31 ]]; then
        pkglist="$pkglist srt-devel"
        if $M32; then
            pkglist="$pkglist srt-devel.i686"
        fi
    fi
    sudo dnf -y install $pkglist

elif [[ -f /etc/redhat-release ]]; then

    # Red Hat or CentOS
    pkglist="gcc-c++ dos2unix curl tar zip doxygen graphviz pcsc-lite pcsc-lite-devel libcurl libcurl-devel rpmdevtools"
    if $STATIC; then
        pkglist="$pkglist glibc-static libstdc++-static"
    fi
    if $M32; then
        pkglist="$pkglist glibc-devel.i686 libstdc++-devel.i686 pcsc-lite-devel.i686 libcurl-devel.i686"
    fi
    sudo yum -y install $pkglist

elif [[ -f /etc/arch-release ]]; then

    # Arch Linux
    pkglist="gcc dos2unix curl tar zip doxygen graphviz pcsclite srt"
    sudo pacman -Sy $pkglist

elif [[ -f /etc/alpine-release ]]; then

    # Alpine Linux
    pkglist="bash coreutils diffutils procps util-linux linux-headers git make g++ dos2unix curl tar zip doxygen graphviz pcsc-lite-dev curl-dev"
    sudo apk add $pkglist

fi
