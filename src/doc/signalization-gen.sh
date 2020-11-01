#!/bin/bash
# This script generates the file signalization.md

SCRIPT=$(basename $BASH_SOURCE)
DOCDIR=$(cd $(dirname $BASH_SOURCE); pwd)
DTVDIR=$(cd "$DOCDIR/../libtsduck/dtv"; pwd)
OUTFILE="$DOCDIR/signalization.md"

# Format a line "| XML name | C++ class | Defining document"
# from all source files in the specified directory.
listsig()
{
    dir="$1"
    for header in $(grep -l "@see" "$dir"/*.h); do
        base=$(basename "$header" .h)
        class=${base/ts/ts::}
        source="$dir/$base.cpp"
        xml=$(grep 2>/dev/null '#define  *MY_XML_NAME  *u"' "$source" | head -1 | sed -e 's/.* u"//' -e 's/".*//')
        if [[ -n "$xml" ]]; then
            doc=$(grep "@see" "$header" | \
                  sed -e 's/.*@see *//' -e 's/ section / /' -e 's/[\. ]*$//' -e 's|^SCTE|ANSI/SCTE|' -e '/()/d' | \
                  head -1)
            echo "| $xml | $class | $doc"
        fi
    done | sort -f -d
}

# Generate the documentation file.
cat >$OUTFILE <<EOF
# Tables and descriptors cross-reference   {#sigxref}
[TOC]

All signalization tables and descriptors which are supported by TSDuck are
documented in the TSDuck user's guide, appendix C "PSI/SI XML Reference Model".

The tables below summarize all available structures and the reference of
the standard which specifies them.

## Tables   {#sigxtables}

| XML name | C++ class | Defining document
| -------- | --------- | -----------------
EOF
listsig "$DTVDIR/tables" >>$OUTFILE
cat >>$OUTFILE <<EOF

## Descriptors   {#sigxdescs}

| XML name | C++ class | Defining document
| -------- | --------- | -----------------
EOF
listsig "$DTVDIR/descriptors" >>$OUTFILE
