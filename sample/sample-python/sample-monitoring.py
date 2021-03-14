#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application running a chain of plugins on the long
# run with CPU and memory monitoring.
#
#----------------------------------------------------------------------------

import tsduck
import os
import tempfile

# Create an asynchronous report to log multi-threaded messages.
rep = tsduck.AsyncReport(severity = tsduck.Report.Verbose)

# Create and start a background system monitor.
mon = tsduck.SystemMonitor(rep)
mon.start()

# Build a temporary file name to download a real TS file.
url = "https://tsduck.io/streams/france-dttv/tnt-uhf30-546MHz-2019-01-22.ts"
tsfile = tempfile.gettempdir() + os.path.sep + tempfile.gettempprefix() + str(os.getpid()) + ".ts"

# First phase: Download the TS file:
print("Downloading %s to %s ..." % (url, tsfile))

tsp = tsduck.TSProcessor(rep)
tsp.input = ['http', url]
tsp.output = ['file', tsfile]
tsp.start()
tsp.waitForTermination()
tsp.delete()

# Second phase: Play the file at regulated speed several times.
# Must use another instance of tsduck.TSProcessor.
print("Playing %s ..." % (tsfile))

tsp = tsduck.TSProcessor(rep)
tsp.input = ['file', tsfile, '--repeat', '2']
tsp.plugins = [ ['regulate'] ]
tsp.output = ['drop']

tsp.start()
tsp.waitForTermination()
tsp.delete()

# Terminate the system monitor.
mon.stop()
mon.waitForTermination()
mon.delete()

# Terminate the asynchronous report.
rep.terminate()
rep.delete()

# Delete temporary TS file.
os.remove(tsfile)
