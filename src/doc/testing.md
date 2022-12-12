# Testing TSDuck   {#testing}

# Testing overview {#testoverview}

TSDuck is highly flexible, allowing an unlimited number of configurations.
Testing it is consequently challenging. Moreover, some use cases are difficult
to automate or require specific hardware. Testing live TS reception using
all possible DVB tuners or Dektec devices requires a lot of material, human
and time resources which are far beyond the capabilities of a free open-source
project.

From a strict industrial standpoint, TSDuck is consequently not fully tested.

However, on a pragmatic standpoint, test suites have been setup to extract the
best of what could be done with limited resources.

Thus, although it is impossible to guarantee that a given release of TSDuck
is bug-free, we may be relatively confident that no major regression has
been introduced.

# Organization of the tests {#testorg}

The code of TSDuck is divided in two parts, a large C++ library (`tsduck.dll` or `libtsduck.so`)
and a collection of small command line tools and plugins.

Similarly, the tests for TSDuck are divided in two parts.

- The TSDuck library has its own unitary test suite based on a custom framework named "TSUnit".
  This test suite is part of the [main tsduck](https://github.com/tsduck/tsduck) repository
  for TSDuck in directory `src/utest`.

- The tools and plugins are less easy to test. They work on large transport stream files which
  would clutter the tsduck repository. The repository [tsduck-test](https://github.com/tsduck/tsduck-test)
  contains those tests and the relevant scripts and data files.

The two test suites are fully automated.

# The TSDuck library test suite {#testlib}

In the main TSDuck repository, the directory `src/utest` contains the source file for
one single program named `utest`. This program is divided in many source files (or test suites).
Each source file contains many unitary tests. The test infrastructure is based on a custom
framework named "TSUnit".

The `utest` executable is built twice, once using the TSDuck shared library and once using
the static library. On UNIX systems, both versions of the test suite are built and run using
`make test` as illustrated below
~~~~
$ make -j10 test
  .....
  [CXX] utestXML.cpp
  [LD] /home/tsduck/bin/release-x86_64-vmubuntu/utest
  [LD] /home/tsduck/bin/release-x86_64-vmubuntu/utest_static
TSPLUGINS_PATH=/home/tsduck/bin/release-x86_64-vmubuntu LD_LIBRARY_PATH=/home/tsduck/bin/release-x86_64-vmubuntu /home/tsduck/bin/release-x86_64-vmubuntu/utest

OK (619 tests, 28730 assertions)

TSPLUGINS_PATH=/home/tsduck/bin/release-x86_64-vmubuntu LD_LIBRARY_PATH=/home/tsduck/bin/release-x86_64-vmubuntu /home/tsduck/bin/release-x86_64-vmubuntu/utest_static

OK (616 tests, 28707 assertions)

$
~~~~

Note that the statically linked version contains slightly less tests.
The missing tests are related to plugin activations and they can run
only in a shared library environment.

On Windows, the Visual Studio project builds two executables named
`utests-tsduckdll.exe` and `utests-tsducklib.exe`. They can be run
manually or from Vidual Studio:

~~~~
D:\tsduck> bin\Release-x64\utests-tsduckdll.exe

OK (619 tests, 28747 assertions)

D:\tsduck> bin\Release-x64\utests-tsducklib.exe

OK (616 tests, 28724 assertions)

D:\tsduck>
~~~~

By default, the test program runs all tests and reports failures only.
But `utest` also accepts a few command options which make it appropriate
to debug individual features.

The available command line options are:

* `-d` : Debug messages are output on standard error.
* `-l` : List all tests but do not execute them.
* `-t name` : Run only one test or test suite.

First, the option `-l` is used to list all available tests:

~~~~
$ utest -l
AlgorithmTest
    AlgorithmTest::testEnumerateCombinations
ArgsTest
    ArgsTest::testAccessors
    ArgsTest::testAmbiguousOption
........
    WebRequestTest::testNoRedirection
    WebRequestTest::testReadMeFile
XMLTest
    XMLTest::testChannels
    XMLTest::testCreation
    XMLTest::testDocument
    XMLTest::testEscape
    XMLTest::testFileBOM
    XMLTest::testInvalid
    XMLTest::testKeepOpen
    XMLTest::testTweaks
    XMLTest::testValidation
$
~~~~

The left-most names are test suite names. They represent one source file in
`src/utest`. It is possible to run all tests in a test suite or one specific
test.

~~~~
$ utest -t ArgsTest
$ utest -t XMLTest::testValidation
~~~~

The option `-d` is used to produce debug message (see examples in the test
source files).

Thus, `utest` alone can be used as an automated fairly complete non-regression
test suite for the TSDuck library or as a debug environment for a given feature
under development (in all good "test-driven development" approaches, the code
is written at the same time as its unitary test).

# The TSDuck tools and plugins test suite {#testtools}

The Git repository [tsduck-test](https://github.com/tsduck/tsduck-test)
contains some tests for the TSDuck tools and plugins. This test suite is
currently less exhaustive that the TSDuck library test suite and it should
be enriched in the future.

This test suite is fully automated and works on test files only. No specific
hardware (DVB tuner or Dektec device), no live stream is tested. This is a
known limitation.

The principle is simple. Reference input files are stored in the repository,
mainly transport stream files which were once captured from real live sources.
Test scripts contain deterministic commands which should always produce the
same outputs (data, logs, etc). These reference outputs are also stored in
the repository.

The test suite runs using a given version of TSDuck and its outputs are
compared with the reference outputs. If differences are detected, there
is a potential problem.

Since different versions of TSDuck may produce slightly different outputs,
a given version of the test suite formally applies to one version of TSDuck
only. Git tags are aligned in both repositories (or should be...) to indicate
the target version.

## Structure of the test suite {#teststruct}

In short, execute the script `run-all-tests.sh` to run the complete test suite.

The repository contains the following subdirectories:

- `tests` : Contains one script per test or set of tests. Each test script can be
  executed individually. All tests are executed using the script `run-all-tests.sh`.

- `common` : Contains utilities and common script.

- `input` : Contains input data files for the tests.

- `reference` : Contains reference output files for the various tests.

- `tmp` : Contains output files which are created by the execution of the tests.
  These files are typically compared against reference output files in `reference`.
  These files are temporary by definition. The subdirectory `tmp` is present on
  test machines only and is excluded from the Git repository.

## Adding new tests {#testadd}

To add a new test:

- Add input files in subdirectory `input`.

- Create the script `test-NNN.sh` in subdirectory `tests`. Use other existing
  test scripts as templates.

- Run the command `tests/test-NNN.sh --init`. If the test is properly written,
  this creates the reference output files in subdirectory `reference`. Manually
  check the created files, verify that they are correct. Be careful with this
  step since these files will be used as references.

- Run the same command without the `--init` option. This time, the output files
  are created in `tmp` and are compared with files in `reference`. Verify that
  all tests pass. Errors may appear if the test script is not properly written
  or if the output files contain unique, non-deterministic, time-dependent,
  system-dependent or file-system-dependent information. Make sure the output
  files are totally reproduceable in all environments. At worst, add code in
  the test script to remove any information from the output files which is
  known to be non-reproduceable.

Sometimes, TSDuck is modified in such in a way that an output file is modified
on purpose. Usually, this starts with a failed test. When analysing the test
failure, it appears that the modification of the output is intentional. In that
case, re-run the command `tests/test-NNN.sh --init` to update the reference
output files. Do not forget to manually validate them since they will act as
the new reference.

## Testing a development version {#testdev}

By default, the test suite uses the TSDuck command from the system path.
Typically, it will use the installed version.

To test a development version, the two Git repositories `tsduck` and
`tsduck-test` shall be checked out at the same level, side by side
in the same directory. First, TSDuck shall be rebuilt in its repository.

Then, when the option `--dev` is specified to a test script or to `run-all-tests.sh`,
the test suite automatically uses the TSDuck executables from the development
repository.

## Large files {#testlargefiles}

The `tsduck-test` repository contains large files, typically transport stream files.

Initially, these files were not stored inside the regular GitHub repository.
Instead, they used the [Git Large File Storage](https://git-lfs.github.com) (LFS) feature of GitHub.
However, using LFS on GitHub happended to be a pain, as experienced by others and explained in
[this article](https://medium.com/@megastep/github-s-large-file-storage-is-no-panacea-for-open-source-quite-the-opposite-12c0e16a9a91).

As a consequence, the transport stream files were re-integrated into the Git
repository. But we now limit their size to 20 MB.
