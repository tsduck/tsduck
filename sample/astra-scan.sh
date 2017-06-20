#!/bin/bash
#
# This script scans the satelite network on Astra 19.2 E.
#
# Options:
#   -r  Reuse exiting intermediate files to speed up the scan.
#

# Input options:
[[ "$1" == "-r" ]] && REUSE_LOG=true || REUSE_LOG=false

# This script:
SCRIPT=$(basename ${BASH_SOURCE[0]} .sh)
SCRIPTDIR=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)
info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Cygwin portability oddities.
shopt -s expand_aliases
if [[ $(uname -o) == Cygwin ]]; then
    path() { cygpath --windows "$1"; }
    alias tsp=tsp.exe
else
    path() { echo "$1"; }
fi

# Reference transponder where the NIT is read:
REF_TUNE="--delivery-system DVB-S --modulation QPSK --frequency 11,597,000,000 --polarity vertical --symbol-rate 22,000,000 --fec 5/6"

# Analysis duration by TS in seconds.
DURATION=20

# Intermediate file:
NIT_LOG=$SCRIPTDIR/astra-nit.log

# Scan the NIT Actual:
if [[ ! -e $NIT_LOG ]] || ! $REUSE_LOG; then
    info scanning the NIT
    tsp -v \
        -I dvb $REF_TUNE \
        -P nitscan --output-file $(path $NIT_LOG) --dvb-options --comment --terminate \
        -O drop
fi

# Read the list of transponders and scan them.
TSID=
while read line; do
    line=$(sed <<<$line -e 's/\r$//')
    if grep <<<$line -q "# TS id:"; then
        TSID=$(sed <<<$line -e 's/# TS id: *//' -e 's/ .*//')
    else
        OUTFILE=$SCRIPTDIR/astra-ts-$TSID.log
        if [[ ! -e $OUTFILE ]] || ! $REUSE_LOG; then
            info analyzing TS id $TSID during $DURATION seconds
            tsp -v \
                -I dvb $line \
                -P analyze --output-file $(path $OUTFILE) \
                -P until --seconds $DURATION \
                -O drop
        fi
    fi
done <$NIT_LOG
