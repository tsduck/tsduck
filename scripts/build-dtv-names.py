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
#  Build the file tsduck.dtv.names, containing names for Digital TV.
#  Syntax: build-dtv-names-names.py out-file in-file-or-dir ...
#
#-----------------------------------------------------------------------------

import tsbuild, sys, os, fnmatch

# Process a .names file or a directory.
def process_file_or_dir(path, output):
    if os.path.isfile(path):
        print('#---------- ' + os.path.basename(path), file=output)
        print('', file=output)
        with open(path, 'r', encoding='utf-8') as input:
            output.write(input.read())
        print('', file=output)
    elif os.path.isdir(path):
        for name in os.listdir(path):
            subpath = path + os.sep + name
            if os.path.isdir(subpath) or fnmatch.fnmatch(name, '*.names'):
                process_file_or_dir(subpath, output)

# Main code.
if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: %s out-file in-file-or-dir ...' % sys.argv[0], file=sys.stderr)
        exit(1)
    with open(sys.argv[1], 'w', encoding='utf-8') as output:
        tsbuild.write_source_header('#', 'Registered names for Digital TV', file=output)
        for i in range(2, len(sys.argv)):
            process_file_or_dir(sys.argv[i], output)
