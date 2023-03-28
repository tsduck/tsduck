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
tsp.app_name = "python-sample-spliceinject" # informational only, for log messages

# Set plugin chain.
tsp.input = ['craft', '--count', '1000', '--pid', '100', '--payload-pattern', '0123']

# sample service id or name
service = "1010"

# Modifies PMT To comply with the SCTE 35 standard
plugin_pmt = f"pmt --service {service} --add-programinfo-id 0x43554549 \
    --add-pid 600/0x86"

# Actual splice injecting of splices from splice*.xml and/or udp 4444
plugin_splice_inject = f"spliceinject --service {service} --files splice*.xml \
    --udp 4444"

# Remove extra input stuffing
plugin_negate_stuffing = "filter --negate --pid 0x1FFF"

tsp.plugins = [
    plugin_pmt.split(),
    plugin_splice_inject(),
    plugin_negate_stuffing.split(),
]

tsp.output = ['drop']

# Run the TS processing and wait until completion.
tsp.start()
tsp.waitForTermination()
tsp.delete()

# Terminate the asynchronous report.
rep.terminate()
rep.delete()
