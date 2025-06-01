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
#  -w name | --workflow name : Workflow name or file name (ie. "release.yml").
#      If omitted, list the known workflows in the repository.
#  -r id | --run id : Run id. Default: most recent run.
#  -o path | --output-directory path : Where to download files.
#      Default: pkg/installers in the work area.
#  -l | --list : List workflow runs. Do not download artifacts.
#  -u | --unzip : Unzip the downloaded artifacts and delete the .zip file.
#
#  Common options:
#  --repo owner/repo : GitHub repository, default: tsduck/tsduck.
#  --token string : GitHub authentication token, default: $GITHUB_TOKEN.
#  --branch name : git branch in the repository, default: master.
#  --dry-run, -n: dry run, don't create anything.
#  --verbose, -v: verbose mode.
#  --debug: debug mode.
#
#-----------------------------------------------------------------------------

import sys, os, urllib.request, zipfile, tsgithub

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
    col1 = 'Workflow'
    col2 = 'File name'
    width1 = len(col1)
    width2 = len(col2)
    workflows = {}
    for w in repo.repo.get_workflows():
        workflows[w.name] = w.path if os.path.dirname(w.path) != '.github/workflows' else os.path.basename(w.path)
        width1 = max(width1, len(w.name))
        width2 = max(width2, len(workflows[w.name]))
    print()
    print('%-*s  %s' % (width1, col1, col2))
    print('%s  %s' % (width1 * '-', width2 * '-'))
    for name in workflows:
        print('%-*s  %s' % (width1, name, workflows[name]))
    print()
    exit(0)

# Search the workflow in the repo.
workflow = None
for w in repo.repo.get_workflows():
    fname = os.path.basename(w.path)
    if w.name == workflow_name or fname == workflow_name or fname == workflow_name + '.yml':
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
        archive = '%s/%s.zip' % (output_dir, arti.name)
        print('Artifact "%s", %d bytes -> %s' % (arti.name, arti.size_in_bytes, archive))
        # Download the artifact archive.
        req = urllib.request.Request(arti.archive_download_url)
        req.add_unredirected_header('Authorization', 'Bearer ' + repo.token)
        with urllib.request.urlopen(req) as input:
            with open(archive, 'wb') as output:
                output.write(input.read())
        # Extract the artifact if required.
        if unzip:
            z = zipfile.ZipFile(archive)
            for name in z.namelist():
                print('Extracting %s' % name)
            z.extractall(output_dir)
            z.close()
            os.remove(archive)
