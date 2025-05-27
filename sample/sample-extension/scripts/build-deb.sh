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
TEMPDIR="$INSTALLERDIR/tmp"

# Make an installation tree in a temporary directory.
rm -rf "$TEMPDIR"
make -C "$ROOTDIR" -j4
make -C "$ROOTDIR" install "DESTDIR=$TEMPDIR"

# Installed files by categories:
EXECS=$(cd "$TEMPDIR"; find . -executable ! -type d ! -name '*.so' ! -name '*.dylib' -printf ' /%P')
SHLIBS=$(cd "$TEMPDIR"; find . \( -name '*.so' -o -name '*.dylib' \) -printf ' /%P')
CONFIGS=$(cd "$TEMPDIR"; find . \( -name '*.xml' -o -name '*.names' \) -printf ' /%P')

# Build the .deb package.
mkdir "$TEMPDIR/DEBIAN"
sed -e "s|{{VERSION}}|$VERSION|g" \
    -e "s|{{ARCH}}|$ARCH|g" \
    "$BUILDDIR/tsduck-extension-foo.control" >"$TEMPDIR/DEBIAN/control"
dpkg-deb --build --root-owner-group "$TEMPDIR" "$INSTALLERDIR"
rm -rf "$TEMPDIR"
