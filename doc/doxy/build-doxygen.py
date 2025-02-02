#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Python script to build the documentation using Doxygen.
#
#-----------------------------------------------------------------------------

import re, os, sys, glob, shutil, subprocess

# Calling script name, project root.
SCRIPT     = os.path.basename(sys.argv[0])
SCRIPTDIR  = os.path.dirname(os.path.abspath(sys.argv[0]))
ROOTDIR    = os.path.dirname(os.path.dirname(SCRIPTDIR))
SRCDIRS    = [os.sep.join([ROOTDIR, 'src', 'libtscore']), os.sep.join([ROOTDIR, 'src', 'libtsduck'])]
DOXYDIR    = os.sep.join([ROOTDIR, 'bin', 'doxy'])
HTMLDIR    = os.sep.join([DOXYDIR, 'html'])
GROUPDIR   = os.sep.join([HTMLDIR, 'group'])
CLASSDIR   = os.sep.join([HTMLDIR, 'class'])
GETVERSION = os.sep.join([ROOTDIR, 'scripts', 'get-version-from-sources.py'])

# Display a warning or fatal error and exit.
def warning(message):
    print('%s: warning: %s', (SCRIPT, message), file=sys.stderr)
def fatal(message):
    print('%s: error: %s', (SCRIPT, message), file=sys.stderr)
    exit(1)

# Get the output of a command as a text string.
def command_text(args):
    return subprocess.run(args, stdout=subprocess.PIPE).stdout.decode('utf-8')

# Search an executable in PATH. On Windows, also search a list of additional glob pattern.
def which(cmd, winglob=None):
    path = shutil.which(cmd)
    if path is None and os.name == 'nt' and winglob is not None:
        if not isinstance(winglob, list):
            winglob = [winglob]
        for dir in winglob:
            files = glob.glob(os.path.join(dir, cmd + '.exe'))
            if files is not None:
                path = files[0]
                break
    return path

# Extract a X.Y.Z version string, in string form and integer form.
def extract_version(s):
    match = re.search(r'([0-9]+)[\.-]([0-9]+)[\.-]([0-9]+)', s)
    if match is None:
        return None, 0
    else:
        return match.group(0), (10000 * int(match.group(1))) + (100 * int(match.group(2))) + int(match.group(3))

# Create an html file which redirects to another one.
def redirect_html(sourcedir, sourcefile, target):
    reltarget = os.path.relpath(target, sourcedir)
    with open(os.path.join(sourcedir, sourcefile), 'w') as output:
        print('<html>', file=output)
        print('<head><meta http-equiv="refresh" content="0; url=%s"></head>' % reltarget, file=output)
        print('<body>Moved <a href="%s">here</a></body>' % reltarget, file=output)
        print('</html>', file=output)

# Search Doxygen and Dot.
doxygen = which('doxygen', r'C:\Program Files*\Doxygen*\bin')
dot = which('dot', r'C:\Program Files*\Graphviz*\bin')
if doxygen is None:
    fatal("doxygen is not installed")
doxy_version, doxy_iversion = extract_version(command_text([doxygen, '--version']))
if doxy_iversion < 10908:
    warning('doxygen version is %s, recommended minimum is 1.9.8, expect spurious error messages' % doxy_version)

# Make sure that the output directory is created (doxygen does not create parent directories).
os.makedirs(DOXYDIR, exist_ok=True)

# Build an environment for execution of doxygen.
env = os.environ.copy()
env['TS_FULL_VERSION'] = command_text([sys.executable, GETVERSION]).strip()
env['DOXY_INCLUDE_PATH'] = ' '.join(sum([[d[0] for d in os.walk(s)] for s in SRCDIRS], start=[]))
if dot is None:
    env['HAVE_DOT'] = 'NO'
else:
    env['HAVE_DOT'] = 'YES'
    env['DOT_PATH'] = dot

# Run doxygen in same directory as this script (where Doxyfile is).
print('Running doxygen version: %s ...' % doxy_version)
status = subprocess.run([doxygen], env=env, cwd=SCRIPTDIR)

# Collect all 'group_*.html' files. Count files and directories.
# Delete empty subdirectories (older versions of doxygen created many for nothing in case of hierachical output).
groups = []
dir_count = 0
file_count = 0
for root, dirs, files in os.walk(DOXYDIR, topdown=False):
    fcount = len(files)
    if len(dirs) == 0 and fcount == 0:
        # Empty directory, remove it.
        os.rmdir(root)
    else:
        dir_count += 1
        file_count += fcount
        # Collect group files.
        for f in files:
            if f.startswith('group_') and f.endswith('.html'):
                groups.append(os.path.join(root, f))

# Create permanent links in a 'group' subdirectory.
os.makedirs(GROUPDIR, exist_ok=True)
for f in groups:
    match = re.fullmatch(r'group_*(.*\.html)', os.path.basename(f))
    if match is not None:
        redirect_html(GROUPDIR, match.group(1), f)
        file_count += 1

# Create permanent links in a 'class' subdirectory for all top-level classes in namespace 'ts'.
os.makedirs(CLASSDIR, exist_ok=True)
file_pattern = re.compile(r'href="([^"]*/classts_1_1[^"/]*\.html)"')
title_pattern = re.compile(r'>ts::([A-Za-z0-9_]+) +[Cc]lass +[Re]eference')
with open(os.path.join(HTMLDIR, 'classes.html')) as input:
    for line in input:
        # Grab all href to "classts_1_1*.html" files.
        for href in file_pattern.findall(line):
            # Read the HTML file and find the class name.
            hfile = os.path.join(HTMLDIR, href)
            classname = None
            with open(hfile) as hinput:
                for hline in hinput:
                    if 'class="headertitle"' in hline:
                        match = title_pattern.search(hline)
                        if match is not None:
                            classname = match.group(1)
                        break
            if classname is not None:
                redirect_html(CLASSDIR, classname + '.html', hfile)
                file_count += 1

print('Generated %d files in %d directories' % (file_count, dir_count))
exit(status.returncode)
