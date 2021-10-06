#!/usr/bin/env bash
# Build the TSDuck extension RPM's for Fedora, Red Hat, CentOS.

BUILDDIR=$(cd $(dirname $0); pwd)
ROOTDIR=$(cd "$BUILDDIR/.."; pwd)
INSTALLERDIR="$ROOTDIR/installers"
RPMBUILDROOT="$HOME/rpmbuild"

NAME=tsduck-extension-foo
SPECFILE="$BUILDDIR/tsduck-extension-foo.spec"
VERSION="$(tsversion)"
MAJOR=$(sed <<<$VERSION -e 's/-.*//')
COMMIT=$(sed <<<$VERSION -e 's/.*-//')
TARNAME="${NAME}-${VERSION}"
TARFILE="$INSTALLERDIR/$TARNAME.tgz"
TMPROOT="$INSTALLERDIR/tmp"

# Build a source tarball
rm -rf "$TMPROOT"
mkdir -p "$TMPROOT/$TARNAME"
tar -C "$ROOTDIR" \
    --exclude .git \
    --exclude tmp \
    --exclude *.tgz \
    --exclude *.rpm \
    --exclude *.deb \
    --exclude *.exe \
    --exclude *.ts \
    -cpf - . |
    tar -C "$TMPROOT/$TARNAME" -xpf -
make -C "$TMPROOT/$TARNAME" clean
tar -C "$TMPROOT" -czf "$TARFILE" -p --owner=0 --group=0 "$TARNAME"
rm -rf "$TMPROOT"

# Linux distro version.
FC_DISTRO=$(grep " release " /etc/fedora-release 2>/dev/null | sed -e 's/^.* release \([0-9]*\).*$/\1/')
EL_DISTRO=$(grep " release " /etc/redhat-release 2>/dev/null | sed -e 's/^.* release \([0-9]*\).*$/\1/')
if [[ -n "$FC_DISTRO" ]]; then
    DISTRO=".fc$FC_DISTRO"
elif [[ -n "$EL_DISTRO" ]]; then
    DISTRO=".el$EL_DISTRO"
else
    DISTRO=""
fi

# Build RPM.
[[ -d "$RPMBUILDROOT" ]] || rpmdev-setuptree
cp -f "$TARFILE" "$RPMBUILDROOT/SOURCES/"
QA_RPATHS=$((0xFFFF)) rpmbuild -ba --clean -D "version $MAJOR" -D "commit $COMMIT" -D "distro $DISTRO" "$SPECFILE"
cp -uf "$RPMBUILDROOT"/RPMS/*/${NAME}-${VERSION}${DISTRO}.*.rpm "$INSTALLERDIR"
cp -uf "$RPMBUILDROOT"/SRPMS/${NAME}-${VERSION}${DISTRO}.src.rpm "$INSTALLERDIR"
