#!/bin/bash
#
# Parameters: A list of .xml files.
# For each X.xml file, create X_xml.h and X_sections.h
#

for f in $*; do
    dir=$(dirname $f)
    X=$(basename $f .xml)
    if [ "$X" = "$(basename $f)" ]; then
        echo >&2 "$f: not a .xml file"
    else
        (
            echo "static const char ${X}_xml[] = "
            sed -e 's/"/\\"/g' \
                -e 's/^/    "/' \
                -e 's/$/\\n"/' \
                -e '$s/$/;/' \
                $dir/$X.xml
        ) >${X}_xml.h
        tstabcomp $dir/$X.xml -o $dir/$X.si
        (
            echo "static const uint8_t ${X}_sections[] = {"
            tsdump -r -c $dir/$X.si
            echo "};"
        ) >${X}_sections.h
        rm -f $dir/$X.si
    fi
done
