#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Clean up the current directory tree, back to a clean state of source files.
#  The directory tree is traversed, the .gitignore files are read and all
#  files and directories which should be ignored by git are deleted.
#
#  With options -n, display files to delete but don't delete.
#
#-----------------------------------------------------------------------------

import sys, os, shutil, fnmatch

# Check if an option is present and remove it.
def has_option(argv, name):
    present = name in argv
    if present:
        argv.remove(name)
    return present

# Sort a list, remove duplicates and empty strings.
def cleanup_list(l):
    l = list(set(l))
    l.sort()
    if '' in l:
        l.remove('')
    return l

# Check if a file name matches a pattern from a list.
def match_any(name, patterns):
    for pat in patterns:
        if fnmatch.fnmatch(name, pat):
            return True
    return False

# Handler for rmtree() error
def rmtree_error(func, file_path, excinfo):
    print('**** error removing %s' % file_path)

# Cleanup a directory tree.
def cleanup_directory(root, keep, remove, norecurse, dry_run):
    # Load .gitignore file if there is one.
    gitignore = root + os.sep + '.gitignore'
    if os.path.isfile(gitignore):
        # Make sure we work on private copies of the lists.
        keep = list(keep)
        remove = list(remove)
        with open(gitignore, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('!'):
                    # A file pattern to keep (not ignored by git).
                    keep.append(line[1:])
                elif not line.startswith('#'):
                    # Not a comment, a file pattern to remove (ignored by git).
                    remove.append(line)
        keep = cleanup_list(keep)
        remove = cleanup_list(remove)
    # Lookup and clean all files in directory.
    for file_name in cleanup_list(os.listdir(root)):
        file_path = root + os.sep + file_name
        is_dir = os.path.isdir(file_path)
        if not match_any(file_name, keep) and match_any(file_name, remove):
            print('---- removing %s' % file_path)
            if not dry_run:
                if is_dir:
                    shutil.rmtree(file_path, onerror = rmtree_error)
                else:
                    os.remove(file_path)
        elif is_dir and not match_any(file_name, norecurse):
            cleanup_directory(file_path, keep, remove, norecurse, dry_run)

# Main program
if __name__ == "__main__":
    opt_dry_run = has_option(sys.argv, '-n')
    keep = ['.git']
    remove = []
    norecurse = ['.git', 'installers']
    if len(sys.argv) < 2:
        cleanup_directory('.', keep, remove, norecurse, opt_dry_run)
    else:
        for i in range(1, len(sys.argv)):
            cleanup_directory(sys.argv[i], keep, remove, norecurse, opt_dry_run)
