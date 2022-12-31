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
#  This script installs all pre-requisites packages to build TSDuck,
#  depending on the system.
#
#  Supported options:
#
#  --m32 : install libraries for 32-bit cross-compilation (when supported)
#  --static : install static libraries for static build (when supported)
#  Additional options are passed to the package manager (dnf, apt, brew, etc.)
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
#  - Alma Linux
#  - Rocky Linux
#  - openSUSE Linux
#  - Arch Linux
#  - Alpine Linux
#  - Gentoo
#  - Linux Mint
#  - FreeBSD
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
SCRIPTDIR=$(cd $(dirname $0); pwd)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Default options.
STATIC=false
M32=false
PKGOPTS=

# Decode command line options.
while [[ $# -gt 0 ]]; do
    case "$1" in
        --m32)
            M32=true
            ;;
        --static)
            STATIC=true
            ;;
        *)
            PKGOPTS="$PKGOPTS $1"
            ;;
    esac
    shift
done

[[ -n "$PKGOPTS" ]] && echo "====> using packager options: $PKGOPTS"

SYSTEM=$(uname -s)
DISTRO=$(lsb_release -i 2>/dev/null | sed -e 's/.*:[\t ]*//')
MAJOR=$(lsb_release -r 2>/dev/null | sed -e 's/.*:[\t ]*//' -e 's/\..*//')
MINOR=$(lsb_release -r 2>/dev/null | sed -e '/\./!d' -e 's/.*:[\t ]*//' -e 's/.*\.//')
VERSION=$(( ${MAJOR:-0} * 100 + ${MINOR:-0} ))

#-----------------------------------------------------------------------------
# macOS (with HomeBrew as open source packager)
#
# Update command: brew update; brew upgrade
#-----------------------------------------------------------------------------

if [[ "$SYSTEM" == "Darwin" ]]; then

    pkglist="git git-lfs gnu-sed grep dos2unix coreutils srt librist libvatek python3 openjdk"
    if [[ -z $(which clang 2>/dev/null) ]]; then
        # Build tools not installed
        xcode-select --install
    fi
    if [[ -z $(which brew 2>/dev/null) ]]; then
        # Homebrew not installed
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    # Sometimes, brew exits with an error status even though the installation completes.
    # Mute this and enforce a good status to avoid GitHub Actions CI failure.
    brew update || true
    brew $PKGOPTS install $pkglist || true
    # Make sure python3 is the default in Homebrew.
    BREW=$(brew --prefix)
    (cd $BREW/bin; ln -sf python3 python)
    # Register the openjdk jvm.
    if [[ ! -e /Library/Java/JavaVirtualMachines/openjdk.jdk ]]; then
        JDK=$BREW/opt/openjdk/libexec/openjdk.jdk
        if [[ -n $(find $JDK -name javac -perm +444 2>/dev/null) ]]; then
            sudo ln -sfn $BREW/opt/openjdk/libexec/openjdk.jdk /Library/Java/JavaVirtualMachines/openjdk.jdk
        fi
    fi

#-----------------------------------------------------------------------------
# FreeBSD
#
# Update command: sudo pkg update; sudo pkg upgrade
#-----------------------------------------------------------------------------

elif [[ "$SYSTEM" == "FreeBSD" ]]; then

    pkglist="git git-lfs curl zip doxygen graphviz bash gsed gnugrep gmake gtar unix2dos coreutils srt librist libedit pcsc-lite python openjdk11"
    sudo pkg install -y $PKGOPTS $pkglist

#-----------------------------------------------------------------------------
# Ubuntu
#
# Update command: sudo apt update; sudo apt upgrade
# Find package providing file: dpkg -S /path/to/file
#-----------------------------------------------------------------------------

elif [[ "$DISTRO" == "Ubuntu" ]]; then

    pkglist="git git-lfs g++ cmake dos2unix curl tar zip doxygen graphviz linux-libc-dev libedit-dev libusb-1.0-0-dev pcscd libpcsclite-dev dpkg-dev python3 default-jdk"
    if [[ "$MAJOR" -le 17 ]]; then
        pkglist="$pkglist libcurl3 libcurl3-dev"
    else
        pkglist="$pkglist libcurl4 libcurl4-openssl-dev"
    fi
    if $M32; then
        pkglist="$pkglist gcc-multilib"
    fi
    if [[ "$VERSION" -ge 2010 ]]; then
        pkglist="$pkglist libsrt-openssl-dev"
    elif [[ "$VERSION" -ge 1904 ]]; then
        pkglist="$pkglist libsrt-dev"
    fi
    if [[ "$VERSION" -ge 2210 ]]; then
        pkglist="$pkglist librist-dev"
    fi
    sudo apt update
    sudo apt install -y $PKGOPTS $pkglist
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# Linux Mint
#
# Update command: sudo apt update; sudo apt upgrade
# Find package providing file: dpkg -S /path/to/file
#-----------------------------------------------------------------------------

elif [[ "$DISTRO" == "Linuxmint" ]]; then

    pkglist="git git-lfs g++ cmake dos2unix curl tar zip doxygen graphviz linux-libc-dev libedit-dev libusb-1.0-0-dev pcscd libpcsclite-dev dpkg-dev python3 default-jdk libcurl4 libcurl4-openssl-dev"
    if [[ "$MAJOR" -ge 21 ]]; then
        pkglist="$pkglist libsrt-openssl-dev"
    elif [[ "$MAJOR" -ge 20 ]]; then
        pkglist="$pkglist libsrt-dev"
    fi
    sudo apt update
    sudo apt install -y $PKGOPTS $pkglist
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# Debian or Raspbian (Raspberry Pi)
#
# Update command: sudo apt update; sudo apt upgrade
# Find package providing file: dpkg -S /path/to/file
#-----------------------------------------------------------------------------

elif [[ "$DISTRO" = "Debian" || "$DISTRO" = "Raspbian" ]]; then

    pkglist="git git-lfs g++ cmake dos2unix curl tar zip doxygen graphviz linux-libc-dev libedit-dev libusb-1.0-0-dev pcscd libpcsclite-dev dpkg-dev python3 default-jdk"
    if $M32; then
        pkglist="$pkglist gcc-multilib"
    fi
    if [[ "$MAJOR" -le 9 ]]; then
        pkglist="$pkglist libcurl3 libcurl3-dev"
    else
        pkglist="$pkglist libcurl4 libcurl4-openssl-dev"
    fi
    if [[ "$MAJOR" -ge 11 ]]; then
        pkglist="$pkglist libsrt-openssl-dev"
    fi
    if [[ "$MAJOR" -ge 12 ]]; then
        pkglist="$pkglist librist-dev"
    fi
    sudo apt update
    sudo apt install -y $PKGOPTS $pkglist
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# Fedora
#
# Update command: sudo dnf update
# Find package providing file: rpm -qf /path/to/file
#-----------------------------------------------------------------------------

elif [[ -f /etc/fedora-release ]]; then

    FC=$(grep " release " /etc/fedora-release 2>/dev/null | sed -e 's/^.* release \([0-9\.]*\) .*$/\1/')
    pkglist="git git-lfs gcc-c++ cmake dos2unix curl tar zip doxygen graphviz kernel-headers libedit-devel libusb1-devel pcsc-tools pcsc-lite-devel libcurl libcurl-devel libatomic rpmdevtools python3 java-latest-openjdk-devel"
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
    sudo dnf -y install $PKGOPTS $pkglist
    sudo alternatives --set python /usr/bin/python3

#-----------------------------------------------------------------------------
# Red Hat, CentOS, Alma Linux, Rocky Linux, etc.
#
# Update command: sudo dnf update
# Find package providing file: rpm -qf /path/to/file
#-----------------------------------------------------------------------------

elif [[ -f /etc/redhat-release ]]; then

    EL=$(grep " release " /etc/redhat-release 2>/dev/null | sed -e 's/$/.99/' -e 's/^.* release \([0-9]*\.[0-9]*\).*$/\1/')
    EL=$(( ${EL/.*/} * 100 + ${EL/*./} ))
    pkglist="git git-lfs gcc-c++ cmake dos2unix curl tar zip doxygen graphviz kernel-headers libedit-devel libusbx-devel pcsc-lite pcsc-lite-devel libcurl libcurl-devel libatomic rpmdevtools python3"
    if $STATIC; then
        pkglist="$pkglist glibc-static libstdc++-static"
    fi
    if $M32; then
        pkglist="$pkglist glibc-devel.i686 libstdc++-devel.i686 pcsc-lite-devel.i686 libcurl-devel.i686"
    fi
    if [[ $EL -ge 802 ]]; then
        pkglist="$pkglist srt-devel"
        if $M32; then
            pkglist="$pkglist srt-devel.i686"
        fi
    fi
    if [[ $EL -lt 800 ]]; then
        pkglist="$pkglist java-latest-openjdk-devel"
        sudo yum -y install $PKGOPTS epel-release
        sudo yum -y install $PKGOPTS $pkglist
    elif [[ $EL -lt 803 ]]; then
        pkglist="$pkglist java-latest-openjdk-devel"
        sudo dnf -y config-manager --set-enabled PowerTools
        sudo dnf -y install $PKGOPTS epel-release
        sudo dnf -y install $PKGOPTS $pkglist
    elif [[ $EL -lt 900 ]]; then
        pkglist="$pkglist java-latest-openjdk-devel"
        sudo dnf -y config-manager --set-enabled powertools
        sudo dnf -y install $PKGOPTS epel-release
        sudo dnf -y install $PKGOPTS $pkglist
    else
        pkglist="$pkglist java-17-openjdk-devel"
        sudo dnf -y config-manager --set-enabled plus
        sudo dnf -y config-manager --set-enabled crb
        sudo dnf -y install $PKGOPTS epel-release
        sudo dnf -y install $PKGOPTS $pkglist
    fi
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# openSUSE
#
# Update command: sudo zypper update -y -l
# Find package providing file: zypper search --provides /path/to/file
# Search package: zypper search partial-package-name
#-----------------------------------------------------------------------------

elif [[ -f /etc/os-release ]] && grep -q -i '^ID.*suse' /etc/os-release; then

    pkglist="git git-lfs make gcc-c++ cmake dos2unix curl tar zip doxygen graphviz linux-glibc-devel libedit-devel libusb-1_0-devel pcsc-tools pcsc-lite-devel curl libcurl-devel srt-devel rpmdevtools python3 java-11-openjdk-devel"
    sudo zypper install -y -l $PKGOPTS $pkglist
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# Arch Linux
#
# Update command: sudo pacman -Syu
# Find package providing file: pacman -Qo /path/to/file
# Search package: pacman -Ss partial-package-name
#-----------------------------------------------------------------------------

elif [[ -f /etc/arch-release ]]; then

    pkglist="git git-lfs make gcc cmake dos2unix core/which inetutils net-tools curl tar zip doxygen graphviz linux-api-headers libedit libusb pcsclite srt python jdk-openjdk"
    sudo pacman -Sy --noconfirm $PKGOPTS $pkglist

#-----------------------------------------------------------------------------
# Alpine Linux
#
# Update command: sudo apk update; sudo apk upgrade
# Find package providing file: apk info --who-owns /path/to/file
# Interactive package search: https://pkgs.alpinelinux.org/packages
#-----------------------------------------------------------------------------

elif [[ -f /etc/alpine-release ]]; then

    AL=$(sed /etc/alpine-release -e '/^[0-9][0-9]*\.[0-9]/!d' -e 's/^\([0-9][0-9]*\.[0-9][0-9]*\).*/\1/' | head -1)
    AL=$(( ${AL/.*/} * 100 + ${AL/*./} ))
    pkglist="bash coreutils diffutils procps util-linux linux-headers git git-lfs make cmake g++ dos2unix curl tar zip dpkg doxygen graphviz libedit-dev libusb-dev pcsc-lite-dev curl-dev libsrt-dev python3 openjdk11 dpkg"
    if [[ $AL -ge 316 ]]; then
        pkglist="$pkglist librist-dev"
    fi
    sudo sed -i '/http.*\/alpine\/v/s/^#//' /etc/apk/repositories
    sudo apk add $PKGOPTS $pkglist
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# Gentoo Linux
#
# Update command: sudo emerge --sync
# or (??): sudo emerge -auvND --with-bdeps=y world
# or (??): sudo emerge --update --deep --changed-use @world
# Find package providing file: equery b /path/to/file
#-----------------------------------------------------------------------------

elif [[ -f /etc/gentoo-release ]]; then

    pkglist="sys-devel/gcc dev-vcs/git dev-vcs/git-lfs dev-util/cmake app-text/dos2unix net-misc/curl app-arch/tar app-arch/zip app-arch/unzip app-doc/doxygen media-gfx/graphviz sys-kernel/linux-headers dev-libs/libedit dev-libs/libusb sys-apps/pcsc-lite net-libs/srt dev-lang/python dev-java/openjdk"
    sudo emerge -n $PKGOPTS $pkglist

fi
