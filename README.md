## TSDuck - The MPEG Transport Stream Toolkit

### Abstract

[TSDuck](https://tsduck.io/) is an extensible toolkit for MPEG transport streams.

TSDuck is used in digital television systems for test, monitoring, integration, debug, lab or demo.

In practice, TSDuck is used for:

- Transport stream acquisition or transmodulation, including DVB, ATSC, ISDB, ASI and IP multicast.
- Analyze transport streams, PSI/SI signalization, bitrates, timestamps.
- Monitor and report conditions on the stream (video and audio properties, bitrates, crypto-periods, signalization).
- On-the-fly transformation or injection of content and signalization.
- Modify, remove, rename, extract services.
- Work on live transport streams, DVB-S/C/T, ATSC, ISDB-T, ASI, UDP ("IP-TV"), HTTP, HLS, SRT, RIST or offline transport stream files.
- Use specialized hardware such as cheap DVB, ATSC or ISDB tuners (USB, PCI), professional Dektec devices, cheap HiDes modulators, VATek-based modulators (e.g. Suntechtv U3, USB).
- Re-route transport streams to other applications.
- Extract or inject Multi-Protocol Encapsulation (MPE) between TS and UDP/IP.
- Analyze and inject SCTE 35 splice information.
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

TSDuck comes with a comprehensive [User's Guide](https://tsduck.io/download/docs/tsduck.pdf).

All utilities and plugins accept the option `--help` to display their syntax.

For programmers, TSDuck provides a large collection of C++ classes in one single library.
These classes manipulate, in a completely portable way, MPEG transport streams, MPEG/DVB/ATSC/ISDB
signalization and many other features. See the [programming guide](https://tsduck.io/doxy/)
and its [tutorial](https://tsduck.io/doxy/libtutorial.html).

Python and Java bindings exist to allow running transport stream processing pipelines from
Python or Java applications.

### Building

TSDuck can be built on Windows, Linux, macOS and BSD systems (FreeBSD, OpenBSD, NetBSD, DragonFlyBSD).
See the [building guide](https://tsduck.io/doxy/building.html) for details.

### Download

Pre-built [binary packages](https://github.com/tsduck/tsduck/releases) are available
for Windows and the very latest versions of some Linux distros
(Fedora, RedHat, CentOS, AlmaLinux, Ubuntu, Debian, Raspbian).
On macOS, [use the Homebrew packager](https://tsduck.io/doxy/installing.html#macinstall).

The latest developments can be tested using [nightly builds](https://tsduck.io/download/prerelease/).

The command `tsversion --check` can be used to check if a new version of TSDuck is available
online. The command `tsversion --upgrade` downloads the latest binaries for the current
operating system and upgrades TSDuck.

### Project resources

TSDuck is maintained by one single developer on spare time and on personal expenses.
You may consider [contributing](https://tsduck.io/donate/) to the hardware and Web hosting costs
using [![Donate](https://tsduck.io/images/donate-paypal.svg)](https://tsduck.io/donate/)

### License

TSDuck is distributed under the terms of the Simplified BSD License.
See the file `LICENSE.txt` for details.

*Copyright (c) 2005-2023, Thierry Lelegard*<br/>
*All rights reserved*
