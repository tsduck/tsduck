# Using the TSDuck library   {#usinglibrary}

# Pre-requisites {#libreq}

To be able to build applications or `tsp` plugins with the TSDuck library,
you must install the TSDuck development environment first.

- On Windows systems, you must select the optional "Development" component
  during the installation.
- On Fedora, Red Hat, CentOS, AlmaLinux, you must install the package `tsduck-devel`.
- On Ubuntu, Debian and Raspbian, you must install the package `tsduck-dev`.
- On macOS systems, the development environment is always installed with TSDuck using Homebrew.
- If you build TSDuck from sources, use `make install install-devel`.

# Building applications on Linux, macOS and BSD systems

The command `tsconfig` generates the appropriate build options for the
current operating system. See the TSDuck user's guide for more details.

The following sample makefile illustrates the creation of a simple
application named `myexec` using one single source file `myexec.cpp`.
~~~~
CXXFLAGS += $(shell tsconfig --cflags)
LDLIBS += $(shell tsconfig --libs)

default: myexec
~~~~
This is as simple as that.

Just run `make` to build the application.
~~~~
$ make
~~~~

By default, the command `tsconfig --cflags` forces C++17 as level
of C++ language standard. If your application requires a more recent
level, define the environment variable `TS_NOSTDCPP` to any non-empty
value. This disables the C++ standard option in `tsconfig`. The
application shall then define its own C++ standard in its command line.
This user-specified C++ standard cannot be lower than C++11.

Alternatively, the command `tsconfig --nostdcpp --cflags` can be used
to omit the C++ standard from the compilation options without defining
the environment variable `TS_NOSTDCPP`.

# Building applications on Windows  {#libwindows}

The "Development" option of the TSDuck installer provides the build
environment for Visual Studio 2022, in debug and release mode, for
64-bit Intel platforms. It may be compatible with Visual Studio 2019
or earlier, but without guarantee.

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

By default, the property file `tsduck.props` forces C++17 as level
of C++ language standard. If your application requires a more recent
level, define the environment variable `TS_NOSTDCPP` to any non-empty
value. This disables the C++ standard option in `tsduck.props`. The
application shall then define its own C++ standard in its project files.
This user-specified C++ standard cannot be lower than C++11.
