#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application using tsp memory plugins.
#
# This sample application uses the input and output memory plugins at the
# same time. Real applications may use one of them only.
#
#----------------------------------------------------------------------------

import tsduck


#----------------------------------------------------------------------------
# An event handler for memory input plugin.
# It is invoked by the "memory" input plugin each time TS packets are needed.
#----------------------------------------------------------------------------

# For the purpose of demonstration, this class sends a predefined list of
# transport stream packets one by one. A real application should get its
# input packets from somewhere else and return them in larger chunks.

class InputHandler(tsduck.AbstractPluginEventHandler):

    # A predefined list of transport stream packets used as input.
    _PACKETS = (
        '4700641027CD888B7299B97E9FC811D834ECFE57FFF9802FF5CD0B05D09F2123604728804EE3AD89FF9285935E41B8CFEDA841EC13227B5C37D2DB06B1B5F5629D3C48995BDDF9661AD1E9E1436FDF170E85D1989764FB18F991B67D5ABF2DADCD28C2DEB2479257F55B1C71C8F0C9213E0DC3F83215EF5581F605A80D83D28D34291C2A0107E4504855D4A8DF94474C2A0A84B56FD4F3878CB12B47850A2D1D9A2B18459901C19379FB94F352241DB4D92F2D95D6FAA8A546625401',
        '470064109E9ED47006DB11D47AA597112EBC7D627CA690B849721ABDA0040D5942DD9129A4D421D163D836C6AED265E0A585E9F9D97E44F17BF2491922EC8D9BBCABE3C90A983FF3AF3B1BE952CF708A2C22F9F8937BAFD899974F54593F69C4FBD693C80080088D1B97B2DD9D392B55A8AA2D15DBF43EC64C38443E040583AA528E81D6692033AAB65CFFB8CBE5CCABB873E8C3DE2538CA8F38F433EEA4A657172FC2CBE9D7CC42CA27FFDAF65D5386BB7E0A62A983D0542D501FBC',
        '47006410377169A409F0E47FCC4CDA9F2190127AEC0448C86C9D0CF23E3E5CCAF398F3CC09446130911BAA668E8A2DDC239BF9E08F6779DD7C6CF83582138B23FF1CE45B531795D00840AE6BFFF980276FE1F28285B6F5005F49F4674584E876E8B9E006121A6A5F167D595A4DA27F4D938593EA792D8CABA16CE40480B18740CB0793EDAA8C82DDA64AFB1F956A2CEF610973079CDA03E04994194CE1CF99E942DECD4FEC79D8F6E2E2EB2A6C662EBE445CC797E23AC9B552A8EF3F')

    # Constructor.
    def __init__(self, report):
        super().__init__()
        self._report = report
        self._next_packet = 0

    # This event handler is called each time the memory plugin needs input packets.
    def handlePluginEvent(self, context, data):
        if self._next_packet < len(InputHandler._PACKETS) and context.max_data_size >= tsduck.PKT_SIZE:
            self._report.info("returning input packet #%d" % (self._next_packet))
            packet = bytearray.fromhex(InputHandler._PACKETS[self._next_packet])
            self._next_packet = self._next_packet + 1
            return packet
        else:
            self._report.info("end of input")


#----------------------------------------------------------------------------
# An event handler for memory output plugin.
# It is invoked by the "memory" output plugin each time TS packets are sent.
#----------------------------------------------------------------------------

class OutputHandler(tsduck.AbstractPluginEventHandler):

    # Constructor.
    def __init__(self, report):
        super().__init__()
        self._report = report

    # This event handler is called each time the memory plugin sends output packets.
    def handlePluginEvent(self, context, data):
        packets_count = len(data) // tsduck.PKT_SIZE
        self._report.info("received %d output packets" % (packets_count))
        for i in range(packets_count):
            packet = data[i * tsduck.PKT_SIZE : tsduck.PKT_SIZE]
            self._report.info("packet #%d: %s" % (i, packet.hex()))


#----------------------------------------------------------------------------
# Application entry point.
#----------------------------------------------------------------------------

# Create a thread-safe asynchronous report.
report = tsduck.AsyncReport()

# Create our event handlers for the memory plugins.
input = InputHandler(report)
output = OutputHandler(report)

# Create a transport stream processor and register our event handlers.
tsp = tsduck.TSProcessor(report)
tsp.registerInputEventHandler(input)
tsp.registerOutputEventHandler(output)

# Build tsp options.
tsp.input = ['memory']
tsp.plugins = [ ['count'] ]
tsp.output = ['memory']

# Run the TS processing and wait until completion.
tsp.start()
tsp.waitForTermination()
tsp.delete()

# Delete the event handlers.
input.delete()
output.delete()

# Terminate the asynchronous report.
report.terminate()
report.delete()
