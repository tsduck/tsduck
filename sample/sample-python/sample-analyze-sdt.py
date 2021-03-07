#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application : analyze the SDT in a transport stream.
# A TSProcessor is used, using plugin "tables" and XML output.
# The XML output is intercepted and parsed in a Python class.
# When found, various information are extracted from the XML structure.
#
#----------------------------------------------------------------------------

import tsduck
import xml.etree.ElementTree as xmlet

# This string is a user-defined marker to locate the XML line in the log.
# It can be anything that is sufficiently weird to be unique in the logs.
sdt_xml_marker = "@@_SDT_XML_@@"

# This method processes the parsed XML data from the SDT.
# Here, we just display the list of services.
def process_xml(root):
    sdt = root.find('./SDT')
    if sdt != None:
        version = int(sdt.attrib['version'], base=0)
        ts_id = int(sdt.attrib['transport_stream_id'], base=0)
        nw_id = int(sdt.attrib['original_network_id'], base=0)
        print('SDT version: %d, TS id: %d, original network id: %d' % (version, ts_id, nw_id))
        for srv in sdt.findall('./service'):
            id = int(srv.attrib['service_id'], base=0)
            sdesc = srv.find('./service_descriptor')
            if sdesc == None:
                name = '(unknown)'
                provider = '(unknown)'
            else:
                name = sdesc.attrib['service_name']
                provider = sdesc.attrib['service_provider_name']
            print('Service id: %d, name: "%s", provider: "%s"' % (id, name, provider))

# A Python class which handles TSDuck log messages.
class Logger(tsduck.AbstractAsyncReport):
    # This method is invoked each time a message is logged by TSDuck.
    def log(self, severity, message):
        # Filter, locate, extract and parse the XML output from plugin "tables".
        pos = message.find(sdt_xml_marker)
        if pos >= 0:
            process_xml(xmlet.fromstring(message[pos+len(sdt_xml_marker):]))

# Main program: see details in "sample-tsp.py" or "sample-message-handling.py".
rep = Logger()
tsp = tsduck.TSProcessor(rep)

tsp.input = ['file', 'file.ts']
tsp.plugins = [ ['tables', '--pid', '0x11', '--tid', '0x42', '--max-tables', '1', '--log-xml-line=' + sdt_xml_marker] ]
tsp.output = ['drop']

tsp.start()
tsp.waitForTermination()
tsp.delete()

rep.terminate()
rep.delete()
