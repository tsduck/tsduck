#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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
#  This script builds the Qt Creator project files for all TSDuck commands
#  and plugins. Each time a new source file appears for a new command or
#  plugin, a specific directory and project file shall be created to make it
#  visible to Qt Creator (used as a C++ IDE, event without using Qt). This is
#  a repetitive task which can be automated. 
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename ${BASH_SOURCE[0]} .sh)

error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Get the project directories.
BUILDDIR=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)
ROOTDIR=$(cd "$BUILDDIR/.."; pwd)
SRCDIR="$ROOTDIR/src"
QTDIR="$BUILDDIR/qtcreator"

# List of tools and plugins.
TOOLS=$(cd "$SRCDIR/tstools"; ls ts*.cpp 2>/dev/null | sed -e 's/\.cpp$//')
PLUGINS=$(cd "$SRCDIR/tsplugins"; ls tsplugin_*.cpp 2>/dev/null | sed -e 's/^tsplugin_//' -e 's/\.cpp$//')

# Build QT Creator project files.
for name in $TOOLS; do
    mkdir -p "$QTDIR/$name"
    cat <<EOF >"$QTDIR/$name/$name.pro"
CONFIG += tstool
TARGET = $name
include(../tsduck.pri)
EOF
done

for name in $PLUGINS; do
    mkdir -p "$QTDIR/tsplugin_$name"
    cat <<EOF >"$QTDIR/tsplugin_$name/tsplugin_$name.pro"
CONFIG += tsplugin
TARGET = tsplugin_$name
include(../tsduck.pri)
EOF
done
