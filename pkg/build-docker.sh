#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script builds the TSDuck installers for a Linux distro using docker.
#  The required parameter is the name of the Docker base image to use.
#  The type of distro is derived from that name, so it must be explicit.
#  The installers are dropped in the "installers" directory.
#
#  Typical docker image names to use:
#  - ubuntu:latest
#  - ubuntu:25.04
#  - debian:latest
#  - debian:trixie
#  - fedora:latest
#  - almalinux:latest
#  - almalinux:10
#  - rockylinux:9 (no :latest available)
#
#  Full list of available containers images here:
#  https://hub.docker.com/search?categories=Operating+Systems
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE)/..; pwd)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Analyze base image name and find distro type.
IMAGE="$1"
case "$IMAGE" in
    ubuntu*) DISTRO=ubuntu ;;
    debian*) DISTRO=debian ;;
    fedora*) DISTRO=fedora ;;
    redhat*|rh*|alma*|rocky*) DISTRO=redhat ;;
    "") error "no docker image specified" ;;
    *) error "unknown docker image type: $IMAGE" ;;
esac

# Build a source tarball of TSDuck.
TMPROOT="$ROOTDIR/bin/tmp"
SOURCES="$TMPROOT/tsduck.tgz"
mkdir -p "$TMPROOT"
make -C "$ROOTDIR" tarball SOURCE_TARBALL="$SOURCES" || exit

# Use that name for the container.
CONT=tsbuilder-$DISTRO

# Delete previous spurious versions of that container.
docker ps | grep -q ' '$CONT'$' && docker kill $CONT
docker ps -a | grep -q ' '$CONT'$' && docker rm $CONT

# Prepare base image.
docker create --name $CONT -it "$IMAGE" || exit
docker start $CONT

# Update the system in the base image.
case $DISTRO in
    debian|ubuntu)
        docker exec $CONT apt update
        docker exec $CONT apt upgrade -y
        docker exec $CONT apt install -y sudo lsb-release git tar
        PACKOPT=
        ;;
    redhat|fedora)
        docker exec $CONT dnf update -y
        docker exec $CONT dnf install -y sudo git tar
        PACKOPT=--allowerasing
        ;;
esac

# Import the source in the container and install prerequisites.
docker cp "$SOURCES" $CONT:/tsduck.tgz
docker exec $CONT tar -xzf /tsduck.tgz -C /
docker exec $CONT /tsduck/scripts/install-prerequisites.sh $PACKOPT

# On Red Hat 9 systems, the default version of GCC is 11. The install-prerequisites.sh
# script installs a newer version (typically GCC 13) which needs a source'd script to
# be executed prior to compilation.
GCC_ENABLE_SCRIPT=
if [[ $DISTRO == redhat ]]; then
    GCC_MAJOR=$(docker exec $CONT gcc --version | sed -e '2,$d' -e 's/([^(]*)//g' -e 's/^[^0-9]*//' -e 's/[^0-9].*$//')
    if [[ -n $GCC_MAJOR && $GCC_MAJOR -lt 13 ]]; then
        GCC_ENABLE_SCRIPT=$(docker exec $CONT bash -c "ls /opt/rh/gcc-*/enable 2>/dev/null" | grep -e -13/ -e -14/ -e -15/ | tail -1)
    fi
fi

# Build TSDuck installers in the container.
if [[ -z $GCC_ENABLE_SCRIPT ]]; then
    docker exec $CONT make -C tsduck installer
else
    docker exec $CONT bash -c "source $GCC_ENABLE_SCRIPT; make -C tsduck installer"
fi

# Install TSDuck in the container and display its build characteristics.
docker exec $CONT make -C tsduck install-installer
docker exec $CONT tsversion --version=all

# Collect the installer files.
docker exec $CONT make -C tsduck installer-tarball INSTALLER_TARBALL=/tmp/tsduck-$DISTRO.tgz
docker cp $CONT:/tmp/tsduck-$DISTRO.tgz "$TMPROOT/tsduck-$DISTRO.tgz"
tar -xvzf "$TMPROOT/tsduck-$DISTRO.tgz" -C "$ROOTDIR/pkg/installers"

# Cleanup the container.
docker stop -t0 $CONT
docker rm $CONT
