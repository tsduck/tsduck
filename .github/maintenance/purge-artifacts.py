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
#  Prerequisite: requests
#  - Install: pip install requests
#  - Documentation: https://requests.readthedocs.io/
#
#-----------------------------------------------------------------------------

import requests
import tsgithub

# The number of copies to keep per artifact.
KEEP = 5

# A dictionary of all artifact counts, by name.
artifact_count = {}
total_count = 0
total_size = 0
deleted_count = 0
deleted_size = 0

# Loop on all pages of artifats
artifacts_url = 'https://api.github.com/repos/' + tsgithub.repo_name + '/actions/artifacts'
url = artifacts_url + '?per_page=100'
while url is not None:
    # Get a page of artifacts.
    resp = requests.get(url, headers = {'Accept': 'application/vnd.github.v3+json'}, auth = ('', tsgithub.token))
    url = resp.links['next']['url'] if ('next' in resp.links and 'url' in resp.links['next']) else None
    data = resp.json()
    # Loop on all artifacts in that page.
    if 'artifacts' in data:
        for art in data['artifacts']:
            if 'name' in art and 'id' in art:
                size = art['size'] if 'size' in art else 0
                date = art['created_at'] if 'created_at' in art else ''
                total_count += 1
                total_size += size
                if art['name'] not in artifact_count:
                    artifact_count[art['name']] = 0
                if ('expired' in art and art['expired']) or artifact_count[art['name']] >= KEEP:
                    print('Deleting %s, id: %d, size: %d, date: %s' % (art['name'], art['id'], size, date))
                    deleted_count += 1
                    deleted_size += size
                    requests.delete('%s/%d' % (artifacts_url, art['id']), auth = ('', tsgithub.token))
                else:
                    artifact_count[art['name']] += 1

# Print final summary.
print('')
print('== Total:     %5d files, %d bytes' % (total_count, total_size))
print('== Deleted:   %5d files, %d bytes' % (deleted_count, deleted_size))
print('== Remaining: %5d files, %d bytes' % (total_count - deleted_count, total_size - deleted_size))
