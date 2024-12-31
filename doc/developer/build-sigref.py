#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script generates the body of an asciidoc table for all supported
#  tables or descriptors, in TSDuck, from the source files.
#
#-----------------------------------------------------------------------------

import sys, os, re, glob

# Need a src/libtsduck/dtv subdirectory as parameter.
if len(sys.argv) < 2:
    print("syntax: %s dtv-subdirectory [output-file]" % sys.argv[0], file=sys.stderr)
    exit(1)

src_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))) + os.sep + 'src'
sig_dir = src_dir + os.sep + 'libtsduck' + os.sep + 'dtv' + os.sep + sys.argv[1]
out_file = sys.argv[2] if len(sys.argv) >= 3 else None

if not os.path.isdir(sig_dir):
    print("%s: directory %s not found" % (sys.argv[0], sig_dir), file=sys.stderr)
    exit(1)

# Build a list of lines to display.
lines = []

# Loop on all .h files in the directory.
for header in glob.glob(sig_dir + os.sep + '**' + os.sep + 'ts*.h', recursive=True):
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
                        # Fix incomplete, incorrect, too verbose, references.
                        doc = re.sub(r'[, ]*\| *', ', ', doc)
                        doc = re.sub(r'[, ]* section +', ', ', doc)
                        doc = re.sub(r'[, ]* clause +', ', ', doc)
                        doc = re.sub(r'[\.\s]*$', '', doc)
                        doc = re.sub(r'ITU-T +Rec\.* H', 'ITU-T H', doc)
                        doc = re.sub(r'^SCTE', 'ANSI/SCTE', doc)
                        lines.append((xml, classname, doc))
                        break

# Write the line in output file.
def write_table(out, lines):
    for l in lines:
        print('|%s' % l[0], file=out)
        print('|%s' % l[1], file=out)
        print('|%s' % l[2], file=out)
        print('', file=out)

# Sort the lines and display them.
lines = sorted(lines, key=lambda tup: tup[0].lower())
if out_file is None:
    write_table(sys.stdout, lines)
else:
    with open(out_file, 'w', encoding='utf-8') as out:
        write_table(out, lines)
