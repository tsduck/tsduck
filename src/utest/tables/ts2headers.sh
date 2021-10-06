#!/usr/bin/env bash
#
# Parameters: A list of .ts files.
# Each file contains captured TS packets with one table.
# For each X.ts file, create the following files:
# - X.txt ......... Analysis of the table
# - X_packets.h ... C/C++ Header files containing the packets.
# - X_sections.h .. C/C++ Header files containing the sections.
# If first parameter is -release or -debug, use freshly built commands.
#

ROOTDIR=$(cd $(dirname $0)/../../..; pwd)

case $1 in
    -rel*)
        source "$ROOTDIR/scripts/setenv.sh"
        shift
        ;;
    -deb*)
        source "$ROOTDIR/scripts/setenv.sh" --debug
        shift
        ;;
esac

for f in $*; do
    dir=$(dirname $f)
    X=$(basename $f .ts)
    if [ "$X" = "$(basename $f)" ]; then
        echo >&2 "$f: not a .ts file"
    elif tstables $f -b $dir/$X.si; then
        tstabdump $dir/$X.si >$X.txt
        echo "static const uint8_t ${X}_packets[] = {" >${X}_packets.h
        tsdump -r -c $dir/$X.ts >>${X}_packets.h
        echo "};" >>${X}_packets.h
        echo "static const uint8_t ${X}_sections[] = {" >${X}_sections.h
        tsdump -r -c $dir/$X.si >>${X}_sections.h
        echo "};" >>${X}_sections.h
    fi
done
