#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Process issues with "close pending" or "close now" label in the TSDuck
#  project on GitHub.
#  - Close open issues with label "close now".
#  - Close open issues with label "close pending" without update for 150 days.
#  - Remove these labels to issues which were already closed.
#
#-----------------------------------------------------------------------------

import sys, datetime, tsgithub

today = datetime.datetime.today().toordinal()

# Get command line options.
repo = tsgithub.repository(sys.argv)
repo.check_opt_final()

# Get the date where an issue was updated last.
# We cannot use 'issue.updated_at' because it returns the date of the last update,
# whatever it is, including setting a label, including setting label 'close pending'.
# We want the date of the last comment.
def get_issue_last_date(issue):
    if issue.comments == 0:
        # No comment, just the issue itself.
        return issue.created_at
    else:
        comms = [c for c in issue.get_comments()]
        return comms[-1].updated_at

# A function to cleanup messages with a given label and age in days.
def check_and_close(label_name, max_days):
    
    # Get all issues at once.
    # We cannot do the cleanup in a get_issues() loop because we change the search criteria and the list can be paginated.
    pending_issues = [i for i in repo.repo.get_issues(state = 'all', labels = [label_name])]

    # Then, do the cleanup in a second pass.
    for issue in pending_issues:
        last_date = get_issue_last_date(issue)
        date = last_date.strftime('%Y-%m-%d')
        age = today - last_date.toordinal()
        comments = [] if issue.pull_request is None else ['PR']
        comments.append(issue.state)
        comments.extend([l.name for l in issue.labels if l.name != label_name])
        print('#%d: %s (%s), updated %s, %d days ago' % (issue.number, issue.title, ', '.join(comments), date, age))
        if issue.state == 'closed':
            print('-> issue #%d is already closed, removing %s label' % (issue.number, label_name))
            if not repo.dry_run:
                issue.remove_from_labels(label_name)
        elif age >= max_days and issue.pull_request is None:
            print('-> issue #%d is too old, closing it' % issue.number)
            if not repo.dry_run:
                issue.create_comment('Automatically closed after %d days without update and %s label set.' % (age, label_name))
                issue.edit(state = 'closed')
                issue.remove_from_labels(label_name)

# Main code.
check_and_close('close now', 0)
check_and_close('close pending', 150)
