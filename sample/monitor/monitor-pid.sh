#!/usr/bin/env bash
# Monitor the bitrate of PID 100, allowed rage: 1 to 2 Mb/s.

tsp --timed-log \
    -I ip 12345 \
    -P bitrate_monitor --pid 100 --min 1,000,000 --max 2,000,000 --time-interval 1 --alarm-command './alarm.sh' \
    -O drop
