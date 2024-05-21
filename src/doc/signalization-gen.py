#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script generates the file signalization.md, in the same directory.
#  The output is a markdown file listing all supported signalization, tables
#  and descriptors, in TSDuck, from the source files.
#
#-----------------------------------------------------------------------------

import sys, os, re, glob

doc_dir = os.path.dirname(os.path.realpath(__file__))
src_dir = os.path.dirname(doc_dir)
dtv_dir = src_dir + os.sep + 'libtsduck' + os.sep + 'dtv' + os.sep
out_file  = doc_dir + os.sep + 'signalization.md'

file_header = """
# Tables and descriptors reference   {#sigxref}

All signalization tables and descriptors which are supported by TSDuck are
documented in the TSDuck user's guide, appendix D "PSI/SI XML Reference Model".

The tables below summarize all available structures and the reference of
the standard which specifies them.
"""

# Format all structures (tables or descriptors) inside a directory tree.
def list_all_structures(output, dirname, title, label):
    print("", file=output)
    print("# %s   {#%s}" % (title, label), file=output)
    print("", file=output)
    print("| XML name | C++ class | Defining document", file=output)
    print("| -------- | --------- | -----------------", file=output)
    # Build a list of lines to display.
    lines = []
    # Loop on all .h files in the directory.
    for header in glob.glob(dirname + os.sep + '**' + os.sep + 'ts*.h', recursive=True):
        classname = re.search(r'^ts(.*)\.h$', os.path.basename(header)).group(1)
        source = os.path.splitext(header)[0] + '.cpp'
        xml = ''
        if os.path.exists(source):
            with open(source, 'r', encoding='utf-8') as input:
                for line in input:
                    match = re.search(r'^\s*#define\s+MY_XML_NAME\s+u"([^"]*)"', line)
                    if match is not None:
                        xml = match.group(1)
                        break
            if xml != '':
                with open(header, 'r', encoding='utf-8') as input:
                    for line in input:
                        match = re.search(r'@see\s+(.*)$', line)
                        if match is not None:
                            doc = match.group(1)
                            doc = re.sub(r'[, ]*\| *', ', ', doc)
                            doc = re.sub(r'[, ]* section +', ', ', doc)
                            doc = re.sub(r'[, ]* clause +', ', ', doc)
                            doc = re.sub(r'[\.\s]*$', '', doc)
                            doc = re.sub(r'ITU-T +Rec\.* H', 'ITU-T H', doc)
                            doc = re.sub(r'^SCTE', 'ANSI/SCTE', doc)
                            lines.append('| %s | ts::%s | %s' % (xml, classname, doc))
                            break
    # Sort the lines and display them.
    lines = sorted(lines, key=str.casefold)
    for l in lines:
        print(l, file=output)

# Main code.
with open(out_file, 'w') as output:
    print(file_header, file=output)
    list_all_structures(output, dtv_dir + "tables", "Tables", "sigxtables")
    list_all_structures(output, dtv_dir + "descriptors", "Descriptors", "sigxdescs")
