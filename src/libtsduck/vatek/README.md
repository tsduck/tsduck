## Vatek devices interface for TSDuck

Interfacing modulator devices based on Vatek chips is visible through the
`vatek` output plugin and the `tsvatek` command. The output plugin can
be used indifferently from the commands `tsp` and `tsswitch`.

The `tsvatek` command is just a small tool to list all modulator devices based
on Vatek chips in the system.

At application level, Vatek USB devices are accessed through a user-mode API, the open-source
[Vatek core library](https://github.com/VisionAdvanceTechnologyInc/vatek_sdk_2).
At a lower-level, this library accesses the USB devices using the open-source
[libusb library](https://github.com/libusb/libusb) on Linux and macOS, and the
[WinUSB generic driver](https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb)
on Windows. Consequently, there is no need for a specific device driver.
This peculiarity makes Vatek devices the only available modulators on macOS.

The installation of the Vatek core library depends on the operating system:

- On Windows, a binary installer for developers is
  [available on GitHub](https://github.com/VisionAdvanceTechnologyInc/vatek_sdk_2/releases).
  TSDuck uses the static library version of the Vatek library. The Vatek API
  is consequently included in the TSDuck shared library.

- On Linux, the Vatek library is automatically downloaded from GitHub before
  the build. This is handled in the makefiles. See also the script `vatek-config.sh`
  in the `scripts` directory. When possible, a binary version is downloaded.
  Otherwise, the source version is downloaded and rebuilt.
  TSDuck uses the static library version of the Vatek library.
  The Vatek API is consequently included in the TSDuck shared library.

- On macOS, the Vatek library is available from Homebrew. 
  TSDuck uses the dynamic library version of the Vatek library.
  Unlike Windows and Linux, the Vatek library is consequently not included
  in the binary package for TSDuck.
  The Homebrew package for TSDuck places a dependency on the Homebrew package
  for the Vatek library to enforce its installation before TSDuck.

When building TSDuck, it is possible to disable Vatek support and remove the dependency
to the Vatek library using `make NOVATEK=1` on Linux and macOS, or defining
the environment variable `TS_NO_VATEK` to any non-empty value on Windows.

Since, on some operating systems, the Vatek library is statically linked
inside the TSDuck shared library, all Vatek-related code for `tsvatek` and
the `vatek` plugin is built inside the TSDuck shared library.
The `tsvatek` command just calls a C++ class named `ts::VatekControl` which
is inside the TSDuck shared library. The `vatek` plugin is located inside
the TSDuck library and not a separated shared library (there are a dozen
plugins in that case for various technical reasons).

When Vatek support is disabled, `tsvatek` and the `vatek` plugins are not
built, not installed and not present in the binary package.

See also:
- https://github.com/VisionAdvanceTechnologyInc/vatek_sdk_2
- https://www.vatek.com.tw/
