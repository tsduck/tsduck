#!/usr/bin/env bash
# Script which analyzes the final TS with embedded MPE.

cd $(dirname "$0")

tsp --verbose \
    -I ip 230.5.6.7:4500 \
    -P until --seconds 30 \
    -P analyze -o output-analysis.txt \
    -P tables -o output-tables.txt --negate-pid --pid 5003 \
    -O drop
