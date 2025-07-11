#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application : analyze a transport stream.
# A TSProcessor is used, using plugin "analyze" and JSON output.
# The JSON output is intercepted and parsed in a Python class.
# When found, the list of services in the transport stream is displayed.
# Any other information on the TS can be similarly extracted and used.
#
#----------------------------------------------------------------------------

import tsduck, json, sys

# This string is a user-defined marker to locate the JSON line in the log.
# It can be anything that is sufficiently weird to be unique in the logs.
json_marker = "@@_JSON_HERE_@@"

# This method processes the parsed JSON data from the TS analysis.
# Here, we just display the list of services.
def process_json(data):
    for srv in data['services']:
        print('Name: "%s", provider: "%s", bitrate: %d b/s' % (srv['name'], srv['provider'], srv['bitrate']))

# A Python class which handles TSDuck log messages.
class Logger(tsduck.AbstractAsyncReport):
    # This method is invoked each time a message is logged by TSDuck.
    def log(self, severity, message):
        # Filter, locate, extract and parse the JSON output from plugin "analyze".
        pos = message.find(json_marker)
        if pos >= 0:
            # This is a log line containing a JSON structure.
            process_json(json.loads(message[pos+len(json_marker):]))
        else:
            # This is a real log message, just display it.
            print(tsduck.Report.header(severity) + message, file=sys.stderr)

# Get input file from command line, default to file.ts.
input_file = sys.argv[1] if len(sys.argv) > 1 else 'file.ts'

# Main program: see details in "sample-tsp.py" or "sample-message-handling.py".
rep = Logger()
tsp = tsduck.TSProcessor(rep)

tsp.input = ['file', input_file]
tsp.plugins = [ ['analyze', '--json-line=' + json_marker] ]
tsp.output = ['drop']

tsp.start()
tsp.waitForTermination()
tsp.delete()

rep.terminate()
rep.delete()
