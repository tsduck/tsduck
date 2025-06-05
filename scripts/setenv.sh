#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script builds the name of the directory which contains binaries.
#  The typical usage is to 'source' it: it adds the binary directory to
#  the path. Other options:
#
#     --display : only display the binary directory, don't set PATH
#     --all : display all current variables, don't set PATH
#     --debug : use debug build
#     --bin dirname : use that directory as binary
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename "${BASH_SOURCE[0]}")
usage() { echo >&2 "syntax: $SCRIPT [--bin dir] [--debug] [--display] [-all]"; exit 1; }

# Default options.
TARGET=release
SHOW_PATH=false
SHOW_ALL=false
BINDIR=

# Decode command line options.
while [[ $# -gt 0 ]]; do
    case "$1" in
        --all)
            SHOW_ALL=true
            ;;
        --bin)
            [[ $# -gt 1 ]] || usage; shift
            BINDIR=$(cd "$1" && pwd)
            ;;
        --debug)
            TARGET=debug
            ;;
        --display)
            SHOW_PATH=true
            ;;
        -h|--help)
            usage
            ;;
    esac
    shift
done

# Build binary directory.
ROOTDIR=$(cd $(dirname "${BASH_SOURCE[0]}")/..; pwd)
TSPYDIR="$ROOTDIR/src/libtsduck/python"
if [[ -z "$BINDIR" ]]; then
    ARCH=$(uname -m | sed -e 's/i.86/i386/' -e 's/^arm.*$/arm/' -e 's/ /-/g')
    HOST=$(hostname | sed -e 's/\..*//')
    BINDIR="$ROOTDIR/bin/$TARGET-$ARCH-$HOST"
fi

# Safely add a directory at the beginning of a path.
addpath() {
    local varname=$1
    local bindir="$2"
    local npath=":${!varname}:"
    npath="${npath//:$bindir:/:}"
    npath="${npath//::/:}"
    npath="${npath/#:/}"
    npath="${npath/%:/}"
    [[ -z "$npath" ]] && export $varname="$bindir:" || export $varname="$bindir:$npath:"
}

# Display or set path.
if $SHOW_PATH; then
    echo "$BINDIR"
elif $SHOW_ALL; then
    echo "PATH=$PATH"
    echo "TSPLUGINS_PATH=$TSPLUGINS_PATH"
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    echo "PYTHONPATH=$PYTHONPATH"
    echo "CLASSPATH=$CLASSPATH"
else
    addpath PATH "$BINDIR"
    addpath TSPLUGINS_PATH "$BINDIR"
    addpath LD_LIBRARY_PATH "$BINDIR"
    addpath PYTHONPATH "$TSPYDIR"
    addpath CLASSPATH "$BINDIR/tsduck.jar"
    # For macOS only: LD_LIBRARY_PATH is not passed to shell-scripts for security reasons.
    # Define a backup version which can be explicitly checked in scripts (typically Python bindings).
    export LD_LIBRARY_PATH2="$LD_LIBRARY_PATH"
fi

# Make sure to exit with success status
true
