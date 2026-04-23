# DSM-CC roadmap

Running list of things I want to get to for the DSM-CC code here and
the `dsmcc` plugin. Grouped by topic, not ordered by priority.

## Finding the carousel

Right now you have to pass `--pid` by hand. It'd be much nicer to get
there from the signalling:

- Read the AIT, find the `transport_protocol_descriptor` (protocol_id
  0x0001 for object carousels, 0x0002 for data carousels), grab the
  `component_tag`, then match it against `stream_identifier_descriptor`
  in the PMT to get the PID.
- If the user already knows the component_tag, short-circuit straight
  to the PMT lookup.
- Probably want `--service`, `--application-id`, and `--component-tag`
  as alternatives to `--pid`.

Separate but related: some streams carry several carousels on the same
PID (different transaction_id groups). Those should be tracked
independently and addressable by carousel id.

A **survey mode** (`--list-carousels` or similar) that scans a TS and
reports every PID carrying DSM-CC — transaction_ids, versions, module
counts, compressed/uncompressed sizes — would save a lot of "grep the
PMT, guess the PID" cycles before the real extraction run.

## Carousel bootstrap

- **Use the DSI IOR.** The DownloadServerInitiate carries an IOR that
  names the ServiceGateway module, but the carousel engine ignores it
  and rediscovers the SRG by scanning every completed module for a
  `srg` kind tag. Works in practice; the cost is that we BIOP-parse
  modules we already know aren't the gateway, and we throw away the
  carousel_id / compatibility_descriptor that DSI also carries. Hook
  DSI into `feedUserToNetwork` so the SRG location is known up front.

## Knowing what the carousel is doing

- **Version tracking.** The upper bits of `transaction_id` encode a
  version. Bumps happen when the broadcaster updates the carousel. We
  should notice, log it, and let the user pick a policy — re-extract,
  diff against last time, or just report. Current version should show
  up in the plugin's status.
- **Completeness stats.** Per module: blocks received vs. expected,
  duplicates, bytes/sec, time to complete. Per carousel: how many
  modules are done, overall %. Mostly useful when you're staring at a
  lossy capture wondering if it's your code or the input.
- **Stall detection.** If a module sits half-finished past some
  timeout, say so. Optionally bail out.
- **Cycle time.** Measure how often each module (and the DSI/DII)
  repeats. Receivers care about this — carousels that cycle too slowly
  don't boot reliably — and the measurement is also what you need if
  you want to tune the input-plugin side (see below).
- **Telemetry.** Once we have these numbers, push them out the same
  way other plugins do (InfluxDB via the `influx` pattern, JSON
  events, whatever lands first).

## Extracting less than everything

- **`dvb://` URLs.** DVB defines
  `dvb://<onid>.<tsid>.<sid>/<path>` (ETSI TS 102 851). Should be
  possible to hand the plugin one of these and have it do the whole
  AIT→PMT→PID→file resolution. If `--pid` is given explicitly, accept
  a plain path instead.
- **Include/exclude globs.** `--include '*.html'`, `--exclude
  'debug/**'` — the obvious thing. Big carousels, partial mirroring.
- **By object kind.** Sometimes you only want Files, sometimes only
  StreamEvents. A `--kind` filter.

## BIOP gaps

- `BIOP::Stream` and `BIOP::StreamEvent` are the big missing ones.
  Event descriptors need to be parsed and exposed, and the "do-it-now"
  events carried as DSM-CC sections on a separate PID need to be
  demuxed and delivered as callbacks. HbbTV apps lean on this for
  broadcast sync.
- TaggedProfile: we handle `BIOPProfileBody` and the lite options
  form. `ServiceLocation` is rare but real. `DeferredAssociationTag`
  chains (object references that cross PIDs) don't resolve yet.
- `BIOP::UserMessage` / user-private modules — don't try to interpret
  them, just expose the bytes for callers that do (OpenTV, mainly).

## Data carousel gaps

`--data-carousel` today dumps assembled modules and stops there.
Enough to pull bytes off a DVB-SSU stream, but it drops most of the
structure the spec gives us:

- **Group hierarchy.** The DSI's `private_data` in data-carousel mode
  carries a `GroupInfoIndication` listing the carousel's groups; each
  DII then describes one group. Parse it, track groups as first-class
  citizens, and expose `carousel → group → module` instead of the
  flat module list we have now.
- **SuperGroupInfoIndication** for the (rarer) case where groups are
  themselves grouped.
- **Group-scoped module ids.** Today modules are keyed purely by
  `module_id`; a multi-group carousel requires `(group_id, module_id)`
  to disambiguate.
- **Carousel / group / module descriptors** beyond
  `compressed_module_descriptor`: `label_descriptor`,
  `caching_priority_descriptor`, and the DVB-SSU set —
  `update_descriptor`, `SSU_location_descriptor`,
  `SSU_scheduling_descriptor`, `SSU_event_name_descriptor`,
  `SSU_URI_linkage_descriptor`. They tell you *what* the payload
  updates, *when*, and *for which receivers*, which is the whole
  point of SSU.
- **Per-group completeness stats** alongside the per-module ones.
- **Output layout.** Modules currently land flat as
  `module_<id>_v<version>.bin`. Data-carousel extraction should
  mirror the group hierarchy on disk and use `label_descriptor`
  contents when present, so the output is self-describing.

## Output

- **XML/JSON dump of the tree.** Serialise the resolved object graph
  (ServiceGateway, Directories, Files, plus Stream/StreamEvent
  metadata). XML for the TSDuck convention, JSON for tooling
  pipelines. Doesn't replace the on-disk extraction, runs alongside.
- **Manifest alongside `files/`.** Little file describing what was
  written: path, size, module id, compressed/uncompressed sizes,
  carousel version. Makes extractions self-describing.

## Other profiles

DSM-CC shows up in more places than European DVB and the code here
should be honest about where it stops:

- **"MHP/GEM"** over DVB is what we mostly follow today, reading
  ETSI TR 101 202 as the primary reference.
- **"ATSC A/90"** uses DSM-CC sections with different semantics. Not
  handled.
- **"ARIB B.23"** (Japan) has its own tweaks on top. Not handled.

Explicit scope saves people wasted debugging time when a non-DVB
capture refuses to parse.

## Diagnostics

For when the extractor disagrees with the stream and you need to see
what's really there:

- `--raw-modules` to dump the post-assembly, pre-BIOP module bytes.
  Lets you inspect a broken carousel without the BIOP layer hiding
  the evidence.
- Hex trace of BIOP messages behind a verbose flag — the sort of
  thing you turn on once a year and are glad exists.
- Coverage report scoped to the dsmcc subdirectory so we can see
  where the test surface is actually thin.

## The other direction: carousel as an input plugin

This is the big one, and it's its own project. A `dsmcc` input/packet
plugin that generates a carousel from a directory and injects it into
a TS:

- Walk a directory, build BIOP ServiceGateway / Directory / File
  messages, pack into modules.
- Emit DSI/DII/DDB on a chosen PID at a chosen cycle rate and block
  size.
- Optional gzip via `compressed_module_descriptor`.
- Inject the signalling a receiver needs to discover the thing:
  PMT entry with a `stream_identifier_descriptor` pointing at the
  carousel component, AIT with the full application /
  transport_protocol / location descriptor set (HbbTV uses
  `simple_application_location_descriptor`, MHP uses
  `dvb_j_application_*`), SDT updates if it's a new service.
- Piggyback on the injection patterns already used by `inject` and
  `spliceinject` for merging PSI/SI.
- Round-trip test: generate carousel, pipe through `tsp`, extract
  with `dsmcc`, diff against the source directory. If that passes,
  both sides are healthy.

## Fitting in with the rest of TSDuck

- **`tsdsmcc` standalone tool.** `tsflute` and `tsnip` both exist as
  non-`tsp` commands for quick one-shot work on files or pcaps. A
  `tsdsmcc` would share code with the plugin and save a lot of
  `tsp -I file ... -P dsmcc ... -O drop` boilerplate.
- **`tsanalyze` / `analyze` integration.** The carousel stats we
  collect for monitoring (modules, version, completion, cycle time)
  belong in the standard analysis report too, next to PID and
  service info.

## Docs and test material

- Public-domain capture for each scenario we claim to support:
  compressed, uncompressed, multi-module, deferred tags, stream
  events. Hard to be confident about the code without these.
- A cookbook in the user guide: extract an HbbTV app, look at a
  DVB-SSU carousel, verify a carousel you just built, watch version
  bumps on a live stream.
