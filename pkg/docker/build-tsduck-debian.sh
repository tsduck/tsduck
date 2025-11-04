#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Sample script to build TSDuck packages for Debian using Docker.
#
#-----------------------------------------------------------------------------

# Note: We use Debian trixie (future Debian 13) because Debian 12 contains
# obsolete compilers only, which are not C++20 conformant.

docker create --name tsbuilder -it debian:latest
docker start tsbuilder
docker exec tsbuilder apt update
docker exec tsbuilder apt upgrade -y
docker exec tsbuilder apt install -y sudo lsb-release git
docker exec tsbuilder git clone --depth 1 https://github.com/tsduck/tsduck.git
docker exec tsbuilder tsduck/scripts/install-prerequisites.sh
docker exec tsbuilder make -C tsduck installer
docker exec tsbuilder make -C tsduck installer-tarball INSTALLER_TARBALL=/tmp/tsduck.tgz
docker cp tsbuilder:/tmp/tsduck.tgz /tmp/tsduck.tgz
docker stop -t0 tsbuilder
docker rm tsbuilder

tar -xzf /tmp/tsduck.tgz
rm /tmp/tsduck.tgz
