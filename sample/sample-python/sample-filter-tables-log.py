#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application running a chain of plugins:
# Filter tables using --log-hexa-line to get binary tables in a Python class.
#
# See sample-filter-tables-event.py for an equivalent example using plugin
# events to get a binary content of the tables.
#
#----------------------------------------------------------------------------

import tsduck, sys

# This string is a user-defined marker to locate the hexa line in the log.
LOG_PREFIX = "#TABLE#"

# A Python class which handles TSDuck log messages.
class Logger(tsduck.AbstractAsyncReport):
    # This method is invoked each time a message is logged by TSDuck.
    def log(self, severity, message):
        # Filter, locate, extract and parse the hexa output from plugin "tables".
        pos = message.find(LOG_PREFIX)
        if pos >= 0:
            # This is a log line containing an hexa dump.
            hexa = message[pos+len(LOG_PREFIX):]
            print("Table: %s" % (hexa))
        else:
            # This is a real log message, just display it.
            print(tsduck.Report.header(severity) + message, file=sys.stderr)

# Create an asynchronous report to log multi-threaded messages.
rep = Logger()

# Create a TS processor, set plugin chain.
tsp = tsduck.TSProcessor(rep)
tsp.input = ['http', 'https://github.com/tsduck/tsduck-test/raw/master/input/test-001.ts']
tsp.plugins = [ ['tables', '--pid', '0', '--log-hexa-line=' + LOG_PREFIX] ]
tsp.output = ['drop']

# Run the TS processing and wait until completion.
tsp.start()
tsp.waitForTermination()
tsp.delete()

# Terminate the asynchronous report.
rep.terminate()
rep.delete()
