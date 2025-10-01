#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Manage TSDuck release on GitHub.
#  The default action is to list all releases.
#
#  Options:
#  --title : display the expected title of the latest release.
#  --text : display the expected body text of the latest release.
#  --verify : verify that the title and body text are correct.
#  --update : same as --verify, then update if incorrect, upload missing
#      packages if present in pkg/installers.
#  --create : create a new version from the latest commit in the repo.
#  --missing, -m : with --create, allow the creation of the release, even if
#      mandatory packages are missing.
#  --draft, -d : with --create, create a draft (unfinished) release.
#  --pre, -p : with --create, create a pre-release.
#  --tag name : with --title, --text, --verify, --update, release to check.
#
#  Common options:
#  --repo owner/repo : GitHub repository, default: tsduck/tsduck.
#  --token string : GitHub authentication token, default: $GITHUB_TOKEN.
#  --branch name : git branch in the repository, default: master.
#  --dry-run, -n: dry run, don't create anything.
#  --verbose, -v: verbose mode.
#  --debug: debug mode.
#
#-----------------------------------------------------------------------------

import os, re, sys, glob, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
opt_title   = repo.has_opt('--title')
opt_text    = repo.has_opt('--text')
opt_verify  = repo.has_opt('--verify')
opt_update  = repo.has_opt('--update')
opt_create  = repo.has_opt('--create')
opt_list    = not (opt_title or opt_text or opt_verify or opt_update or opt_create)
opt_missing = repo.has_opt(['-m', '--missing'])
opt_draft   = repo.has_opt(['-d', '--draft'])
opt_prerel  = repo.has_opt(['-p', '--pre'])
opt_tag     = repo.get_opt('--tag', None)
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
        self.files = []
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
# Some of them are for reference only and not mandatory. Not having them does not
# prevent the creation of the release. Typically, only Intel and Arm packages are
# are natively built. Others are built in Qemu emulated virtual machines.
installers = [
    installer('TSDuck-Win64-{VERSION}.exe',                  False, True,  'Windows (Intel, 64 bits'),
    installer('TSDuck-Win32-{VERSION}.exe',                  False, False, 'Windows (Intel, 32 bits)'),
    installer('TSDuck-Arm64-{VERSION}.exe',                  False, False, 'Windows (Arm, 64 bits)'),
    installer('TSDuck-Win64-{VERSION}-Portable.zip',         False, True,  'Windows (Intel, 64 bits, portable)'),
    installer('TSDuck-Win32-{VERSION}-Portable.zip',         False, False, 'Windows (Intel, 32 bits, portable)'),
    installer('TSDuck-Arm64-{VERSION}-Portable.zip',         False, False, 'Windows (Arm, 64 bits, portable)'),
    installer('tsduck-{VERSION}.el{OS}.x86_64.rpm',          False, True,  'RedHat and clones (Intel, 64 bits)'),
    installer('tsduck-{VERSION}.el{OS}.aarch64.rpm',         False, False, 'RedHat and clones (Arm, 64 bits)'),
    installer('tsduck-{VERSION}.fc{OS}.x86_64.rpm',          False, True,  'Fedora (Intel, 64 bits)'),
    installer('tsduck-{VERSION}.fc{OS}.aarch64.rpm',         False, False, 'Fedora (Arm, 64 bits)'),
    installer('tsduck_{VERSION}.debian{OS}_amd64.deb',       False, True,  'Debian (Intel, 64 bits)'),
    installer('tsduck_{VERSION}.debian{OS}_arm64.deb',       False, False, 'Debian (Arm, 64 bits)'),
    installer('tsduck_{VERSION}.debian{OS}_ppc64.deb',       False, False, 'Debian (PowerPC, 64 bits)'),
    installer('tsduck_{VERSION}.ubuntu{OS}_amd64.deb',       False, True,  'Ubuntu (Intel, 64 bits)'),
    installer('tsduck_{VERSION}.ubuntu{OS}_arm64.deb',       False, False, 'Ubuntu (Arm, 64 bits)'),
    installer('tsduck_{VERSION}.ubuntu{OS}_s390x.deb',       False, False, 'Ubuntu (IBM s390x)'),
    installer('tsduck_{VERSION}.ubuntu{OS}_riscv64.deb',     False, False, 'Ubuntu (RISC-V, 64 bits)'),
    installer('tsduck_{VERSION}.raspbian{OS}_armhf.deb',     False, False, 'Raspbian (Raspberry Pi, 32 bits)'),
    installer('tsduck-devel-{VERSION}.el{OS}.x86_64.rpm',    True,  True,  'RedHat and clones (Intel, 64 bits)'),
    installer('tsduck-devel-{VERSION}.el{OS}.aarch64.rpm',   True,  False, 'RedHat and clones (Arm, 64 bits)'),
    installer('tsduck-devel-{VERSION}.fc{OS}.x86_64.rpm',    True,  True,  'Fedora (Intel, 64 bits)'),
    installer('tsduck-devel-{VERSION}.fc{OS}.aarch64.rpm',   True,  False, 'Fedora (Arm, 64 bits)'),
    installer('tsduck-dev_{VERSION}.debian{OS}_amd64.deb',   True,  True,  'Debian (Intel, 64 bits)'),
    installer('tsduck-dev_{VERSION}.debian{OS}_arm64.deb',   True,  False, 'Debian (Arm, 64 bits)'),
    installer('tsduck-dev_{VERSION}.debian{OS}_ppc64.deb',   True,  False, 'Debian (PowerPC, 64 bits)'),
    installer('tsduck-dev_{VERSION}.ubuntu{OS}_amd64.deb',   True,  True,  'Ubuntu (Intel, 64 bits)'),
    installer('tsduck-dev_{VERSION}.ubuntu{OS}_arm64.deb',   True,  False, 'Ubuntu (Arm, 64 bits)'),
    installer('tsduck-dev_{VERSION}.ubuntu{OS}_s390x.deb',   True,  False, 'Ubuntu (IBM s390x)'),
    installer('tsduck-dev_{VERSION}.ubuntu{OS}_riscv64.deb', True,  False, 'Ubuntu (RISC-V, 64 bits)'),
    installer('tsduck-dev_{VERSION}.raspbian{OS}_armhf.deb', True,  False, 'Raspbian (Raspberry Pi, 32 bits)')
]

# Get most recent release and verify the format of its tag.
# The most recent can be a draft or pre-release, not always the one with "latest" attribute.
def get_release(tag):
    release = None
    try:
        for rel in repo.repo.get_releases():
            if tag is None or rel.tag_name == tag:
                release = rel
                break
    except:
        pass
    if release is None:
        if tag is None:
            repo.fatal('no release found')
        else:
            repo.fatal('no release found for tag %s' % tag)
    if re.fullmatch('v' + pattern_version, release.tag_name) is None:
        repo.fatal('invalid tag "%s"' % release.tag_name)
    return release

# A function to get the TSDuck version from tsVersion.h in the repo.
def get_tsduck_version():
    match = re.search(r'^ *#define +TS_VERSION_MAJOR +(\d+) *$.*' +
                      r'^ *#define +TS_VERSION_MINOR +(\d+) *$.*' +
                      r'^ *#define +TS_COMMIT +(\d+) *$',
                      repo.get_text_file('src/libtscore/tsVersion.h'),
                      re.MULTILINE | re.DOTALL)
    return None if match is None else '%s.%s-%s' % (match.group(1), match.group(2), match.group(3))

# A function to locate all local installer packages.
# Update the installers list with 'files' elements.
# Return True if all installers were found, False otherwise.
def search_installers(version, silent, allow_missing):
    # Get directory where installation packages are located.
    dir = repo.scriptdir
    while True:
        pkgdir = dir + '/pkg/installers'
        if os.path.isdir(pkgdir):
            break
        parent = os.path.dirname(dir)
        if os.path.samefile(dir, parent):
            repo.error('pkg/installers directory not found, starting from %s, upwards' % repo.scriptdir)
            return False
        dir = parent
    # Search expected installer files.
    success = True
    for i in range(len(installers)):
        # Find files matching the pattern in the directory.
        pattern = installers[i].glob_pattern(version)
        files = [f for f in glob.glob(pkgdir + '/' + pattern)]
        if len(files) > 0:
            installers[i].files = files
            if len(files) > 1 and not silent:
                repo.verbose('found %d packages matching %s' % (len(files), pattern))
        elif installers[i].required and not allow_missing:
            if not silent:
                repo.error('no package matching %s' % pattern)
            success = False
        elif not silent and not allow_missing:
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
    def add_line(self, line):
        self.text += line + '\r\n'
    def add_ref(self, prefix, name, url):
        self.text += '* ' + prefix + ': [' + name + '](' + url + ')\r\n'
    def add_all_urls(self, prefix, pattern):
        for a in self.assets:
            if re.fullmatch(pattern, a.name) is not None:
                self.add_ref(prefix, a.name, a.browser_download_url)

# Build the body text of a release.
def build_body_text(release):
    body = body_builder(release)
    body.add_line('Binaries for command-line tools and plugins:')
    for ins in installers:
        if not ins.dev:
            body.add_all_urls(ins.name, ins.re_pattern())
    body.add_ref('macOS', 'use Homebrew', 'https://tsduck.io/docs/tsduck-dev.html#macinstall')
    body.add_line('')
    body.add_line('Binaries for development environment:')
    body.add_line('* Windows: Included in installer (select option "Development")')
    for ins in installers:
        if ins.dev:
            body.add_all_urls(ins.name, ins.re_pattern())
    body.add_line('* macOS: Included in Homebrew package')
    return body.get_text()

# Get the version string "major.minor-commit" of a release.
def release_to_version(release):
    return release.tag_name[1:]

# Build the title of a release.
def build_title(release):
    return 'Version %s' % release_to_version(release)

# Upload missing installers in the release.
def upload_assets(release):
    assets = [a for a in release.get_assets()]
    for ins in installers:
        for file in ins.files:
            asset_name = os.path.basename(file)
            if len([a for a in assets if a.name == asset_name]) == 0:
                repo.info("Uploading %s" % file)
                if not repo.dry_run:
                    release.upload_asset(file)

# List all releases.
def list_releases():
    latest = repo.repo.get_latest_release()
    text = [['Tag', 'Type', 'Date', 'Title']]
    for rel in repo.repo.get_releases():
        date = '%04d-%02d-%02d' % (rel.created_at.year, rel.created_at.month, rel.created_at.day)
        type = 'draft' if rel.draft else ('prerelease' if rel.prerelease else ('latest' if rel.id == latest.id else ''))
        text.append([rel.tag_name, type, date, rel.title])
    width = [0] * len(text[0])
    for line in text:
        for i in range(len(line)):
            width[i] = max(width[i], len(line[i]))
    print()
    first = True
    for line in text:
        sep = ''
        for c in range(len(line)):
            print('%s%-*s' % (sep, width[c], line[c]), end='')
            sep = '  '
        print()
        if first:
            first = False
            sep = ''
            for c in range(len(line)):
                print('%s%s' % (sep, '-' * width[c]), end='')
                sep = '  '
            print()
    print()

# Main code.
if opt_list:
    list_releases()

if opt_title:
    repo.info(build_title(get_release(opt_tag)))

if opt_text:
    repo.info(build_body_text(get_release(opt_tag)), end='')

if opt_verify:
    # Same as --update --dry-run
    opt_update = True
    repo.dry_run = True

if opt_update:
    release = get_release(opt_tag)
    version = release.tag_name[1:]
    repo.info('Release: %s' % release.title)
    repo.info('Version: %s' % version)

    # Locate packages for that version and upload missing ones.
    search_installers(version, True, True)
    upload_assets(release)

    # Update title and body.
    title = build_title(release)
    body = build_body_text(release)
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
            release.update_release(title, body, draft=release.draft, prerelease=release.prerelease)

if opt_create:
    # Get the version from tsVersion.h in the repo.
    version = get_tsduck_version()
    title = 'Version %s' % version
    repo.info('TSDuck version: %s' % version)

    # Locate the installer packages to upload.
    if not search_installers(version, False, opt_missing):
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
        release = repo.repo.create_git_release(tag_name, title, message='', draft=opt_draft, prerelease=opt_prerel)

    # Upload assets which are not yet uploaded.
    upload_assets(release)

    # Finally publish the release.
    if not repo.dry_run:
        release.update_release(title, build_body_text(release), draft=release.draft, prerelease=release.prerelease)
