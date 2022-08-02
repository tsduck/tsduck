#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2022, Thierry Lelegard
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
#  This script updates the Git hooks when required.
#
#  A few standard Git hooks are established to manage the "commit count"
#  which is used to identify the product version.
#
#  The actual code to execute is in scripts/git-hook.sh.
#  The template for a hook script is:
#
#    #!/usr/bin/env bash
#    exec $GIT_DIR/../scripts/git-hook.sh <hook-name>
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE)/..; pwd)

GITHOOKS_DIR="$ROOTDIR/.git/hooks"
GITHOOKS_CMD="scripts/git-hook.sh"
GITHOOKS_LIST="pre-commit post-merge"

# Activate Git LFS at user level.
cd "$HOME"
(git config --list --global | fgrep -q filter.lfs) || git lfs install

# Activate Git LFS in repo (replace some Git hooks).
cd "$ROOTDIR"
(cat "$GITHOOKS_DIR"/* 2>/dev/null | grep -q 'git  *lfs') || git lfs update --force

# Update all hooks.
for hook in $GITHOOKS_LIST; do
    hookfile="$GITHOOKS_DIR/$hook"
    if [[ ! -e "$hookfile" ]]; then
        echo '#!/usr/bin/env bash' >"$hookfile"
        chmod a+x "$hookfile"
    fi
    if ! fgrep -q "/../../$GITHOOKS_CMD $hook" "$hookfile"; then
        echo "Updating Git hook $hook"
        echo "\$(dirname \$0)/../../$GITHOOKS_CMD $hook" >>"$hookfile"
    fi
done
