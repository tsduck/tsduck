#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Create a pull request on HomeBrew for the latest version of TSDuck.
#  Use a dry run by default. Specify option '-f' to force the creation.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename "$0" .sh)
ROOTDIR=$(cd $(dirname "$0")/../..; pwd)

info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Dry run by default.
if [[ $1 == -f ]]; then
    OPT=
else
    info "using dry-run mode, specify -f to force the creation of the release"
    OPT=--dry-run
fi

# Get the tag of the most recent release in the tsduck repository.
TAG=$(curl -sL https://api.github.com/repos/tsduck/tsduck/releases/latest | jq -r '.tag_name?')

# Check that the tag value looks good.
[[ -n "$TAG" ]] || error "no tag found for latest tsduck release"
[[ $TAG = v* ]] || error "suspect tag '$TAG', should be 'vX.Y-NNNN'"

# Extract the version number from the tag.
VERSION=${TAG/v/}
info "version: $VERSION"

# URL of the source tarball.
URL="https://github.com/tsduck/tsduck/archive/refs/tags/$TAG.tar.gz"
info "tarball: $URL"

# Download and verify tarball.
TMPROOT="$ROOTDIR/bin/homebrew"
TARFILE="$TMPROOT/$TAG.tar.gz"
rm -rf "$TMPROOT"
mkdir -p "$TMPROOT"
curl -sL "$URL" -o "$TARFILE" || error "error downloading $URL"
[[ -f "$TARFILE" ]] || error "error downloading $URL"
tar tzf "$TARFILE" | grep -q src/libtscore/tsVersion.h || error "invalid source tar file, tsVersion.h not found"
V=$(tar xzf "$TARFILE" --to-stdout '*/tsVersion.h' | sed -e '/^#define TS_/!d' -e 's/^#define TS_.* //' | tr '\n' .)
[[ ${V/%./} == ${VERSION//-/.} ]] || error "version in tsVersion.h does not match $VERSION"

# Get checksum of source tarball.
SHA256=$(shasum -a 256 "$TARFILE" | sed -e 's/ .*//')
info "sha256: $SHA256"

# Create the pull request.
brew bump-formula-pr $OPT --url "$URL" --sha256 "$SHA256" tsduck
