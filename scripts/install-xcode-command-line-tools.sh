#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Installation / upgrade of Xcode Command Line Tools on macOS.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $0 .sh)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }
usage() { echo >&2 "invalid command, try \"$SCRIPT --help\""; exit 1; }

# Default values for command line options.
MINVERSION=

# Display help text
showhelp()
{
    cat >&2 <<EOF

Installation / upgrade of Xcode Command Line Tools on macOS.

Usage: $SCRIPT [options]

Options:

  --help
      Display this help text.

  -m major-version
  --minimum major-version
      If the installed version is lower than the specified major version
      (e.g. "16"), force an update to the latest version.

EOF
    exit 1
}

# Decode command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --help)
            showhelp
            ;;
        -m|--min*)
            [[ $# -gt 1 ]] || usage; shift
            MINVERSION="$1"
            ;;
        *)
            usage
            ;;
    esac
    shift
done

# Verify current version of clang.
CLANG_MAJOR=$(clang --version 2>/dev/null | head -1 | sed 's/.* \([0-9][0-9]*\)\.[0-9][0-9]*.*/\1/')
if [[ -n $CLANG_MAJOR && ( -z $MINVERSION || $CLANG_MAJOR -ge $MINVERSION ) ]]; then
    echo "Clang $CLANG_MAJOR is already installed"
    exit 0
fi

# Need to install or upgrade Xcode command line tools: not so easy....
# There is no identified way of updating the Xcode command line tools.
# We can only remove (or move) them and reinstall. Additionally, it has
# been noted that when several versions are installed, removing one version
# selects another one. Therefore, we move all versions until none is defined.
XCODE_PATH=$(xcode-select --print-path 2>/dev/null)
while [[ -n $XCODE_PATH && -d "$XCODE_PATH" ]]; do
    OLD="$XCODE_PATH.old"
    [[ -d "$OLD" ]] && sudo rm -rf "$OLD"
    echo "Moving '$XCODE_PATH' to .old"
    sudo mv -f "$XCODE_PATH" "$OLD"
    XCODE_PATH=$(xcode-select --print-path 2>/dev/null)
done

# The usual command to install Xcode command line tools is "xcode-select --install".
# However, it launches a GUI and, in the case of a script on a remote server without
# user, the installation never completes. As another option, the software update command
# is "softwareupdate". However, it will not find anything to update if the command line
# tools are already installed. The trick is to create a given file in /tmp which makes
# the command think that an installation is required.

# Touch a file so that softwareupdate thinks the install is required.
TRICK=/tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress
touch $TRICK

# Get the list of updatable software, keep the last version of Xcode command line tools.
NAME=$(softwareupdate --list | grep "Label:.*Command Line Tools" | tail -1 | sed -e 's/^.*: *//' )

# Install it.
echo "Installing $NAME ..."
softwareupdate --install "$NAME" --verbose
rm $TRICK

# Make sure we use the right version of the command line tools.
sudo xcode-select --reset
