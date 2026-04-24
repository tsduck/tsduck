#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Set a label on several issues.
#  Syntax: set-label.py 'label-name' issue-number ...
#
#-----------------------------------------------------------------------------

import sys, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
args = repo.get_remaining_args()
if len(args) < 2:
    repo.fatal('usage: set-label.py "label-name" issue-number ...')

label = args[0]
for inum in args[1:]:
    try:
        issue = repo.repo.get_issue(int(inum))
    except:
        repo.error('issue %s not found' % inum)
    else:
        repo.verbose('setting label "%s" on issue %s' % (label, inum))
        issue.add_to_labels(label)
