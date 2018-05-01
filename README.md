## TSDuck - The MPEG Transport Stream Toolkit

### Abstract

[TSDuck](https://tsduck.github.io/) is an extensible toolkit for MPEG/DVB transport streams.

TSDuck is used in digital television systems for test, monitoring, integration, debug, lab or demo.

In practice, TSDuck is used for:

- Transport stream acquisition or transmodulation, including DVB, ASI and IP multicast.
- Analyze transport streams, PSI/SI signalization, bitrates, timestamps.
- Monitor and report conditions on the stream (video and audio properties, bitrates, crypto-periods, signalization).
- On-the-fly transformation or injection of content and signalization.
- Modify, remove, rename, extract services.
- Work on live transport streams, DVB-S/C/T, ASI, IP-TV or offline transport stream files.
- Use specialized hardware such as cheap DVB tuners (USB, PCI) or professional Dektec devices.
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

TSDuck comes with a comprehensive [User's Guide](https://github.com/tsduck/tsduck/raw/master/doc/tsduck.pdf).

All utilities and plugins accept the options `--help` and `--version`.
Using `--help`, each utility displays its syntax and exits.

For programmers, TSDuck provides a large collection of C++ classes in one single library.
These classes manipulate, in a completely portable way, MPEG transport streams, MPEG/DVB
signalization and many other features. See the
[programming guide](https://tsduck.github.io/doxy/html/)
and its [tutorial](https://tsduck.github.io/doxy/html/libtutorial.html).

### Building

TSDuck can be built on Windows, Linux and macOS systems. See the
[building guide](https://tsduck.github.io/doxy/html/building.html) for details.

### Download

Pre-built [binary packages](https://github.com/tsduck/tsduck/releases)
are available for Windows, Fedora and Ubuntu. On macOS,
[use the Homebrew packager](https://github.com/tsduck/homebrew-tsduck/blob/master/README.md).

### License

TSDuck is distributed under the terms of the Simplified BSD License.
See the file `LICENSE.txt` for details.

*Copyright (c) 2005-2018, Thierry Lelegard*<br/>
*All rights reserved*
