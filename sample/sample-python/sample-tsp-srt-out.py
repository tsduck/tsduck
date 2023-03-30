#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python usage of SRT output plugin
# Copyright (c) 2023, Marwan Yassin
# All rights reserved.
#
#----------------------------------------------------------------------------

import tsduck

# Create an asynchronous report to log multi-threaded messages.
# Initial level is verbose, using time-stamped messages.
rep = tsduck.AsyncReport(severity = tsduck.Report.Debug, timed_log = True)
rep.info("TSDuck version: %s" % tsduck.__version__)

# Create a TS processor using the report.
tsp = tsduck.TSProcessor(rep)

# Set some global TS processing options.
tsp.add_input_stuffing = [1, 10]   # one null packet every 10 input packets
tsp.bitrate = 1000000              # nominal bitrate is 1 Mb/s
tsp.app_name = "srt-out"              # informational only, for log messages

# Set plugin chain.
tsp.input = ['http', 'https://github.com/tsduck/tsduck-test/raw/master/input/test-001.ts']

tsp.plugins = [
    ['count'],
]

# Output plugin
tsp.output = ['srt', '--multiple', '--listener', '127.0.0.1:4444', '--transtype', 'live']

# Run the TS processing and wait until completion.
tsp.start()
tsp.waitForTermination()
tsp.delete()

# Terminate the asynchronous report.
rep.terminate()
rep.delete()
