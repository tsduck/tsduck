#!/usr/bin/env bash
# Sample build using a local installation

SYSROOT=$HOME/tmp
SYSPREFIX=/sysroot

cd $(dirname $0)
make -C ../.. -j10 install SYSROOT=$SYSROOT SYSPREFIX=$SYSPREFIX

export PATH="$SYSROOT$SYSPREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$SYSROOT$SYSPREFIX/lib:$LD_LIBRARY_PATH"
[[ $(uname -s) == Darwin ]] && export DYLD_LIBRARY_PATH="$SYSROOT$SYSPREFIX/lib:$DYLD_LIBRARY_PATH"
make
