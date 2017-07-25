# Using the TSDuck library   {#usinglibrary}
[TOC]

TSDuck can be used as a general purpose C++ library for third-party applications
outside the TSDuck set of tools and plugins. The TSDuck library offers generic
C++ classes for operating systems features as well as specialized classes for
MPEG/DVB transport streams.

All classes which are documented here are available from the TSDuck library
and can be used by any application.

# Pre-requisites {#libreq}

To be able to build applications with the TSDuck library, you must install the
TSDuck development environment first. On Windows systems, you must select the
optional "Development" component during the installation. On Ubuntu systems,
you must install the package `tsduck-dev`. On Fedora, Red Hat and CentOS, you
must install the package `tsduck-devel`.

# Building applications on Linux  {#liblinux}

The TSDuck header files are located in `/usr/include/tsduck`.
In the same directory, a partial makefile named `tsduck.mk` defines
all options and requirements to use the TSDuck library.

The following sample makefile illustrates the creation of a simple
application named `myexec` using one single source file `myexec.cpp`.
~~~~
include /usr/include/tsduck/tsduck.mk
default: myexec
~~~~
This is as simple as that.

Just run "make" to build the application.
~~~~
$ make
~~~~

By default, the application is built against the TSDuck dynamic
library in `/usr/bin`. Define the make symbol `TS_STATIC` to link against
the TSDuck static library:
~~~~
$ make TS_STATIC=true
~~~~

# Building applications on Windows  {#libwindows}

The "Development" option of the TSDuck installer provides the build
environment for Visual Studio 2017, in debug and release mode, for
32-bit and 64-bit platforms.

The environment variable `TSDUCK` is defined to the root of the
TSDuck installation tree. A Visual Studio property file named
`tsduck.props` is installed here. It provides all definitions
and options to use the TSDuck library.

Create the solution and projects for your application. Then, manually
edit the project file, named for instance `app.vcxproj`, and insert
the following line just before the final `</Project>` closing tag:
~~~~
<Import Project="$(TSDUCK)\tsduck.props" />
~~~~

Then build your project normally.
