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
#  Clean up the current directory tree, back to a clean state of source files.
#  The directory tree is traversed, the .gitignore files are read and all
#  files and directories which should be ignored by git are deleted.
#
#-----------------------------------------------------------------------------

import sys, os, shutil, fnmatch

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
def cleanup_directory(root, keep, remove, norecurse):
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
            if is_dir:
                shutil.rmtree(file_path, onerror = rmtree_error)
            else:
                os.remove(file_path)
        elif is_dir and not match_any(file_name, norecurse):
            cleanup_directory(file_path, keep, remove, norecurse)

# Main program
if __name__ == "__main__":
    keep = ['.git']
    remove = []
    norecurse = ['.git', 'installers']
    if len(sys.argv) < 2:
        cleanup_directory('.', keep, remove, norecurse)
    else:
        for i in range(1, len(sys.argv)):
            cleanup_directory(sys.argv[i], keep, remove, norecurse)
