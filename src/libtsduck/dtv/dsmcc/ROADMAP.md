# DSM-CC roadmap

Running list of things I want to get to for the DSM-CC code here and
the `dsmcc` plugin. Grouped by topic, not ordered by priority.

## Finding the carousel

Right now you have to pass `--pid` by hand. It'd be much nicer to get
there from the signalling:

- [ ] Read the AIT, find the `transport_protocol_descriptor` (protocol_id
  0x0001 for object carousels, 0x0002 for data carousels), grab the
  `component_tag`, then match it against `stream_identifier_descriptor`
  in the PMT to get the PID.
- [ ] If the user already knows the component_tag, short-circuit
  straight to the PMT lookup.
- [ ] `--service`, `--application-id`, and `--component-tag` as
  alternatives to `--pid`.
- [ ] Multi-carousel support: streams carrying several carousels on the
  same PID (different transaction_id groups) should be tracked
  independently and addressable by carousel id.
- [ ] **Survey mode** (`--list-carousels` or similar) that scans a TS
  and reports every PID carrying DSM-CC — transaction_ids, versions,
  module counts, compressed/uncompressed sizes.

## Carousel bootstrap

- [x] **Use the DSI IOR.** The DownloadServerInitiate's IOR now
  pre-seeds the ServiceGateway location in the name resolver via
  `DSMCCCarousel::bootstrapFromDSI`; the scan-based detection stays as
  a fallback.

## Knowing what the carousel is doing

- [ ] **Version tracking.** The upper bits of `transaction_id` encode a
  version. Bumps happen when the broadcaster updates the carousel.
  Notice, log it, and let the user pick a policy — re-extract, diff
  against last time, or just report. Current version should show up in
  the plugin's status.
- [ ] **Completeness stats.** Per module: blocks received vs. expected,
  duplicates, bytes/sec, time to complete. Per carousel: how many
  modules are done, overall %.
- [ ] **Stall detection.** If a module sits half-finished past some
  timeout, say so. Optionally bail out.
- [ ] **Cycle time.** Measure how often each module (and the DSI/DII)
  repeats. Needed for tuning the input-plugin side.
- [ ] **Telemetry.** Push stats out via InfluxDB (the `influx` pattern),
  JSON events, etc.

## Extracting less than everything

- [ ] **`dvb://` URLs.** DVB defines `dvb://<onid>.<tsid>.<sid>/<path>`
  (ETSI TS 102 851). Hand the plugin one of these and have it do the
  whole AIT→PMT→PID→file resolution.
- [ ] **Include/exclude globs.** `--include '*.html'`, `--exclude
  'debug/**'`.
- [ ] **By object kind.** `--kind` filter to select Files only, or
  StreamEvents only, etc.

## BIOP gaps

- [ ] **`BIOP::Stream` and `BIOP::StreamEvent`.** Event descriptors
  need to be parsed and exposed; "do-it-now" events on a separate PID
  need to be demuxed and delivered as callbacks. HbbTV apps lean on
  this for broadcast sync.
- [x] **TaggedProfile: `BIOPProfileBody`.**
- [x] **TaggedProfile: `LiteOptionsProfileBody`** (opaque bytes
  exposed).
- [ ] **TaggedProfile: `ServiceLocation`.** Rare but real.
- [ ] **TaggedProfile: `DeferredAssociationTag` chains** — object
  references that cross PIDs.
- [ ] **`BIOP::UserMessage` / user-private modules.** Expose the bytes
  without interpretation, for callers like OpenTV.

## Data carousel gaps

`--data-carousel` today dumps assembled modules and stops there.
Enough to pull bytes off a DVB-SSU stream, but it drops most of the
structure the spec gives us:

- [ ] **`GroupInfoIndication` parsing.** DSI's `private_data` in
  data-carousel mode carries a `GroupInfoIndication` listing the
  carousel's groups (not an IOR). The UNM table parser currently
  always reads it as an IOR; proper dispatch by carousel type is
  needed.
- [ ] **`SuperGroupInfoIndication`** for the (rarer) case where groups
  are themselves grouped.
- [ ] **Group hierarchy.** Track groups as first-class citizens; expose
  `carousel → group → module` instead of the flat module list.
- [ ] **Group-scoped module ids.** Today modules are keyed purely by
  `module_id`; a multi-group carousel requires `(group_id, module_id)`.
- [ ] **Wire module/group/carousel descriptors into the extractor.**
  The descriptor classes already exist under but `DSMCCExtractor`
  only looks up `compressed_module_descriptor`. Extend the DII
  handling to surface the rest.
- [ ] **Per-group completeness stats** alongside the per-module ones.
- [ ] **Output layout.** Mirror the group hierarchy on disk and use
  `label_descriptor` contents when present, so data-carousel output
  is self-describing.

## Output

- [ ] **XML/JSON dump of the tree.** Serialise the resolved object
  graph (ServiceGateway, Directories, Files, plus Stream/StreamEvent
  metadata). XML for the TSDuck convention, JSON for tooling
  pipelines.
- [ ] **Manifest alongside `files/`.** Per-extraction description:
  path, size, module id, compressed/uncompressed sizes, carousel
  version.

## Other profiles

DSM-CC shows up in more places than European DVB. Current scope:

- [x] **MHP / GEM over DVB.** Primary reference: ETSI TR 101 202.
- [ ] **ATSC A/90** — different semantics on DSM-CC sections.
- [ ] **ARIB B.23** (Japan) — Japan-specific tweaks on top.

## Diagnostics

- [ ] `--raw-modules` flag to dump post-assembly, pre-BIOP module
  bytes.
- [ ] Hex trace of BIOP messages behind a verbose flag.
- [ ] Coverage report scoped to the dsmcc subdirectory.

## The other direction: carousel as an input plugin

A `dsmcc` input/packet plugin that generates a carousel from a
directory and injects it into a TS:

- [ ] Walk a directory, build BIOP ServiceGateway / Directory / File
  messages, pack into modules.
- [ ] Emit DSI/DII/DDB on a chosen PID at a chosen cycle rate and
  block size.
- [ ] Optional gzip via `compressed_module_descriptor`.
- [ ] Inject the signalling a receiver needs to discover the thing:
  PMT entry with a `stream_identifier_descriptor`, AIT with the full
  application / transport_protocol / location descriptor set (HbbTV
  uses `simple_application_location_descriptor`, MHP uses
  `dvb_j_application_*`), SDT updates for new services.
- [ ] Piggyback on the injection patterns used by `inject` and
  `spliceinject` for merging PSI/SI.
- [ ] Round-trip test: generate carousel, pipe through `tsp`, extract
  with `dsmcc`, diff against the source directory.

## Fitting in with the rest of TSDuck

- [x] **`tsdsmcc` standalone tool** — shares code with the plugin via
  `DSMCCExtractor`, saves the `tsp -I file ... -P dsmcc ... -O drop`
  boilerplate.
- [ ] **`tsanalyze` / `analyze` integration.** Carousel stats
  (modules, version, completion, cycle time) belong in the standard
  analysis report too.

## Docs and test material

- [ ] Public-domain capture for each scenario we claim to support:
  compressed, uncompressed, multi-module, deferred tags, stream
  events.
- [ ] A cookbook in the user guide: extract an HbbTV app, look at a
  DVB-SSU carousel, verify a carousel you just built, watch version
  bumps on a live stream.
