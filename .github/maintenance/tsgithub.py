#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2022, Thierry Lelegard
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
#  This Pythonn module shall be imported by all scripts in this directory
#  working on the TSDuck repository using GitHub.
#
#  When access permissions are required (to update the repo for instance),
#  a GitHub access token is required. It is extracted in this order:
#  1. Command line option --github-token TOKEN on the calling script
#  2. Environment variable GITHUB_TOKEN
#  3. Environment variable HOMEBREW_GITHUB_API_TOKEN
#
#  Prerequisite: PyGitHub
#  - Install: pip install PyGithub
#  - Documentation: https://pygithub.readthedocs.io/
#
#  Warning: There is a PyPI module named "github" which is different and
#  incompatible with PyGitHub. The two declare the module "github" with
#  different contents and classes. You cannot install the two at the same
#  time. If you accidentally installed "github" instead of "PyGitHub",
#  run "pip uninstall github" before "pip install PyGithub".
#
#-----------------------------------------------------------------------------

import os
import sys
import github

# Calling script name.
script = os.path.basename(sys.argv[0])

# Command line options default values.
token = None
repo_name = 'tsduck/tsduck'

# Decode command line options, remove common options from argv.
i = 0
while i < len(sys.argv):
    if sys.argv[i] == '--github-token':
        sys.argv.pop(i)
        if i < len(sys.argv):
            token = sys.argv[i]
            sys.argv.pop(i)
    elif sys.argv[i] == '--github-repo':
        sys.argv.pop(i)
        if i < len(sys.argv):
            repo_name = sys.argv[i]
            sys.argv.pop(i)
    elif sys.argv[i] == '--github-debug':
        sys.argv.pop(i)
        github.enable_console_debug_logging()
    else:
        i += 1

# Default value for token.
if token is None:
    for var in ['GITHUB_TOKEN', 'HOMEBREW_GITHUB_API_TOKEN']:
        if var in os.environ:
            token = os.environ[var]
            break
if token is None:
    print('%s: warning: no GitHub access token defined, limited access only' % script, file=sys.stderr)

# Get GitHub connection.
gh = github.Github(login_or_token = token, per_page = 100)

# Get TSDuck repository.
repo = gh.get_repo(repo_name)
