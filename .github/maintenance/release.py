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
#  Manage TSDuck release on GitHub:
#  --title: display the expected title of the latest release.
#  --text: display the expected body text of the latest release.
#  --verify: verify that the title and body text are correct.
#  --update: same as --verify, then update if incorrect.
#  --create: create a new version from the latest commit in the repo.
#
#-----------------------------------------------------------------------------

import os, re, sys, glob, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
opt_title  = repo.has_opt('--title')
opt_text   = repo.has_opt('--text')
opt_verify = repo.has_opt('--verify')
opt_update = repo.has_opt('--update')
opt_create = repo.has_opt('--create')
repo.check_opt_final()

# A regular expression matching a version number.
pattern_version = r'\d+\.\d+-\d+'

# A class which describes one installer package.
class installer:
    def __init__(self, pattern, dev, required, name):
        self.pattern = pattern
        self.dev = dev
        self.required = required
        self.name = name
        self.file = None
    def glob_pattern(self, version=None):
        return self.pattern \
                   .replace('{VERSION}', '*' if version is None else version) \
                   .replace('{OS}', '*')
    def re_pattern(self, version=None):
        return self.pattern \
                   .replace('.', r'\.') \
                   .replace('{VERSION}', pattern_version if version is None else version.replace('.', r'\.')) \
                   .replace('{OS}', r'\d*')

# Description of expected installers.
installers = [
    installer('TSDuck-Win64-{VERSION}.exe',                  False, True,  'Windows (Intel, 64 bits'),
    installer('TSDuck-Win32-{VERSION}.exe',                  False, False, 'Windows (Intel, 32 bits)'),
    installer('TSDuck-Win64-{VERSION}-Portable.zip',         False, True,  'Windows (Intel, 64 bits, portable)'),
    installer('TSDuck-Win32-{VERSION}-Portable.zip',         False, False, 'Windows (Intel, 32 bits, portable)'),
    installer('tsduck-{VERSION}.el{OS}.x86_64.rpm',          False, True,  'RedHat, AlmaLinux (Intel, 64 bits)'),
    installer('tsduck-{VERSION}.el{OS}.arm64.rpm',           False, False, 'RedHat, AlmaLinux (Arm, 64 bits)'),
    installer('tsduck-{VERSION}.fc{OS}.x86_64.rpm',          False, True,  'Fedora (Intel, 64 bits)'),
    installer('tsduck-{VERSION}.fc{OS}.arm64.rpm',           False, False, 'Fedora (Arm, 64 bits)'),
    installer('tsduck_{VERSION}.debian{OS}_amd64.deb',       False, True,  'Debian (Intel, 64 bits)'),
    installer('tsduck_{VERSION}.debian{OS}_arm64.deb',       False, False, 'Debian (Arm, 64 bits)'),
    installer('tsduck_{VERSION}.ubuntu{OS}_amd64.deb',       False, True,  'Ubuntu (Intel, 64 bits)'),
    installer('tsduck_{VERSION}.ubuntu{OS}_arm64.deb',       False, False, 'Ubuntu (Arm, 64 bits)'),
    installer('tsduck_{VERSION}.raspbian{OS}_armhf.deb',     False, True,  'Raspbian (Raspberry Pi, 32 bits)'),
    installer('tsduck-devel-{VERSION}.el{OS}.x86_64.rpm',    True,  True,  'RedHat, AlmaLinux (Intel, 64 bits)'),
    installer('tsduck-devel-{VERSION}.el{OS}.arm64.rpm' ,    True,  False, 'RedHat, AlmaLinux (Arm, 64 bits)'),
    installer('tsduck-devel-{VERSION}.fc{OS}.x86_64.rpm',    True,  True,  'Fedora (Intel, 64 bits)'),
    installer('tsduck-devel-{VERSION}.fc{OS}.arm64.rpm',     True,  False, 'Fedora (Arm, 64 bits)'),
    installer('tsduck-dev_{VERSION}.debian{OS}_amd64.deb',   True,  True,  'Debian (Intel, 64 bits)'),
    installer('tsduck-dev_{VERSION}.debian{OS}_arm64.deb',   True,  False, 'Debian (Arm, 64 bits)'),
    installer('tsduck-dev_{VERSION}.ubuntu{OS}_amd64.deb',   True,  True,  'Ubuntu (Intel, 64 bits)'),
    installer('tsduck-dev_{VERSION}.ubuntu{OS}_arm64.deb',   True,  False, 'Ubuntu (Arm, 64 bits)'),
    installer('tsduck-dev_{VERSION}.raspbian{OS}_armhf.deb', True,  True,  'Raspbian (Raspberry Pi, 32 bits)')
]

# Get latest release and verify the format of its tag.
def get_latest_release():
    release = repo.repo.get_latest_release()
    if re.fullmatch('v' + pattern_version, release.tag_name) is None:
        repo.fatal('invalid tag "%s"' % release.tag_name)
    return release

# A function to get the TSDuck version from tsVersion.h in the repo.
def get_tsduck_version():
    match = re.search(r'^ *#define +TS_VERSION_MAJOR +(\d+) *$.*' +
                      r'^ *#define +TS_VERSION_MINOR +(\d+) *$.*' +
                      r'^ *#define +TS_COMMIT +(\d+) *$',
                      repo.get_text_file('src/libtsduck/tsVersion.h'),
                      re.MULTILINE | re.DOTALL)
    return None if match is None else '%s.%s-%s' % (match.group(1), match.group(2), match.group(3))

# A function to locate all local installer packages.
# Update the installers list with 'file' elements.
# Return True if all installers were found, False otherwise.
def search_installers(version):
    # Get directory where installation packages are located.
    dir = repo.scriptdir
    while True:
        pkgdir = dir + '/installers'
        if os.path.isdir(pkgdir):
            break
        parent = os.path.dirname(dir)
        if os.path.samefile(dir, parent):
            repo.error('installers directory not found, starting from %s, upwards' % repo.scriptdir)
            return False
        dir = parent
    # Search expected installer files.
    success = True
    for i in range(len(installers)):
        # Find files matching the pattern in the directory.
        pattern = installers[i].glob_pattern(version)
        files = [f for f in glob.glob(pkgdir + '/' + pattern)]
        if len(files) == 1:
            installers[i].file = files[0]
        elif len(files) > 1:
            repo.error('more than one package matching %s' % pattern)
            success = False
        elif installers[i].required:
            repo.error('no package matching %s' % pattern)
            success = False
        else:
            repo.verbose('optional package for "%s" not found, ignored' % installers[i].name)
    return success

# A class to build the body of the release body text.
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
        for a in self.assets:
            if re.fullmatch(pattern, a.name) is not None:
                self.ref(prefix, a.name, a.browser_download_url)
                break

# Build the body text of a release.
def build_body_text(release):
    body = body_builder(release)
    body.line('Binaries for command-line tools and plugins:')
    for ins in installers:
        if not ins.dev:
            body.url(ins.name, ins.re_pattern())
    body.ref('macOS', 'use Homebrew', 'https://tsduck.io/doxy/installing.html#macinstall')
    body.line('')
    body.line('Binaries for development environment:')
    body.line('* Windows: Included in installer (select option "Development")')
    for ins in installers:
        if ins.dev:
            body.url(ins.name, ins.re_pattern())
    body.line('* macOS: Included in Homebrew package')
    return body.get_text()

# Build the title of a release.
def build_title(release):
    return 'Version %s' % release.tag_name[1:]

# Main code.
if not (opt_title or opt_text or opt_verify or opt_update or opt_create):
    repo.fatal('specify one of --title --text --verify --update --create')

if opt_title:
    print(build_title(get_latest_release()))

if opt_text:
    print(build_body_text(get_latest_release()), end='')

if opt_verify:
    # Same as --update --dry-run
    opt_update = True
    repo.dry_run = True
    
if opt_update:
    release = get_latest_release()
    title = build_title(release)
    body = build_body_text(release)
    repo.info('Release: %s' % release.title)
    if title == release.title:
        repo.info('Release title is already set')
    if body == release.body:
        repo.info('Release body text is already set')
    if title != release.title or body != release.body:
        if repo.dry_run:
            if title != release.title:
                repo.warning('title should be changed to: %s' % title)
            if body != release.body:
                repo.warning('body text should be updated')
        else:
            # Actually perform the update.
            repo.info('Updating release title and body text')
            release.update_release(title, body)

if opt_create:
    # Get the version from tsVersion.h in the repo.
    version = get_tsduck_version()
    title = 'Version %s' % version
    repo.info('TSDuck version: %s' % version)

    # Locate the installer packages to upload.
    if not search_installers(version):
        repo.fatal('cannot create version, fix package files first')

    # Check if the tag already exists in the repository.
    tag_name = 'v' + version
    tags = [t for t in repo.repo.get_tags() if t.name == tag_name]
    if len(tags) > 0:
        repo.info('Tag %s already exists' % tag_name)
    else:
        repo.info('Tag %s does not exist, creating it on last commit' % tag_name)
        if not repo.dry_run:
            for last_commit in repo.repo.get_commits():
                break
            tag = repo.repo.create_git_tag(tag_name, message='', type='commit', object=last_commit.sha)
            repo.repo.create_git_ref('refs/tags/{}'.format(tag.tag), tag.sha)

    # Check if a release exists for that tag.
    releases = [rel for rel in repo.repo.get_releases() if rel.tag_name == tag_name]
    if len(releases) > 0:
        release = releases[0]
        repo.info('A release already exists for tag %s (%s)' % (tag_name, release.title))
    else:
        title = 'Version %s' % version
        repo.info('Creating release "%s"' % title)
        if repo.dry_run:
            # In case of dry run, we cannot do anything else.
            exit(0)
        release = repo.repo.create_git_release(tag_name, title, '')

    # Upload assets which are not yet uploaded.
    assets = [a for a in release.get_assets()]
    for ins in installers:
        if ins.file is not None:
            asset_name = os.path.basename(ins.file)
            if len([a for a in assets if a.name == asset_name]) > 0:
                repo.info("File %s already uploaded" % asset_name)
            else:
                repo.info("Uploading %s" % ins.file)
                if not repo.dry_run:
                    release.upload_asset(ins.file)

    # Finally publish the release.
    if not repo.dry_run:
        release.update_release(title, build_body_text(release))
