#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2022, Thierry Lelegard
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
#  are deleted.
#
#-----------------------------------------------------------------------------

import sys, requests, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv, False)
repo.check_opt_final()

# The number of copies to keep per artifact.
KEEP = 5
print('Purging artifacts in repo %s, keeping %d copies of each artifact ...' % (repo.repo_name, KEEP))

# A dictionary of all artifact counts, by name.
artifact_count = {}
to_delete = []
total_count = 0
total_size = 0
deleted_count = 0
deleted_size = 0
error_count = 0
error_size = 0

# Loop on all pages of artifacts.
# We need to get all artifacts first, and then delete the extraneous ones.
# If we read a page, delete artifacts, then read next page, the page boundaries have changed.
artifacts_url = 'https://api.github.com/repos/' + repo.repo_name + '/actions/artifacts'
url = artifacts_url + '?per_page=100'
while url is not None:
    # Get a page of artifacts.
    resp = requests.get(url, headers = {'Accept': 'application/vnd.github.v3+json'}, auth = ('', repo.token))
    url = resp.links['next']['url'] if ('next' in resp.links and 'url' in resp.links['next']) else None
    data = resp.json()
    # Loop on all artifacts in that page.
    if 'artifacts' in data:
        for art in data['artifacts']:
            if 'name' in art and 'id' in art:
                size = art['size_in_bytes'] if 'size_in_bytes' in art else 0
                date = art['created_at'] if 'created_at' in art else ''
                expired = 'expired' in art and art['expired']
                total_count += 1
                total_size += size
                if art['name'] not in artifact_count:
                    # First time we see this artifact.
                    artifact_count[art['name']] = 0
                if not expired and artifact_count[art['name']] < KEEP:
                    # Keep this artifact.
                    artifact_count[art['name']] += 1
                    repo.verbose('Keeping {}, {:,} bytes, {}'.format(art['name'], size, date))
                else:
                    repo.verbose('Will delete {}, {:,} bytes{}'.format(art['name'], size, ' (expired)' if expired else ''))
                    to_delete.append({'id': art['id'], 'name': art['name'], 'size': size, 'date': date})

# Actually delete the artifacts.
for art in to_delete:
    repo.info('Deleting {}, id: {}, size: {:,}, date: {}'.format(art['name'], art['id'], art['size'], art['date']))
    if not repo.dry_run:
        resp = requests.delete('%s/%d' % (artifacts_url, art['id']), auth = ('', repo.token))
        if resp.ok:
            deleted_count += 1
            deleted_size += size
        else:
            repo.error('status code: %d, reason: %s' % (resp.status_code, resp.reason))
            error_count += 1
            error_size += size

# Print final summary.
print('')
print('== Total:     {:5,} files, {:,} bytes'.format(total_count, total_size))
print('== Deleted:   {:5,} files, {:,} bytes'.format(deleted_count, deleted_size))
print('== Error:     {:5,} files, {:,} bytes'.format(error_count, error_size))
print('== Remaining: {:5,} files, {:,} bytes'.format(total_count - deleted_count - error_count, total_size - deleted_size - error_size))
