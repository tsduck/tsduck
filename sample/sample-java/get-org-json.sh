#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
TMPROOT="$SCRIPTDIR/temp"
rm -rf "$TMPROOT"
mkdir -p "$TMPROOT"
echo "Expanding $TAR_FILE ..."
tar xzf "$TAR_FILE" -C "$TMPROOT"

# Find the final source directory (all classes are in org.json).
DEEP_SRC_JSON=$(find "$TMPROOT" -type d -path '*/src/main/java/org/json' | head -1)
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
rm -rf  "$TMPROOT" "$ROOT_SRC_ORG" "$TAR_FILE"
