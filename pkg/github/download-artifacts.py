#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Download all artifacts of a workflow run.
#
#  Options:
#    -w name | --workflow name
#        Workflow name or file name (ie. "release.yml"). If omitted, list the
#        known workflows in the repository.
#    -r id | --run id
#        Run id. Default: most recent run.
#    -o path | --output-directory path
#        Where to download files. Default: pkg/installers in the work area.
#    -l | --list
#        List workflow runs. Do not download artifacts.
#    -u | --unzip
#        Unzip the downloaded artifacts and delete the .zip file.
#
#-----------------------------------------------------------------------------

import sys, os, urllib, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
workflow_name = repo.get_opt(['-w', '--workflow'])
run_id = repo.get_opt(['-r', '--run'])
output_dir = repo.get_opt(['-o', '--output-directory'], tsgithub.repo_root() + '/pkg/installers')
list_only = repo.has_opt(['-l', '--list'])
unzip = repo.has_opt(['-u', '--unzip'])
repo.check_opt_final()

# List workflows only.
if workflow_name is None:
    workflows = {}
    width = 0
    for w in repo.repo.get_workflows():
        workflows[w.name] = w.path if os.path.dirname(w.path) != '.github/workflows' else os.path.basename(w.path)
        width = max(width, len(w.name))
    for name in workflows:
        print('%-*s  %s' % (width, name, workflows[name]))
    exit(0)

# Search the workflow in the repo.
workflow = None
for w in repo.repo.get_workflows():
    if w.name == workflow_name or os.path.basename(w.path) == workflow_name:
        workflow = w
        break
if workflow is None:
    repo.fatal('Workflow "%s" not found in repository' % workflow_name)

# List workflow runs only.
if list_only:
    for r in workflow.get_runs():
        a = r.get_artifacts()
        print('Run %d, %s, %d artifacts' % (r.id, r.run_started_at, a.totalCount if a is not None else 0))
    exit(0)

# Get workflow run.
if run_id is None:
    runs = workflow.get_runs()
    if runs is None or runs.totalCount == 0:
        repo.fatal('No run found for workflow %s' % workflow.name)
    run = runs[0]
else:
    run = repo.repo.get_workflow_run(int(run_id))

# We need a valid output directory.
if not os.path.isdir(output_dir):
    repo.fatal('Directory %s not found')

# Get artifacts.
artifacts = [a for a in run.get_artifacts()]
print('Workflow "%s", run id %d, %d artifacts' % (workflow.name, run.id, len(artifacts)))
for arti in artifacts:
    if arti.expired:
        print('Artifact "%s" has expired, cannot be downloaded' % arti.name)
    else:
        print('Artifact "%s", %d bytes' % (arti.name, arti.size_in_bytes))
        zipfile = '%s/%s.zip' % (output_dir, arti.name)
        print(arti.archive_download_url)
        urllib.request.urlretrieve(arti.archive_download_url, zipfile)
