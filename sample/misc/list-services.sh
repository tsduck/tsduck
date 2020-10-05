#!/usr/bin/env bash
# List all services from a set of TS files.

group_digits() { sed <<<$1 -r ':L;s=\b([0-9]+)([0-9]{3})\b=\1,\2=g;t L'; }

echo "Service name                             Provider        Access     TS id Sv id      Bitrate"
echo "---------------------------------------- --------------- ---------- ----- ----- ------------"

for tsfile in "$@"; do
    tsanalyze "$tsfile" --normalized | \
        grep -a '^service:' | \
        sed -e 's/\r//g' | \
        while read line; do
            id=${line/*:id=/}
            tsid=${line/*:tsid=/}
            access=${line/*:access=/}
            bitrate=${line/*:bitrate=/}
            provider=${line/*:provider=/}
            printf '%-40.40s %-15.15s %-10.10s %5s %5s %12s\n' \
                   "${line/*:name=/}" \
                   "${provider/:*/}" \
                   "${access/:*/}" \
                   "${tsid/:*/}" \
                   "${id/:*/}" \
                   $(group_digits "${bitrate/:*/}")
        done
done | sort -d -i
