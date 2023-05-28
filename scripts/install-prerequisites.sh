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
#  -n       : dry run, display list of packages, do not install them
#  --static : install static libraries for static build (when supported)
#  NOxxx=1  : deselect some dependencies, see below
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
#  - OpenBSD
#  - NetBSD
#  - DragonFlyBSD
#
#  The following options are the same symbols which can be used with make.
#  They disable a TSDuck feature and remove some dependencies. If specified
#  when calling this script, the corresponding prerequisites are not installed.
#  Some other make symbols such as "NOTEST" or "NODEKTEC" are silently ignored
#  when they have no impact on the list of installed packages. Just like with
#  make, this symbols may also be defined as environment variables.
#
#  - NOVATEK    : No Vatek-based device support.
#  - NOCURL     : No HTTP support, remove dependency to libcurl.
#  - NOPCSC     : No smartcard support, remove dependency to pcsc-lite.
#  - NOSRT      : No SRT support, remove dependency to libsrt.
#  - NORIST     : No RIST support, remove dependency to librist.
#  - NOEDITLINE : No interactive line editing, remove dependency to libedit.
#  - NOJAVA     : No support for Java applications.
#  - NODOXYGEN  : No production of doxygen docs, do not install doxygen.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
SCRIPTDIR=$(cd $(dirname $0); pwd)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Default options.
DRYRUN=false
STATIC=
PKGOPTS=()
PKGLIST=()

# Decode command line options.
while [[ $# -gt 0 ]]; do
    case "$1" in
        -n)       DRYRUN=true ;;
        --static) STATIC=true ;;
        NO*=*)    eval ${1/=*/}=true ;;
        *)        PKGOPTS+=("$1") ;;
    esac
    shift
done

[[ "${#PKGOPTS[@]}" -gt 0 ]] && echo "====> using packager options: ${PKGOPTS[*]}"

# System description. Versions are empty if lsb_release is not present.
SYSTEM=$(uname -s)
DISTRO=$(lsb_release -i 2>/dev/null | sed -e 's/.*:[\t ]*//')
MAJOR=$(lsb_release -r 2>/dev/null | sed -e 's/.*:[\t ]*//' -e 's/\..*//')
MINOR=$(lsb_release -r 2>/dev/null | sed -e '/\./!d' -e 's/.*:[\t ]*//' -e 's/.*\.//')
VERSION=$(( ${MAJOR:-0} * 100 + ${MINOR:-0} ))

#-----------------------------------------------------------------------------
# == macOS == (with HomeBrew as open source packager)
# Update command: brew update; brew upgrade
#-----------------------------------------------------------------------------

if [[ "$SYSTEM" == "Darwin" ]]; then

    PKGLIST+=(git git-lfs gnu-sed grep dos2unix coreutils python3)
    [[ -z $NORIST    ]] && PKGLIST+=(librist)
    [[ -z $NOSRT     ]] && PKGLIST+=(srt)
    [[ -z $NOVATEK   ]] && PKGLIST+=(libvatek)
    [[ -z $NOJAVA    ]] && PKGLIST+=(openjdk)
    [[ -z $NODOXYGEN ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

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
    brew "${PKGOPTS[@]}" install "${PKGLIST[@]}" || true
    # Make sure python3 is the default in Homebrew.
    BREW=$(brew --prefix)
    (cd $BREW/bin; ln -sf python3 python)
    # Register the openjdk jvm.
    if [[ ! -e /Library/Java/JavaVirtualMachines/openjdk.jdk ]]; then
        JDK=$BREW/opt/openjdk/libexec/openjdk.jdk
        if [[ -n $(find $JDK -name javac -perm +444 2>/dev/null) ]]; then
            sudo ln -sfn $JDK /Library/Java/JavaVirtualMachines/openjdk.jdk
        fi
    fi

#-----------------------------------------------------------------------------
# == FreeBSD ==
# Update command: sudo pkg update; sudo pkg upgrade
#-----------------------------------------------------------------------------

elif [[ "$SYSTEM" == "FreeBSD" ]]; then

    PKGLIST+=(git git-lfs curl zip bash gsed gnugrep gmake gtar unix2dos coreutils python)
    [[ -z $NOEDITLINE ]] && PKGLIST+=(libedit)
    [[ -z $NOPCSC     ]] && PKGLIST+=(pcsc-lite)
    [[ -z $NORIST     ]] && PKGLIST+=(librist)
    [[ -z $NOSRT      ]] && PKGLIST+=(srt)
    [[ -z $NOJAVA     ]] && PKGLIST+=(openjdk11)
    [[ -z $NODOXYGEN  ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo pkg install -y "${PKGOPTS[@]}" "${PKGLIST[@]}"

#-----------------------------------------------------------------------------
# == DragonFly BSD ==
# Update command: sudo pkg update; sudo pkg upgrade
#-----------------------------------------------------------------------------

elif [[ "$SYSTEM" == "DragonFly" ]]; then

    PKGLIST+=(git git-lfs curl zip bash gsed gnugrep gmake gtar unix2dos coreutils python)
    [[ -z $NOEDITLINE ]] && PKGLIST+=(libedit)
    [[ -z $NOPCSC     ]] && PKGLIST+=(pcsc-lite)
    [[ -z $NORIST     ]] && PKGLIST+=(librist)
    [[ -z $NOSRT      ]] && PKGLIST+=(srt)
    [[ -z $NOJAVA     ]] && PKGLIST+=(openjdk11)
    [[ -z $NODOXYGEN  ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo pkg install -y "${PKGOPTS[@]}" "${PKGLIST[@]}"

#-----------------------------------------------------------------------------
# == OpenBSD ==
# Update command: sudo pkg_add -u
#-----------------------------------------------------------------------------

elif [[ "$SYSTEM" == "OpenBSD" ]]; then

    disamb_pkg() { pkg_info -Q $1 | grep "^$1-[0-9]" | grep -v -e -static | sort | tail -1 | sed -e 's/ .*//'; }

    PKGLIST+=(git git-lfs curl zip bash gsed ggrep gmake $(disamb_pkg gtar) dos2unix coreutils $(disamb_pkg python))
    [[ -z $NOPCSC    ]] && PKGLIST+=(pcsc-lite)
    [[ -z $NOJAVA    ]] && PKGLIST+=($(disamb_pkg jdk))
    [[ -z $NODOXYGEN ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo pkg_add -I "${PKGOPTS[@]}" "${PKGLIST[@]}"
    sudo ln -sf python3 /usr/local/bin/python

#-----------------------------------------------------------------------------
# == NetBSD ==
# Update command: sudo pkgin update; sudo pkgin upgrade
# List package content: pkgin pkg-content <package-name>
#-----------------------------------------------------------------------------

elif [[ "$SYSTEM" == "NetBSD" ]]; then

    PKGLIST+=(git git-lfs curl mozilla-rootcerts zip bash gsed grep gmake gtar dos2unix coreutils python310 py310-expat)
    [[ -z $NOEDITLINE ]] && PKGLIST+=(editline)
    [[ -z $NOPCSC     ]] && PKGLIST+=(pcsc-lite)
    [[ -z $NOJAVA     ]] && PKGLIST+=(openjdk17)
    [[ -z $NODOXYGEN  ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo pkgin -y install "${PKGOPTS[@]}" "${PKGLIST[@]}"
    sudo /usr/pkg/sbin/mozilla-rootcerts install
    (cd /usr/pkg/bin; sudo ln -sf $(ls python* | grep '^python[0-9\.]*$' | gsort --version-sort | tail -1) python)

#-----------------------------------------------------------------------------
# == Ubuntu ==
# Update command: sudo apt update; sudo apt upgrade
# Find package providing file: dpkg -S /path/to/file
#-----------------------------------------------------------------------------

elif [[ "$DISTRO" == "Ubuntu" ]]; then

    PKGLIST+=(git git-lfs g++ cmake dos2unix curl tar zip linux-libc-dev dpkg-dev python3)
    [[ -z $NOEDITLINE                                      ]] && PKGLIST+=(libedit-dev)
    [[ -z $NOPCSC                                          ]] && PKGLIST+=(pcscd libpcsclite-dev)
    [[ -z $NOCURL && $MAJOR -le 17                         ]] && PKGLIST+=(libcurl3 libcurl3-dev)
    [[ -z $NOCURL && $MAJOR -gt 17                         ]] && PKGLIST+=(libcurl4 libcurl4-openssl-dev)
    [[ -z $NORIST && $VERSION -ge 2210                     ]] && PKGLIST+=(librist-dev)
    [[ -z $NOSRT && $VERSION -ge 1904 && $VERSION -lt 2010 ]] && PKGLIST+=(libsrt-dev)
    [[ -z $NOSRT && $VERSION -ge 2010                      ]] && PKGLIST+=(libsrt-openssl-dev)
    [[ -z $NOVATEK                                         ]] && PKGLIST+=(libusb-1.0-0-dev)
    [[ -z $NOJAVA                                          ]] && PKGLIST+=(default-jdk)
    [[ -z $NODOXYGEN                                       ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo apt update
    sudo apt install -y "${PKGOPTS[@]}" "${PKGLIST[@]}"
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# == Linux Mint ==
# Update command: sudo apt update; sudo apt upgrade
# Find package providing file: dpkg -S /path/to/file
#-----------------------------------------------------------------------------

elif [[ "$DISTRO" == "Linuxmint" ]]; then

    PKGLIST+=(git git-lfs g++ cmake dos2unix curl tar zip linux-libc-dev dpkg-dev python3)
    [[ -z $NOEDITLINE                              ]] && PKGLIST+=(libedit-dev)
    [[ -z $NOPCSC                                  ]] && PKGLIST+=(pcscd libpcsclite-dev)
    [[ -z $NOSRT && $MAJOR -ge 20 && $MAJOR -lt 21 ]] && PKGLIST+=(libsrt-dev)
    [[ -z $NOSRT && $MAJOR -ge 21                  ]] && PKGLIST+=(libsrt-openssl-dev)
    [[ -z $NOCURL                                  ]] && PKGLIST+=(libcurl4 libcurl4-openssl-dev)
    [[ -z $NOVATEK                                 ]] && PKGLIST+=(libusb-1.0-0-dev)
    [[ -z $NOJAVA                                  ]] && PKGLIST+=(default-jdk)
    [[ -z $NODOXYGEN                               ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo apt update
    sudo apt install -y "${PKGOPTS[@]}" "${PKGLIST[@]}"
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# == Debian, Raspbian (Raspberry Pi) ==
# Update command: sudo apt update; sudo apt upgrade
# Find package providing file: dpkg -S /path/to/file
#-----------------------------------------------------------------------------

elif [[ "$DISTRO" = "Debian" || "$DISTRO" = "Raspbian" ]]; then

    PKGLIST+=(git git-lfs g++ cmake dos2unix curl tar zip linux-libc-dev dpkg-dev python3)
    [[ -z $NOEDITLINE              ]] && PKGLIST+=(libedit-dev)
    [[ -z $NOPCSC                  ]] && PKGLIST+=(pcscd libpcsclite-dev)
    [[ -z $NOCURL && $MAJOR -le 9  ]] && PKGLIST+=(libcurl3 libcurl3-dev)
    [[ -z $NOCURL && $MAJOR -gt 9  ]] && PKGLIST+=(libcurl4 libcurl4-openssl-dev)
    [[ -z $NOSRT && $MAJOR -ge 11  ]] && PKGLIST+=(libsrt-openssl-dev)
    [[ -z $NORIST && $MAJOR -ge 12 ]] && PKGLIST+=(librist-dev)
    [[ -z $NOVATEK                 ]] && PKGLIST+=(libusb-1.0-0-dev)
    [[ -z $NOJAVA                  ]] && PKGLIST+=(default-jdk)
    [[ -z $NODOXYGEN               ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo apt update
    sudo apt install -y "${PKGOPTS[@]}" "${PKGLIST[@]}"
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# == Fedora ==
# Update command: sudo dnf update
# Find package providing file: rpm -qf /path/to/file
#-----------------------------------------------------------------------------

elif [[ -f /etc/fedora-release ]]; then

    FC=$(grep " release " /etc/fedora-release 2>/dev/null | sed -e 's/^.* release \([0-9\.]*\) .*$/\1/')

    PKGLIST+=(git git-lfs gcc-c++ cmake dos2unix curl tar zip kernel-headers libatomic rpmdevtools python3)
    [[ -z $NOEDITLINE           ]] && PKGLIST+=(libedit-devel)
    [[ -z $NOPCSC               ]] && PKGLIST+=(pcsc-tools pcsc-lite-devel)
    [[ -z $NORIST && $FC -ge 36 ]] && PKGLIST+=(librist-devel)
    [[ -z $NOSRT && $FC -ge 31  ]] && PKGLIST+=(srt-devel)
    [[ -z $NOCURL               ]] && PKGLIST+=(libcurl libcurl-devel)
    [[ -z $NOVATEK              ]] && PKGLIST+=(libusb1-devel)
    [[ -z $NOJAVA               ]] && PKGLIST+=(java-latest-openjdk-devel)
    [[ -z $NODOXYGEN            ]] && PKGLIST+=(doxygen graphviz)
    [[ -n $STATIC               ]] && PKGLIST+=(glibc-static libstdc++-static)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo dnf -y install "${PKGOPTS[@]}" "${PKGLIST[@]}"
    sudo alternatives --set python /usr/bin/python3

#-----------------------------------------------------------------------------
# == Red Hat, CentOS, Alma Linux, Rocky Linux ==
# Update command: sudo dnf update
# Find package providing file: rpm -qf /path/to/file
#-----------------------------------------------------------------------------

elif [[ -f /etc/redhat-release ]]; then

    EL=$(grep " release " /etc/redhat-release 2>/dev/null | sed -e 's/$/.99/' -e 's/^.* release \([0-9]*\.[0-9]*\).*$/\1/')
    EL=$(( ${EL/.*/} * 100 + ${EL/*./} ))

    PKGLIST+=(git git-lfs gcc-c++ cmake dos2unix curl tar zip kernel-headers libatomic rpmdevtools python3)
    [[ -z $NOEDITLINE            ]] && PKGLIST+=(libedit-devel)
    [[ -z $NOPCSC                ]] && PKGLIST+=(pcsc-lite pcsc-lite-devel)
    [[ -z $NORIST && $EL -ge 902 ]] && PKGLIST+=(librist-devel)
    [[ -z $NOSRT && $EL -ge 802  ]] && PKGLIST+=(srt-devel)
    [[ -z $NOCURL                ]] && PKGLIST+=(libcurl libcurl-devel)
    [[ -z $NOVATEK               ]] && PKGLIST+=(libusbx-devel)
    [[ -z $NOJAVA && $EL -lt 900 ]] && PKGLIST+=(java-latest-openjdk-devel)
    [[ -z $NOJAVA && $EL -ge 900 ]] && PKGLIST+=(java-17-openjdk-devel)
    [[ -z $NODOXYGEN             ]] && PKGLIST+=(doxygen graphviz)
    [[ -n $STATIC                ]] && PKGLIST+=(glibc-static libstdc++-static)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    if [[ $EL -lt 800 ]]; then
        sudo yum -y install "${PKGOPTS[@]}" epel-release
        sudo yum -y install "${PKGOPTS[@]}" "${PKGLIST[@]}"
    elif [[ $EL -lt 803 ]]; then
        sudo dnf -y config-manager --set-enabled PowerTools
        sudo dnf -y install "${PKGOPTS[@]}" epel-release
        sudo dnf -y install "${PKGOPTS[@]}" "${PKGLIST[@]}"
    elif [[ $EL -lt 900 ]]; then
        sudo dnf -y config-manager --set-enabled powertools
        sudo dnf -y install "${PKGOPTS[@]}" epel-release
        sudo dnf -y install "${PKGOPTS[@]}" "${PKGLIST[@]}"
    else
        sudo dnf -y config-manager --set-enabled plus
        sudo dnf -y config-manager --set-enabled crb
        sudo dnf -y install "${PKGOPTS[@]}" epel-release
        sudo dnf -y install "${PKGOPTS[@]}" "${PKGLIST[@]}"
    fi
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# == openSUSE ==
# Update command: sudo zypper update -y -l
# Find package providing file: zypper search --provides /path/to/file
# Search package: zypper search partial-package-name
#-----------------------------------------------------------------------------

elif [[ -f /etc/os-release ]] && grep -q -i '^ID.*suse' /etc/os-release; then

    # OpenSuse Leap has a version. OpenSuse Tumbleweed is a rolling upgrde and does not have a version.
    OS=$(grep '^VERSION=' /etc/os-release | head -1 | sed -e 's/.*="//' -e 's/".*//')
    [[ $OS == *.* ]] && OS=$(( ${OS/.*/} * 100 + ${OS/*./} )) || OS=

    PKGLIST+=(git git-lfs make gcc-c++ cmake dos2unix curl tar zip linux-glibc-devel rpmdevtools python3)
    [[ -z $NOEDITLINE                           ]] && PKGLIST+=(libedit-devel)
    [[ -z $NOPCSC                               ]] && PKGLIST+=(pcsc-tools pcsc-lite-devel)
    [[ -z $NORIST && ( -z $OS || $OS -ge 1505 ) ]] && PKGLIST+=(librist-devel)
    [[ -z $NOSRT                                ]] && PKGLIST+=(srt-devel)
    [[ -z $NOCURL                               ]] && PKGLIST+=(libcurl-devel)
    [[ -z $NOVATEK                              ]] && PKGLIST+=(libusb-1_0-devel)
    [[ -z $NOJAVA                               ]] && PKGLIST+=(java-11-openjdk-devel)
    [[ -z $NODOXYGEN                            ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo zypper install -y -l "${PKGOPTS[@]}" "${PKGLIST[@]}"
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# == Arch Linux ==
# Update command: sudo pacman -Syu
# Find package providing file: pacman -Qo /path/to/file
# Search package: pacman -Ss partial-package-name
#-----------------------------------------------------------------------------

elif [[ -f /etc/arch-release ]]; then

    PKGLIST+=(git git-lfs make gcc cmake dos2unix core/which inetutils net-tools curl tar zip linux-api-headers python)
    [[ -z $NOEDITLINE ]] && PKGLIST+=(libedit)
    [[ -z $NOPCSC     ]] && PKGLIST+=(pcsclite)
    [[ -z $NOSRT      ]] && PKGLIST+=(srt)
    [[ -z $NOVATEK    ]] && PKGLIST+=(libusb)
    [[ -z $NOJAVA     ]] && PKGLIST+=(jdk-openjdk)
    [[ -z $NODOXYGEN  ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo pacman -Sy --noconfirm "${PKGOPTS[@]}" "${PKGLIST[@]}"

#-----------------------------------------------------------------------------
# == Alpine Linux ==
# Update command: sudo apk update; sudo apk upgrade
# Find package providing file: apk info --who-owns /path/to/file
# Interactive package search: https://pkgs.alpinelinux.org/packages
#-----------------------------------------------------------------------------

elif [[ -f /etc/alpine-release ]]; then

    AL=$(sed /etc/alpine-release -e '/^[0-9][0-9]*\.[0-9]/!d' -e 's/^\([0-9][0-9]*\.[0-9][0-9]*\).*/\1/' | head -1)
    AL=$(( ${AL/.*/} * 100 + ${AL/*./} ))

    PKGLIST+=(bash coreutils diffutils procps util-linux linux-headers git git-lfs make cmake g++ dos2unix curl tar zip dpkg python3)
    [[ -z $NOEDITLINE            ]] && PKGLIST+=(libedit-dev)
    [[ -z $NOPCSC                ]] && PKGLIST+=(pcsc-lite-dev)
    [[ -z $NORIST && $AL -ge 316 ]] && PKGLIST+=(librist-dev)
    [[ -z $NOSRT                 ]] && PKGLIST+=(libsrt-dev)
    [[ -z $NOCURL                ]] && PKGLIST+=(curl-dev)
    [[ -z $NOVATEK               ]] && PKGLIST+=(libusb-dev)
    [[ -z $NOJAVA                ]] && PKGLIST+=(openjdk11)
    [[ -z $NODOXYGEN             ]] && PKGLIST+=(doxygen graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo sed -i '/http.*\/alpine\/v/s/^#//' /etc/apk/repositories
    sudo apk add "${PKGOPTS[@]}" "${PKGLIST[@]}"
    sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 2

#-----------------------------------------------------------------------------
# == Gentoo Linux ==
# Update command: sudo emerge --sync
# or (??): sudo emerge -auvND --with-bdeps=y world
# or (??): sudo emerge --update --deep --changed-use @world
# Find package providing file: equery b /path/to/file
#-----------------------------------------------------------------------------

elif [[ -f /etc/gentoo-release ]]; then

    PKGLIST+=(sys-devel/gcc dev-vcs/git dev-vcs/git-lfs dev-util/cmake app-text/dos2unix net-misc/curl app-arch/tar app-arch/zip app-arch/unzip sys-kernel/linux-headers dev-lang/python)
    [[ -z $NOEDITLINE ]] && PKGLIST+=(dev-libs/libedit)
    [[ -z $NOPCSC     ]] && PKGLIST+=(sys-apps/pcsc-lite)
    [[ -z $NOSRT      ]] && PKGLIST+=(net-libs/srt)
    [[ -z $NOVATEK    ]] && PKGLIST+=(dev-libs/libusb)
    [[ -z $NOJAVA     ]] && PKGLIST+=(dev-java/openjdk)
    [[ -z $NODOXYGEN  ]] && PKGLIST+=(app-doc/doxygen media-gfx/graphviz)

    echo "Packages: ${PKGLIST[*]}"
    $DRYRUN && exit 0

    sudo emerge -n "${PKGOPTS[@]}" "${PKGLIST[@]}"

fi
