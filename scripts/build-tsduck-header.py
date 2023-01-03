#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
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
#  Rebuilt tsduck.h, the global header for the TSDuck library. This script is
#  useful when source files are added to or removed from src/libtsduck.
#  Syntax: build-tsduck-header.py [out-file]
#
#-----------------------------------------------------------------------------

import tsbuild, sys, os, fnmatch

headers = {
    '': [],
    'private': [],
    'unix': [],
    'linux': [],
    'mac': [],
    'bsd': [],
    'freebsd': [],
    'netbsd': [],
    'openbsd': [],
    'dragonflybsd': [],
    'windows': []
}
unixen  = ['linux', 'mac', 'freebsd', 'netbsd', 'openbsd', 'dragonflybsd']
bsds    = ['freebsd', 'netbsd', 'openbsd', 'dragonflybsd']
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
        if os.path.isdir(path):
            collect_headers(path)
        elif name not in exclude and fnmatch.fnmatch(name, '*.h') and not fnmatch.fnmatch(name, '*Template.h'):
            headers[index].append(name)

# Generate the header file.
def generate_header(out):
    # Collect header files.
    rootdir = tsbuild.repo_root()
    collect_headers(rootdir + '/src/libtsduck')
    # Extend Unix-like systems with common Unix definitions.
    for osname in unixen:
        headers[osname].extend(headers['unix'])
    for osname in bsds:
        headers[osname].extend(headers['bsd'])
    # Remove non-public or unused definitions.
    del headers['private']
    del headers['unix']
    del headers['bsd']

    # Insert common source file header.
    tsbuild.write_source_header('//', 'TSDuck global header (include all headers)', file=out)

    # Print include directives.
    intro = '#pragma once'
    for system, files in headers.items():
        files = sorted(files, key = str.casefold)
        print(intro, file=out)
        intro = ''
        if system != '':
            print('#if defined(TS_%s)' % system.upper(), file=out)
        for name in (files):
            print('#include "%s"' % name, file=out)
        if system != '':
            print('#endif', file=out)

# Main code.
if __name__ == '__main__':
    if len(sys.argv) < 2:
        generate_header(sys.stdout)
    else:
        with open(sys.argv[1], 'w') as f:
            generate_header(f)
