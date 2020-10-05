#!/usr/bin/env bash
# Script which creates a TS from scratch and then insert MPE from an UDP stream.
# The output TS has two data services and no PCR at all.

cd $(dirname "$0")

tsp --verbose --bitrate 30000000 --max-flushed-packets 70 \
    -I null \
    -P regulate --packet-burst 14 \
    -P inject pat.xml --pid 0 --bitrate 15000 \
    -P inject sdt.xml --pid 17 --bitrate 15000 \
    -P inject pmt-int.xml --pid 5000 --bitrate 15000 \
    -P inject int.xml --pid 5001 --bitrate 15000 \
    -P inject pmt-mpe.xml --pid 5002 --bitrate 15000 \
    -P mpeinject 230.2.3.4:4000 --pid 5003 --max-queue 512 \
    -O ip 230.5.6.7:4500 --packet-burst 7 --enforce-burst
