#!/usr/bin/env bash
#
# Parameters: A list of .xml files.
# For each X.xml file, create X_xml.h and X_sections.h
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
