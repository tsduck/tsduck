#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Rebuild a FreeBSD Port package for TSDuck using poudriere.
#  Option -d is used to delete a previously existing jail for builds.
#  By default, reuse a jail and its builds when they exist.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE)/../..; pwd)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }
info() { echo >&2 "$SCRIPT: $*"; }
usage() { echo >&2 "syntax: $SCRIPT [-d]"; exit 1; }

PORTROOT=/usr/ports
PORTBRANCH=main
PRODUCT=tsduck
CATEGORY=multimedia
PACKNAME=$CATEGORY/$PRODUCT
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

# Update /usr/port work area.
(
    cd $PORTROOT
    git checkout $PORTBRANCH --quiet || exit
    git pull --quiet || exit
    [[ -f $MAKEFILE ]] || error "no port defined for $PACKNAME"
)

# Build TSDuck version from makefile.
DISTVERSION=$(grep -w DISTVERSION $MAKEFILE | sed 's/.*=[ \t]*//' | head -1)
DISTVERSIONSUFFIX=$(grep -w DISTVERSIONSUFFIX $MAKEFILE | sed 's/.*=[ \t]*//' | head -1)
PORTREVISION=$(grep -w PORTREVISION $MAKEFILE | sed 's/.*=[ \t]*//' | head -1)
[[ ${PORTREVISION:-0} == 0 ]] && PORTREVISION= || PORTREVISION=_$PORTREVISION
VERSION=${DISTVERSION}${PORTREVISION}
info "building $PRODUCT version $VERSION"

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
    ! grep -q '^# *ZPOOL=' $PCONF && error "configure ZPOOL in $PCONF"
    info "configuring ZPOOL to default"
    sudo sed -I .backup 's/^# *ZPOOL=/ZPOOL=/' $PCONF
fi
if ! grep -q '^FREEBSD_HOST=' $PCONF; then
    sudo sed -I .backup '$a\'$'\n''FREEBSD_HOST=https://download.FreeBSD.org' $PCONF
elif grep -q '^FREEBSD_HOST=_PROTO_' $PCONF; then
    sudo sed -I .backup 's/^FREEBSD_HOST=_PROTO_.*$/FREEBSD_HOST=https:\/\/download.FreeBSD.org/' $PCONF
fi
if grep -q '^# *ALLOW_MAKE_JOBS=' $PCONF; then
    sudo sed -I .backup 's/^# *ALLOW_MAKE_JOBS=.*$/ALLOW_MAKE_JOBS=yes/' $PCONF
elif ! grep -q '^ALLOW_MAKE_JOBS=' $PCONF; then
    sudo sed -I .backup '$a\'$'\n''ALLOW_MAKE_JOBS=yes' $PCONF
fi

# Create jail to build TSDuck.
if poudriere jail -l -q -n | grep -q "^$JAILNAME\$" && $DELETE_JAIL; then
    info "deleting existing jail $JAILNAME"
    sudo poudriere jail -d -y -j $JAILNAME
fi
if ! poudriere jail -l -q -n | grep -q "^$JAILNAME\$"; then
    info "creating jail $JAILNAME"
    sudo poudriere jail -c -j $JAILNAME -v $SYSVER -a $(uname -m).$(uname -p)
fi

# Get port tree.
[[ -d /usr/local/poudriere/ports/default/$PACKNAME ]] || sudo poudriere ports -c -p default

# Forces more compilations than processors.
NPROC=$(sysctl -n hw.ncpu)
NPROC=$(( $NPROC + $NPROC/2 ))
echo "MAKE_JOBS_NUMBER=$NPROC" | sudo tee /usr/local/etc/poudriere.d/${JAILNAME}-make.conf

# Build TSDuck package.
sudo poudriere bulk -j $JAILNAME -p default $PACKNAME

# Check and get built package.
PKGDIR=/usr/local/poudriere/data/packages/${JAILNAME}-default/All
PKGOUT=$ROOTDIR/pkg/installers/${PRODUCT}-${VERSION}.${JAILNAME}.pkg
[[ -e $PKGDIR/${PRODUCT}-${VERSION}.pkg ]] || error "${PRODUCT}-${VERSION}.pkg not found in $PKGDIR"
cp $PKGDIR/${PRODUCT}-${VERSION}.pkg $PKGOUT
ls -l $PKGOUT

# Note: to list the content of the package file, use: pkg info -l -F filename.pkg
