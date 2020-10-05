#!/usr/bin/env bash
# This script demonstrates the impact of the "foo" extension on the various TSDuck analysis tool.

cd $(dirname $0)

tsp -I file demo.ts \
    -P analyze -o demo.analysis.txt \
    -P psi -a -o demo.psi.txt \
    -P tables --no-duplicate -o demo.tables.txt \
    -O drop
