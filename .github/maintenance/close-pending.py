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
#  Process issues with "close pending" label in the TSDuck project on GitHub.
#  - Close open issues without update for 150 days.
#  - Remove the "close pending" label to issues which were already closed.
#
#-----------------------------------------------------------------------------

import sys, datetime, tsgithub

max_age = 150 # days
today = datetime.datetime.today().toordinal()

# Get command line options.
repo = tsgithub.repository(sys.argv)
repo.check_opt_final()

# First, get all "close pending" issues at once.
# We cannot do the cleanup in a get_issues() loop because we change the search criteria and the list can be paginated.
pending_issues = [i for i in repo.repo.get_issues(state = 'all', labels = ['close pending'])]

# Then, do the cleanup in a second pass.
for issue in pending_issues:
    date = issue.updated_at.strftime('%Y-%m-%d')
    age = today - issue.updated_at.toordinal()
    comments = [] if issue.pull_request is None else ['PR']
    comments.append(issue.state)
    comments.extend([l.name for l in issue.labels if l.name != 'close pending'])
    print('#%d: %s (%s), updated %s, %d days ago' % (issue.number, issue.title, ', '.join(comments), date, age))
    if issue.state == 'closed':
        print('-> issue #%d is already closed, removing close pending label' % issue.number)
        if not repo.dry_run:
            issue.remove_from_labels('close pending')
    elif age > max_age and issue.pull_request is None:
        print('-> issue #%d is too old, closing it' % issue.number)
        if not repo.dry_run:
            issue.create_comment('Automatically closed after %d days without update and "close pending" label set.' % age)
            issue.edit(state = 'closed')
            issue.remove_from_labels('close pending')
