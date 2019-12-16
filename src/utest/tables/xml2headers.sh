#!/bin/bash
#
# Parameters: A list of .xml files.
# For each X.xml file, create X_xml.h and X_sections.h
# If first parameter is -release or -debug, use freshly built commands.
#

SRCDIR=$(cd $(dirname $0)/../..; pwd)
ARCH=$(uname -m | sed -e 's/i.86/i386/' -e 's/arm.*/arm/')

case $1 in
    -rel*)
        SETENV=$SRCDIR/tstools/release-$ARCH/setenv.sh
        shift
        ;;
    -deb*)
        SETENV=$SRCDIR/tstools/debug-$ARCH/setenv.sh
        shift
        ;;
    *)
        SETENV=
        ;;
esac

[[ -n "$SETENV" && -e $SETENV ]] && source $SETENV

for f in $*; do
    dir=$(dirname $f)
    X=$(basename $f .xml)
    if [ "$X" = "$(basename $f)" ]; then
        echo >&2 "$f: not a .xml file"
    else
        (
            echo "static const ts::UChar ${X}_xml[] ="
            sed -e 's/"/\\"/g' \
                -e 's/^/    u"/' \
                -e 's/$/\\n"/' \
                -e '$s/$/;/' \
                $dir/$X.xml
        ) >$dir/${X}_xml.h
        tstabcomp $dir/$X.xml -o $dir/$X.bin
        (
            echo "static const uint8_t ${X}_sections[] = {"
            tsdump -r -c $dir/$X.bin
            echo "};"
        ) >$dir/${X}_sections.h
        rm -f $dir/$X.bin
    fi
done
