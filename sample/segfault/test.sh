#!/usr/bin/env bash

cd $(dirname "$0")

# Create a sample TS file: 100,000 null packets || 1 packet PID 100 || 133,000 null packets
FILE=test.ts
tsp -I null 100,000 -O file $FILE
tsp -I craft --pid 100 --count 1 -O file $FILE --append
tsp -I null 133,000 -O file $FILE --append

# This chain of plugin creates a crash on the first packet from PID 100.
PLUGINS="-P filter -p 100 --set-label 1 -P debug --only-label 1 --segfault"

# Run the test.
./find-segfault.sh $FILE "$PLUGINS"
