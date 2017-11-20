#!/bin/bash
#
# This script tracks the usage of plain 8-bit strings in TSDuck source code.
#
# Initially, TSDuck was developed using standard C++ strings. Later, Unicode
# strings (ts::UString) were introduced and the entire code base was migrated
# to Unicode strings. This scripts tracks obsolete usages of 8-bit strings.
#
# Lines containing the comment //[STRING8] are ignored. This can be used to
# mark valid intentional usage of 8-bit strings.
#

# Change to src directory.
cd $(dirname $0)/../src || exit 1

# Cleanup environent if too large for xargs (Cygwin)
unset $(env | sed 's/=.*//' | grep -v -e PATH -e LANG -e TZ -e HOME -e PWD -e SHELL -e TERM -e TEMP -e USER -e USERNAME) 2>/dev/null
 
find . -iname '*.h' -o -iname '*.cpp' |
    xargs grep -n 'std::string' |
    grep -v -F '//[STRING8]' |
    sed -e 's|//.*$||' |
    grep 'std::string'

find . -iname '*.h' -o -iname '*.cpp' |
    xargs grep -n '"' |
    grep -v -e '//\[STRING8\]' -e '^[^:]*:[0-9]*: *#' |
    sed -e 's|//.*$||' -e 's|\\"||g' -e 's|u"[^"]*"||g' |
    grep -e '"'
