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
#  This script verifies that the source files are correctly organized in
#  src/libtsduck. Specifically, it verifies that all included headers are
#  strictly contained in a subdirectory or its dependencies.
#
#  The current dependency order of src/libtsduck subdirectories is:
#
#     base <--- crypto <-- dtv <-- plugin
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename ${BASH_SOURCE[0]} .sh)

cd $(dirname ${BASH_SOURCE[0]})/../src/libtsduck

# Loop on all subdirectories in reverse order of dependency.
DEPDIRS=
for subdir in base crypto dtv plugin; do

    echo "==== Checking subtree $subdir"

    # Directories to check for headers
    DEPDIRS="$subdir $DEPDIRS"

    # Collect all header files which are used in a the subdir tree.
    # Exclude a couple of known headers which are out of dependencies
    grep -Irh '^ *#include *"' $subdir |
        sed -e 's/^ *#include *"//' -e 's/".*$//' |
        sort -u |
        grep -v DTAPI.h |
        while read header; do
            [[ -z $(find $DEPDIRS -path \*/$header) ]] && echo "$header not found in $DEPDIRS"
        done

done
