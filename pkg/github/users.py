#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Get all active "users" of the TSDuck project on GitHub.
#  Get the list of contributors and fork owners.
#
#  Options:
#  -c --cont: get contributors only (default: all)
#  -f --fork: get forks only (default: all)
#  -r --ref: get a one-liner with '@' reference
#  -v --verbose
#
#-----------------------------------------------------------------------------

import sys, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
opt_cont = repo.has_opt(['-c', '--cont'])
opt_fork = repo.has_opt(['-f', '--fork'])
opt_ref  = repo.has_opt(['-r', '--ref'])
repo.check_opt_final()

# Default to all.
if not opt_cont and not opt_fork:
    opt_cont = opt_fork = True

# Collect names from the repo.
names = []
if opt_cont:
    names += [contrib.login for contrib in repo.repo.get_contributors()]
if opt_fork:
    names += [fork.full_name.split('/')[0] for fork in repo.repo.get_forks()]

# Sort and remove duplicates.
names = list(dict.fromkeys(names))
names.sort(key=str.lower)
repo.verbose('found %d users' % len(names))

if len(names) == 0:
    repo.error("no user found")
elif opt_ref:
    print('@' + ' @'.join(names))
else:
    for n in names:
        print(n)
