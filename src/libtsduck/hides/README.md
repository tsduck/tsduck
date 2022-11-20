## HiDes devices interface  {#hides}

Interfacing HiDes modulator devices is visible through the `hides` output plugin
and the `tshides` command.

HiDes devices can be used only where HiDes drivers are available: Windows and Linux.

The drivers are provided by ITE, the vendor of the main chip in HiDes modulators.
The drivers for Linux and Windows are very different, there is no common structure.
There is no stable common userland API above the drivers. TSDuck directly interfaces
the ITE drivers.

To enforce a system-independent interface, the C++ class `ts::HiDesDevice` provides
a uniform access to HiDes devices. The subdirectories `linux`, `windows` and `mac`
contain the system-specific variants of this class. Note that the macOS implementation
simply returns errors and error messages.

See also:
- https://tsduck.io/download/hides
- https://github.com/tsduck/hides-drivers
