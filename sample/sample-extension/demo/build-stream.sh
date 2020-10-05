#!/usr/bin/env bash
# This script builds a fake TS with specific data from the TSDuck "foo" extension.

cd $(dirname $0)

# Build fake video and audio data. Just packets with some data.
tsp -I craft --pid 0x0101 --cc 0 --count 1 --pcr 0 --pusi --payload-pattern 00000101010101010101 -O file video.ts
tsp -I craft --pid 0x0101 --cc 1 --count 15 --payload-pattern 02 -O file video.ts --append
tsp -I craft --pid 0x0102 --cc 0 --count 1 --pcr 0 --pusi --payload-pattern 00000111111111111111 -O file audio.ts
tsp -I craft --pid 0x0102 --cc 1 --count 15 --payload-pattern 12 -O file audio.ts --append

# Build complete transport stream.
tsp --bitrate 2,000,000 --add-start-stuffing 10 --add-input-stuffing 1/10 \
    -I file --interleave=2 --infinite video.ts audio.ts \
    -P inject pat.xml  --pid 0x0000 --bitrate 10,000 --stuffing \
    -P inject cat.xml  --pid 0x0001 --bitrate 10,000 --stuffing \
    -P inject sdt.xml  --pid 0x0011 --bitrate 10,000 --stuffing \
    -P inject pmt.xml  --pid 0x0100 --bitrate 10,000 --stuffing \
    -P inject ecm.xml  --pid 0x0103 --bitrate 10,000 --stuffing \
    -P inject foot.xml --pid 0x0104 --bitrate 10,000 --stuffing \
    -P inject emm.xml  --pid 0x0200 --bitrate 10,000 \
    -P filter --pid 0x1FFF --negate \
    -P pcradjust \
    -P until --packets 10,000 \
    -O file demo.ts

rm audio.ts video.ts
