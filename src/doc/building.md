# Building TSDuck   {#building}
[TOC]

TSDuck can be built on Windows, Linux and macOS.

Support for Dektec devices, DVB tuners and HiDes modulators is implemented only
on Windows and Linux. macOS can only support files and IP for TS input and output.

# Pre-requisites: build tools {#buildreq}

## Windows {#reqwindows}

- Visual Studio 2017 Community Edition. This is the free version of Visual Studio.
  If can be downloaded [here](https://www.visualstudio.com/downloads/). This link
  allows the download of the latest version of Visual Studio but offers no way to
  download a specific previous version.

- CppUnit binary libraries. Download [here](https://sourceforge.net/projects/cppunit-msvc/files/).

- Doxygen for Windows. Download [here](http://www.doxygen.org/download.html).

- Graphviz for Windows (used by Doxygen to generate graphs and diagrams).
  Download [here](https://graphviz.gitlab.io/_pages/Download/Download_windows.html).

- NSIS, the NullSoft Scriptable Install System.
  Download [here](http://nsis.sourceforge.net/Download).
  Note that TSDuck is usually built with
  [NSIS Version 2.46](https://sourceforge.net/projects/nsis/files/NSIS%202/2.46/nsis-2.46-setup.exe/download).

- Optional Dektec Windows SDK (DTAPI): Execute the PowerShell script `dektec\Download-Install-Dtapi.ps1`.
  It downloads and installs the Dektec Windows SDK from the Dektec site.
  Alternatively, you may download it [here](http://www.dektec.com/products/SDK/DTAPI/Downloads/WinSDK.zip).
  TSDuck project files will detect DTAPI automatically. See the Visual Studio property
  file `build\msvc2017\msvc-use-dtapi.props` for details.

## Fedora {#reqfedora}

- Setup for a TSDuck native build:
~~~~
dnf install gcc-c++ doxygen dos2unix graphviz curl pcsc-tools pcsc-lite-devel cppunit-devel libcurl libcurl-devel rpmdevtools
~~~~

- Setup to build 32-bit TSDuck on 64-bit system (command `make m32`):
~~~~
dnf install glibc-devel.i686 libstdc++-devel.i686 pcsc-lite-devel.i686 cppunit-devel.i686 libcurl-devel.i686
~~~~

## Red Hat Entreprise Linux, CentOS {#reqrhel}

- Setup for a TSDuck native build:
~~~~
yum install gcc-c++ doxygen dos2unix graphviz curl pcsc-tools pcsc-lite-devel cppunit-devel libcurl libcurl-devel rpmdevtools
~~~~

- Setup to build 32-bit TSDuck on 64-bit system (command `make m32`):
~~~~
yum install glibc-devel.i686 libstdc++-devel.i686 pcsc-lite-devel.i686 cppunit-devel.i686 libcurl-devel.i686
~~~~

## Ubuntu, Debian, Raspbian {#requbuntu}

- Setup for a TSDuck native build:
~~~~
apt install g++ dpkg-dev doxygen dos2unix graphviz curl pcscd libpcsclite-dev libcppunit-dev libcurl3 libcurl3-dev
~~~~

- Starting with Ubuntu 18.04, `libcurl3` has been replaced by `libcurl4` and the installation commmand becomes:
~~~~
apt install g++ dpkg-dev doxygen dos2unix graphviz curl pcscd libpcsclite-dev libcppunit-dev libcurl4 libcurl4-openssl-dev
~~~~

- It is not possible to build 32-bit TSDuck on 64-bit Ubuntu system (command `make m32`) because
  there is no 32-bit cross-compiled package for pcsc and cppunit on Ubuntu 64-bit.

## Specific GCC requirement on Raspian and Debian {#reqraspbian}

It has been noted that GCC 6 and 7 are broken and fail to compile TSDuck version 3.17 and higher.
As of TSDuck version 3.17, the latest versions of the major Linux distros (Fedora, CentOS,
Red Hat Entreprise, Ubuntu) have either older or newer versions of GCC. However, Raspbian 9.x and
Debian 9.x (stretch) embed GCC 6.3.0, one of these buggy versions of GCC.

If you have such a broken GCC, you need to install an older or newer version of GCC.

The following method has been successfully used to build TSDuck on Raspbian 9.9.
It probably also works with Debian. First, install the version 4.9 of GCC using:
~~~~
apt install gcc-4.9 g++-4.9
~~~~

Then, build TSDuck using the following command:
~~~~
make GCC=gcc-4.9 CXX=g++-4.9 AR=gcc-ar-4.9 NOTEST=true
~~~~

The compiler and associated tools are redirected to their version 4.9.
The flag `NOTEST=true` is required because the version of CppUnit which
is installed with the system is not compatible with GCC 4.9.

If you chose to install a more recent version of the compiler,
[this article](https://solarianprogrammer.com/2017/12/08/raspberry-pi-raspbian-install-gcc-compile-cpp-17-programs/)
explains how to install GCC 8.1.0 on Raspbian.

## All Linux distros {#reqlinux}

- Optional Dektec DTAPI: The command `make` at the top level will automatically
  download the LinuxSDK from the Dektec site. See `dektec/Makefile` for details.

## macOS {#reqmac}

- Install the Xcode command line utilities (in other words, the _clang_ compiler suite):
~~~~
xcode-select --install
~~~~

- Install the [Homebrew](https://brew.sh/) package manager:
~~~~
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
~~~~

- Install common open source tools using Homebrew:
~~~~
brew install pcsc-lite cppunit doxygen graphviz gnu-sed grep
~~~~

# Building the TSDuck binaries {#buildbin}

## Windows {#buildwindows}

Execute the PowerShell script `build\Build.ps1`. The TSDuck binaries, executables and
DLL's, are built in directories `msvc2017\Release-Win32` and `msvc2017\Release-x64`
for 32-bit and 64-bit platforms respectively.

## Linux and macOS {#buildlinux}

Execute the command `make` at top level. The TSDuck binaries, executables and shared
objects (`.so`), are built in the `src` directory tree in subdirectories `release-i386`
and `release-x86_64` for 32-bit and 64-bit platforms respectively.

To build a 32-bit version of TSDuck on a 64-bit system, execute the command `make m32`.

### Building without specialized dependencies

In specific configurations, you may want to disable some external libraries
such as `libcurl` or `pcsc-lite`. Of course, the corresponding features in
TSDuck will be disabled but the impact is limited. For instance, disabling
`libcurl` will disable the plugin `http` (the plugin will still be there
but it will report an error when used).

The following `make` variables can be defined:

- `NOTEST`  : No unitary test, remove dependency to `CppUnit`.
- `NODTAPI` : No Dektec support, remove dependency to `DTAPI`.
- `NOCURL`  : No HTTP support, remove dependency to `libcurl`.
- `NOPCSC`  : No smartcard support, remove dependency to `pcsc-lite`.
- `NOTELETEXT` : No Teletext support, remove teletext handling code.

The following command, for instance, builds TSDuck without dependency
to `CppUnit`, `pcsc-lite`, `libcurl` and Dektec `DTAPI`:
~~~
make NOTEST=1 NOPCSC=1 NOCURL=1 NODTAPI=1
~~~

# Building the TSDuck installers {#buildinst}

There is no need to build the TSDuck binaries before building the installers.
Building the binaries, when necessary, is part of the installer build.

All installation packages are dropped into the subdirectory `installers`.
The packages are not deleted by the cleanup procedures. They are not pushed
into the git repository either.

## Windows {#instwindows}

Execute the PowerShell script `build\Build-Installer.ps1`.
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
`build\msvc2017\<target>-<platform>`. The commands can be run using their
complete path.

For instance, to run the released 64-bit version of `tsp`, use:
~~~~
D:\tsduck> build\msvc2017\Release-x64\tsp.exe --version
tsp: TSDuck - The MPEG Transport Stream Toolkit - version 3.12-730
~~~~

For other combinations (release vs. debug and 32 vs. 64 bits), the paths
from the repository root are:
~~~~
build\msvc2017\Release-x64\tsp.exe
build\msvc2017\Debug-x64\tsp.exe
build\msvc2017\Release-Win32\tsp.exe
build\msvc2017\Debug-Win32\tsp.exe
~~~~

## Linux and macOS {#rununix}

On all Unix systems, the binaries, plugins and tests are built in
subdirectories of their respective source directories. Specifically,
the tools and plugins are not in the same directory.

To run a tool from its build directory, a few environment variables
shall be defined (including `$PATH`). A shell-script named `setenv.sh`
is automatically created in each build directory. This script defines
the appropriate environment for running binaries which are in this
build directory.

Depending on your target (release vs. debug, 32 bits vs. 64 bits, Intel vs. ARM),
execute one of:
~~~~
source src/tstools/release-x86_64/setenv.sh
source src/tstools/debug-x86_64/setenv.sh
source src/tstools/release-i386/setenv.sh
source src/tstools/debug-i386/setenv.sh
source src/tstools/release-arm/setenv.sh
source src/tstools/debug-arm/setenv.sh
~~~~

Note the usage of the `source` command to make sure that the environment
variables are defined in the current shell.

Example:
~~~~
$ source src/tstools/release-x86_64/setenv.sh
$ which tsp
~/tsduck/src/tstools/release-x86_64/tsp
$ tsp --version
tsp: TSDuck - The MPEG Transport Stream Toolkit - version 3.12-730
~~~~

# Cleaning up {#buildcleanup}

On Windows, to cleanup a repository tree and return to a pristine source state,
execute the following PowerShell script:
~~~~
build\Cleanup.ps1
~~~~

On Linux and macOS, the same cleanup task is achieved using the following command:
~~~~
make distclean
~~~~
