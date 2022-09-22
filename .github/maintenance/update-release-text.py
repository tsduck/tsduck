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
#  This script updates the latest TSDuck release on GitHub with the proper
#  title and body message, based on the list of assets.
#
#  Just display what should be done with option --dry-run or -n.
#
#  Steps:
#  - Create and push the tag for the release.
#  - Create the release on GitHub from that tag and upload binaries.
#  - Run this script to generate the markdown text and update the release.
#
#-----------------------------------------------------------------------------

import re
import sys
import tsgithub

dry_run = '-n' in sys.argv or '--dry-run' in sys.argv

# Get latest release characteristics.
release = tsgithub.repo.get_latest_release()
print('Latest release tag: %s' % release.tag_name)

# A class to build the body of the release.
class body_builder:
    def __init__(self, release):
        # Get all assets in this release.
        self.text = ''
        self.assets = [a for a in release.get_assets()]
    def get_text(self):
        return self.text
    def line(self, line):
        self.text += line + '\r\n'
    def ref(self, prefix, name, url):
        self.text += '* ' + prefix + ': [' + name + '](' + url + ')\r\n'
    def url(self, prefix, pattern):
        reg = re.compile(pattern)
        for a in self.assets:
            match = reg.match(a.name)
            if match is not None and match.start() == 0 and match.end() == len(a.name):
                self.ref(prefix, a.name, a.browser_download_url)
                break

# Build the release message.
body = body_builder(release)
body.line('Binaries for command-line tools and plugins:')
body.url('Windows 32 bits', r'TSDuck-Win32-.*\.exe')
body.url('Windows 64 bits', r'TSDuck-Win64-.*\.exe')
body.url('Windows 32 bits (portable)', r'TSDuck-Win32-.*-Portable\.zip')
body.url('Windows 64 bits (portable)', r'TSDuck-Win64-.*-Portable\.zip')
body.url('RedHat, AlmaLinux 64 bits', r'tsduck-\d.*\.el\d*\.x86_64\.rpm')
body.url('Fedora 64 bits', r'tsduck-\d.*\.fc\d*\.x86_64\.rpm')
body.url('Debian 64 bits', r'tsduck_\d.*\.debian.*_amd64\.deb')
body.url('Ubuntu 64 bits', r'tsduck_\d.*\.ubuntu.*_amd64\.deb')
body.url('Raspbian 32 bits (Raspberry Pi)', r'tsduck_\d.*_armhf\.deb')
body.ref('macOS', 'use Homebrew', 'https://tsduck.io/doxy/installing.html#macinstall')
body.line('')
body.line('Binaries for development environment:')
body.line('* Windows: Included in installer (select option "Development")')
body.url('RedHat, AlmaLinux 64 bits', r'tsduck-devel-\d.*\.el\d*\.x86_64\.rpm')
body.url('Fedora 64 bits', r'tsduck-devel-\d.*\.fc\d*\.x86_64\.rpm')
body.url('Debian 64 bits', r'tsduck-dev_\d.*\.debian.*_amd64\.deb')
body.url('Ubuntu 64 bits', r'tsduck-dev_\d.*\.ubuntu.*_amd64\.deb')
body.url('Raspbian 32 bits (Raspberry Pi)', r'tsduck-dev_\d.*_armhf\.deb')
body.line('* macOS: Included in Homebrew package')

# Adjust release title and body.
title = 'Version %s' % release.tag_name
if title == release.title:
    print('Release title is already set')
if body.get_text() == release.body:
    print('Release body text is already set')
if title != release.title or body.get_text() != release.body:
    if dry_run:
        if title != release.title:
            print('Title should be changed to: %s' % title)
        if body.get_text() != release.body:
            print('Body text should be changed to:')
            print(body.get_text())
    else:
        # Actually perform the update.
        print('Updating release title and body text')
        release.update_release(title, body.get_text())
