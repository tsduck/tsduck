#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
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
#  This script updates the "tsduck" Homebrew formula for the most recent
#  version. The update is performed on the local copy of the homebrew-core
#  repository. Homebrew is the open-source packager for macOS. TSDuck is
#  distributed on macOS through Homebrew.
#
#  By default, use the latest official release of TSDuck on GitHub.
#  With --commit, use the latest commit in the TSDuck repository on GitHub.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
SCRIPTDIR=$(cd $(dirname $0); pwd)

info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Get the formula file.
REPO=$(brew --repo homebrew/core)
FORMULA="$REPO/Formula/tsduck.rb"
[[ -s "$FORMULA" ]] || error "formula file not found: $FORMULA"

# Use gsed instead of sed when available (typically on macOS).
[[ -n $(which gsed) ]] && sed() { gsed "$@"; }

# Latest release or latest commit?
if [[ "$1" == --commit ]]; then

    # Get the latest commit.
    SHA=$(curl -sL 'https://api.github.com/repos/tsduck/tsduck/commits?per_page=1' | jq -r '.[0].sha?')

    # Check that the commit SHA looks good.
    [[ $(echo -n "$SHA" | wc -c) -eq 40 ]] || error "suspect commit SHAR '$SHA', should be 40 hex chars"

    # Get the TSDuck version from the source file.
    VERSION=$(curl -sL "https://raw.githubusercontent.com/tsduck/tsduck/$SHA/src/libtsduck/tsVersion.h" |
              sed -e '/#define *TS_/!d' -e 's|^.*#define *[A-Z_]* *||' |
              tr '\r\n' '  ' |
              sed -e 's|^ *\([0-9]*\)  *\([0-9]*\)  *\([0-9]*\) *$|\1.\2-\3|')

    info "commit: $SHA"
    info "version: $VERSION"

    # URL of the source tarball.
    URL="https://github.com/tsduck/tsduck/archive/${SHA}.tar.gz"
    info "tarball: $URL"

    # Get checksum of source tarball.
    SHA256=$(curl -sL "$URL" | shasum -a 256 | awk '{print $1}')
    info "sha256: $SHA256"

    # Update formula for latest release.
    sed -i \
        -e "/version *\".*\"/d" \
        -e "s|url *\".*\"|url \"$URL\"|" \
        -e "s|sha256 *\".*\"|sha256 \"$SHA256\"|" \
        "$FORMULA"

    sed -i -e "/^ *url *\"/a\  version \"$VERSION\"" "$FORMULA"

else

    # Get the tag of the most recent release in the tsduck repository.
    TAG=$(curl -sL https://api.github.com/repos/tsduck/tsduck/releases/latest | jq -r '.tag_name?')

    # Check that the tag value looks good.
    [[ -n "$TAG" ]] || error "no tag found for latest tsduck release"
    [[ $TAG = v* ]] || errro "suspect tag '$TAG', should be 'vX.Y-YYYYMMDD'"

    # Extract the version number from the tag.
    VERSION=${TAG/v/}
    info "version: $VERSION"

    # URL of the source tarball.
    URL="https://github.com/tsduck/tsduck/archive/${TAG}.tar.gz"
    info "tarball: $URL"

    # Get checksum of source tarball.
    SHA256=$(curl -sL "$URL" | shasum -a 256 | awk '{print $1}')
    info "sha256: $SHA256"

    # Update formula for latest release.
    sed -i \
        -e "/version *\".*\"/d" \
        -e "s|url *\".*\"|url \"$URL\"|" \
        -e "s|sha256 *\".*\"|sha256 \"$SHA256\"|" \
        "$FORMULA"

fi
