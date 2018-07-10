This directory contains a sample third-party TSDuck plugin.

Prerequisites:

To be able to build this sample plugin, you must install the TSDuck
development environment first. On Windows systems, you must select the
optional "Development" component during the installation. On Ubuntu systems,
you must install the package tsduck-dev. On Fedora, Red Hat and CentOS, you
must install the package tsduck-devel.

Building the plugin on Linux:

Just run "make". The makefile in this directory contains the following line:

  include /usr/include/tsduck/tsduck.mk

The included makefile sets all required options to use the TSDuck library.

Building the plugin on Windows:

Either run the PowerShell script "build.ps1" or open the solution file
"sample-plugin.sln" using Visual Studio 2017 and build the solution.

The project file "tsplugin_sample.vcxproj" contains the following additional
line, just before the final line:

  <Import Project="$(TSDUCK)\tsduck.props" />

The included property file sets all required options to use the TSDuck library.

Using the plugin:

Either copy the plugin DLL or shared library into the same directory as the
tsp executable or define the environment variable TSPLUGINS_PATH to include
the path of the plugin.

See the test scripts test_sample.sh and test_sample.ps1.
