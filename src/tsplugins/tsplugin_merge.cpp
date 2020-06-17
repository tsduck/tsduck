//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsPluginRepository.h"
#include "tsTSForkPipe.h"
#include "tsTSPacketQueue.h"
#include "tsPSIMerger.h"
#include "tsThread.h"
TSDUCK_SOURCE;

#define DEFAULT_MAX_QUEUED_PACKETS  1000            // Default size in packet of the inter-thread queue.
#define SERVER_THREAD_STACK_SIZE    (128 * 1024)    // Size in byte of the thread stack.


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MergePlugin: public ProcessorPlugin, private Thread
    {
        TS_NOBUILD_NOCOPY(MergePlugin);
    public:
        // Implementation of plugin API
        MergePlugin (TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

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
        bool              _terminate;         // Terminate processing after last merged packet.
        PIDSet            _allowed_pids;      // List of PID's to merge.
        bool              _abort;             // Error, give up asap.
        bool              _got_eof;           // Got end of merged stream.
        PacketCounter     _pkt_count;         // Packet counter in the main stream.
        TSForkPipe        _pipe;              // Executed command.
        TSPacketQueue     _queue;             // TS packet queur from merge to main.
        PIDSet            _main_pids;         // Set of detected PID's in main stream.
        PIDSet            _merge_pids;        // Set of detected PID's in merged stream that we pass in main stream.
        PIDContextMap     _pcr_pids;          // Description of PID's with PCR's from the merged stream.
        PSIMerger         _psi_merger;        // Used to merge PSI/SI from both streams.
        TSPacketFormat    _format;            // Packet format on the pipe
        TSPacketMetadata::LabelSet _setLabels;    // Labels to set on output packets.
        TSPacketMetadata::LabelSet _resetLabels;  // Labels to reset on output packets.

        // Process a --drop or --pass option.
        bool processDropPassOption(const UChar* option, bool allowed);

        // There is one thread which receives packet from the created process and passes
        // them to the main plugin thread. The following method is the thread main code.
        virtual void main() override;

        // Process one packet coming from the merged stream.
        Status processMergePacket(TSPacket&, TSPacketMetadata&);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"merge", ts::MergePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MergePlugin::MergePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Merge TS packets coming from the standard output of a command", u"[options] 'command'"),
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _merge_psi(false),
    _pcr_restamp(false),
    _ignore_conflicts(false),
    _terminate(false),
    _allowed_pids(),
    _abort(false),
    _got_eof(false),
    _pkt_count(0),
    _pipe(),
    _queue(),
    _main_pids(),
    _merge_pids(),
    _pcr_pids(),
    _psi_merger(duck, PSIMerger::NONE, *tsp),
    _format(TSPacketFormat::AUTODETECT),
    _setLabels(),
    _resetLabels()
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

    option(u"format", 0, TSPacketFormatEnum);
    help(u"format", u"name",
         u"Specify the format of the input stream. "
         u"By default, the format is automatically detected. "
         u"But the auto-detection may fail in some cases "
         u"(for instance when the first time-stamp of an M2TS file starts with 0x47). "
         u"Using this option forces a specific format.");

    option(u"ignore-conflicts");
    help(u"ignore-conflicts",
         u"Ignore PID conflicts. By default, when packets with the same PID are "
         u"present in the two streams, the PID is dropped from the merged stream. "
         u"Warning: this is a dangerous option which can result in an inconsistent "
         u"transport stream.");

    option(u"joint-termination", 'j');
    help(u"joint-termination",
        u"Perform a \"joint termination\" when the merged stream is terminated. "
        u"See \"tsp --help\" for more details on \"joint termination\".");

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

    option(u"terminate");
    help(u"terminate",
        u"Terminate packet processing when the merged stream is terminated. "
        u"By default, when packet insertion is complete, the transmission "
        u"continues and the stuffing is no longer modified.");

    option(u"transparent", 't');
    help(u"transparent",
         u"Pass all PID's without logical transformation. "
         u"Equivalent to --no-psi-merge --ignore-conflicts --pass 0x00-0x1F.");

    option(u"set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-label", u"label1[-label2]",
         u"Set the specified labels on the merged packets. "
         u"Apply to original packets from the merged stream only, not to updated PSI. "
         u"Several --set-label options may be specified.");

    option(u"reset-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"reset-label", u"label1[-label2]",
         u"Clear the specified labels on the merged packets. "
         u"Apply to original packets from the merged stream only, not to updated PSI. "
         u"Several --reset-label options may be specified.");
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
    _format = enumValue<TSPacketFormat>(u"format", TSPacketFormat::AUTODETECT);
    _merge_psi = !transparent && !present(u"no-psi-merge");
    _pcr_restamp = !present(u"no-pcr-restamp");
    _ignore_conflicts = transparent || present(u"ignore-conflicts");
    _terminate = present(u"terminate");
    tsp->useJointTermination(present(u"joint-termination"));
    getIntValues(_setLabels, u"set-label");
    getIntValues(_resetLabels, u"reset-label");

    if (_terminate && tsp->useJointTermination()) {
        tsp->error(u"--terminate and --joint-termination are mutually exclusive");
        return false;
    }

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

    // Configure the PSI merger.
    if (_merge_psi) {
        _psi_merger.reset(PSIMerger::MERGE_PAT |
                          PSIMerger::MERGE_CAT |
                          PSIMerger::MERGE_SDT |
                          PSIMerger::MERGE_EIT |
                          PSIMerger::NULL_MERGED |
                          PSIMerger::NULL_UNMERGED);

        // Let the PSI Merger manage the packets from the merged PID's.
        _allowed_pids.set(PID_PAT);
        _allowed_pids.set(PID_CAT);
        _allowed_pids.set(PID_SDT);
        _allowed_pids.set(PID_EIT);
    }

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
                               ForkPipe::STDIN_NONE,
                               _format);

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
        if (!_pipe.readStreamComplete(buffer, PKT_SIZE * buffer_size, read_size, *tsp)) {
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

ts::ProcessorPlugin::Status ts::MergePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Merge PSI/SI.
    if (_merge_psi) {
        _psi_merger.feedMainPacket(pkt);
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

    // Stuffing packets are potential candidate for replacement from merged stream.
    const Status status = pid == PID_NULL ? processMergePacket(pkt, pkt_data) : TSP_OK;

    // Count packets in the main stream.
    _pkt_count++;

    return status;
}


//----------------------------------------------------------------------------
// Process one packet coming from the merged stream.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MergePlugin::processMergePacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    BitRate merge_bitrate = 0;

    // Replace current null packet in main stream with next packet from merged stream.
    if (!_queue.getPacket(pkt, merge_bitrate)) {
        // No packet available, keep original null packet.
        if (!_got_eof && _queue.eof()) {
            // Report end of input stream once.
            _got_eof = true;
            tsp->verbose(u"end of merged stream");
            // If processing terminated, either exit or transparently pass packets
            if (tsp->useJointTermination()) {
                tsp->jointTerminate();
                return TSP_OK;
            }
            else if (_terminate) {
                return TSP_END;
            }
        }
        return TSP_OK;
    }

    // Merge PSI/SI.
    if (_merge_psi) {
        _psi_merger.feedMergedPacket(pkt);
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

    // Apply labels on muxed packets.
    pkt_data.setLabels(_setLabels);
    pkt_data.clearLabels(_resetLabels);

    return TSP_OK;
}
