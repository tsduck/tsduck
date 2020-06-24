#!/bin/bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
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

# Do nothing if branch is not "master".
[[ "$BRANCH" == "master" ]] || exit 0

# The commit number is in tsVersion.h
SRCFILE="$ROOTDIR/src/libtsduck/tsVersion.h"
PREFIX="#define TS_COMMIT"

# Use GNU variants of sed and grep when available.
# Add /usr/local/bin at end of PATH in case we can find them there.
export PATH=$PATH:/usr/local/bin
[[ -n "$(which gsed 2>/dev/null)" ]] && sed() { gsed "$@"; }
[[ -n "$(which ggrep 2>/dev/null)" ]] && grep() { ggrep "$@"; }

# Get commit number from the source file.
get-src-commit-count()
{
    local cnum=0
    [[ -e "$SRCFILE" ]] || error "$SRCFILE not found"
    cnum=$(grep "^$PREFIX" "$SRCFILE" | head -1 | awk '{print $3}')
    [[ -n "$cnum" ]] || error "no commit count found in $SRCFILE"
    echo $cnum
}

# Filter by hook.
case "$HOOK" in

    pre-commit|post-merge)

        # Get commit number in source file.
        SRCCOMMIT=$(get-src-commit-count)

        # Compute the lowest next commit number to set.
        # Can be forced externally using environment variable TS_GIT_COMMIT.
        COMMIT="$TS_GIT_COMMIT"
        if [[ -z "$COMMIT" ]]; then
            # Get max number of commits from every local or remote branch.
            COMMIT=$SRCCOMMIT
            BRANCHES=$(git branch -a | sed -e 's/^[\* ]*//' -e 's/ .*//')
            for n in $(for b in $BRANCHES; do git rev-list --count $b; done); do
                [[ "$n" -gt "$COMMIT" ]] && COMMIT=$n
            done
            # With pre-commit, we must have at least this max + 1.
            [[ "$HOOK" == "pre-commit" ]] && COMMIT=$(($COMMIT + 1))
        fi

        # Update the commit count in source file if not up to date.
        if [[ "$SRCCOMMIT" -lt "$COMMIT" ]]; then
            sed -i -e "/^$PREFIX/s/.*/$PREFIX $COMMIT/" "$SRCFILE"
            git add "$SRCFILE"
            info "Updated commit count to $COMMIT"
            # In post-merge, we commit the change.
            [[ "$HOOK" == "post-merge" ]] && TS_GIT_COMMIT=$COMMIT git commit -m "Updated commit count to $COMMIT after merge"
        fi
        exit 0
        ;;
    *)
        # Nothing to do in other hooks.
        exit 0
esac
