#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Sample script to build TSDuck packages for Red Hat and clones using Docker.
#
#-----------------------------------------------------------------------------

# Note: With RHEL 9.x, we need to setup GCC 13 before building. The installation
# of the GCC 13 packaging is part of install-prerequisites.sh.

docker create --name tsbuilder -it almalinux:latest
docker start tsbuilder
docker exec tsbuilder dnf update -y
docker exec tsbuilder dnf install -y sudo git
docker exec tsbuilder git clone --depth 1 https://github.com/tsduck/tsduck.git
docker exec tsbuilder tsduck/scripts/install-prerequisites.sh --allowerasing
docker exec tsbuilder bash -c "source /opt/rh/gcc-toolset-13/enable; make -C tsduck installer"
docker exec tsbuilder make -C tsduck installer-tarball INSTALLER_TARBALL=/tmp/tsduck.tgz
docker cp tsbuilder:/tmp/tsduck.tgz /tmp/tsduck.tgz
docker stop -t0 tsbuilder
docker rm tsbuilder

tar -xzf /tmp/tsduck.tgz
rm /tmp/tsduck.tgz
