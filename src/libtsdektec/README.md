## Dektec devices interface  {#dektec}

Interfacing Dektec devices is visible through the `dektec` input plugin, the
`dektec` output plugin and the `tsdektec` command. The input/output plugins
can be used indifferently from the commands `tsp` and `tsswitch`.

The `tsdektec` command is just a small tool to list Dektec devices in the system
and set a few permanent options (input/output mode of bidirectional ports,
power-mode of USB modulators, light LED's on PCI cards to identify which is which).

Dektec devices can be used only where DTAPI and drivers are available: Windows and
Linux (Intel CPU's only). Alpine Linux is not supported because it uses the `musl`
libc instead of `glibc` (DTAPI references symbols from `glibc`).

### TSDuck code structure

Most of the TSDuck code is in a few shared libraries (`.so` on Linux, `.dylib` on
macOS, `.dll` on Windows). The idea is that everything can be used by applications,
including third-party applications, as C++ classes. The TSDuck commands are small
executable files. They are small wrappers on top of the library.

Most TSDuck plugins are shared libraries, named as `tsplugin_foo.so`, or `.dylib`,
or `.dll`, for plugin named `foo`. The idea is that users may develop their own
plugins and use them without rebuilding TSDuck. A few plugins are embedded inside
`libtsduck.so`. Most of them are very basic.

The common shared libraries are:

- `libtscore`: Generic C++ features, not specifically for Digital TV.
- `libtsduck`: Digital TV features (most of the TSDuck code in practice).
- `libtsdektec`: Specific C++ classes for Dektec devices.

The Dektec devices are accessed through a specific userland library called DTAPI.
This library is provided for free by Dektec in binary format. The source code
is proprietary and not publicly available. The programming documentation for DTAPI
is exhaustive and precise.

On Linux, DTAPI is provided as one big pre-linked object file (`.o`). On Windows,
DTAPI is provided as one static library (`.lib`). The only supported architectures
are Intel x86 and x64. No binary for other architectures are available.

On Windows, DTAPI was compiled without dllexport directives. Therefore, it is
not possible to make a DTAPI DLL. DTAPI must be statically linked with the code
which directly references it. Because TSDuck is split in many shared libraries
and executables, all code which uses DTAPI is grouped into one single shared
library, `libtsdektec`. The DTAPI code is also embedded into that shared
library and is not accessible to external applications (at least on Windows
because of the lack of dllexport directives in DTAPI code).

The C++ classes for the two `dektec` plugins, input and output, are in `libtsdektec`.
The shared library `tsplugin_dektec` does nothing but registering these C++
classes as plugins. Similarly, the `tsdektec` command just calls a C++ class
named `ts::DektecControl` which is inside `libtsdektec`.

The shared library `tsplugin_dektec` and the executable `tsdektec` are the
only binaries which reference `libtsdektec`. These three components form the Dektec
subsystem of TSDuck. The shared library `libtsdektec` is the only component in
TSDuck which contains proprietary code (the DTAPI library). If, for some reason,
it is not possible to include proprietary code in some distribution of TSDuck,
it is possible to remove `libtsdektec`, `tsplugin_dektec`, and `tsdektec`, and ship
them separately, through more permissive channels.

The TSDuck programming environment, which is used by third-party applications
for Digital TV, has interfaces for `libtscore` and `libtsduck`, but not for
`libtsdektec`. This shared library is specific to `tsplugin_dektec` and `tsdektec`.

### Installation prerequisites

On Windows, the Dektec SDK and drivers shall be installed first. This is automated
in the PowerShell script `install-dektec.ps1` in the `scripts/install-wintools`
directory. It is automatically invoked by `scripts/install-prerequisites.ps1`.

On Linux, the DTAPI is automatically downloaded from https://dektec.com before
the build. This is handled in the makefiles. See also the script `dtapi-config.sh`
in the `scripts` directory. It is possible to disable Dektec support and remove
the dependency to DTAPI using `make NODEKTEC=1`.

On macOS or unsupported Linux platforms (Alpine, Arm processors), TSDuck is
automatically compiled with `NODEKTEC` set.

When Dektec support is disabled, `tsdektec` and the `dektec` plugins are not
built, not installed and not present in the binary package.

See also:
- https://www.dektec.com/downloads/SDK
