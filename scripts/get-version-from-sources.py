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
#  This script extracts the TSDuck version from the source files.
#
#-----------------------------------------------------------------------------

import tsbuild, sys

arg = sys.argv[1] if len(sys.argv) > 1 else ''
major, minor, commit = tsbuild.version()

if arg == '':
    print('%s.%s-%s' % (major, minor, commit))
elif arg == '--major':
    print(major)
elif arg == '--minor':
    print(minor)
elif arg == '--commit':
    print(commit)
elif arg == '--distro':
    print(tsbuild.distro_suffix())
elif arg == '--full':
    print('%s.%s-%s%s' % (major, minor, commit, tsbuild.distro_suffix()))
elif arg == '--main':
    print('%s.%s' % (major, minor))
elif arg == '--dotted':
    print('%s.%s.%s' % (major, minor, commit))
elif arg == '--windows':
    print('%s.%s.%s.0' % (major, minor, commit))
else:
    print('invalid option: %s' % arg, file=sys.stderr)
    exit(1)
