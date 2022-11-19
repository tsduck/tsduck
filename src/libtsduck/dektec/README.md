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

Most of the TSDuck code is in a large shared library named `libtsduck.so` (Linux),
`libtsduck.dylib` (macOS) or `tsduck.dll` (Windows). The idea is that everything
can be used by applications, including third-party applications, as C++ classes.
The TSDuck commands are small executable files. They are small wrappers on top of
the library.

Most TSDuck plugins are shared libraries, named as `tsplugin_foo.so` or `.dll` for
plugin named `foo`. The idea is that users may develop their own plugins and use
them without rebuilding TSDuck. A few plugins are embedded inside `libtsduck.so`.
Most of them are very basic.

The `dektec` plugins are the exception here: they are also embedded inside the
TSDuck shared library for technical reasons. On Windows, DTAPI is now only
available as a static library, not a DLL. This code can only be statically
linked with the code which uses it. A long time ago, there was a DTAPI.DLL and
the `dektec` plugins were in their own DLL, just like the other plugins. The
plugins, `tsp` and `tsdektec` all referenced DTAPI.DLL. When DTAPI.DLL was dropped,
the whole thing had to be redesigned. We could not build a DTAPI.DLL on our own
because the DTAPI was compiled without dllexport directives. Now, all code
using DTAPI is inside tsduck.dll, including the `dektec` plugins. The `tsdektec`
command just calls a C++ class named `ts::DektecControl` which is inside tsduck.dll.

Since there is one single code base for all OS, Linux also uses the same structure
(although it is still technically possible to build a libdtapi.so on Linux and use it).

On Windows, the Dektec SDK and drivers shall be installed first. This is automated
in the PowerShell script `install-dektec.ps1` in the `scripts` directory.

On Linux, the DTAPI is automatically downloaded from https://dektec.com before
the build. This is handled in the makefiles. See also the script `dtapi-config.sh`
in the `scripts` directory. It is possible to disable Dektec support and remove
the dependency to DTAPI using `make NODEKTEC=1`.

On macOS or unsupported Linux platforms (Alpine, Arm processors), TSDuck is
automatically compiled with `NODEKTEC` set.

When Dektec support is disabled, `tsdektec` and the `dektec` plugins are not
built, not installed and not present in the binary package.

Even when Dektec support is disabled, the Dektec-related C++ classes exist in
the TSDuck shared library so that applications using these classes cab be built.
However, any operation will fail with an error message.

See also:
- https://tsduck.io/download/dektec
- https://github.com/tsduck/dektec-dkms
- https://www.dektec.com/downloads/SDK
