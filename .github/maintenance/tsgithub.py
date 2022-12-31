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
#  This Python module shall be imported by all scripts in this directory
#  working on the TSDuck repository using GitHub.
#
#-----------------------------------------------------------------------------

import re, os, sys, base64, datetime, github

# A class referencing the repository based on command line options.
class repository:

    # Constructor.
    def __init__(self, argv=sys.argv, open_repo=True):
        # Keep a reference on argv. Options are removed as they are analyzed.
        self.argv = argv

        # Calling script name.
        self.script = os.path.basename(argv[0])
        self.scriptdir = os.path.dirname(os.path.abspath(argv[0]))

        # Decode command line options, remove common options from argv.
        self.token = self.get_opt('--token', os.getenv('GITHUB_TOKEN', os.getenv('HOMEBREW_GITHUB_API_TOKEN')))
        self.repo_name = self.get_opt('--repo', 'tsduck/tsduck')
        self.repo_url = 'https://github.com/%s/' % self.repo_name
        self.repo_branch = self.get_opt('--branch', 'master')
        self.dry_run = self.has_opt(['-n', '--dry-run'])
        self.verbose_mode = self.has_opt(['-v', '--verbose'])
        self.debug_mode = self.has_opt('--debug')

        if self.debug_mode:
            github.enable_console_debug_logging()

        # Get TSDuck repository if required.
        if open_repo:
            if self.token is None:
                self.warning('no GitHub access token defined, limited access only')
            self.github = github.Github(login_or_token=self.token, per_page=100)
            self.repo = self.github.get_repo(self.repo_name)
        else:
            self.github = None
            self.repo = None

    # Extract an option with a value from command line. Use a name or list of names.
    def get_opt(self, names, default=None):
        if type(names) is str:
            names = [names]
        value = default
        i = 0
        while i < len(self.argv):
            if self.argv[i] in names:
                self.argv.pop(i)
                if i < len(self.argv):
                    value = self.argv[i]
                    self.argv.pop(i)
            else:
                i += 1
        return value

    # Check if an option without value is in command line. Use a name or list of names.
    def has_opt(self, names):
        if type(names) is str:
            names = [names]
        value = False
        i = 0
        while i < len(self.argv):
            if self.argv[i] in names:
                self.argv.pop(i)
                value = True
            else:
                i += 1
        return value

    # Check that all command line options were recognized.
    def check_opt_final(self):
        if len(self.argv) > 1:
            self.fatal('extraneous options: %s' % ' '.join(self.argv[1:]))

    # Message reporting.
    def verbose(self, message):
        if self.verbose_mode:
            print(message, file=sys.stderr)
    def info(self, message):
        print(message, file=sys.stderr)
    def warning(self, message):
        print('%s: warning: %s' % (self.script, message), file=sys.stderr)
    def error(self, message):
        print('%s: error: %s' % (self.script, message), file=sys.stderr)
    def fatal(self, message):
        self.error(message)
        exit(1)

    # Get the content of a text file in the repo.
    def get_text_file(self, path):
        file = self.repo.get_contents(path)
        return base64.b64decode(file.content).decode('utf8')

# A progressive display context.
class progress:
    def __init__(self, name):
        print('Fetching %s ' % name, file=sys.stderr, end='', flush=True)
        self.count = 0
    def more(self):
        self.count += 1
        if self.count % 100 == 0:
            print(' %d ' % self.count, file=sys.stderr, end='', flush=True)
        elif self.count % 10 == 0:
            print('.', file=sys.stderr, end='', flush=True)
    def end(self):
        print('. %d done' % self.count, file=sys.stderr)

# Convert a date or string as datetime.
def to_datetime(date):
    if isinstance(date, datetime.datetime):
        return date
    if not isinstance(date, str):
        return datetime.now()
    try:
        return datetime.fromisoformat(date)
    except:
        pass
    date = re.sub(r'^[\w]*,\s*', '', date) # remove weekday, if any
    match = re.match(r'\s*(\d{2}\s+[a-zA-Z]{3}\s+\d{4}\s+\d{2}:\d{2}:\d{2})', date)
    if match is not None:
        try:
            return datetime.datetime.strptime(match.group(1), '%d %b %Y %H:%M:%S')
        except:
            pass
    match = re.match(r'\s*(\d{2}\s+[a-zA-Z]{3}\s+\d{4})', date)
    if match is not None:
        try:
            return datetime.datetime.strptime(match.group(1), '%d %b %Y')
        except:
            pass
    return datetime.now()
