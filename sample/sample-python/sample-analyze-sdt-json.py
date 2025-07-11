#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application : analyze the SDT in a transport stream.
# A TSProcessor is used, using plugin "tables" and JSON output.
# The JSON output is intercepted and parsed in a Python class.
# When found, various information are extracted from the JSON structure.
# The file sample-analyze-sdt-xml.py does the same thing using XML output.
#
#----------------------------------------------------------------------------

import tsduck, json, sys

# This string is a user-defined marker to locate the JSON line in the log.
# It can be anything that is sufficiently weird to be unique in the logs.
sdt_json_marker = "@@_SDT_JSON_@@"

# This method processes the parsed JSON data from the SDT.
# Here, we just display the list of services.
def process_json(root):
    if root['#name'] == 'SDT':
        version = root['version']
        ts_id = root['transport_stream_id']
        nw_id = root['original_network_id']
        print('SDT version: %d, TS id: %d, original network id: %d' % (version, ts_id, nw_id))
        for srv in [n for n in root['#nodes'] if n['#name'] == 'service']:
            id = srv['service_id']
            name = '(unknown)'
            provider = '(unknown)'
            for sdesc in [n for n in srv['#nodes'] if n['#name'] == 'service_descriptor']:
                name = sdesc['service_name']
                provider = sdesc['service_provider_name']
            print('Service id: %d, name: "%s", provider: "%s"' % (id, name, provider))

# A Python class which handles TSDuck log messages.
class Logger(tsduck.AbstractAsyncReport):
    # This method is invoked each time a message is logged by TSDuck.
    def log(self, severity, message):
        # Filter, locate, extract and parse the JSON output from plugin "tables".
        pos = message.find(sdt_json_marker)
        if pos >= 0:
            # This is a log line containing a JSON structure.
            process_json(json.loads(message[pos+len(sdt_json_marker):]))
        else:
            # This is a real log message, just display it.
            print(tsduck.Report.header(severity) + message, file=sys.stderr)

# Get input file from command line, default to file.ts.
input_file = sys.argv[1] if len(sys.argv) > 1 else 'file.ts'

# Main program: see details in "sample-tsp.py" or "sample-message-handling.py".
rep = Logger()
tsp = tsduck.TSProcessor(rep)

tsp.input = ['file', input_file]
tsp.plugins = [ ['tables', '--pid', '0x11', '--tid', '0x42', '--max-tables', '1', '--log-json-line=' + sdt_json_marker] ]
tsp.output = ['drop']

tsp.start()
tsp.waitForTermination()
tsp.delete()

rep.terminate()
rep.delete()
