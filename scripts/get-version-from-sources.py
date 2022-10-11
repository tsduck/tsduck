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
#  This script extracts the TSDuck version from the source files.
# 
#-----------------------------------------------------------------------------

import sys, os, re, struct, subprocess

rootdir = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
versfile = (rootdir + '/src/libtsduck/tsVersion.h').replace('/', os.sep)

# Run a command and get stdout+stderr.
def run(cmd, err=subprocess.STDOUT):
    try:
        return subprocess.check_output(cmd, stderr=err).decode('utf-8')
    except:
        return ''

# Get the content of a text file.
def read(filename):
    with open(filename, 'r') as f:
        return f.read()

# Get a description of the distro.
def distro():
    if os.name == 'nt':
        return '-Win%d' % (8 * struct.calcsize('P'))
    if os.uname().sysname == 'Darwin':
        version = re.sub(r'\s', '', run(['sw_vers', '-productVersion']))
        return '.macos' + re.sub(r'\..*', '', version)
    name = re.sub(r'\s', '', run(['lsb_release', '-si'], None)).lower().replace('linuxmint', 'mint')
    if name != '':
        version = re.sub(r'\s', '', run(['lsb_release', '-sr'], None))
        return '.' + name + re.sub(r'\..*', '', version)
    if os.path.exists('/etc/fedora-release'):
        match = re.search(r' release (\d+)', read('/etc/fedora-release'), re.MULTILINE | re.DOTALL | re.IGNORECASE)
        return '.fc' + ('' if match is None else match.group(1))
    if os.path.exists('/etc/redhat-release'):
        match = re.search(r' release (\d+)', read('/etc/redhat-release'), re.MULTILINE | re.DOTALL | re.IGNORECASE)
        return '.el' + ('' if match is None else match.group(1))
    if os.path.exists('/etc/alpine-release'):
        match = re.search(r'^(\d[\.\d]+)\.[^\.]$', read('/etc/alpine-release'), re.MULTILINE | re.DOTALL | re.IGNORECASE)
        return '.alpine' + ('' if match is None else match.group(1).replace('.',''))
    return ''

# Extract the version from the header file.
match = re.search(r'^ *#define +TS_VERSION_MAJOR +(\d+) *$.*' +
                  r'^ *#define +TS_VERSION_MINOR +(\d+) *$.*' +
                  r'^ *#define +TS_COMMIT +(\d+) *$',
                  read(versfile),
                  re.MULTILINE | re.DOTALL)
if match is None:
    major, minor, commit = 0, 0, 0
else:
    major, minor, commit = match.group(1), match.group(2), match.group(3)

# Print requested version.
arg = sys.argv[1] if len(sys.argv) > 1 else ''
if arg == '':
    print('%s.%s-%s' % (major, minor, commit))
elif arg == '--major':
    print(major)
elif arg == '--minor':
    print(minor)
elif arg == '--commit':
    print(commit)
elif arg == '--distro':
    print(distro())
elif arg == '--full':
    print('%s.%s-%s%s' % (major, minor, commit, distro()))
elif arg == '--main':
    print('%s.%s' % (major, minor))
elif arg == '--dotted':
    print('%s.%s.%s' % (major, minor, commit))
elif arg == '--windows':
    print('%s.%s.%s.0' % (major, minor, commit))
else:
    print('invalid option: %s' % arg, file=sys.stderr)
    exit(1)
