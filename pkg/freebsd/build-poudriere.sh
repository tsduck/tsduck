#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Rebuild a FreeBSD Port package for TSDuck.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }
info() { echo >&2 "$SCRIPT: $*"; }
usage() { echo >&2 "syntax: $SCRIPT [-d]"; exit 1; }

PORTROOT=/usr/ports
PACKNAME=multimedia/tsduck
MAKEFILE=$PORTROOT/$PACKNAME/Makefile

# Default values for command line options.
DELETE_JAIL=false

# Decode command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -d) DELETE_JAIL=true ;;
        *) usage ;;
    esac
    shift
done

# Verify that /usr/ports is configured as a git repo for ports.
[[ -d $PORTROOT/.git ]] || error "$PORTROOT is not a git work area"
[[ -f $MAKEFILE ]] || error "no port defined for $PACKNAME"

# Build TSDuck version from makefile.
DISTVERSION=$(grep -w DISTVERSION $MAKEFILE | sed 's/.*=[ \t]*//' | head -1)
DISTVERSIONSUFFIX=$(grep -w DISTVERSIONSUFFIX $MAKEFILE | sed 's/.*=[ \t]*//' | head -1)
PORTREVISION=$(grep -w PORTREVISION $MAKEFILE | sed 's/.*=[ \t]*//' | head -1)
[[ ${PORTREVISION:-0} == 0 ]] && PORTREVISION= || PORTREVISION=_$PORTREVISION
VERSION=${DISTVERSION}${DISTVERSIONSUFFIX}${PORTREVISION}
info "building tsduck version $VERSION"

# The jail name contains the FreeBSD version and processor architecture.
SYSVER=$(freebsd-version | cut -d- -f1,2)
JAILNAME="$(freebsd-version | sed 's/\([0-9]*\)\.\([0-9]*\)-.*/\1\2/')$(uname -m)"
info "system version: $SYSVER, jail name: $JAILNAME"

# Install and configure poudriere if necessary.
if [[ -z $(which poudriere 2>/dev/null) ]]; then
    info "installing poudiere ..."
    sudo pkg install -y poudriere
fi
PCONF=/usr/local/etc/poudriere.conf
if [[ ! -e $PCONF ]]; then
    [[ -e $PCONF.sample ]] || error "no $PCONF or $PCONF.sample found"
    info "creating $PCONF"
    sudo cp $PCONF.sample $PCONF || exit
fi
if ! grep -q '^ZPOOL=' $PCONF; then
    ! grep -q '^#ZPOOL=' $PCONF && error "configure ZPOOL in $PCONF"
    info "configuring ZPOOL to default"
    sudo sed -I .backup 's/^#ZPOOL=/ZPOOL=/' $PCONF
fi

# Create jail to build TSDuck.
if poudriere jail -l -q -n | grep -q "^$JAILNAME\$" && $DELETE_JAIL; then
    echo "deleting existing jail $JAILNAME"
    sudo poudriere jail -d -y -j $JAILNAME
fi
if ! poudriere jail -l -q -n | grep -q "^$JAILNAME\$"; then
    echo "creating jail $JAILNAME"
    sudo poudriere jail -c -j $JAILNAME -v $SYSVER -a $(uname -m).$(uname -p)
fi

# Get port tree.
[[ -d /usr/local/poudriere/ports/default/$PACKNAME ]] || sudo poudriere ports -c -p default
# sudo poudriere options -j $JAILNAME -p default $PACKNAME

# Build TSDuck package.
sudo poudriere bulk -j $JAILNAME -p default $PACKNAME
