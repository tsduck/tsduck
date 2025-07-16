#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Build the file tsduck.tables.model.xml, the XML model for signalization.
#  Syntax: build-tables-model.py out-file in-file-or-dir ...
#
#-----------------------------------------------------------------------------

import tsbuild, sys, os, fnmatch
import xml.etree.ElementTree as ET

# Process a .xml file or a directory.
def process_file_or_dir(path, root):
    if os.path.isdir(path):
        # Recurse in all subdirectories and XML files.
        for name in os.listdir(path):
            subpath = path + os.sep + name
            if os.path.isdir(subpath) or fnmatch.fnmatch(name, '*.xml'):
                process_file_or_dir(subpath, root)
    elif os.path.isfile(path):
        # Load XML file and process all first level elements.
        for topic in ET.parse(path).getroot():
            rtopic = root.find(topic.tag)
            if rtopic is None:
                # Topic not yet known in resulting XML, simply copy it.
                root.append(topic)
            else:
                # Copy all subtopics.
                for subtopic in topic:
                    rtopic.append(subtopic)

# Main code.
if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: %s out-file in-file-or-dir ...' % sys.argv[0], file=sys.stderr)
        exit(1)
    # Read first XML file as initial template.
    root = ET.parse(sys.argv[2]).getroot()
    # Merge all other input files.
    for f in sys.argv[3:]:
        process_file_or_dir(f, root)
    # Sort subtopics (all tables and descriptors) in each topic.
    for topic in root:
        topic[:] = sorted(topic, key=lambda e: e.tag.lower() if isinstance(e.tag, str) else '')
    # Pretty print the XML file.
    if hasattr(ET, 'indent'):
        ET.indent(root)
    else:
        tsbuild.error('XML pretty printing not supported by Python %s' % tsbuild.python_version())
    # Generate the output file, remove empty lines.
    with open(sys.argv[1], 'w', encoding='utf-8') as output:
        print('<?xml version="1.0" encoding="UTF-8"?>', file=output)
        print('', file=output)
        tsbuild.write_source_header('--', 'XML model for all tables and descriptors', firstline='<!--', lastline='-->', file=output)
        for line in ET.tostring(root, encoding='unicode').splitlines():
            line = line.rstrip()
            if line:
                print(line, file=output)
