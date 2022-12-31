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
#  This script purges extraneous artifacts in the TSDuck GitHub repo CI/CD.
#  Only the 5 most recent versions of each artifact are left. Older artifacts
#  are deleted. Require PyGithub version 1.57 or higher.
#
#-----------------------------------------------------------------------------

import sys, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
repo.check_opt_final()

# The number of copies to keep per artifact.
KEEP = 5
print('Purging artifacts in repo %s, keeping %d copies of each artifact ...' % (repo.repo_name, KEEP))

# First, get all artifacts at once.
# We cannot do the cleanup in a get_artifacts() loop because we delete some and the list can be paginated.
artifacts = [i for i in repo.repo.get_artifacts()]

# A dictionary of all artifact counts, by name.
artifact_count = {}
total_count = 0
total_size = 0
deleted_count = 0
deleted_size = 0
error_count = 0
error_size = 0

# Then, do the cleanup in a second pass.
for art in artifacts:
    total_count += 1
    total_size += art.size_in_bytes
    date = art.created_at.strftime('%Y-%m-%d')
    if art.name not in artifact_count:
        # First time we see this artifact.
        artifact_count[art.name] = 0
    if not art.expired and artifact_count[art.name] < KEEP:
        # Keep this artifact.
        artifact_count[art.name] += 1
        repo.verbose('Keeping {}, {:,} bytes, {}'.format(art.name, art.size_in_bytes, date))
    else:
        repo.info('Deleting {}, {:,} bytes, {}{}'.format(art.name, art.size_in_bytes, date, ' (expired)' if art.expired else ''))
        if not repo.dry_run:
            if art.delete():
                deleted_count += 1
                deleted_size += art.size_in_bytes
            else:
                repo.error('cannot delete artifact %s' % (art.name))
                error_count += 1
                error_size += art.size_in_bytes

# Print final summary.
print('')
print('== Total:     {:5,} files, {:,} bytes'.format(total_count, total_size))
print('== Deleted:   {:5,} files, {:,} bytes'.format(deleted_count, deleted_size))
print('== Error:     {:5,} files, {:,} bytes'.format(error_count, error_size))
print('== Remaining: {:5,} files, {:,} bytes'.format(total_count - deleted_count - error_count, total_size - deleted_size - error_size))
