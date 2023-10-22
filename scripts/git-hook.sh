#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script is a "universal hook" which is invoked by Git.
#  It must be invoked with hook name as argument.
#  Must be executable on Linux, MacOS and Windows (Git Bash).
#
#-----------------------------------------------------------------------------

HOOK="$1"
SCRIPT=$(basename ${BASH_SOURCE[0]} .sh)
ROOTDIR=$(cd $(dirname ${BASH_SOURCE[0]})/..; pwd)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }
info()  { echo >&2 "$SCRIPT: $*"; }

# Get current branch (output and syntax varies across versions of git).
BRANCH=$(git branch | sed -e '/^$/d' -e '/^[^*]/d' -e 's/^\* *//' 2>/dev/null)

# Name of the master branch.
MASTER=master

# Do nothing if branch is not "master".
[[ "$BRANCH" == "$MASTER" ]] || exit 0

# The commit number is in tsVersion.h
SRCFILE="$ROOTDIR/src/libtsduck/tsVersion.h"
PREFIX="#define TS_COMMIT"

# Use GNU variants of sed and grep when available.
# Add /usr/local/bin at end of PATH in case we can find them there.
export PATH=$PATH:/usr/local/bin
[[ -n "$(which gsed 2>/dev/null)" ]] && sed() { gsed "$@"; }
[[ -n "$(which ggrep 2>/dev/null)" ]] && grep() { ggrep "$@"; }

# Filter by hook.
case "$HOOK" in

    pre-commit|pre-merge-commit|post-merge)

        # Get commit number in source file.
        [[ -e "$SRCFILE" ]] || error "$SRCFILE not found"
        SRCCOMMIT=$(grep "^$PREFIX" "$SRCFILE" | head -1 | awk '{print $3}')
        [[ -n "$SRCCOMMIT" ]] || error "no commit count found in $SRCFILE"

        # Compute the lowest next commit number to set.
        # Can be forced externally using environment variable TS_GIT_COMMIT.
        COMMIT="$TS_GIT_COMMIT"
        if [[ -z "$COMMIT" ]]; then
            # Get max number of commits from master branch. Previously, we
            # did it on every local or remote branch but this could create
            # inconsistencies on local repos with many branches.
            COMMIT=$(git rev-list --count $MASTER)
            [[ "$SRCCOMMIT" -gt "$COMMIT" ]] && COMMIT=$SRCCOMMIT
            # With pre-commit, we must have at least this max + 1.
            [[ "$HOOK" == "pre-commit" || "$HOOK" == "pre-merge-commit" ]] && COMMIT=$(($COMMIT + 1))
        fi

        # Update the commit count in source file if not up to date.
        if [[ "$SRCCOMMIT" -lt "$COMMIT" ]]; then
            sed -i -e "/^$PREFIX/s/.*/$PREFIX $COMMIT/" "$SRCFILE"
            git add "$SRCFILE"
            info "Updated commit count to $COMMIT"
            # In post-merge, we commit the change. Increment number since we create a commit.
            [[ "$HOOK" == "post-merge" ]] && TS_GIT_COMMIT=$(($COMMIT + 1)) git commit -m "Updated commit count to $COMMIT after merge"
        fi
        exit 0
        ;;
    *)
        # Nothing to do in other hooks.
        exit 0
esac
