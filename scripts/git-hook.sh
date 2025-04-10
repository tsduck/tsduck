#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script is a "universal hook" which is invoked by Git.
#  It must be invoked with hook name as argument.
#  Must be executable on Linux, MacOS and Windows (Git Bash).
#
#  Define environment variable GIT_HOOK_DEBUG to something non-empty
#  to display additional information.
#
#-----------------------------------------------------------------------------

HOOK="$1"
SCRIPT=$(basename ${BASH_SOURCE[0]} .sh)
ROOTDIR=$(cd $(dirname ${BASH_SOURCE[0]})/..; pwd)
[[ ! -d "$ROOTDIR/.git" && -d .git ]] && ROOTDIR=$(pwd)

error() { echo >&2 "$SCRIPT: $HOOK: $*"; exit 1; }
info()  { echo >&2 "$SCRIPT: $HOOK: $*"; }
debug() { [[ -n "$GIT_HOOK_DEBUG" ]] && echo >&2 "$SCRIPT: (debug) $HOOK: $*"; }

# Get current branch (output and syntax varies across versions of git).
BRANCH=$(git branch | sed -e '/^$/d' -e '/^[^*]/d' -e 's/^\* *//' 2>/dev/null)
debug "entering git hook, branch '$BRANCH'"

# Name of the master branch.
MASTER=master

# Do nothing if branch is not "master".
[[ "$BRANCH" == "$MASTER" ]] || exit 0

# Do nothing on git pull, we don't want to update the version.
debug "GIT_REFLOG_ACTION='$GIT_REFLOG_ACTION'"
[[ $GIT_REFLOG_ACTION == pull* ]] && exit 0

# Use GNU variants of sed and grep when available.
# Add /usr/local/bin at end of PATH in case we can find them there.
export PATH=$PATH:/usr/local/bin
[[ -n "$(which gsed 2>/dev/null)" ]] && sed() { gsed "$@"; }
[[ -n "$(which ggrep 2>/dev/null)" ]] && grep() { ggrep "$@"; }

# The commit number is in tsVersion.h
SRCFILE="$ROOTDIR/src/libtscore/tsVersion.h"
PREFIX="#define TS_COMMIT"

# Max of two values.
max() {
    [[ "$1" -gt "$2" ]] && echo "$1" || echo "$2"
}

# Get commit from tsVersion.h.
get-src-commit() {
    [[ -e "$SRCFILE" ]] || error "$SRCFILE not found"
    local commit=$(grep "^$PREFIX" "$SRCFILE" | head -1 | awk '{print $3}')
    [[ -n "$commit" ]] || error "no commit count found in $SRCFILE"
    debug "Commit from tsVersion.h: $commit"
    echo $commit
}

# Update commit in tsVersion.h.
set-src-commit() {
    sed -i -e "/^$PREFIX/s/.*/$PREFIX $1/" "$SRCFILE"
    git add "$SRCFILE"
    info "Updated commit count to $1"
}

# Get number of commits on master branch.
get-commit-count() {
    local commit=$(git rev-list --count $MASTER)
    debug "Number of commits on $MASTER: $commit"
    echo $commit
}

# Filter by hook.
case "$HOOK" in
    pre-commit)
        if [[ -n "$TS_GIT_COMMIT" ]]; then
            debug "Forced commit: $TS_GIT_COMMIT"
            set-src-commit $TS_GIT_COMMIT
        else
            set-src-commit $(($(max $(get-commit-count) $(get-src-commit)) + 1))
        fi
        ;;
    post-merge)
        # --> Disable forced commit count update after merge. Will see later if we need to restore this.
        # export TS_GIT_COMMIT=$(($(max $(get-commit-count) $(get-src-commit)) + 1))
        # set-src-commit $TS_GIT_COMMIT
        # git commit -m "Updated commit count to $TS_GIT_COMMIT after merge"
        ;;
    *)
        ;;
esac
exit 0
