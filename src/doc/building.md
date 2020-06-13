# Building TSDuck   {#building}
[TOC]

TSDuck can be built on Windows, Linux and macOS.

Support for Dektec devices, DVB tuners and HiDes modulators is implemented only
on Windows and Linux. macOS can only support files and IP for TS input and output.

# Pre-requisites: build tools {#buildreq}

## Windows {#reqwindows}

First, install Visual Studio Community Edition.
This is the free version of Visual Studio.
It can be downloaded [here](https://www.visualstudio.com/downloads/).
If you already have Visual Studio Enterprise Edition (the commercial version),
it is fine, no need to install the Community Edition.

Then, execute the PowerShell script `build\install-prerequisites.ps1`.
It downloads and installs the requested packages which are necessary
to build TSDuck on Windows.

If you prefer to collect the various installers yourself, follow the links to
[NSIS downloads](http://nsis.sourceforge.net/Download),
[SRT downloads](https://github.com/tsduck/srt-win-installer/releases/latest),
[DTAPI downloads](https://www.dektec.com/downloads/SDK),
[Doxygen downloads](http://www.doxygen.org/download.html) and
[Graphviz downloads](https://graphviz.gitlab.io/_pages/Download/Download_windows.html).

## Linux and macOS {#reqfedora}

Execute the shell-script `build/install-prerequisites.sh`.
It downloads and installs the requested packages which are necessary
to build TSDuck. The list of packages and how to install them depend
on the operating system distribution and version.

Currently, the script supports the following operating systems:
- macOS
- Ubuntu
- Debian
- Raspbian (Debian for Raspberry Pi)
- Fedora
- Red Hat Enterprise Linux
- CentOS
- Arch Linux
- Alpine Linux

Dektec DTAPI: The command `make` at the top level will automatically
download the LinuxSDK from the Dektec site. See `dektec/Makefile` for details.
There is no manual setup for DTAPI on Linux.

But note that the Dektec DTAPI is available only for Linux distros on Intel CPU's
with the GNU libc. Non-Intel systems (for instance ARM-based devices such as
Raspberry Pi) cannot use Dektec devices. Similarly, Intel-based distros using
a non-standard libc (for instance Alpine Linux which uses musl libc) cannot use Dektec
devices either.

# Building the TSDuck binaries {#buildbin}

## Windows {#buildwindows}

Execute the PowerShell script `build\build.ps1`. The TSDuck binaries, executables and
DLL's, are built in directories `bin\Release-Win32` and `bin\Release-x64`
for 32-bit and 64-bit platforms respectively.

## Linux and macOS {#buildlinux}

Execute the command `make` at top level. The TSDuck binaries, executables and shared
objects (`.so`), are built in directories `bin/release-i386-<hostname>`,
`bin/release-x86_64-<hostname>` and `bin/release-arm-<hostname>` for Intel 32-bit,
64-bit and ARM platforms respectively.

To build a 32-bit version of TSDuck on a 64-bit system, execute the command `make m32`.
Of course, this works only if your 64-bit system has all required 32-bit development
tools and libraries.

### Building without specialized dependencies

In specific configurations, you may want to disable some external libraries
such as `libcurl` or `pcsc-lite`. Of course, the corresponding features in
TSDuck will be disabled but the impact is limited. For instance, disabling
`libcurl` will disable the plugin `http` (the plugin will still be there
but it will report an error when used).

The following `make` variables can be defined:

- `NOTEST`  : Do not build unitary tests.
- `NODTAPI` : No Dektec support, remove dependency to `DTAPI`.
- `NOCURL`  : No HTTP support, remove dependency to `libcurl`.
- `NOPCSC`  : No smartcard support, remove dependency to `pcsc-lite`.
- `NOSRT`   : No SRT (Secure Reliable Transport), remove dependency to `libsrt`.
- `NOTELETEXT` : No Teletext support, remove teletext handling code.
- `ASSERTIONS` : Keep assertions in production mode (slower code).

The following command, for instance, builds TSDuck without dependency
to `pcsc-lite`, `libcurl` and Dektec `DTAPI`:
~~~
make NOPCSC=1 NOCURL=1 NODTAPI=1
~~~

# Building the TSDuck installers {#buildinst}

There is no need to build the TSDuck binaries before building the installers.
Building the binaries, when necessary, is part of the installer build.

All installation packages are dropped into the subdirectory `installers`.
The packages are not deleted by the cleanup procedures. They are not pushed
into the git repository either.

## Windows {#instwindows}

Execute the PowerShell script `build\build-installer.ps1`.
Two installers are built, for 32-bit and 64-bit systems respectively.

## Fedora, CentOS, Red Hat Entreprise Linux {#instrhel}

Execute the command `make rpm` at top level to build a `.rpm` package for the same
architecture as the build system. On 64-bit systems, execute the command `make rpm32`
to build a 32-bit package.

## Ubuntu, Debian, Raspbian {#instubuntu}

Execute the command `make deb` at top level to build a `.deb` package for the same
architecture as the build system.

## Installer files {#instfiles}

The following table summarizes the packages which are built and dropped
into the `installers` directory, through a few examples, assuming that the
current version of TSDuck is 3.12-745.

| File name                             | Description
| ------------------------------------- | -----------------------------------------------------
| TSDuck-3.12-745-src.zip               | Source archive on Windows
| tsduck-3.12-745.tgz                   | Source archive on Linux and macOS
| tsduck_3.12-745_amd64.deb             | Binary package for 64-bit Ubuntu
| tsduck_3.12-745_armhf.deb             | Binary package for 32-bit Raspbian (Raspberry Pi)
| tsduck-3.12-745.el7.i386.rpm          | Binary package for 32-bit Red Hat or CentOS 7.x
| tsduck-3.12-745.el7.x86_64.rpm        | Binary package for 64-bit Red Hat or CentOS 7.x
| tsduck-3.12-745.el7.src.rpm           | Source package for Red Hat or CentOS 7.x
| tsduck-3.12-745.fc25.i386.rpm         | Binary package for 32-bit Fedora 25
| tsduck-3.12-745.fc25.x86_64.rpm       | Binary package for 64-bit Fedora 25
| tsduck-3.12-745.fc25.src.rpm          | Source package for Fedora 25
| tsduck-dev_3.12-745_amd64.deb         | Development package for 64-bit Ubuntu
| tsduck-dev_3.12-745_armhf.deb         | Development package for 32-bit Raspbian (Raspberry Pi)
| tsduck-devel-3.12-745.el7.i386.rpm    | Development package for 32-bit Red Hat or CentOS 7.x
| tsduck-devel-3.12-745.el7.x86_64.rpm  | Development package for 64-bit Red Hat or CentOS 7.x
| tsduck-devel-3.12-745.fc25.i386.rpm   | Development package for 32-bit Fedora 25
| tsduck-devel-3.12-745.fc25.x86_64.rpm | Development package for 64-bit Fedora 25
| TSDuck-Win32-3.12-745.exe             | Binary installer for 32-bit Windows
| TSDuck-Win64-3.12-745.exe             | Binary installer for 64-bit Windows
| TSDuck-Win32-3.12-745-Portable.zip    | Portable package for 32-bit Windows
| TSDuck-Win64-3.12-745-Portable.zip    | Portable package for 64-bit Windows

On Linux systems, there are two different packages. The package `tsduck` contains
the tools and plugins. This is the only required package if you just need to use
TSDuck. The package named `tsduck-devel` (or `tsduck-dev` on Ubuntu) contains the
development environment. It is useful only for third-party applications which use
the TSDuck library.

On Windows systems, there is only one binary installer which contains the tools,
plugins, documentation and development environment. The user can select which
components shall be installed. The development environment is unselected by default.

# Installing in non-standard locations {#nonstdinst}

On systems where you have no administration privilege and consequently no right
to use the standard installers, you may want to manually install TSDuck is some
arbitrary directory.

On Windows systems, a so-called _portable_ package is provided. This is a zip
archive file which can be expanded anywhere.

On Unix systems (which include Linux and macOS), you have to rebuild TSDuck from
the source repository and install it using a command like this one:

~~~~
make install SYSPREFIX=$HOME/usr/local
~~~~

In all cases, Windows or Unix, the TSDuck commands are located in the `bin`
subdirectory and can be executed from here without any additional setup.
It is probably a good idea to add this `bin` directory in your `PATH`
environment variable.

# Running from the build location {#runbuild}

It is sometimes useful to run a TSDuck binary, `tsp` or any other, directly
from the build directory, right after compilation. This can be required for
testing or debugging.

## Windows {#runwindows}

On Windows, the binaries and all plugins are built in a subdirectory named
`bin\<target>-<platform>`. The commands can be run using their
complete path.

For instance, to run the released 64-bit version of `tsp`, use:
~~~~
D:\tsduck> bin\Release-x64\tsp.exe --version
tsp: TSDuck - The MPEG Transport Stream Toolkit - version 3.12-730
~~~~

For other combinations (release vs. debug and 32 vs. 64 bits), the paths
from the repository root are:
~~~~
bin\Release-x64\tsp.exe
bin\Release-Win32\tsp.exe
bin\Debug-x64\tsp.exe
bin\Debug-Win32\tsp.exe
~~~~

## Linux and macOS {#rununix}

On all Unix systems, the binaries and all plugins are built in a subdirectory
named `bin\<target>-<platform>-<hostname>`. The commands can be run using their
complete path.

For instance, to run the latest build of `tsp` on a Mac system, use:
~~~~
$ bin/release-x86_64-mymac/tsp --version
tsp: TSDuck - The MPEG Transport Stream Toolkit - version 3.22-1823
~~~~

Because the binary directory name contains the host name, it is possible to build
TSDuck using the same shared source tree from various systems or virtual machines.
All builds will coexist using distinct names under the `bin` subdirectory.

For _bash_ users who wish to include the binary directory in the `PATH`, simply
"source" the script `build/setenv.sh`. Example:
~~~~
$ . build/setenv.sh 
$ which tsp
/Users/devel/tsduck/bin/release-x86_64-mymac/tsp
~~~~

This script can also be used with option `--display` to display the actual
path of the binary directory. The output can be used in other scripts
(including from any other shell than _bash_). Example:
~~~~
$ build/setenv.sh --display
/Users/devel/tsduck/bin/release-x86_64-mymac
~~~~

# Cleaning up {#buildcleanup}

On Windows, to cleanup a repository tree and return to a pristine source state,
execute the following PowerShell script:
~~~~
build\cleanup.ps1
~~~~

On Linux and macOS, the same cleanup task is achieved using the following command:
~~~~
make distclean
~~~~
