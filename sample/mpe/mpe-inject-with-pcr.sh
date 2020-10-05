#!/usr/bin/env bash
# Script which creates a TS from scratch and then insert MPE from an UDP stream.
# The output TS has two data services and a forged PCR PID.

cd $(dirname "$0")

# Target output bitrate:
BITRATE=30000000

# Number of PCR's per seconds:
PCR_PER_SEC=5
PCR_DISTANCE=$(( $BITRATE / $(( $PCR_PER_SEC * 188 * 8 )) ))
PCR_PID=5004

# MPE insertion command:
tsp --verbose --bitrate $BITRATE --max-flushed-packets 70 \
    -I null \
    -P regulate --packet-burst 14 \
    -P filter --every $PCR_DISTANCE --set-label 1 \
    -P craft --only-label 1 --pid $PCR_PID --no-payload --pcr 0 \
    -P continuity --pid $PCR_PID --fix \
    -P pcradjust --pid $PCR_PID \
    -P inject pat.xml --pid 0 --bitrate 15000 \
    -P inject sdt.xml --pid 17 --bitrate 15000 \
    -P inject pmt-int.xml --pid 5000 --bitrate 15000 \
    -P inject int.xml --pid 5001 --bitrate 15000 \
    -P inject pmt-mpe-with-pcr.xml --pid 5002 --bitrate 15000 \
    -P mpeinject 230.2.3.4:4000 --pid 5003 --max-queue 512 \
    -O ip 230.5.6.7:4500 --packet-burst 7 --enforce-burst
