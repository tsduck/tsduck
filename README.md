## TSDuck - The MPEG Transport Stream Toolkit

### Abstract

[TSDuck](https://tsduck.io/) is the free and open-source reference framework for MPEG transport streams.

TSDuck is used in digital television systems for test, monitoring, integration, debug, lab or demo.

In practice, TSDuck is used for:

- Transport stream acquisition or transmodulation, including DVB, ATSC, ISDB, ASI and IP multicast.
- Analyze transport streams, PSI/SI signalization, bitrates, timestamps.
- On-the-fly transformation or injection of content and signalization.
  - Manipulation of tables and descriptors using XML, JSON or binary formats.
  - Most standard tables and descriptors are supported, as defined by MPEG, DVB, ISDB, ATSC, SCTE.
- Modify, remove, rename, extract services.
- Analyze and inject SCTE 35 splice information.
- Extract or inject Multi-Protocol Encapsulation (MPE) between TS and UDP/IP.
- Generate Electronic Program Guide (EPG), inject EIT according to ETSI TS 101 211.
- Monitor and report conditions on the stream (video and audio properties, bitrates, crypto-periods, signalization).
- Send bitrate and ETSI TR 101 290 metrics to InfluxDB and Grafana for system monitoring.
- Monitor Inter-packet Arrival Time (IAT) on datagram-based networks.
- Work on live transport streams, DVB-S/C/T, ATSC, ISDB-S/T, ASI, UDP ("IP-TV"), HTTP, HLS, SRT, RIST or
  offline transport stream files and `pcap` network capture files.
- Receive from or send to specialized hardware such as:
  - Cheap DVB, ATSC or ISDB tuners (USB, PCI).
  - Professional [Dektec](https://www.dektec.com) devices, ASI, modulators (USB, PCI).
  - [HiDes](http://www.hides.com.tw/product_cg74469_eng.html) modulators (USB).
  - [VATek](https://www.vatek.com.tw/A%20series/)-based modulators (USB) such as the
    [Suntechtv U3](https://www.suntechtv.com/web/Home/ProductDetail?key=e593s&productId=23673).
- Re-route transport streams to other applications.
- Extract specific encapsulated data (Teletext, T2-MI).
- Emulate a CAS head-end using DVB SimulCrypt interfaces to and from ECMG or EMMG.
- And more...

TSDuck is developed in C++ in a modular architecture. It is easy to extend
through plugins.

TSDuck is simple; it is a collection of command line tools and plugins. There is
no sophisticated GUI. Each utility or plugin performs only one elementary feature
but they can be combined in any order.

Through `tsp`, the Transport Stream Processor, many types of analysis and
transformation can be applied on live or recorded transport streams.
This utility can be extended through plugins. Existing plugins can be
enhanced and new plugins can be developed using a library of C++ classes.

### Usage

TSDuck comes with a comprehensive [User Guide](https://tsduck.io/docs/tsduck.html).

All utilities and plugins accept the option `--help` to display their syntax.

TSDuck is developed in C++, using modern C++20 coding practices.
For programmers, TSDuck provides a large collection of C++ classes in one single library.
These classes manipulate, in a completely portable way, MPEG transport streams, MPEG/DVB/ATSC/ISDB
signalization and many other features.
See the [Developer Guide](https://tsduck.io/docs/tsduck-dev.html)
and the [Programming Reference](https://tsduck.io/doxy/).

Python and Java bindings exist to allow running transport stream processing pipelines from
Python or Java applications.

### Building

TSDuck can be built on Windows, Linux, macOS and BSD systems.
The primary target architectures are Intel x86_64 and Arm64 but
TSDuck is regularly built and tested on x86, Arm32, RISC-V, PowerPC or IBM s390x.
See the [building section](https://tsduck.io/docs/tsduck-dev.html#building)
in the developer guide for more details.

### Download

- On macOS, [use the Homebrew packager](https://tsduck.io/docs/tsduck-dev.html#macinstall): `brew install tsduck`
- On Windows, [use winget](https://learn.microsoft.com/en-us/windows/package-manager/winget/): `winget install tsduck`

Pre-built [binary packages](https://github.com/tsduck/tsduck/releases) are available
for Windows and the very latest versions of some Linux distros (Fedora, RedHat and clones, Ubuntu, Debian),
on Intel x64 and Arm64 architectures.

The latest developments can be tested using [nightly builds](https://tsduck.io/prerelease/).

The command `tsversion --check` can be used to check if a new version of TSDuck is available
online. The command `tsversion --upgrade` downloads the latest binaries for the current
operating system and upgrades TSDuck.

### Project resources

TSDuck is maintained by one single developer on spare time and on personal expenses.
You may consider [contributing](https://tsduck.io/donate/) to the hardware and Web hosting costs
using [PayPal](https://tsduck.io/donate/)

### License

TSDuck is distributed under the terms of the Simplified 2-Clause BSD License.
See the file `LICENSE.txt` for details.

*Copyright (c) 2005-2025, Thierry Lelegard*<br/>
*All rights reserved*
