#!/usr/bin/env bash
# Demonstrate the use of the "foot" plugin.

cd $(dirname $0)

tsp -I file demo.ts \
    -P foot --pid 0x0104 --id 8 --name "modified-name" \
    -P tables --pid 0x0104 -o demo.modified.foot.txt \
    -O drop
