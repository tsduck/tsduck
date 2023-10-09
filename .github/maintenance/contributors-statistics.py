#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
#
#  Output a summary of all contributors to the TSDuck prokect on GitHub.
#  The summary is sorted in decreasing order of number of commits.
#
#-----------------------------------------------------------------------------

import sys, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
repo.check_opt_final()

# User context:
class user_context:
    def __init__(self, name, fullname=''):
        self.name = name
        self.fullname = fullname
        self.commits = 0
        self.first = None
        self.last = None

    def add(self, date):
        self.commits += 1
        date = tsgithub.to_datetime(date)
        if self.first == None or self.first > date:
            self.first = date
        if self.last == None or self.last < date:
            self.last = date

    def __lt__(self, other):
        return self.commits < other.commits

    def __str__(self):
        sfirst = '' if self.first is None else self.first.strftime('%Y-%m-%d')
        slast = '' if self.last is None else self.last.strftime('%Y-%m-%d')
        return '%-20s %8d %-10s %-10s %s' % (self.name, self.commits, sfirst, slast, self.fullname)

    header = '%-20s %8s %-10s %-10s %s' % ('GitHub user', 'Commits', 'First', 'Last', 'Full name')

# A dictionary of all users.
users = {}

# Total of all users in one single instance.
total = user_context('Total')

# Analyze all issues.
prog = tsgithub.progress('commits')
for commit in repo.repo.get_commits():
    prog.more()
    # Get user name and index in users dictionary.
    name = '-'
    fullname = ''
    email = ''
    is_named_user = type(commit.author) is tsgithub.github.NamedUser.NamedUser
    if is_named_user:
        name = index = commit.author.login
    elif type(commit.author) is str:
        name = index = commit.author
    elif 'commit' in commit.raw_data and 'author' in commit.raw_data['commit']:
        email = index = commit.raw_data['commit']['author']['email']
    else:
        name = index = '(unknown)'
    # Collect additional information only once per user for performance reasons.
    if not index in users:
        altname = ''
        if is_named_user:
            fullname = commit.author.name
        if fullname is None:
            fullname = ''
        if 'commit' in commit.raw_data and 'author' in commit.raw_data['commit']:
            altname = commit.raw_data['commit']['author']['name']
            if email is None or email == '':
                email = commit.raw_data['commit']['author']['email']
        if fullname != altname and altname != '':
            if fullname != '':
                fullname += ', '
            fullname += altname
        if email is not None and email != '':
            if fullname != '':
                fullname += ' (' + email + ')'
            else:
                fullname = email
        users[index] = user_context(name, fullname)
    # Count that commit.
    users[index].add(commit.last_modified)
    total.add(commit.last_modified)
prog.end()

# Print final summary
total.name += ' (%d users)' % len(users)
print(user_context.header)
print(str(total))
for user in sorted(users.values(), reverse = True):
    print(str(user))
