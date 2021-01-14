#!/usr/bin/env python3
#----------------------------------------------------------------------------
#
# TSDuck sample Python application running a chain of plugins
# and handling the TSDuck log messages in a Python class.
#
#----------------------------------------------------------------------------

import ts

# A pure Python class which handles TSDuck log messages.
class Logger(ts.AbstractAsyncReport):

    # Constructor.
    def __init__(self, severity = ts.Report.Info, sync_log = False, log_msg_count = 0):
        super().__init__(severity, sync_log, log_msg_count)

    # Log a message to the report.
    # This method is invoked each time a message is logged by TSDuck.
    def log(self, severity, message):
        print("Severity: %d, message: %s%s" % (severity, ts.Report.header(severity), message))


# Create an asynchronous report to log multi-threaded messages.
# In this example, this is a user-defined Python class which collects messages.
rep = Logger(severity = ts.Report.Verbose)
rep.info("TSDuck version: %s" % ts.__version__)

# Create a TS processor using the report.
tsp = ts.TSProcessor(rep)

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
