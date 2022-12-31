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
#  This script updates the Git hooks when required.
#
#  A few standard Git hooks are established to manage the "commit count"
#  which is used to identify the product version.
#
#  The actual code to execute is in scripts/git-hook.sh.
#  The template for a hook script is:
#
#    #!/usr/bin/env bash
#    $(dirname "$0")/../scripts/git-hook.sh <hook-name>
#
#-----------------------------------------------------------------------------

import tsbuild, sys, os, stat

root_dir = tsbuild.repo_root()
githooks_dir = root_dir + '/.git/hooks'
githooks_list = ('pre-commit', 'pre-merge-commit', 'post-merge')

# Give up if not in a Git repo.
if not os.path.isdir(githooks_dir):
    exit(0)

# Activate Git LFS at user level.
if 'filter.lfs' not in tsbuild.run(['git', 'config', '--list', '--global'], cwd=os.getenv('HOME')):
    print('Activating Git LFS at user level')
    tsbuild.run(['git', 'lfs', 'install'])

# Activate Git LFS in repo (replace some Git hooks).
lfs_set = False
for filename in os.listdir(githooks_dir):
    file = githooks_dir + os.sep + filename
    if os.path.isfile(file) and 'git lfs' in tsbuild.read(file):
        lfs_set = True
        break;
if not lfs_set:
    print('Activating Git LFS at repository level')
    tsbuild.run(['git', 'lfs', 'update', '--force'], cwd=root_dir)

# Update all hooks.
for hook in githooks_list:
    file = githooks_dir + os.sep + hook
    content = tsbuild.read(file) if os.path.isfile(file) else ''
    if '/scripts/git-hook.sh' not in content:
        print('Updating Git hook %s' % hook)
        if content == '':
            content = '#!/usr/bin/env bash\n'
        content += '$(dirname "$0")/../../scripts/git-hook.sh ' + hook + '\n'
        tsbuild.write(file, content)
        os.chmod(file, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IXGRP)
