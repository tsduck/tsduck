#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application : reorganize EIT's according to DVB rules
#
#----------------------------------------------------------------------------

import tsduck, os

# Create a SectionFile.
rep = tsduck.StdErrReport()
duck = tsduck.DuckContext(rep)
file = tsduck.SectionFile(duck)

# Loading inline XML EIT's. There is one EITp/f without following event and an EIT
# Schedule with two events which should not be grouped and in the wrong order.
file.loadXML("""<?xml version="1.0" encoding="UTF-8"?>
<tsduck>
  <EIT type="pf" service_id="1" transport_stream_id="10" original_network_id="20">
    <event event_id="1000" start_time="2020-06-10 15:00:00" duration="00:10:00">
      <short_event_descriptor language_code="eng">
        <event_name>Event 1000 name</event_name>
        <text>Event 1000 text</text>
      </short_event_descriptor>
    </event>
  </EIT>
  <EIT type="0" service_id="1" transport_stream_id="10" original_network_id="20">
    <event event_id="2001" start_time="2020-06-13 11:30:00" duration="00:10:00">
      <short_event_descriptor language_code="foo">
        <event_name>Event 2001 name</event_name>
        <text>Event 2001 text</text>
      </short_event_descriptor>
    </event>
    <event event_id="2000" start_time="2020-06-10 18:30:00" duration="00:10:00">
      <short_event_descriptor language_code="foo">
        <event_name>Event 2000 name</event_name>
        <text>Event 2000 text</text>
      </short_event_descriptor>
    </event>
  </EIT>
</tsduck>""")

# Reorganize EIT sections according to ETSI TS 101 211 rules.
file.reorganizeEITs()

# Save binary section file.
file.saveBinary("eit.bin")

# Launch a "tstabdump" command to dump the EIT sections.
os.system("tstabdump eit.bin")

# Deallocate C++ resources (in reverse order from creation).
file.delete()
duck.delete()
rep.delete()
