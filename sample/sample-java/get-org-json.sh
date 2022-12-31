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
#  Download and compile the org.json Java package. Required for sample Java
#  applications using JSON.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
SCRIPTDIR=$(cd $(dirname $0); pwd)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

echo "Get org.json Java package"
API="https://api.github.com/repos/stleary/JSON-java"

# Get URL of source archive and download it.
TAR_URL=$(curl -sL $API/releases/latest | jq -r .tarball_url)
TAR_FILE="$SCRIPTDIR/org.json.tgz"
[[ -z "$TAR_URL" ]] && error "error in $API/releases/latest"

echo "Downloading org.json source code from $TAR_URL ..."
curl -sL "$TAR_URL" -o "$TAR_FILE"
[[ -f "$TAR_FILE" ]] || error "error downloading $TAR_URL"

# Extract to a temporary directory.
TEMPDIR="$SCRIPTDIR/temp"
rm -rf "$TEMPDIR"
mkdir -p "$TEMPDIR"
echo "Expanding $TAR_FILE ..."
tar xzf "$TAR_FILE" -C "$TEMPDIR"

# Find the final source directory (all classes are in org.json).
DEEP_SRC_JSON=$(find "$TEMPDIR" -type d -path '*/src/main/java/org/json' | head -1)
echo "Found source code in $DEEP_SRC_JSON"
DEEP_SRC_ORG=$(dirname $DEEP_SRC_JSON)
ROOT_SRC_ORG="$SCRIPTDIR/org"

# Move it directly under current directory (org.json).
echo "Moving $DEEP_SRC_ORG to $ROOT_SRC_ORG ..."
rm -rf "$ROOT_SRC_ORG"
mv "$DEEP_SRC_ORG" "$ROOT_SRC_ORG"

# Compile org.json classes.
JAR_FILE="$SCRIPTDIR/org.json.jar"
echo "Compiling org.json classes ..."
cd "$SCRIPTDIR"
javac org/json/*.java
jar cf "$JAR_FILE" org/json/*.class
echo "Final jar: $JAR_FILE"

# Cleanup.
rm -rf  "$TEMPDIR" "$ROOT_SRC_ORG" "$TAR_FILE"
