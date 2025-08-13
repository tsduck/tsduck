#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Python script to build the documentation using Doxygen.
#
#  With option -e or --enforce-groups, update all "@ingroup" directives in
#  source header files to add group "libtscore" or "libtsduck". However,
#  because of a bug in doxygen, this does not work on enum types, typdefs,
#  or functions at namespace level.
#
#-----------------------------------------------------------------------------

import re, os, sys, glob, shutil, subprocess

# With option -e or --enforce-groups, update all "@ingroup" directives.
update_groups = '-e' in sys.argv or '--enforce-groups' in sys.argv

# With option -s or --skip-doxygen, do not call doxygen, assume already built.
skip_doxygen = '-s' in sys.argv or '--skip-doxygen' in sys.argv

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

# Process a source header file, enforce "libtscore" or "libtsduck" in the "@ingroup" directives.
# Return 0 on success, the number of missing names if the file had to be rewritten with added group names.
def check_file(filename, groupname):
    content = ''
    missing = 0
    with open(filename, 'r') as input:
        for lin in input:
            match = re.search(r'^(.*//! *@ingroup) +(.*)$', lin)
            if match is not None:
                names = match.group(2).split()
                if groupname not in names:
                    # Target group name is missing.
                    missing += 1
                    # Cleanup incorrect library group names.
                    names = [n for n in names if not n.startswith('libts')]
                    # Rebuild the line with library group name in first position.
                    lin = match.group(1) + ' ' + groupname + ' ' + ' '.join(names) + '\n'
            content += lin
    if missing > 0:
        print('error: missing %d "%s" group name in %s' % (missing, groupname, filename), file=sys.stderr)
        with open(filename, 'w') as output:
            output.write(content)
    return missing

# Check a tree of source header files.
# Return the total number of missing group names.
def check_file_tree(dirname, groupname):
    missing = 0
    for dir, dnames, fnames in os.walk(dirname):
        for fnam in fnames:
            if fnam.endswith('.h'):
                missing += check_file(dir + os.sep + fnam, groupname)
    return missing

# Create an html file which redirects to another one.
def redirect_html(sourcedir, sourcefile, target):
    reltarget = os.path.relpath(target, sourcedir)
    with open(os.path.join(sourcedir, sourcefile), 'w') as output:
        print('<html>', file=output)
        print('<head><meta http-equiv="refresh" content="0; url=%s"></head>' % reltarget, file=output)
        print('<body>Moved <a href="%s">here</a></body>' % reltarget, file=output)
        print('</html>', file=output)

# Check and fix "@ingroup" directives in all header files.
missing_group_names = 0
if update_groups:
    for dir in SRCDIRS:
        missing_group_names += check_file_tree(dir, os.path.basename(dir))

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
if skip_doxygen:
    doxy_status = 0
else:
    print('Running doxygen version: %s ...' % doxy_version)
    status = subprocess.run([doxygen], env=env, cwd=SCRIPTDIR)
    doxy_status = status.returncode

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

# Create permanent links in a 'class' subdirectory for all classes in namespace 'ts'.
os.makedirs(CLASSDIR, exist_ok=True)
file_pattern = re.compile(r'href="([^"]*/classts_1_1[^"/]*\.html)"')
title_pattern = re.compile(r'>ts::([A-Za-z0-9_:]+) +[Cc]lass +[Re]eference')
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
                redirect_html(CLASSDIR, classname.replace(':', '_') + '.html', hfile)
                file_count += 1

print('Generated %d files in %d directories' % (file_count, dir_count))
if missing_group_names > 0:
    print('Missing %d libtscore or libtsduck group names' % missing_group_names)

exit(min(1, missing_group_names + doxy_status))
