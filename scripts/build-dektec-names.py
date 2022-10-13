#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2022, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
# 
#  Build the file tsduck.dektec.names, containing Dektec-specific names.
#  Syntax: build-dektec-names.py in-file out-file
# 
#-----------------------------------------------------------------------------

import sys, os, re

debug_mode = '--debug' in sys.argv
def debug(msg):
    if debug_mode:
        print('[debug] %s' % msg)

if debug_mode:
    sys.argv.remove('--debug')
if len(sys.argv) != 3:
    print('Usage: %s in-file out-file' % sys.argv[0], file=sys.stderr)
    exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]
debug('script: %s' % os.path.abspath(sys.argv[0]))
debug('input file: %s' % input_file)
debug('output file: %s' % output_file)
    
with open(output_file, 'w') as output:
    print('# Auto-generated file', file=output)
    print('[DtCaps]', file=output)
    if sys.argv[1] != '':
        with open(input_file, 'r', encoding='utf-8') as input:
            for line in input:
                match = re.search(r'#define\s+DTAPI_CAP_.*\sDtapi::DtCaps\(([\d]+)\)\s*//\s*(.+)$', line.strip())
                if match is not None:
                    print('%s = %s' % (match.group(1), match.group(2)), file=output)

debug('output file exists: %s' % os.path.exists(output_file))
debug('output file size: %d' % os.path.getsize(output_file) if os.path.exists(output_file) else 0)
