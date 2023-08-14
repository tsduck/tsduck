# Build automation  {#automation}

TSDuck is a free open-source project which is maintained on spare time by
very few developers (only one at the time of this writing). As a consequence,
the maintainance of the project is driven by the lack of time and resource.
To face these constraints, automation is essential.

This is briefly explained in [this presentation](https://tsduck.io/download/docs/tsduck-project.pdf).

For future maintainers or contributors, this page lists a few automation
procedures which are used by the project.

## Generation of the user's guide  {#auto_ug}

The TSDuck user's guide is a large Microsoft Word file (`doc/tsduck.docx`).
Each time the Word file is modified, a number of repetitive tasks shall be
performed: updating the TSDuck version, the release date, the table of contents,
creating the PDF file, etc. A Powershell script named `doc/build-user-guide-pdf.ps1`
automates all these tasks.

Documentation update procedure:
- Double-click on `doc/tsduck.docx` to open it with Microsoft Word, update the
  documentation (new options or commands), save and quit.
- Double-click on `doc/build-user-guide-pdf.ps1`.

If you have defined the default action for PowerShell script to run them, the script
is executed, the Word file is automatically updated with all version and date information,
all fields are updated, the PDF file is generated.

Be sure to commit and push the updated Word and PDF files.

## Continuous integration  {#auto_ci}

Each time a commit or a pull request is pushed to GitHub, the workflow
`.github/workflows/continuous-integration.yml` is automatically run to
verify that the project can be built on most supported platforms and
contains no regression.

- The workflow is run on Linux Ubuntu, macOS, and Windows (64 and 32 bits).
- The compilation is done using two revisions of the C++ language: C++11
  (or C++14 on Windows) and C++20.
- On Linux, the builds are repeated using two compilers, `gcc` and `clang`.
- In each configuration of platform, compiler and language level:
  - TSDuck is fully built.
  - The unitary tests are run.
  - The full test-suite is downloaded and run.
  - The sample programs are built using a typical TSDuck installation of the
    binaries which were just built (Linux and macOS only).
- The doxygen documentation is rebuilt. Missing documentation for new features can be
  found in the log. This is done on Ubuntu only.

## Nightly builds  {#auto_nightly}

Every night, if there were any modification in the TSDuck repo during the last day,
"nightly builds" are automatically produced and published on the [TSDuck web site](https://tsduck.io/).
No manual action is required, everything is automated.

### Building binaries and documentation

The workflow `.github/workflows/nightly-build.yml` is automatically run every day at 00:40 UTC.

- On Linux Ubuntu and Windows (64 bits only), the TSDuck binary packages are built
  for the current state of the master branch in the repository.
- The produced binaries are installed on the CI/CD system.
- The full test-suite is downloaded and run.
- The complete set of doxygen documentation (a.k.a. "TSDuck programmer's guide")
  is built and packaged in an archive. If the installed version of doxygen is known
  to contain bugs in the generation of the TSDuck documentation (many were found),
  the source code for a known good version of doxygen is downloaded and rebuilt first.
  This is done to ensure that the documentation is always correct.
- The Linux Ubuntu binaries, the Windows binaries and the documentation package are
  published as "artefacts" of the workflow. They are publicly downloadable from
  [GitHub](https://github.com/tsduck/tsduck/actions/).

### Publishing on the web site

At the end of the `nightly-build.yml` workflow, the update worklow named
`.github/workflows/nightly-build-update.yml` is automatically run.

It triggers a signal on the [TSDuck web site](https://tsduck.io/). A PHP script
is automatically run on the web site to retrieve, download and publish the latest
[nighly binaries](https://tsduck.io/download/prerelease/) and
[programming documentation](https://tsduck.io/doxy/).

Note: It had been observed, in rare cases, that the artefacts of the nighlty build
workflow are published with some delay, typically after the start of the update
workflow. In that case, the PHP script on the web site does not find any new
artefact and the nighty builds are not updated. To solve this issue, the update
workflow is also scheduled to run every day at 11:15 UTC. Thus, if the nightly
builds were missed at the end of their workflow, they will be grabbed around midday.

## Release creation  {#auto_release}

Creating an "official" TSDuck release is relatively easy. Although the build
time for all binaries can be long, everything is automated and run in the
background without requiring the maintainer's attention.

Since TSDuck is maintained with little resource and time, there is no product
management cycle. Thanks to the [continuous integration process](#auto_ci),
any state of the repository can be used as a release candidate, as long as
there is no issue in the CI process and no major development or fix is in progress.

As a rule of thumb, a new release is produced every 3 to 6 months, when enough
new features or fixes are available, based on the
[CHANGELOG file](https://tsduck.io/download/changelog/).

### Building the various binaries

Binaries must be built for Windows (64 bits only) and a few major Linux distros
(Fedora, RedHat, Ubuntu, Debian, and 32-bit Raspian). Additional binaries are
now built for some Arm64 Linux distros.

The idea is to have a central Unix system (Linux or macOS) from which the builds
are orchestrated. The script `scripts/build-remote.sh` can be used to build
TSDuck binaries on various remote systems and collect the resulting installers.

The remote systems can be physical systems or virtual machines running on the
central system. When a virtual machines is not active, it is automatically
booted, the binaries are built and collected, and then the virtual machines is shut down.
The currently supported hypervisors to managed virtual machines are VMware
and Parallels Desktop (macOS). All systems shall be preconfigured so that
the central user is authorized to ssh on each them without password
(use public key authentication, not passwordless accounts!)

The maintainer shall typically maintain a personal script, outside the
repository, which calls `build-remote.sh` on all systems.

Typical example of such a script, with one physical remote system (a Raspberry Pi)
and 5 virtual machines on the local host:
~~~
$HOME/tsduck/scripts/build-remote.sh --host raspberry
$HOME/tsduck/scripts/build-remote.sh --host vmfedora --parallels Fedora
$HOME/tsduck/scripts/build-remote.sh --host vmredhat --parallels RedHat
$HOME/tsduck/scripts/build-remote.sh --host vmubuntu --parallels Ubuntu
$HOME/tsduck/scripts/build-remote.sh --host vmdebian --parallels Debian
$HOME/tsduck/scripts/build-remote.sh --host vmwindows --parallels Windows --windows --directory Documents/tsduck --timeout 20
~~~

So, building a release is as simple as running that script. After a couple
of hours, all binaries and build log files are available in the `installers`
subdirectory.

Be sure to check that all binaries are present. Check the log files if
some of them are missing.

### Creating the GitHub release

Once the binary installers are collected, simply run this command to
create and publish the release on GitHub:
~~~
.github/maintenance/release.py --create
~~~

The current state of the repo on GitHub is used as base for the release.
A tag is created with the version number. The release is created with
a explanatory description. All binaries are published as assets of that
release.

### Creating the HomeBrew release

The GitHub release contains binaries for Linux and Windows. On macOS,
TSDuck is distributed through [HomeBrew](https://brew.sh/). The binaries
for all macOS versions, Intel and Arm platforms, are built inside the
HomeBrew CI/CD pipeline when an updated formula is pushed in a pull
request.

After the creation of the release on GitHub, create the pull request
for the new TSDuck version in HomeBrew using the following command:
~~~
.homebrew/brew-bump-formula-pr.sh -f
~~~

The option `-f` forces the creation of the pull request. Without it,
it works in "dry run" mode. It can be safe to start with a dry run,
as a test.

When the HomeBrew CI/CD pipeline has finished to process the pull
request, the new version of TSDuck is available for all flavors
of macOS.

### Updating the version number

Once a release is published, the minor version number of TSDuck
`TS_VERSION_MINOR` must be updated in the source file `src/libtsduck/tsVersion.h`.

This is currently not automated and shall be manually updated before
the first commit following the publication of a new release.

## Cleanup of long-standing issues  {#auto_issues}

The [issues area on GitHub](https://github.com/tsduck/tsduck/issues) is used to report problems,
ask questions, and support any discussion about TSDuck. When an issue is obviously
completed, because a complete answer was provided or a fixed is pushed, the issue is
closed. Sometimes, a plausible response or fix is provided but some feedback is expected
from the user to confirm this. When a positive feedback is provided, the issue is closed.

However, some users never provide a feedback after their problem is solved. In that case,
the issue remains open forever.

To solve this, there is a label named "close pending". When a plausible response, solution
or fix is provided, the maintainer of the project sets the "close pending" label on the
issue. It remains open. However, if the issue is not updated in the next 150 days, it will
be automatically closed.

This is achieved by the workflow `.github/workflows/cleanup-issues.yml`.
This workflow is scheduled every week on Sunday at 02:00 UTC. It runs the Python
script `.github/maintenance/close-pending.py` which automatically closes all issues
with label "close pending" and no update within the last 150 days.
