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
#  Analyze all contributions in the issues area of the TSDuck project on GitHub.
#  Output a summary of all users and their contributions (issues, pull requests
#  and comments). The summary is sorted in decreasing order of number of
#  contributions.
#
#-----------------------------------------------------------------------------

import sys, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
repo.check_opt_final()

# User context:
class user_context:
    def __init__(self, name):
        self.name = name
        self.issues = 0
        self.prs = 0
        self.comments = 0
        self.characters = 0
    def total(self):
        return self.issues + self.prs + self.comments
    def __lt__(self, other):
        return self.total() < other.total()
    def __str__(self):
        return '%-20s %8d %8d %8d %8d %8d' % (self.name, self.issues, self.prs, self.comments, self.total(), self.characters)
    header = '%-20s %8s %8s %8s %8s %8s' % ('User name', 'Issues', 'PR', 'Comments', 'Total', 'Size')

# A dictionary of all users.
users = {}

# Total of all users in one single instance.
total = user_context('Total')

# Analyze all issues.
prog = tsgithub.progress('issues')
for issue in repo.repo.get_issues(state = 'all'):
    prog.more()
    user_name = issue.user.login
    if not user_name in users:
        users[user_name] = user_context(user_name)
    if issue.pull_request is None:
        # This is a standard issue.
        users[user_name].issues += 1
        total.issues += 1
    else:
        # This is a pull request.
        users[user_name].prs += 1
        total.prs += 1
    if not issue.body is None:
        size = len(issue.body)
        users[user_name].characters += size
        total.characters += size
prog.end()

# Analyze all comments
prog = tsgithub.progress('comments')
for comment in repo.repo.get_issues_comments():
    prog.more()
    user_name = comment.user.login
    if not user_name in users:
        users[user_name] = user_context(user_name)
    users[user_name].comments += 1
    total.comments += 1
    if not comment.body is None:
        size = len(comment.body)
        users[user_name].characters += size
        total.characters += size
prog.end()

# Print final summary
total.name += ' (%d users)' % len(users)
print(user_context.header)
print(str(total))
for user in sorted(users.values(), reverse = True):
    print(str(user))
