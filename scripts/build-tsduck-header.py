#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        elif name not in exclude and fnmatch.fnmatch(name, '*.h'):
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
