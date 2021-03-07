#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application running a chain of plugins
# and handling plugin events in a Python class.
#
#----------------------------------------------------------------------------

import tsduck

# A 32-bit plugin event code ("MPE1" ASCII)
EVENT_CODE = 0x4D504531

# A pure Python class which handles TSDuck plugin events.
class EventHandler(tsduck.AbstractPluginEventHandler):

    # This method is invoked each time a TSDuck plugin signals an event for which we registered.
    def handlePluginEvent(self, context, data):
        print("==== Event code: 0x%X from plugin #%d (%s), data size: %d bytes, at TS packet %d" %
              (context.event_code, context.plugin_index, context.plugin_name, len(data), context.plugin_packets))
        print("MPE datagram: %s" % (data.hex()))
        return True

# Create an asynchronous report to log multi-threaded messages.
rep = tsduck.AsyncReport()

# Create an event handler to catch plugin events.
handler = EventHandler()

# Create a TS processor, register our event handler, set plugin chain.
tsp = tsduck.TSProcessor(rep)
tsp.registerEventHandler(handler, EVENT_CODE)
tsp.input = ['http', 'https://github.com/tsduck/tsduck-test/raw/master/input/test-016.ts']
tsp.plugins = [
    ['mpe', '--pid', '2001', '--max-datagram', '2', '--event-code', str(EVENT_CODE)],
]
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
