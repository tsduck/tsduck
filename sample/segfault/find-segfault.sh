#!/usr/bin/env bash
#
# Run a chain of plugins causing a segmentation fault on a file.
# Extract the smallest possible file reproducing the crash.
#
# Syntax: find-segfault.sh input-file "plugin-chain"

SCRIPT=$(basename $BASH_SOURCE)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

[[ $# -ne 2 ]] && error "syntax: $SCRIPT input-file plugin-chain"

FILE="$1"
PLUGINS="$2"
OUTPUT=$(dirname "$FILE")/$(basename "$FILE" .ts)-subset.ts

# Target size in packets for final file
TARGETSIZE=1000

# Exit code for segmentation fault
SEGFAULT=139

echo "* Running the command once to make sure it crashes..."
tsp -I file "$FILE" $PLUGINS -O drop
if [[ $? -ne $SEGFAULT ]]; then
   echo "Error: no segmentation fault on file $FILE"
   exit 1
fi
echo "* Segmentation fault confirmed, starting analysis"

echo "==== Starting step 1: locating start of segment"
FILESIZE=$(($(stat --printf="%s" "$FILE") / 188))
printf "* File size: %'d packets\n" $FILESIZE
START=0
END=$FILESIZE
TEST=$(($FILESIZE / 2))
while [[ $(($END - $START)) -gt $TARGETSIZE ]]; do
    printf "* Testing start at packet %'d\n" $TEST
    tsp -I file "$FILE" -p $TEST $PLUGINS -O drop
    if [[ $? -eq $SEGFAULT ]]; then
        START=$TEST
    else
        END=$TEST
    fi
    TEST=$(($START + (($END - $TEST) / 2)))
done
printf "* Start located at packet %'d\n" $START

echo "==== Starting step 2: determining segment size"
LENGTH=$(($FILESIZE - $START))
TEST=$(($LENGTH / 2))
while [[ $LENGTH -gt $TARGETSIZE ]]; do
    printf "* Testing on %'d packets\n" $TEST
    tsp -I file "$FILE" -p $START -P until -p $TEST $PLUGINS -O drop
    if [[ $? -eq $SEGFAULT ]]; then
        LENGTH=$TEST
        TEST=$(($LENGTH / 2))
    else
        TEST=$(($TEST + (($LENGTH - $TEST) / 2)))
        [[ $(($LENGTH - $TEST)) -lt $TARGETSIZE ]] && break
    fi
done
printf "* Segment size: %'d packets\n" $LENGTH

# Final step: extract the small segment causing the crash.
tsp -I file "$FILE" -p $START -P until -p $LENGTH -O file "$OUTPUT"
printf "* Segment extracted as %s, %'d bytes\n" "$OUTPUT" $(stat --printf="%s" "$OUTPUT")

echo "* Running the command on segment to make sure it still crashes"
tsp -I file "$OUTPUT" $PLUGINS -O drop
if [[ $? -ne $SEGFAULT ]]; then
    echo "Error: no segmentation fault on file $OUTPUT"
else
    echo "* Segmentation fault confirmed on file $OUTPUT"
fi
