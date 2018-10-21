//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Merge TS packets coming from the standard output of a command.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsForkPipe.h"
#include "tsTSPacketQueue.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsThread.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsSDT.h"
#include "tsCADescriptor.h"
TSDUCK_SOURCE;

#define DEFAULT_MAX_QUEUED_PACKETS  1000            // Default size in packet of the inter-thread queue.
#define SERVER_THREAD_STACK_SIZE    (128 * 1024)    // Size in byte of the thread stack.
#define DEMUX_MAIN                  1               // Id of the demux from the main TS.
#define DEMUX_MERGE                 2               // Id of the demux from the secondary TS to merge.


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MergePlugin: public ProcessorPlugin, private Thread, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        MergePlugin (TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Definitions:
        // - Main stream: the TS which is processed by tsp, including this plugin.
        // - Merged stream: the additional TS which is read by this plugin through a pipe.

        // Each PID with PCR's in the merged stream is described by a structure like this.
        class PIDContext
        {
        public:
            uint64_t      last_pcr;  // Last PCR value in this PID, after adjustment in main stream.
            PacketCounter pcr_pkt;   // Index of the packet with the last PCR in the main stream.

            // Constructor:
            PIDContext(uint64_t pcr = 0, PacketCounter pkt = 0) : last_pcr(pcr), pcr_pkt(pkt) {}
        };

        // Map of PID contexts, indexed by PID.
        typedef std::map<PID,PIDContext> PIDContextMap;

        // Plugin private data.
        bool              _merge_psi;         // Merge PSI/SI information.
        bool              _pcr_restamp;       // Restamp PCR from the merged stream.
        bool              _ignore_conflicts;  // Ignore PID conflicts.
        PIDSet            _allowed_pids;      // List of PID's to merge.
        bool              _abort;             // Error, give up asap.
        bool              _got_eof;           // Got end of merged stream.
        PacketCounter     _pkt_count;         // Packet counter in the main stream.
        ForkPipe          _pipe;              // Executed command.
        TSPacketQueue     _queue;             // TS packet queur from merge to main.
        PIDSet            _main_pids;         // Set of detected PID's in main stream.
        PIDSet            _merge_pids;        // Set of detected PID's in merged stream that we pass in main stream.
        PIDContextMap     _pcr_pids;          // Description of PID's with PCR's from the merged stream.
        SectionDemux      _main_demux;        // Demux on main transport stream.
        SectionDemux      _merge_demux;       // Demux on merged transport stream.
        CyclingPacketizer _pat_pzer;          // Packetizer for modified PAT in main TS.
        CyclingPacketizer _cat_pzer;          // Packetizer for modified CAT in main TS.
        CyclingPacketizer _sdt_pzer;          // Packetizer for modified SDT/BAT in main TS.
        PAT               _main_pat;          // Last input PAT from main TS (version# is current output version).
        PAT               _merge_pat;         // Last input PAT from merged TS.
        CAT               _main_cat;          // Last input CAT from main TS (version# is current output version).
        CAT               _merge_cat;         // Last input CAT from merged TS.
        SDT               _main_sdt;          // Last input SDT from main TS (version# is current output version).
        SDT               _merge_sdt;         // Last input SDT from merged TS.

        // Process a --drop or --pass option.
        bool processDropPassOption(const UChar* option, bool allowed);

        // There is one thread which receives packet from the created process and passes
        // them to the main plugin thread. The following method is the thread main code.
        virtual void main() override;

        // Invoked when a complete table is available from any demux.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Process one packet coming from the merged stream.
        Status processMergePacket(TSPacket&);

        // Generate new/merged tables.
        void mergePAT();
        void mergeCAT();
        void mergeSDT();

        // Copy a table into another, preserving the previous version number it the table is valid.
        template<class TABLE, typename std::enable_if<std::is_base_of<AbstractLongTable, TABLE>::value>::type* = nullptr>
        void copyTableKeepVersion(TABLE& dest, const TABLE& src)
        {
            const bool was_valid = dest.isValid();
            const uint8_t version = dest.version;
            dest = src;
            if (was_valid) {
                dest.version = version;
            }
        }

        // Inaccessible operations
        MergePlugin() = delete;
        MergePlugin(const MergePlugin&) = delete;
        MergePlugin& operator=(const MergePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(merge, ts::MergePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MergePlugin::MergePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Merge TS packets coming from the standard output of a command", u"[options] 'command'"),
    _merge_psi(false),
    _pcr_restamp(false),
    _ignore_conflicts(false),
    _allowed_pids(),
    _abort(false),
    _got_eof(false),
    _pkt_count(0),
    _pipe(),
    _queue(),
    _main_pids(),
    _merge_pids(),
    _pcr_pids(),
    _main_demux(this),
    _merge_demux(this),
    _pat_pzer(),
    _cat_pzer(),
    _sdt_pzer(),
    _main_pat(),
    _merge_pat(),
    _main_cat(),
    _merge_cat(),
    _main_sdt(),
    _merge_sdt()
{
    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specifies the command line to execute in the created process.");

    option(u"drop", 'd', STRING, 0, UNLIMITED_COUNT);
    help(u"drop", u"pid[-pid]",
         u"Drop the specified PID or range of PID's from the merged stream. By "
         u"default, the PID's 0x00 to 0x1F are dropped and all other PID's are "
         u"passed. This can be modified using options --drop and --pass. Several "
         u"options --drop can be specified.");

    option(u"ignore-conflicts");
    help(u"ignore-conflicts",
         u"Ignore PID conflicts. By default, when packets with the same PID are "
         u"present in the two streams, the PID is dropped from the merged stream. "
         u"Warning: this is a dangerous option which can result in an inconsistent "
         u"transport stream.");

    option(u"max-queue", 0, POSITIVE);
    help(u"max-queue",
         u"Specify the maximum number of queued TS packets before their "
         u"insertion into the stream. The default is " +
         UString::Decimal(DEFAULT_MAX_QUEUED_PACKETS) + u".");

    option(u"no-pcr-restamp");
    help(u"no-pcr-restamp",
         u"Do not restamp PCR's from the merged TS into the main TS. By default, "
         u"PCR's in the merged stream are restamped to match their position in the "
         u"final stream. The DTS and PTS are never restamped because they are "
         u"independent from their position in the stream. When the PCR's in the "
         u"merged stream have discontinuities (such as when cycling a TS file), "
         u"restamping the PCR's can break the video playout since they become "
         u"decorrelated with the DTS and PTS.");

    option(u"no-psi-merge");
    help(u"no-psi-merge",
         u"Do not merge PSI/SI from the merged TS into the main TS. By default, the "
         u"PAT, CAT and SDT are merged so that the services from the merged stream "
         u"are properly referenced and PID's 0x00 to 0x1F are dropped from the merged "
         u"stream.");

    option(u"no-wait");
    help(u"no-wait",
         u"Do not wait for child process termination at end of processing.");

    option(u"pass", 'p', STRING, 0, UNLIMITED_COUNT);
    help(u"pass", u"pid[-pid]",
         u"Pass the specified PID or range of PID's from the merged stream. By "
         u"default, the PID's 0x00 to 0x1F are dropped and all other PID's are "
         u"passed. This can be modified using options --drop and --pass. Several "
         u"options --pass can be specified.");

    option(u"transparent", 't');
    help(u"transparent",
         u"Pass all PID's without logical transformation. "
         u"Equivalent to --no-psi-merge --ignore-conflicts --pass 0x00-0x1F.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MergePlugin::start()
{
    // Get command line arguments
    UString command(value());
    const bool nowait = present(u"no-wait");
    const bool transparent = present(u"transparent");
    const size_t max_queue = intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_PACKETS);
    _merge_psi = !transparent && !present(u"no-psi-merge");
    _pcr_restamp = !present(u"no-pcr-restamp");
    _ignore_conflicts = transparent || present(u"ignore-conflicts");

    // By default, drop all base PSI/SI (PID 0x00 to 0x1F).
    _allowed_pids.set();
    if (!transparent) {
        for (PID pid = 0x00; pid <= PID_DVB_LAST; ++pid) {
            _allowed_pids.reset(pid);
        }
    }
    if (!processDropPassOption(u"drop", false) || !processDropPassOption(u"pass", true)) {
        return false;
    }

    // Resize the inter-thread packet queue.
    _queue.reset(max_queue);

    // Configure the demux. We need to analyze and modify the PAT, CAT and SDT
    // from the two transport streams.
    _main_demux.setDemuxId(DEMUX_MAIN);
    _main_demux.addPID(PID_PAT);
    _main_demux.addPID(PID_CAT);
    _main_demux.addPID(PID_SDT);
    _merge_demux.setDemuxId(DEMUX_MERGE);
    _merge_demux.addPID(PID_PAT);
    _merge_demux.addPID(PID_CAT);
    _merge_demux.addPID(PID_SDT);

    // Configure the packetizers.
    _pat_pzer.reset();
    _cat_pzer.reset();
    _sdt_pzer.reset();
    _pat_pzer.setPID(PID_PAT);
    _cat_pzer.setPID(PID_CAT);
    _sdt_pzer.setPID(PID_SDT);

    // Make sure that all input tables are invalid.
    _main_pat.invalidate();
    _merge_pat.invalidate();
    _main_cat.invalidate();
    _merge_cat.invalidate();
    _main_sdt.invalidate();
    _merge_sdt.invalidate();

    // Other states.
    _main_pids.reset();
    _merge_pids.reset();
    _pcr_pids.clear();
    _pkt_count = 0;
    _got_eof = false;
    _abort = false;

    // Create pipe & process
    const bool ok = _pipe.open(command,
                               nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                               PKT_SIZE * DEFAULT_MAX_QUEUED_PACKETS,
                               *tsp,
                               ForkPipe::STDOUT_PIPE,
                               ForkPipe::STDIN_NONE);

    // Start the internal thread which receives the TS to merge.
    if (ok) {
        Thread::start();
    }

    return ok;
}


//----------------------------------------------------------------------------
// Process a --drop or --pass option.
//----------------------------------------------------------------------------

bool ts::MergePlugin::processDropPassOption(const UChar* option, bool allowed)
{
    const size_t max = count(option);
    bool status = true;

    // Loop on all occurences of the option.
    for (size_t i = 0; i < max; ++i) {

        // Next occurence of the option.
        const UString str(value(option, u"", i));
        PID pid1 = PID_NULL;
        PID pid2 = PID_NULL;
        size_t num = 0;
        size_t last = 0;

        // The accepted format is: pid[-pid]
        str.scan(num, last, u"%d-%d", {&pid1, &pid2});
        if (num < 1 || last != str.size() || pid1 >= PID_MAX || pid2 >= PID_MAX || (num == 2 && pid1 > pid2)) {
            tsp->error(u"invalid PID range \"%s\" for --%s, use \"pid[-pid]\"", {str, option});
            status = false;
        }
        else {
            while (pid1 <= pid2) {
                _allowed_pids.set(pid1++, allowed);
            }
        }
    }
    return status;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::MergePlugin::stop()
{
    // Send the stop condition to the internal packet queue.
    _queue.stop();

    // Close the pipe and terminate the created process.
    _pipe.close(*tsp);

    // Wait for actual thread termination.
    Thread::waitForTermination();
    return true;
}


//----------------------------------------------------------------------------
// Implementation of the receiver thread.
// It simply reads TS packets and passes them to the plugin thread.
//----------------------------------------------------------------------------

void ts::MergePlugin::main()
{
    tsp->debug(u"receiver thread started");

    // Loop on packet reception until the plugin request to stop.
    while (!_queue.stopped()) {

        TSPacket* buffer = nullptr;
        size_t buffer_size = 0;  // In TS packets.
        size_t read_size = 0;    // In bytes.

        // Wait for free space in the internal packet queue.
        // We don't want to read too many small data sizes, so we wait for at least 16 packets.
        if (!_queue.lockWriteBuffer(buffer, buffer_size, 16)) {
            // The plugin thread has signalled a stop condition.
            break;
        }

        assert(buffer != nullptr);
        assert(buffer_size > 0);

        // Read TS packets from the pipe, up to buffer size (but maybe less).
        // We request to read only multiples of 188 bytes (the packet size).
        if (!_pipe.read(buffer, PKT_SIZE * buffer_size, PKT_SIZE, read_size, *tsp)) {
            // Read error or end of file, cannot continue in all cases.
            // Signal end-of-file to plugin thread.
            _queue.setEOF();
            break;
        }

        assert(read_size % PKT_SIZE == 0);

        // Pass the read packets to the inter-thread queue.
        // The read size was returned in bytes, we must give a number of packets.
        _queue.releaseWriteBuffer(read_size / PKT_SIZE);
    }

    tsp->debug(u"receiver thread completed");
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MergePlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    // Demux sections from the main transport stream.
    // This is required only to merge PSI/SI.
    if (_merge_psi) {
        _main_demux.feedPacket(pkt);
    }

    // Check PID conflicts.
    if (!_ignore_conflicts && pid != PID_NULL && !_main_pids.test(pid)) {
        // First time we see that PID on the main stream.
        _main_pids.set(pid);
        if (_merge_pids.test(pid)) {
            // We have already merged some packets from this PID.
            tsp->error(u"PID conflict: PID 0x%X (%d) exists in the two streams, dropping from merged stream, but some packets were already merged", {pid, pid});
        }
    }

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // Final status for this packet.
    Status status = TSP_OK;

    // Process packagets depending on PID.
    switch (pid) {
        case PID_PAT: {
            // Replace PAT packets using packetizer if a new PAT was generated.
            if (_main_pat.isValid() && _merge_pat.isValid()) {
                _pat_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_CAT: {
            // Replace CAT packets using packetizer if a new CAT was generated.
            if (_main_cat.isValid() && _merge_cat.isValid()) {
                _cat_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_SDT: {
            // Replace SDT/BAT packets using packetizer if a new SDT was generated.
            if (_main_sdt.isValid() && _merge_sdt.isValid()) {
                _sdt_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_NULL: {
            // Stuffing, potential candidate for replacement from merged stream.
            status = processMergePacket(pkt);
            break;
        }
        default: {
            // Other PID's are left unmodified.
            break;
        }
    }

    // Count packets in the main stream.
    _pkt_count++;

    return status;
}


//----------------------------------------------------------------------------
// Process one packet coming from the merged stream.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MergePlugin::processMergePacket(TSPacket& pkt)
{
    BitRate merge_bitrate = 0;

    // Replace current null packet in main stream with next packet from merged stream.
    if (!_queue.getPacket(pkt, merge_bitrate)) {
        // No packet available, keep original null packet.
        if (!_got_eof && _queue.eof()) {
            // Report end of input stream once.
            _got_eof = true;
            tsp->verbose(u"end of merged stream");
        }
        return TSP_OK;
    }

    // Demux sections from the merged stream.
    // This is required only to merge PSI/SI.
    if (_merge_psi) {
        _merge_demux.feedPacket(pkt);
    }

    // Drop selected PID's from merged stream. Replace them with a null packet.
    const PID pid = pkt.getPID();
    if (!_allowed_pids.test(pid)) {
        return TSP_NULL;
    }

    // Check PID conflicts.
    if (!_ignore_conflicts) {
        if (pid != PID_NULL && !_merge_pids.test(pid)) {
            // First time we see that PID on the merged stream.
            _merge_pids.set(pid);
            if (_main_pids.test(pid)){
                tsp->error(u"PID conflict: PID 0x%X (%d) exists in the two streams, dropping from merged stream", {pid, pid});
            }
        }
        if (pid != PID_NULL && _main_pids.test(pid)) {
            // The same PID already exists in the main PID, drop from merged stream.
            // Error message already reported.
            return TSP_NULL;
        }
    }

    // Adjust PCR's in packets from the merge streams.
    if (_pcr_restamp && pkt.hasPCR()) {

        // In each PID with PCR's in the merge stream, we keep the first PCR
        // value unchanged. Then, we need to adjust all subsequent PCR's.
        // PCR's are system clock values. They must be synchronized with the
        // transport stream rate. So, the difference between two PCR's shall
        // be the transmission time in PCR units.
        //
        // We can compute new precise PCR values when the bitrate is fixed.
        // However, with a variable bitrate, our computed values will be inaccurate.
        //
        // Also note that we do not modify DTS and PTS. First, we can't access
        // PTS and DTS in scrambled streams (unlike PCR's). Second, we MUST NOT
        // change them because they indicate at which time the frame shall be
        // _processed_, not _transmitted_.

        const uint64_t pcr = pkt.getPCR();
        const BitRate main_bitrate = tsp->bitrate();

        // Check if we know this PID.
        PIDContextMap::iterator ctx(_pcr_pids.find(pid));
        if (ctx == _pcr_pids.end()) {
            // First time we see a PCR in this PID, create the context.
            // Save the initial PCR value bu do not modify it.
            _pcr_pids.insert(std::make_pair(pid, PIDContext(pcr, _pkt_count)));
        }
        else if (main_bitrate > 0) {
            // We have seen PCR's in this PID.
            // Compute the transmission time since last PCR in PCR units.
            // We base the result on the main stream bitrate and the number of packets.
            assert(_pkt_count > ctx->second.pcr_pkt);
            ctx->second.last_pcr += ((_pkt_count - ctx->second.pcr_pkt) * 8 * PKT_SIZE * SYSTEM_CLOCK_FREQ) / uint64_t(main_bitrate);
            ctx->second.pcr_pkt = _pkt_count;
            // Update the PCR in the packet.
            pkt.setPCR(ctx->second.last_pcr);
            // In debug mode, report the displacement of the PCR.
            // This may go back and forth around zero but should never diverge.
            const SubSecond moved = ctx->second.last_pcr - pcr;
            tsp->debug(u"adjusted PCR by %'d (%'d ms) in PID 0x%X (%d)", {moved, (moved * MilliSecPerSec) / SYSTEM_CLOCK_FREQ, pid, pid});
        }
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Invoked when a complete table is available from any demux.
//----------------------------------------------------------------------------

void ts::MergePlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (demux.demuxId()) {
        case DEMUX_MAIN: {
            // Table coming from the main transport stream.
            // The processing the same for PAT, CAT and SDT-Actual:
            // update last input table and merge with table from the other stream.
            switch (table.tableId()) {
                case TID_PAT: {
                    const PAT pat(table);
                    if (pat.isValid() && table.sourcePID() == PID_PAT) {
                        copyTableKeepVersion(_main_pat, pat);
                        mergePAT();
                    }
                    break;
                }
                case TID_CAT: {
                    const CAT cat(table);
                    if (cat.isValid() && table.sourcePID() == PID_CAT) {
                        copyTableKeepVersion(_main_cat, cat);
                        mergeCAT();
                    }
                    break;
                }
                case TID_SDT_ACT: {
                    const SDT sdt(table);
                    if (sdt.isValid() && table.sourcePID() == PID_SDT) {
                        copyTableKeepVersion(_main_sdt, sdt);
                        mergeSDT();
                    }
                    break;
                }
                case TID_BAT:
                case TID_SDT_OTH: {
                    if (table.sourcePID() == PID_SDT) {
                        // This is a BAT or an SDT-Other.
                        // It must be reinserted without modification in the SDT/BAT PID.
                        _sdt_pzer.removeSections(table.tableId(), table.tableIdExtension());
                        _sdt_pzer.addTable(table);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
        case DEMUX_MERGE: {
            // Table coming from the merged transport stream.
            // The processing the same for PAT, CAT and SDT-Actual:
            // update last input table and merge with table from the other stream.
            switch (table.tableId()) {
                case TID_PAT: {
                    const PAT pat(table);
                    if (pat.isValid() && table.sourcePID() == PID_PAT) {
                        _merge_pat = pat;
                        mergePAT();
                    }
                    break;
                }
                case TID_CAT: {
                    const CAT cat(table);
                    if (cat.isValid() && table.sourcePID() == PID_CAT) {
                        _merge_cat = cat;
                        mergeCAT();
                    }
                    break;
                }
                case TID_SDT_ACT: {
                    const SDT sdt(table);
                    if (sdt.isValid() && table.sourcePID() == PID_SDT) {
                        _merge_sdt = sdt;
                        mergeSDT();
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
        default: {
            // Should not get there.
            assert(false);
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Merge the PAT's and build a new one into the packetizer.
//----------------------------------------------------------------------------

void ts::MergePlugin::mergePAT()
{
    // Check that we have valid tables to merge.
    if (!_main_pat.isValid() || !_merge_pat.isValid()) {
        return;
    }

    // Build a new PAT based on last main PAT with incremented version number.
    PAT pat(_main_pat);
    pat.version = (pat.version + 1) & SVERSION_MASK;

    // Add all services from merged stream into main PAT.
    for (auto merge = _merge_pat.pmts.begin(); merge != _merge_pat.pmts.end(); ++merge) {
        // Check if the service already exists in the main PAT.
        if (pat.pmts.find(merge->first) != pat.pmts.end()) {
            tsp->error(u"service conflict, service 0x%X (%d) exists in the two streams, dropping from merged stream", {merge->first, merge->first});
        }
        else {
            pat.pmts[merge->first] = merge->second;
            tsp->verbose(u"adding service 0x%X (%d) in PAT from merged stream", {merge->first, merge->first});
        }
    }

    // Replace the PAT in the packetizer.
    _pat_pzer.removeSections(TID_PAT);
    _pat_pzer.addTable(pat);

    // Save PAT version number for later increment.
    _main_pat.version = pat.version;
}


//----------------------------------------------------------------------------
// Merge the CAT's and build a new one into the packetizer.
//----------------------------------------------------------------------------

void ts::MergePlugin::mergeCAT()
{
    // Check that we have valid tables to merge.
    if (!_main_cat.isValid() || !_merge_cat.isValid()) {
        return;
    }

    // Build a new CAT based on last main CAT with incremented version number.
    CAT cat(_main_cat);
    cat.version = (cat.version + 1) & SVERSION_MASK;

    // Add all CA descriptors from merged stream into main CAT.
    for (size_t index = _merge_cat.descs.search(DID_CA); index < _merge_cat.descs.count(); index = _merge_cat.descs.search(DID_CA, index + 1)) {
        const CADescriptor ca(*_merge_cat.descs[index]);
        // Check if the same EMM PID already exists in the main CAT.
        if (CADescriptor::SearchByPID(_main_cat.descs, ca.ca_pid) < _main_cat.descs.count()) {
            tsp->error(u"EMM PID conflict, PID 0x%X (%d) referenced in the two streams, dropping from merged stream", {ca.ca_pid, ca.ca_pid});
        }
        else {
            cat.descs.add(_merge_cat.descs[index]);
            tsp->verbose(u"adding EMM PID 0x%X (%d) in CAT from merged stream", {ca.ca_pid, ca.ca_pid});
        }
    }

    // Replace the CAT in the packetizer.
    _cat_pzer.removeSections(TID_CAT);
    _cat_pzer.addTable(cat);

    // Save CAT version number for later increment.
    _main_cat.version = cat.version;
}


//----------------------------------------------------------------------------
// Merge the SDT's and build a new one into the packetizer.
//----------------------------------------------------------------------------

void ts::MergePlugin::mergeSDT()
{
    // Check that we have valid tables to merge.
    if (!_main_sdt.isValid() || !_merge_sdt.isValid()) {
        return;
    }

    // Build a new SDT based on last main SDT with incremented version number.
    SDT sdt(_main_sdt);
    sdt.version = (sdt.version + 1) & SVERSION_MASK;

    // Add all services from merged stream into main SDT.
    for (auto merge = _merge_sdt.services.begin(); merge != _merge_sdt.services.end(); ++merge) {
        // Check if the service already exists in the main SDT.
        if (sdt.services.find(merge->first) != sdt.services.end()) {
            tsp->error(u"service conflict, service 0x%X (%d) exists in the two streams, dropping from merged stream", {merge->first, merge->first});
        }
        else {
            sdt.services[merge->first] = merge->second;
            tsp->verbose(u"adding service \"%s\", id 0x%X (%d) in SDT from merged stream", {merge->second.serviceName(), merge->first, merge->first});
        }
    }

    // Replace the SDT in the packetizer.
    _sdt_pzer.removeSections(TID_SDT_ACT, sdt.ts_id);
    _sdt_pzer.addTable(sdt);

    // Save SDT version number for later increment.
    _main_sdt.version = sdt.version;
}
