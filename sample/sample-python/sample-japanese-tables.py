#!/usr/bin/env python
#----------------------------------------------------------------------------
#
# TSDuck sample Python application : manipulate PSI/SI tables using various
# options (DTV standard, character sets, etc.)
#
# The input file japanese-tables.bin, located in the same directory as the
# sample source code, contains a TOT and an SDT from the Japanese DTTV.
# The standard is ISDB-T which reuses DVB tables such as TOT and SDT but
# uses different representations for character strings, time reference and
# define ISDB-specific descriptors. When interpreted with the DVB defaults,
# the strings, times and some descriptors are incorrect. The proper settings
# for Japan shall be set before deserializing the tables.
#
#----------------------------------------------------------------------------

import ts

# Create a SectionFile.
rep = ts.StdErrReport()
duck = ts.DuckContext(rep)
file = ts.SectionFile(duck)

# Load a binary file containing tables which were capture on a Japanese TS.
file.loadBinary("japanese-tables.bin")
print("Loaded %d bytes, %d sections, %d tables" % (file.binarySize(), file.sectionsCount(), file.tablesCount()))
print()

# Convert to XML.
print("---- XML file content with default DVB settings ----")
print(file.toXML())

# Use typical settings for Japan.
duck.addStandards(ts.DuckContext.ISDB | ts.DuckContext.JAPAN)
duck.setDefaultCharset("ARIB-STD-B24")
duck.setTimeReference("JST")

# Convert to XML again, see the difference.
print("---- XML file content with Japanese settings ----")
print(file.toXML())

# Deallocate C++ resources (in reverse order from creation).
file.delete()
duck.delete()
rep.delete()
