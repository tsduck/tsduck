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
#  Rebuilt tsduck.h, the global header for the TSDuck library. The content of
#  tsduck.h is output on standard output. This script is useful when source
#  files are added to or removed from the directory src/libtsduck.
# 
#-----------------------------------------------------------------------------

import sys, os, fnmatch

headers = {'': [], 'private': [], 'unix': [], 'linux': [], 'mac': [], 'windows': []}
exclude = ['tsduck.h', 'tsBeforeStandardHeaders.h', 'tsAfterStandardHeaders.h']

# Recursively collect header files.
def collect_headers(root):
    # List into which the local files are appended.
    index = os.path.basename(root)
    if index not in headers.keys():
        index = ''
    # Loop on all files in directory
    for name in os.listdir(root):
        path = root + os.sep + name
        if name not in exclude and fnmatch.fnmatch(name, '*.h') and not fnmatch.fnmatch(name, '*Template.h'):
            headers[index].append(name)
        elif os.path.isdir(path):
            collect_headers(path)

# Collect header files.
rootdir = os.path.abspath('.' if len(sys.argv) < 2 else sys.argv[1]).rstrip(os.sep)
collect_headers(rootdir)
headers['linux'].extend(headers['unix'])
headers['mac'].extend(headers['unix'])
del headers['private']
del headers['unix']

# Insert common source file header.
with open(os.path.dirname(rootdir) + os.sep + 'HEADER.txt') as f:
    print(f.read())

# Print include directives.
separator = '#pragma once'
for system, files in headers.items():
    files = sorted(files, key = str.casefold)
    print(separator)
    separator = ''
    if system != '':
        print('#if defined(TS_%s)' % system.upper())
    for name in (files):
        print('#include "%s"' % name)
    if system != '':
        print('#endif')
