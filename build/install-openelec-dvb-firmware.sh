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
#  This script downloads and installs all DVB firmware for Linux which are
#  provided by the OpenELEC project. This project provides a lot of firmware
#  files for DVB devices which are not otherwise found in standard packages
#  such as linux-firmware. The firmware files are directly downloaded from
#  the GitHut repository of the OpenELEC project.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# URL of OpenELEC firmware repository in tarball format.
URL=https://github.com/OpenELEC/dvb-firmware/tarball/master

# Firmware destination directory. Use SYSROOT to redirect to another root.
FWDIR="$SYSROOT/lib/firmware"

# Check that the firmware target directory exists.
[[ -d "$FWDIR" ]] || error "directory $FWDIR does not exist, maybe not the right system"

# Download and install in one pass.
# Exclude the first two levels of directory (down to "firmware" directory).
curl -sL $URL | tar -C "$FWDIR" -xzpf - --strip-components=2 'OpenELEC-dvb-firmware-*/firmware'
