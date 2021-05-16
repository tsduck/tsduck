#!/usr/bin/env bash
# Capture a sample SPTS file for the packager demo (any other SPTS would be fine).
# Capture a "clean" 2 minutes snapshot of clear channel Arte on DTTV, Paris area.
# Keep only one audio PID and one subtitle PID. Remove HbbTV PID's.

tsp -v \
    -I dvb --uhf 30 \
    -P until --seconds 120 \
    -P zap arte --audio-pid 0x014C --subtitles-pid 0x0155 \
    -P pmt --remove-pid 0x0172 --remove-pid 0x0173 --remove-pid 0x0174 \
    -P rmorphan \
    -O file input.ts
