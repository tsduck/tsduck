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
githooks_list = ('pre-commit', 'post-merge')

# Give up if not in a Git repo.
if not os.path.isdir(githooks_dir):
    exit(0)

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
