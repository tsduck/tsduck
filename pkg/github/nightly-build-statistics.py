#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Report all executions of nightly-build workflow in reverse order.
#
#-----------------------------------------------------------------------------

import sys, tsgithub

# Get command line options.
repo = tsgithub.repository(sys.argv)
repo.check_opt_final()

workflow = repo.repo.get_workflow('nightly-build.yml')

print('  Run#   Created               Started               Duration   Status     Origin')
print('------   -------------------   -------------------   --------   --------   ---------')

for run in workflow.get_runs():
    created = run.created_at.strftime('%Y-%m-%d %H:%M:%S')
    started = run.run_started_at.strftime('%Y-%m-%d %H:%M:%S')
    try:
        seconds = int(run.timing().run_duration_ms / 1000)
    except:
        seconds = 0
    duration = '%02d:%02d:%02d' % (seconds // 3600, (seconds // 60) % 60, seconds % 60)
    print('%6d   %s   %s   %s   %-8s   %s' % (run.run_number, created, started, duration, run.conclusion, run.event))



