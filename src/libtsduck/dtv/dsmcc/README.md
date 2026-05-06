# DSM-CC

This directory holds the DSM-CC (Digital Storage Media Command and
Control) data structures that aren't tables or descriptors, together
with the carousel-extraction engine shared by the `dsmcc` TSP plugin
and the `tsdsmcc` standalone command. The wire format is defined by
MPEG in ISO/IEC 13818-6; DVB adds extensions in ETSI EN 301 192, and
ATSC adds more in A/90.

## What lives where

| Location | What |
| --- | --- |
| `libtsduck/dtv/tables/mpeg` | DSM-CC **sections and tables** defined by MPEG (DSI, DII, DDB — carried in `UserToNetworkMessage` / `DownloadDataMessage` envelopes). |
| `libtsduck/dtv/tables/{mpeg,dvb,atsc}` | **Descriptors** carried inside those tables, split by the standard that defines them (e.g. `compressed_module_descriptor` in MPEG). |
| `libtsduck/dtv/dsmcc` *(this directory)* | **Everything else**: BIOP messages, IOR / TaggedProfile / LiteComponent, the descriptors (`compatibility`, `resource`, `tap`) that live inside BIOP objects rather than in DVB SI, plus the **carousel**, **module-assembler** and **extractor** classes. |
| `src/tsplugins/tsplugin_dsmcc.cpp` | The `dsmcc` TSP plugin — a thin shell that wires a `SectionDemux` to `DSMCCExtractor` and forwards TS packets. |
| `src/tstools/tsdsmcc.cpp` | The `tsdsmcc` command — a one-shot equivalent of `tsp -P dsmcc`, sharing the same library engine and option set. |

Roadmap for what's still missing (stream events, auto-discovery,
carousel generation as input plugin, etc.) lives in
[ROADMAP.md](ROADMAP.md).

## High-level architecture

The carousel engine is a pure library — no `tsp` dependency below the
extractor layer. `DSMCCExtractor` owns the section demux + on-disk
output policy; `DSMCCCarousel` is purely message-driven and can be
driven by anything that can supply parsed DSM-CC messages.

```
  +-----------------------------------------------------------------+
  |  dsmcc TSP plugin  (thin)        |   tsdsmcc command  (thin)    |
  |                                                                 |
  |  Both wire raw TS packets to the same DSMCCExtractor and share  |
  |  one option set (--pid, --output-directory, --dump-modules,     |
  |  --data-carousel) via DSMCCExtractorArgs.                       |
  +--------------------------------+--------------------------------+
                                   |
                          feed TS packets
                                   |
                                   v
  +-----------------------------------------------------------------+
  |                   DSMCCExtractor  (library driver)              |
  |                                                                 |
  |   - Holds the SectionDemux, the carousel engine and the output  |
  |     policy. Renders the list-mode hierarchical report.          |
  +--------------------------------+--------------------------------+
                                   |
                      feed DSI/DII/DDB, receive
                      module / object / group callbacks
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
  |   |  - decompresses gzipped |     |  - absolute-path        |   |
  |   |    modules              |     |    resolution           |   |
  |   +-------------------------+     |  - deferred emission    |   |
  |                                   +-------------------------+   |
  |                                                                 |
  |   per-(download_id) GroupContext bookkeeping; supporting        |
  |   structures: DSMCCIOR, DSMCCTaggedProfile, DSMCCLiteComponent, |
  |   DSMCCTap, DSMCCCompatibilityDescriptor, DSMCCResourceDescriptor|
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

- **`DSMCCExtractor`** — library-level driver. Owns the section demux
  and on-disk output policy; renders the list-mode hierarchical report.
  Shared between the `dsmcc` plugin and the `tsdsmcc` command via
  `DSMCCExtractorArgs`.
- **`DSMCCCarousel`** — library facade. Holds the assembler and name
  resolver, exposes module / object / group-completion callbacks and
  a `setScanBIOP()` switch that picks between object-carousel and
  data-carousel behaviour. Tracks each `download_id` as a first-class
  `GroupContext` (announced by the DSI's `GroupInfoIndication` or
  synthesized from the first DII).
- **`DSMCCModuleAssembler`** — state machine over DSI/DII/DDB, keys
  modules by `(download_id, module_id)`. Learns each module's size,
  block count, version and compression flag from the DII; gathers DDB
  blocks into a payload; decompresses when a
  `compressed_module_descriptor` is present. Buffers DDBs that arrive
  before their DII and replays them once the module is registered —
  this is what makes small service-gateway modules complete in
  finite-file captures.
- **`BIOPMessage`** and its subclasses — parsers for BIOP `File` and
  `BindingList` messages (used for both `Directory` and
  `ServiceGateway`), with `Stream` / `StreamEvent` still partial (see
  [ROADMAP.md](ROADMAP.md)).
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
  parses each completed module as BIOP, resolves absolute paths from
  ServiceGateway / Directory bindings, and fires the object callback
  per BIOP message. The plugin writes resolved `File` objects under
  `<output-directory>/files/<path>` and materialises empty
  `Directory` objects as on-disk directories under the same root.
  Path segments equal to `..` or `.` are rejected so a crafted
  carousel cannot escape the output directory.
- **Data carousel** — `setScanBIOP(false)`, enabled in the plugin via
  `--data-carousel`. Module payloads are opaque; only the module
  callback fires. The plugin writes each module under
  `<output-directory>/<download_id>/<label_or_module_XXXX>.bin`,
  mirroring the carousel's group hierarchy on disk. Intended for
  DVB-SSU (ETSI TS 102 006) and other streams where module contents
  are private.

When `--output-directory` is omitted, the plugin runs in list-only
mode and prints a hierarchical, `tstables`-style report: one `*`
block per carousel/`download_id`, modules as `- Module 0xNNNN`
sub-blocks (version, blocks, size, compression, status, BIOP-object
list with kinds, per-DII descriptor decode), then a final
`* Carousel Tree` section showing the resolved file/directory tree
with byte sizes.

`--dump-modules` (object-carousel mode) writes raw assembled module
payloads to `<output-directory>/modules/<download_id>/module_XXXX.bin`,
nested under the download_id so multi-group streams with overlapping
`module_id`s do not collide.

