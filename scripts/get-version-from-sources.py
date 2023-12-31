#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
