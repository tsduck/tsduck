#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

# Print an error.
def error(message):
    print('%s: %s' % (os.path.basename(sys.argv[0]), message), file=sys.stderr)

# Print a fatal error and exit.
def fatal_error(message):
    error(message)
    exit(1)

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
# Return as major, minor, commit.
def version():
    match = re.search(r'^ *#define +TS_VERSION_MAJOR +(\d+) *$.*' +
                      r'^ *#define +TS_VERSION_MINOR +(\d+) *$.*' +
                      r'^ *#define +TS_COMMIT +(\d+) *$',
                      read(repo_root() + '/src/libtscore/tsVersion.h'),
                      re.MULTILINE | re.DOTALL)
    if match is None:
        return 0, 0, 0
    else:
        return match.group(1), match.group(2), match.group(3)

# Get Python version.
def python_version():
    return '%d.%d.%d' % (sys.version_info.major, sys.version_info.minor, sys.version_info.micro)

# Get a description of the operating system and distro.
def distro():
    if os.name == 'nt':
        return 'win%d' % (8 * struct.calcsize('P'))
    if os.uname().sysname == 'Darwin':
        version = re.sub(r'\s', '', run(['sw_vers', '-productVersion']))
        return 'macos' + re.sub(r'\..*', '', version)
    name = re.sub(r'\s', '', run(['lsb_release', '-si'])).lower().replace('linuxmint', 'mint')
    if name != '':
        version = re.sub(r'[^0-9]', '', re.sub(r'\..*', '', re.sub(r'\s', '', run(['lsb_release', '-sr']))))
        if version == '' and os.path.exists('/etc/debian_version'):
            # Debian versions:
            n2v = {'jessie': 8, 'stretch': 9, 'buster': 10, 'bullseye': 11, 'bookworm': 12, 'trixie': 13, 'forky': 14}
            dv = re.sub(r'/.*', '', read('/etc/debian_version').lower().strip())
            if dv in n2v:
                version = str(n2v[dv])
        return name + version
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
    return '.' + suffix if suffix != '' else ''

# Write a standard source header in an output file.
def write_source_header(comment_prefix, description=None, firstline=None, lastline=None, file=sys.stdout):
    with open(repo_root() + '/src/HEADER.txt') as input:
        last = ''
        if firstline is not None:
            print(firstline, file=file)
        for line in input:
            last = comment_prefix + line
            print(last, end='', file=file)
        if description is not None:
            print(comment_prefix, file=file)
            print(comment_prefix + '  ' + description, file=file)
            print(comment_prefix, file=file)
            print(last, end='', file=file)
        if lastline is not None:
            print(lastline, file=file)
        print('', file=file)
