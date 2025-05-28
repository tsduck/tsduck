#!/usr/bin/env bash
# Build the TSDuck extension .deb package for Ubuntu or Debian.

BUILDDIR=$(cd $(dirname $0); pwd)
ROOTDIR=$(cd "$BUILDDIR/.."; pwd)
INSTALLERDIR="$ROOTDIR/installers"

NAME=tsduck-extension-foo
VERSION="$(tsversion)"
MAJOR=$(sed <<<$VERSION -e 's/-.*//')
COMMIT=$(sed <<<$VERSION -e 's/.*-//')
ARCH=$(dpkg-architecture -qDEB_BUILD_ARCH)
TMPROOT="$INSTALLERDIR/tmp"

# Make an installation tree in a temporary directory.
rm -rf "$TMPROOT"
make -C "$ROOTDIR" -j4
make -C "$ROOTDIR" install "DESTDIR=$TMPROOT"

# Installed files by categories:
EXECS=$(cd "$TMPROOT"; find . -executable ! -type d ! -name '*.so' ! -name '*.dylib' -printf ' /%P')
SHLIBS=$(cd "$TMPROOT"; find . \( -name '*.so' -o -name '*.dylib' \) -printf ' /%P')
CONFIGS=$(cd "$TMPROOT"; find . \( -name '*.xml' -o -name '*.names' \) -printf ' /%P')

# Build the .deb package.
mkdir "$TMPROOT/DEBIAN"
sed -e "s|{{VERSION}}|$VERSION|g" \
    -e "s|{{ARCH}}|$ARCH|g" \
    "$BUILDDIR/tsduck-extension-foo.control" >"$TMPROOT/DEBIAN/control"
dpkg-deb --build --root-owner-group "$TMPROOT" "$INSTALLERDIR"
rm -rf "$TMPROOT"
