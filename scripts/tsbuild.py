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
#  Python common utilities.
#
#-----------------------------------------------------------------------------

import sys, os, re, subprocess, struct

# Get the scripts directory (directory of this module).
def scripts_dir():
    return os.path.dirname(os.path.abspath(sys.argv[0] if __name__ == '__main__' else __file__))

# Get the root of the repository.
def repo_root():
    dir = scripts_dir()
    while True:
        if os.path.isdir(dir + '/.git') or os.path.isfile(dir + '/LICENSE.txt'):
            return dir
        parent = os.path.dirname(dir)
        if os.path.samefile(dir, parent):
            raise ValueError('repository directory not found below %s' % dir)
        dir = parent

# Run a command and get stdout+stderr.
# Need a list of strings as command.
def run(cmd, err=subprocess.STDOUT, cwd=None):
    try:
        return subprocess.check_output(cmd, stderr=err, cwd=cwd).decode('utf-8')
    except:
        return ''

# Get the content of a text file.
def read(filename):
    with open(filename, 'r', encoding='utf-8') as f:
        return f.read()

# Write the content of a text file.
def write(filename, content):
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(content)

# Get the TSDuck version from the file tsVersion.h.
# Return as major, minor, commit/
def version():
    match = re.search(r'^ *#define +TS_VERSION_MAJOR +(\d+) *$.*' +
                      r'^ *#define +TS_VERSION_MINOR +(\d+) *$.*' +
                      r'^ *#define +TS_COMMIT +(\d+) *$',
                      read(repo_root() + '/src/libtsduck/tsVersion.h'),
                      re.MULTILINE | re.DOTALL)
    if match is None:
        return 0, 0, 0
    else:
        return match.group(1), match.group(2), match.group(3)

# Get a description of the operating system and distro.
def distro():
    if os.name == 'nt':
        return 'win%d' % (8 * struct.calcsize('P'))
    if os.uname().sysname == 'Darwin':
        version = re.sub(r'\s', '', run(['sw_vers', '-productVersion']))
        return 'macos' + re.sub(r'\..*', '', version)
    name = re.sub(r'\s', '', run(['lsb_release', '-si'], None)).lower().replace('linuxmint', 'mint')
    if name != '':
        version = re.sub(r'\s', '', run(['lsb_release', '-sr'], None))
        return name + re.sub(r'\..*', '', version)
    if os.path.exists('/etc/fedora-release'):
        match = re.search(r' release (\d+)', read('/etc/fedora-release'), re.MULTILINE | re.DOTALL | re.IGNORECASE)
        return 'fc' + ('' if match is None else match.group(1))
    if os.path.exists('/etc/redhat-release'):
        match = re.search(r' release (\d+)', read('/etc/redhat-release'), re.MULTILINE | re.DOTALL | re.IGNORECASE)
        return 'el' + ('' if match is None else match.group(1))
    if os.path.exists('/etc/alpine-release'):
        match = re.search(r'^(\d[\.\d]+)\.[^\.]$', read('/etc/alpine-release'), re.MULTILINE | re.DOTALL | re.IGNORECASE)
        return 'alpine' + ('' if match is None else match.group(1).replace('.',''))
    return ''

# Get a description of the operating system and distro as a file suffix.
def distro_suffix():
    suffix = distro()
    if suffix != '':
        suffix = '.' + suffix
    return suffix

# Write a standard source header in an output file.
def write_source_header(comment_prefix, description=None, file=sys.stdout):
    with open(repo_root() + '/src/HEADER.txt') as input:
        last = ''
        for line in input:
            last = comment_prefix + line
            print(last, end='', file=file)
        if description is not None:
            print(comment_prefix, file=file)
            print(comment_prefix + '  ' + description, file=file)
            print(comment_prefix, file=file)
            print(last, end='', file=file)
        print('', file=file)
