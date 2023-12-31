#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Shell script to build the documentation using Doxygen.
#
#  If option --update-doxygen is specified and the installed version of
#  doxygen is obsolete (meaning has bugs when processing TSDuck doc),
#  a more recent version of doxygen is rebuilt first.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Get the project directories.
ROOTDIR=$(cd $(dirname ${BASH_SOURCE[0]})/..; pwd)
BINDIR="$ROOTDIR/bin"
DOXYDIR="$BINDIR/doxy"
DOCDIR="$ROOTDIR/doc"
SRCDIR="$ROOTDIR/src/libtsduck"

# Get doxygen version.
get-doxy-version() { doxygen --version 2>/dev/null; }
doxy-version-name() { local a=${1/ */}; echo ${a//./_}; }
doxy-version-int() { local a=(${1//./ }); echo $(( ${a[0]} * 10000 + ${a[1]} * 100 + ${a[2]} )); }

DOXY_VERSION=$(get-doxy-version)
[[ -z $DOXY_VERSION ]] && error "doxygen not installed"

# Minimum, maximum and preferred doxygen versions if update is required.
DOXY_MINVERSION=1.9.8
DOXY_MAXVERSION=  # can be empty
DOXY_PREFVERSION=1.9.8

if [[ $(doxy-version-int $DOXY_VERSION) -lt $(doxy-version-int $DOXY_MINVERSION) ||
      ( -n $DOXY_MAXVERSION && $(doxy-version-int $DOXY_VERSION) -gt $(doxy-version-int $DOXY_MAXVERSION) ) ]]
then
    echo "-- Obsolete or buggy Doxygen version $DOXY_VERSION installed"
    if [[ "$1" != "--update-doxygen" ]]; then
        echo "-- Consider using --update-doxygen"
    else
        NEWDOXY_DIR="$BINDIR/doxygen"
        if [[ ! -x "$NEWDOXY_DIR/build/bin/doxygen" ]]; then
            echo "-- Downloading and rebuilding version $DOXY_PREFVERSION"
            NAME=$(doxy-version-name $DOXY_PREFVERSION)
            mkdir -p "$NEWDOXY_DIR"
            curl -sL https://github.com/doxygen/doxygen/archive/Release_$NAME.tar.gz | tar xzf - -C "$NEWDOXY_DIR"
            pushd "$NEWDOXY_DIR"
            # On macOS, get the Homebrew-installed bison and flex
            if [[ $(uname -s) == Darwin && -n $(which brew 2>/dev/null) ]]; then
                for cmd in bison flex; do
                    getbrew() { find /usr/local /opt/homebrew -type f -perm +111 -name $1 2>/dev/null | tail -1; }
                    FILE=$(getbrew $cmd)
                    if [[ -z $FILE ]]; then
                        brew install $cmd
                        FILE=$(getbrew $cmd)
                    fi
                    [[ -n $FILE ]] && export PATH=$(dirname $FILE):$PATH
                done
            fi
            cmake -G "Unix Makefiles" -S doxygen-Release_$NAME -B build
            make -C build
            popd
        fi
        export PATH="$NEWDOXY_DIR/build/bin:$PATH"
        DOXY_VERSION=$(get-doxy-version)
        echo "-- Now using Doxygen $DOXY_VERSION"
    fi
fi

# Make sure that the output directory is created (doxygen does not create parent directories).
mkdir -p "$DOXYDIR"

# Environment variables, used in Doxyfile.
export TS_FULL_VERSION=$("$ROOTDIR/scripts/get-version-from-sources.py")
export DOT_PATH=$(which dot 2>/dev/null)
[[ -n "DOT_PATH" ]] && export HAVE_DOT=YES || export HAVE_DOT=NO
export DOXY_INCLUDE_PATH=$(find "$SRCDIR" -type d | tr '\n' ' ')

# Generate a summary file of all signalization.
"$ROOTDIR/src/doc/signalization-gen.py"

# Run doxygen.
cd "$DOCDIR"
doxygen

# Delete empty subdirectories (many of them created for nothing in case of hierachical output).
find "$DOXYDIR" -type d -empty -delete
