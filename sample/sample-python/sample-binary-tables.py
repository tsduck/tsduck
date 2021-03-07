#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application : PSI/SI tables using Python byte arrays.
#
#----------------------------------------------------------------------------

import tsduck

# Create a SectionFile.
rep = tsduck.StdErrReport()
duck = tsduck.DuckContext(rep)
file1 = tsduck.SectionFile(duck)

# Loading inline XML table.
file1.loadXML("""<?xml version="1.0" encoding="UTF-8"?>
<tsduck>
  <PAT transport_stream_id="10">
    <service service_id="1" program_map_PID="100"/>
    <service service_id="2" program_map_PID="200"/>
  </PAT>
</tsduck>""")

# Convert to binary.
data = file1.toBinary()
print("---- Binary content ----")
print(data.hex())
print()

# Build another SectionFile and load the binary data.
file2 = tsduck.SectionFile(duck)
file2.fromBinary(data)

# Convert the second SectionFile to XML.
print("---- XML content ----")
print(file2.toXML())

# Deallocate C++ resources (in reverse order from creation).
file2.delete()
file1.delete()
duck.delete()
rep.delete()
