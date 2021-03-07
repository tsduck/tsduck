#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application running a chain of plugins:
# Filter tables using plugin events to get binary tables in a Python class.
#
# See sample-filter-tables-log.py for an equivalent example using log lines
# to get a hexadecimal representation of the binary content of the tables.
#
#----------------------------------------------------------------------------

import tsduck

# A 32-bit plugin event code ("TABL" ASCII)
EVENT_CODE = 0x5441424C

# A pure Python class which handles TSDuck plugin events.
class EventHandler(tsduck.AbstractPluginEventHandler):

    # This method is invoked each time a TSDuck plugin signals an event for which we registered.
    def handlePluginEvent(self, context, data):
        print("==== Event code: 0x%X from plugin #%d (%s), data size: %d bytes, at TS packet %d" %
              (context.event_code, context.plugin_index, context.plugin_name, len(data), context.plugin_packets))
        print("Section: %s" % (data.hex()))

# Create an asynchronous report to log multi-threaded messages.
rep = tsduck.AsyncReport()

# Create an event handler to catch plugin events.
handler = EventHandler()

# Create a TS processor, register our event handler, set plugin chain.
tsp = tsduck.TSProcessor(rep)
tsp.registerEventHandler(handler, EVENT_CODE)
tsp.input = ['http', 'https://github.com/tsduck/tsduck-test/raw/master/input/test-001.ts']
tsp.plugins = [ ['tables', '--pid', '0', '--event-code', str(EVENT_CODE)] ]
tsp.output = ['drop']

# Run the TS processing and wait until completion.
tsp.start()
tsp.waitForTermination()
tsp.delete()

# Delete the handler.
handler.delete()

# Terminate the asynchronous report.
rep.terminate()
rep.delete()
