# Sample usage of ts::SignalizationDemux

This directory contains a sample third-party application which is not part
of TSDuck but uses the TSDuck library.

## Prerequisites

To be able to build this sample application, you must install the TSDuck
development environment first. On Windows systems, you must select the
optional "Development" component during the installation. On Debian or
Ubuntu systems, you must install the package tsduck-dev. On Fedora, Red Hat
and clones, you must install the package tsduck-devel.

## Building the application on Linux

Just run `make`. The makefile in this directory contains various calls to
the 'tsconfig' utility which generates the various required compilation
and link options.

## Building the application on Windows

Open the solution file `sample-app.sln` using Visual Studio 2017 and build
the application. The project file `sample-app.vcxproj` contains the
following additional line, just before the final line:

~~~
<Import Project="$(TSDUCK)\tsduck.props" />
~~~

The included property file sets all required options to use the TSDuck library.

## Sample output

The sample application produces the following output when used on two TS files,
a DVB one and an ATSC one. We can see that the virtual method `handleService()`
is called each time some new information is available on a service.

~~~
$ ./myexec tnt-uhf30-546MHz-2019-01-22.ts 473.ts
==== Analyzing tnt-uhf30-546MHz-2019-01-22.ts
-- Got PAT, 5 services
-- Got service 0x0401 (1025), no PMT
-- Got service 0x0402 (1026), no PMT
-- Got service 0x0407 (1031), no PMT
-- Got service 0x0415 (1045), no PMT
-- Got service 0x0416 (1046), no PMT
-- Got PMT, service id 1026, 8 components
-- Got service 0x0402 (1026), 8 components
-- Got PMT, service id 1045, 6 components
-- Got service 0x0415 (1045), 6 components
-- Got PMT, service id 1025, 8 components
-- Got service 0x0401 (1025), 8 components
-- Got PMT, service id 1031, 10 components
-- Got service 0x0407 (1031), 10 components
-- Got PMT, service id 1046, 8 components
-- Got service 0x0416 (1046), 8 components
-- Got SDT Actual, TS id 4, 5 services
-- Got service "M6", 0x0401 (1025), 8 components
-- Got service "W9", 0x0402 (1026), 8 components
-- Got service "Arte", 0x0407 (1031), 10 components
-- Got service "France 5", 0x0415 (1045), 6 components
-- Got service "6ter", 0x0416 (1046), 8 components
-- Got NIT Actual, network id 8442, 7 TS
-- Got service "M6", 0x0401 (1025), #6, 8 components
-- Got service "W9", 0x0402 (1026), #9, 8 components
-- Got service "Arte", 0x0407 (1031), #7, 10 components
-- Got service "France 5", 0x0415 (1045), #5, 6 components
-- Got service "6ter", 0x0416 (1046), #22, 8 components
==== Analyzing 473.ts
-- Got TVCT (ATSC), 4 channels
-- Got service "KULX   ", 10.1, 0x0003 (3), no PMT
-- Got service "TelXito", 10.2, 0x0004 (4), no PMT
-- Got service "LightTV", 10.3, 0x0005 (5), no PMT
-- Got service "Quest  ", 10.4, 0x0006 (6), no PMT
-- Got PAT, 4 services
-- Got service "KULX   ", 10.1, 0x0003 (3), no PMT
-- Got service "TelXito", 10.2, 0x0004 (4), no PMT
-- Got service "LightTV", 10.3, 0x0005 (5), no PMT
-- Got service "Quest  ", 10.4, 0x0006 (6), no PMT
-- Got PMT, service id 4, 2 components
-- Got service "TelXito", 10.2, 0x0004 (4), 2 components
-- Got PMT, service id 3, 2 components
-- Got service "KULX   ", 10.1, 0x0003 (3), 2 components
-- Got PMT, service id 6, 2 components
-- Got service "Quest  ", 10.4, 0x0006 (6), 2 components
-- Got PMT, service id 5, 2 components
-- Got service "LightTV", 10.3, 0x0005 (5), 2 components
~~~
