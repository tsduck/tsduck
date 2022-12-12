## VATek devices interface  {#vatek}

Interfacing modulator devices based on VATek chips is visible through the
`vatek` output plugin and the `tsvatek` command. The output plugin can
be used indifferently from the commands `tsp` and `tsswitch`.

The `tsvatek` command is just a small tool to list all modulator devices based
on VATek chips in the system.

At application level, VATek USB devices are accessed through a user-mode API, the open-source
[VATek core library](https://github.com/VisionAdvanceTechnologyInc/vatek_sdk_2).
At a lower-level, this library accesses the USB devices using the open-source
[libusb library](https://github.com/libusb/libusb) on Linux and macOS, and the
[WinUSB generic driver](https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb)
on Windows. Consequently, there is no need for a specific device driver.
This peculiarity makes VATek devices the only available modulators on macOS.

The installation of the VATek core library depends on the operating system:

- On Windows, a binary installer for developers is
  [available on GitHub](https://github.com/VisionAdvanceTechnologyInc/vatek_sdk_2/releases).
  TSDuck uses the static library version of the VATek library. The VATek API
  is consequently included in the TSDuck shared library.

- On Linux, the VATek library is automatically downloaded from GitHub before
  the build. This is handled in the makefiles. See also the script `vatek-config.sh`
  in the `scripts` directory. When possible, a binary version is downloaded.
  Otherwise, the source version is downloaded and rebuilt.
  TSDuck uses the static library version of the VATek library.
  The VATek API is consequently included in the TSDuck shared library.

- On macOS, the VATek library is available from Homebrew.
  TSDuck uses the dynamic library version of the VATek library.
  Unlike Windows and Linux, the VATek library is consequently not included
  in the binary package for TSDuck.
  The Homebrew package for TSDuck places a dependency on the Homebrew package
  for the VATek library to enforce its installation before TSDuck.

When building TSDuck, it is possible to disable VATek support and remove the dependency
to the VATek library using `make NOVATEK=1` on Linux and macOS, or defining
the environment variable `TS_NO_VATEK` to any non-empty value on Windows.

Since, on some operating systems, the VATek library is statically linked
inside the TSDuck shared library, all VATek-related code for `tsvatek` and
the `vatek` plugin is built inside the TSDuck shared library.
The `tsvatek` command just calls a C++ class named `ts::VatekControl` which
is inside the TSDuck shared library. The `vatek` plugin is located inside
the TSDuck library and not a separated shared library (there are a dozen
plugins in that case for various technical reasons).

When VATek support is disabled, `tsvatek` and the `vatek` plugins are not
built, not installed and not present in the binary package.

See also:
- https://github.com/VisionAdvanceTechnologyInc/vatek_sdk_2
- https://www.vatek.com.tw/
