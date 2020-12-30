#!/usr/bin/env bash
# Script to be run as alarm command by plugin bitrate_monitor.

(
    echo -n "Date: "
    date '+%Y/%m/%d %H:%M:%S'
    echo "Message: $1"
    echo "Target PID: $2"
    echo "Alarm state: $3"
    echo "Current bitrate: $4"
    echo "Minimum bitrate: $5"
    echo "Maximum bitrate: $6"
    echo
) >>monitor.log
