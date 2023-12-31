#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
