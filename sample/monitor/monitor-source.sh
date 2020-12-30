#!/usr/bin/env bash
# Send the source TS for monitor-pid.sh
# Loop on various bitrates by cycles of 10 seconds:

for bitrate in 1,500,000 500,000 1,500,000 2,500,000 1,500,000; do
    tsp -I craft --pid 100 \
        -P regulate --bitrate $bitrate \
        -P until --seconds 10 \
        -O ip localhost:12345
done
