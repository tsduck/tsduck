#!/usr/bin/env bash
# Script which simulates the UDP/IP which will be encapsulated in MPE stream.
# The input file is an SPTS which is generated from the TSDuck stream repository.
# It can be any TS in fact.

cd $(dirname "$0")

tsp --verbose \
    -I http --infinite https://tsduck.io/streams/france-dttv/tnt-uhf30-546MHz-2019-01-22.ts \
    -P zap arte \
    -P pcrbitrate \
    -P regulate --packet-burst 14 \
    -O ip 230.2.3.4:4000 --packet-burst 7 --enforce-burst
