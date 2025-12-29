#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Sample build script for the tsduck container.
#
#-----------------------------------------------------------------------------

cd $(dirname "$0")

# Build an archive of the TSDuck source tree. Will be imported in the builder
# container (COPY in Dockerfile cannot access parents of current directory).
tar -cpzf tsduck.tgz --exclude .git --exclude bin --exclude __pycache__ --exclude sample --exclude pkg --exclude msvc --exclude qtcreator -C ../.. .

docker build -t tsduck .
