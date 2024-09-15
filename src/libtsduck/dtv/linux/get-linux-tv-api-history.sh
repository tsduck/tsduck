#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Get the history of the LinuxTV API, using the Linux kernel git repository.
#  Keep only versions which bring something new.
#  Syntax: ./get-linux-tv-api-history.sh output-directory-name
#
#-----------------------------------------------------------------------------

KERNELGIT=https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git

SCRIPT=$(basename $BASH_SOURCE)
info() { echo >&2 "==== $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Output directory.
OUTDIR="$1"
[[ -z "$OUTDIR" ]] && error "specify output directory"
mkdir -p "$OUTDIR" || exit
OUTDIR=$(realpath "$OUTDIR")

# Linux kernel repository.
KERNELDIR="$OUTDIR/kernel"
if [[ -d "$KERNELDIR/.git" ]]; then
    info "updating kernel repo..."
    cd "$KERNELDIR"
    git fetch origin
elif [[ -d "$KERNELDIR" ]]; then
    error "directory $KERNELDIR exists but is not a git repo"
else
    info "cloning kernel repo..."
    git clone "$KERNELGIT" "$KERNELDIR"
    cd "$KERNELDIR"
fi

# Rebuild all versions of the LinuxTV API.
rm -rf "$OUTDIR"/dvb_*
previous=
for tag in $(git tag --sort=creatordate); do
    info "$tag"
    git checkout $tag
    dvbdir=$(find include -name dvb -type d 2>/dev/null | head -1)
    if [[ -d $dvbdir ]]; then

        # Build DVB API version in this tag.
        maj=$(grep -w '#define[[:space:]]*DVB_API_VERSION' $dvbdir/version.h | head -1 | awk '{print $3}')
        if [[ -z $maj ]]; then
            version=v0
        else
            min=$(grep -w '#define[[:space:]]*DVB_API_VERSION_MINOR' $dvbdir/version.h | head -1 | awk '{print $3}')
            version=v${maj}.${min:-0}
        fi
        dir=dvb_${version}_kernel_${tag}
        cp -r $dvbdir "$OUTDIR/$dir"

        # Process current tag from kernel tree.
        if [[ -z $previous ]]; then
            info "----> first version in $tag"
            previous=$dir
        else
            outdiff="$OUTDIR/${previous}_${dir}.diff"
            diff -Nawr "$OUTDIR/$previous" "$OUTDIR/$dir" >"$outdiff"
            if [[ -s "$outdiff" ]]; then
                info "----> changes in $tag"
                previous=$dir
            else
                rm -rf "$OUTDIR/$dir" "$outdiff"
            fi
        fi

    fi
done
