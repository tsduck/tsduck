#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Build the file tsduck.dektec.names, containing Dektec-specific names.
#  Syntax: build-dektec-names.py in-file out-file
#
#-----------------------------------------------------------------------------

import tsbuild, sys, os, re

if len(sys.argv) != 3:
    print('Usage: %s in-file out-file' % sys.argv[0], file=sys.stderr)
    exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

with open(output_file, 'w') as output:
    tsbuild.write_source_header('#', 'Registered names for Dektec devices', file=output)
    print('[DtCaps]', file=output)
    print('Bits = 32', file=output)
    if input_file != '':
        with open(input_file, 'r', encoding='utf-8') as input:
            for line in input:
                match = re.search(r'#define\s+DTAPI_CAP_.*\sDtapi::DtCaps\(([\d]+)\)\s*//\s*(.+)$', line.strip())
                if match is not None:
                    print('%s = %s' % (match.group(1), match.group(2)), file=output)
