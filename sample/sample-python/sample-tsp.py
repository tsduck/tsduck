#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application running a chain of plugins.
#
#----------------------------------------------------------------------------

import tsduck

# Create an asynchronous report to log multi-threaded messages.
# Initial level is verbose, using time-stamped messages.
rep = tsduck.AsyncReport(severity = tsduck.Report.Verbose, timed_log = True)
rep.info("TSDuck version: %s" % tsduck.__version__)

# Create a TS processor using the report.
tsp = tsduck.TSProcessor(rep)

# Set some global TS processing options.
tsp.add_input_stuffing = [1, 10]   # one null packet every 10 input packets
tsp.bitrate = 1000000              # nominal bitrate is 1 Mb/s
tsp.app_name = "demo"              # informational only, for log messages

# Set plugin chain.
tsp.input = ['craft', '--count', '1000', '--pid', '100', '--payload-pattern', '0123']
tsp.plugins = [
    ['until', '--packet', '100'],
    ['count'],
]
tsp.output = ['drop']

# Run the TS processing and wait until completion.
tsp.start()
tsp.waitForTermination()
tsp.delete()

# Terminate the asynchronous report.
rep.terminate()
rep.delete()
