# DSM-CC

This directory holds the DSM-CC (Digital Storage Media Command and
Control) data structures that aren't tables or descriptors, together
with the carousel-extraction engine used by the `dsmcc` TSP plugin.
The wire format is defined by MPEG in ISO/IEC 13818-6; DVB adds
extensions in ETSI EN 301 192, and ATSC adds more in A/90.

## What lives where

| Location | What |
| --- | --- |
| `libtsduck/dtv/tables/mpeg` | DSM-CC **sections and tables** defined by MPEG (DSI, DII, DDB — carried in `UserToNetworkMessage` / `DownloadDataMessage` envelopes). |
| `libtsduck/dtv/tables/{mpeg,dvb,atsc}` | **Descriptors** carried inside those tables, split by the standard that defines them (e.g. `compressed_module_descriptor` in MPEG). |
| `libtsduck/dtv/dsmcc` *(this directory)* | **Everything else**: BIOP messages, IOR / TaggedProfile / LiteComponent, the descriptors (`compatibility`, `resource`, `tap`) that live inside BIOP objects rather than in DVB SI, plus the **carousel** and **module-assembler** classes. |
| `src/tsplugins/tsplugin_dsmcc.cpp` | The `dsmcc` TSP plugin — a thin shell that wires a `SectionDemux` to `DSMCCCarousel` and writes the results to disk. |

Roadmap for what's still missing (stream events, auto-discovery,
carousel generation as input plugin, etc.) lives in
[ROADMAP.md](ROADMAP.md).

## High-level architecture

The carousel engine is a pure library — no demux, no filesystem, no
`tsp` dependency. The plugin is the only consumer today, but anything
that can supply parsed DSM-CC messages can drive it.

```
  +-----------------------------------------------------------------+
  |                   dsmcc TSP plugin  (thin)                      |
  |                                                                 |
  |   Runs a SectionDemux on the carousel PID, passes DSM-CC        |
  |   tables into the library, writes results to disk.              |
  |                                                                 |
  |   Options: --pid  --output-directory  --list                    |
  |            --dump-modules  --data-carousel                      |
  +--------------------------------+--------------------------------+
                                   |
                      feed DSI/DII/DDB, receive
                      module + object callbacks
                                   |
                                   v
  +-----------------------------------------------------------------+
  |                  DSMCCCarousel  (library facade)                |
  |                                                                 |
  |   +-------------------------+     +-------------------------+   |
  |   |  DSMCCModuleAssembler   | --> |   BIOP message parsers  |   |
  |   |                         |     |       +                 |   |
  |   |  - FSM over DSI/DII/DDB |     |   BIOPNameResolver      |   |
  |   |  - buffers orphan DDBs  |     |                         |   |
  |   |  - decompresses gzipped |     |  - absolute-path         |   |
  |   |    modules              |     |    resolution            |   |
  |   +-------------------------+     |  - deferred emission    |   |
  |                                   +-------------------------+   |
  |                                                                 |
  |   supporting structures: DSMCCIOR, DSMCCTaggedProfile,          |
  |   DSMCCLiteComponent, DSMCCTap,                                 |
  |   DSMCCCompatibilityDescriptor, DSMCCResourceDescriptor         |
  +--------------------------------+--------------------------------+
                                   |
                             built on top of
                                   |
                                   v
  +-----------------------------------------------------------------+
  |               DSM-CC tables and descriptors                     |
  |                                                                 |
  |   DSI / DII / DDB wrapped in UserToNetworkMessage and           |
  |   DownloadDataMessage; compressed_module_descriptor,            |
  |   stream_event_descriptor, etc.                                 |
  |                                                                 |
  |   Location: libtsduck/dtv/tables/{mpeg,dvb,atsc}                |
  +-----------------------------------------------------------------+
```

## Key classes

- **`DSMCCCarousel`** — library facade. Holds the assembler and name
  resolver, exposes two callbacks (module complete, BIOP object) and
  a `setScanBIOP()` switch that picks between object-carousel and
  data-carousel behaviour.
- **`DSMCCModuleAssembler`** — state machine over DSI/DII/DDB. Learns
  each module's size, block count, version and compression flag from
  the DII; gathers DDB blocks into a payload; decompresses when a
  `compressed_module_descriptor` is present. Buffers DDBs that arrive
  before their DII and replays them once the module is registered —
  this is what makes small service-gateway modules complete in
  finite-file captures.
- **`BIOPMessage`** and its subclasses — parsers for BIOP
  `ServiceGateway`, `Directory`, `File`, with `Stream` / `StreamEvent`
  still partial (see [ROADMAP.md](ROADMAP.md)).
- **`BIOPNameResolver`** — walks the name bindings in SRG and
  Directory messages and emits each child object with its absolute
  path. Objects whose parent hasn't been parsed yet stay deferred
  until the parent shows up; `flushPendingObjects()` drains whatever
  remains at stop.
- **`DSMCCIOR`**, **`DSMCCTaggedProfile`**, **`DSMCCLiteComponent`**,
  **`DSMCCTap`**, **`DSMCCCompatibilityDescriptor`**,
  **`DSMCCResourceDescriptor`** — CORBA IOR and related structures
  embedded in BIOP messages. They encode cross-object references and
  the metadata needed to locate a target (PIDs, component tags,
  resource requirements).

Every XML-serialisable class has a matching `.xml` template next to
its `.h` / `.cpp`, following the usual TSDuck convention.

## Two carousel modes

- **Object carousel** (default) — `setScanBIOP(true)`. The library
  pathrses each completed module as BIOP, resolves absolute paths from
  ServiceGateway / Directory bindings, and fires the object callback
  per BIOP message. The plugin writes resolved `File` objects under
  `<output-directory>/files/<path>`. Path segments equal to `..` or
  `.` are rejected so a crafted carousel cannot escape the output
  directory.
- **Data carousel** — `setScanBIOP(false)`, enabled in the plugin via
  `--data-carousel`. Module payloads are opaque; only the module
  callback fires. The plugin writes each module flat as
  `<output-directory>/module_XXXX.bin`. Intended for DVB-SSU (ETSI TS
  102 006) and other streams where module contents are private.

