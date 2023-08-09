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
#  Create a pull request on HomeBrew for the latest version of TSDuck.
#  With option -n, use a dry run (check but no action).
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
SCRIPTDIR=$(cd $(dirname $0); pwd)

info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Dry run.
[[ $1 == -n ]] && OPT=--dry-run || OPT=

# Get the tag of the most recent release in the tsduck repository.
TAG=$(curl -sL https://api.github.com/repos/tsduck/tsduck/releases/latest | jq -r '.tag_name?')

# Check that the tag value looks good.
[[ -n "$TAG" ]] || error "no tag found for latest tsduck release"
[[ $TAG = v* ]] || error "suspect tag '$TAG', should be 'vX.Y-YYYYMMDD'"

# Extract the version number from the tag.
VERSION=${TAG/v/}
info "version: $VERSION"

# URL of the source tarball.
URL="https://github.com/tsduck/tsduck/archive/${TAG}.tar.gz"
info "tarball: $URL"

# Get checksum of source tarball.
SHA256=$(curl -sL "$URL" | shasum -a 256 | awk '{print $1}')
info "sha256: $SHA256"

# Create the pull request.
brew bump-formula-pr $OPT --url "$URL" --sha256 "$SHA256" tsduck
