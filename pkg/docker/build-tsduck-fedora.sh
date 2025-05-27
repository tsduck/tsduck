#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Sample script to build TSDuck packages for Fedora using Docker.
#
#-----------------------------------------------------------------------------

docker create --name tsbuilder -it fedora:latest
docker start tsbuilder
docker exec tsbuilder dnf update -y
docker exec tsbuilder dnf install -y sudo git
docker exec tsbuilder git clone --depth 1 https://github.com/tsduck/tsduck.git
docker exec tsbuilder tsduck/scripts/install-prerequisites.sh --allowerasing
docker exec tsbuilder make -C tsduck installer
docker exec tsbuilder make -C tsduck installer-tarball INSTALLER_TARBALL=/tmp/tsduck.tgz
docker cp tsbuilder:/tmp/tsduck.tgz /tmp/tsduck.tgz
docker stop -t0 tsbuilder
docker rm tsbuilder

tar -xzf /tmp/tsduck.tgz
rm /tmp/tsduck.tgz
