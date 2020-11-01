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
#include "tsSignalizationHandlerInterface.h"
#include "tsSignalizationDemux.h"
#include "tsTSForkPipe.h"
#include "tsTSPacketQueue.h"
#include "tsPacketInsertionController.h"
#include "tsPSIMerger.h"
#include "tsSafePtr.h"
#include "tsThread.h"
#include "tsFatal.h"
TSDUCK_SOURCE;

#define DEFAULT_MAX_QUEUED_PACKETS  1000            // Default size in packet of the inter-thread queue.
#define SERVER_THREAD_STACK_SIZE    (128 * 1024)    // Size in byte of the thread stack.


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MergePlugin:
        public ProcessorPlugin,
        private SignalizationHandlerInterface,
        private Thread
    {
        TS_NOBUILD_NOCOPY(MergePlugin);
    public:
        // Implementation of plugin API
        MergePlugin (TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Definitions:
        // - Main stream: the TS which is processed by tsp, including this plugin.
        // - Merged stream: the additional TS which is read by this plugin through a pipe.

        // Each PID in the merged stream is described by a structure like this. The map is indexed by PID.
        class MergedPIDContext;
        typedef SafePtr<MergedPIDContext> MergedPIDContextPtr;
        typedef std::map<PID, MergedPIDContextPtr> MergedPIDContextMap;

        // Command line options.
        UString        _command;             // Command which generates the main stream.
        TSPacketFormat _format;              // Packet format on the pipe
        size_t         _max_queue;           // Maximum number of queued packets.
        size_t         _accel_threshold;     // Queue threshold after which insertion is accelerated.
        bool           _no_wait;             // Do not wait for command completion.
        bool           _merge_psi;           // Merge PSI/SI information.
        bool           _pcr_restamp;         // Restamp PCR from the merged stream.
        bool           _incremental_pcr;     // Use incremental method to restamp PCR's.
        bool           _merge_smoothing;     // Smoothen packet insertion.
        bool           _ignore_conflicts;    // Ignore PID conflicts.
        bool           _pcr_reset_backwards; // Reset PCR restamping when DTS/PTD move backwards the PCR.
        bool           _terminate;           // Terminate processing after last merged packet.
        BitRate        _user_bitrate;        // User-specified bitrate of the merged stream.
        PIDSet         _allowed_pids;        // List of PID's to merge (other PID's from the merged stream are dropped).
        TSPacketMetadata::LabelSet _setLabels;    // Labels to set on output packets.
        TSPacketMetadata::LabelSet _resetLabels;  // Labels to reset on output packets.

        // Working data.
        bool          _got_eof;            // Got end of merged stream.
        PacketCounter _merged_count;       // Number of merged packets.
        PacketCounter _hold_count;         // Number of times we didn't try to merge to perform smoothing insertion.
        PacketCounter _empty_count;        // Number of times we could merge but there was no packet to merge.
        TSForkPipe    _pipe;               // Executed command.
        TSPacketQueue _queue;              // TS packet queur from merge to main.
        PIDSet        _main_pids;          // Set of detected PID's in main stream.
        PIDSet        _merge_pids;         // Set of detected PID's in merged stream that we pass in main stream.
        MergedPIDContextMap _merged_ctx;   // Description of PID's from the merged stream.
        SignalizationDemux  _merged_demux; // Analyze the signalization in the merged stream.
        PSIMerger           _psi_merger;   // Used to merge PSI/SI from both streams.
        PacketInsertionController _insert_control;  // Used to control insertion points for the merge

        // Process a --drop or --pass option.
        bool processDropPassOption(const UChar* option, bool allowed);

        // There is one thread which receives packet from the created process and passes
        // them to the main plugin thread. The following method is the thread main code.
        virtual void main() override;

        // Receives all PMT's of all services in the merged stream.
        virtual void handlePMT(const PMT& table, PID pid) override;

        // Get the description of a PID inside the merged stream.
        MergedPIDContextPtr getContext(PID pid);

        // Process one packet coming from the merged stream.
        Status processMergePacket(TSPacket&, TSPacketMetadata&);

        // PID context in the merged stream.
        class MergedPIDContext
        {
            TS_NOBUILD_NOCOPY(MergedPIDContext);
        public:
            const PID     pid;            // The described PID.
            PID           pcr_pid;        // Associated PCR PID (can be the PID itself).
            uint64_t      first_pcr;      // First original PCR value in this PID.
            PacketCounter first_pcr_pkt;  // Index in the main stream of the packet with the first PCR.
            uint64_t      last_pcr;       // Last PCR value in this PID, after adjustment in main stream.
            PacketCounter last_pcr_pkt;   // Index in the main stream of the packet with the last PCR.
            uint64_t      last_pts;       // Last PTS value in this PID.
            PacketCounter last_pts_pkt;   // Index in the main stream of the packet with the last PTS.
            uint64_t      last_dts;       // Last DTS value in this PID.
            PacketCounter last_dts_pkt;   // Index in the main stream of the packet with the last DTS.

            // Constructor.
            MergedPIDContext(PID);

            // Get the DTR or PTS (whichever is defined and early).
            // Adjust it according to a bitrate and current packet.
            // Return INVALID_DTS if none defined.
            uint64_t adjustedPDTS(PacketCounter, BitRate) const;
        };
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"merge", ts::MergePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MergePlugin::MergePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Merge TS packets coming from the standard output of a command", u"[options] 'command'"),
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _command(),
    _format(TSPacketFormat::AUTODETECT),
    _max_queue(DEFAULT_MAX_QUEUED_PACKETS),
    _accel_threshold(_max_queue / 2),
    _no_wait(false),
    _merge_psi(false),
    _pcr_restamp(false),
    _incremental_pcr(false),
    _merge_smoothing(false),
    _ignore_conflicts(false),
    _pcr_reset_backwards(false),
    _terminate(false),
    _user_bitrate(0),
    _allowed_pids(),
    _setLabels(),
    _resetLabels(),
    _got_eof(false),
    _merged_count(0),
    _hold_count(0),
    _empty_count(0),
    _pipe(),
    _queue(),
    _main_pids(),
    _merge_pids(),
    _merged_ctx(),
    _merged_demux(duck, this),
    _psi_merger(duck, PSIMerger::NONE, *tsp),
    _insert_control(*tsp)
{
    _insert_control.setMainStreamName(u"main stream");
    _insert_control.setSubStreamName(u"merged stream");

    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specifies the command line to execute in the created process.");

    option(u"acceleration-threshold", 0, UNSIGNED);
    help(u"acceleration-threshold",
         u"When the insertion of the merged stream is smoothened, packets are inserted "
         u"in the main stream at some regular interval, leaving additional packets in "
         u"the queue until their natural insertion point. However, to avoid losing packets, "
         u"if the number of packets in the queue is above the specified threshold, "
         u"the insertion is accelerated. When set to zero, insertion is never accelerated. "
         u"The default threshold is half the size of the packet queue.");

    option(u"bitrate", 'b', POSITIVE);
    help(u"bitrate",
         u"Specify the target bitrate of the merged stream, in bits/seconds. "
         u"By default, the bitrate of the merged stream is computed from its PCR. "
         u"The bitrate of the merged stream is used to smoothen packet insertion "
         u"in the main stream.");

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

    option(u"incremental-pcr-restamp");
    help(u"incremental-pcr-restamp",
         u"When restamping PCR's from the merged TS into the main TS, compute each new "
         u"PCR from the last restampted one. By default, all PCR's are restampted from "
         u"the initial PCR in the PID. The default method is more precise on constant "
         u"bitrate (CBR) streams. The incremental method gives better results on "
         u"variable bitrate (VBR) streams. See also option --no-pcr-restamp.");

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

    option(u"no-smoothing");
    help(u"no-smoothing",
         u"Do not attempt to smoothen the insertion of the merged stream. "
         u"Incoming packets from the merged stream are inserted as soon as null "
         u"packets are available in the main stream. If the main stream contains "
         u"a lot of null packets, this may lead to bursts in the merged packets. "
         u"By default, if the bitrate of the merged stream is known, the merged "
         u"packets are inserted at the target interval in the main stream.");

    option(u"no-wait");
    help(u"no-wait",
         u"Do not wait for child process termination at end of processing.");

    option(u"pass", 'p', STRING, 0, UNLIMITED_COUNT);
    help(u"pass", u"pid[-pid]",
         u"Pass the specified PID or range of PID's from the merged stream. By "
         u"default, the PID's 0x00 to 0x1F are dropped and all other PID's are "
         u"passed. This can be modified using options --drop and --pass. Several "
         u"options --pass can be specified.");

    option(u"pcr-reset-backwards");
    help(u"pcr-reset-backwards",
         u"When restamping PCR's, the PCR adjustment is usually small and stays behind the PTS and DTS. "
         u"But, after hours of continuous restamping, some inaccuracy my appear and the recomputed PCR "
         u"may move ahead of PCR and DTS. With this option, as soon as a recomputed PCR is ahead of "
         u"the PTS or DTS in the same packet, PCR restamping is reset and restarts from the original "
         u"PCR value in this packet. Note that this creates a small PCR leap in the stream. "
         u"The option has, of course, no effect on scrambled streams.");

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
// Get command line options.
//----------------------------------------------------------------------------

bool ts::MergePlugin::getOptions()
{
    getValue(_command);
    _no_wait = present(u"no-wait");
    const bool transparent = present(u"transparent");
    _max_queue = intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_PACKETS);
    _accel_threshold = intValue<size_t>(u"acceleration-threshold", _max_queue / 2);
    _format = enumValue<TSPacketFormat>(u"format", TSPacketFormat::AUTODETECT);
    _merge_psi = !transparent && !present(u"no-psi-merge");
    _pcr_restamp = !present(u"no-pcr-restamp");
    _incremental_pcr = present(u"incremental-pcr-restamp");
    _merge_smoothing = !present(u"no-smoothing");
    _ignore_conflicts = transparent || present(u"ignore-conflicts");
    _pcr_reset_backwards = present(u"pcr-reset-backwards");
    _terminate = present(u"terminate");
    _user_bitrate = intValue<BitRate>(u"bitrate", 0);
    tsp->useJointTermination(present(u"joint-termination"));
    getIntValues(_setLabels, u"set-label");
    getIntValues(_resetLabels, u"reset-label");

    if (_terminate && tsp->useJointTermination()) {
        tsp->error(u"--terminate and --joint-termination are mutually exclusive");
        return false;
    }

    // Compute list of allowed PID's from the merged stream.
    _allowed_pids.set();
    if (!transparent) {
        // By default, drop all base PSI/SI (PID 0x00 to 0x1F).
        for (PID pid = 0x00; pid <= PID_DVB_LAST; ++pid) {
            _allowed_pids.reset(pid);
        }
    }
    if (!processDropPassOption(u"drop", false) || !processDropPassOption(u"pass", true)) {
        return false;
    }
    if (_merge_psi) {
        // Let the PSI Merger manage the packets from the merged PID's.
        _allowed_pids.set(PID_PAT);
        _allowed_pids.set(PID_CAT);
        _allowed_pids.set(PID_SDT);
        _allowed_pids.set(PID_EIT);
    }

    return true;
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
// Start method
//----------------------------------------------------------------------------

bool ts::MergePlugin::start()
{
    // Resize the inter-thread packet queue.
    _queue.reset(_max_queue);

    // Configure the PSI merger.
    if (_merge_psi) {
        _psi_merger.reset(PSIMerger::MERGE_PAT |
                          PSIMerger::MERGE_CAT |
                          PSIMerger::MERGE_SDT |
                          PSIMerger::MERGE_EIT |
                          PSIMerger::NULL_MERGED |
                          PSIMerger::NULL_UNMERGED);
    }

    // Capture all PMT's from the merged stream.
    _merged_demux.reset();
    _merged_demux.addTableId(TID_PMT);

    // Configure insertion control when somothing insertion.
    _insert_control.reset();
    _insert_control.setMainBitRate(tsp->bitrate());
    _insert_control.setSubBitRate(_user_bitrate); // zero if unspecified
    _insert_control.setWaitPacketsAlertThreshold(_accel_threshold);

    // Other states.
    _main_pids.reset();
    _merge_pids.reset();
    _merged_ctx.clear();
    _merged_count = _hold_count = _empty_count = 0;
    _got_eof = false;

    // Create pipe & process.
    // Note on buffer size: we use DEFAULT_MAX_QUEUED_PACKETS instead of _max_queue
    // because this is the size of the system pipe buffer (Windows only). This is
    // a limited resource and we cannot let a user set an arbitrary large value for it.
    // The user can only change the queue size in tsp's virtual memory.
    const bool ok = _pipe.open(_command,
                               _no_wait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
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
// Stop method
//----------------------------------------------------------------------------

bool ts::MergePlugin::stop()
{
    // Debug smoothing counters.
    tsp->debug(u"stopping, last merge bitrate: %'d, merged: %'d, hold: %'d, empty: %'d",
               {_insert_control.currentSubBitRate(), _merged_count, _hold_count, _empty_count});

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

    // Specify the bitrate of the incoming stream.
    // When zero, packet queue will compute it from the PCR.
    _queue.setBitrate(_user_bitrate);

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
        if (!_pipe.readStreamChunks(buffer, PKT_SIZE * buffer_size, PKT_SIZE, read_size, *tsp)) {
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

    // Declare that one packet passed in the main stream.
    _insert_control.declareMainPackets(1);

    // Stuffing packets are potential candidate for replacement from merged stream.
    return pid == PID_NULL ? processMergePacket(pkt, pkt_data): TSP_OK;
}


//----------------------------------------------------------------------------
// Process one packet coming from the merged stream.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MergePlugin::processMergePacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PacketCounter current_pkt = tsp->pluginPackets();
    const BitRate main_bitrate = tsp->bitrate();
    _insert_control.setMainBitRate(main_bitrate);

    // In case of packet insertion smoothing, check if we need to insert packets here.
    if (_merge_smoothing && !_insert_control.mustInsert(_queue.currentSize())) {
        // Don't insert now, would burst over target merged bitrate.
        _hold_count++;
        return TSP_NULL;
    }

    // Replace current null packet in main stream with next packet from merged stream.
    BitRate merged_bitrate = 0;
    if (!_queue.getPacket(pkt, merged_bitrate)) {
        // No packet available, keep original null packet.
        _empty_count++;
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

    // Report merged bitrate change.
    _insert_control.setSubBitRate(merged_bitrate);

    // Declare that one packet was merged. Must be done here, before dropping unused PID's,
    // because it is used in computation involving the bitrate of the complete merged stream.
    _insert_control.declareSubPackets(1);
    _merged_count++;

    // Collect and merge PSI/SI when needed.
    if (_pcr_restamp && _pcr_reset_backwards) {
        _merged_demux.feedPacket(pkt);
    }
    if (_merge_psi) {
        _psi_merger.feedMergedPacket(pkt);
    }

    // Drop selected PID's from merged stream. Replace them with a null packet.
    const PID pid = pkt.getPID();
    if (!_allowed_pids.test(pid)) {
        return TSP_NULL;
    }

    // Check PID conflicts.
    if (!_ignore_conflicts && pid != PID_NULL) {
        if (!_merge_pids.test(pid)) {
            // First time we see that PID on the merged stream.
            _merge_pids.set(pid);
            if (_main_pids.test(pid)){
                tsp->error(u"PID conflict: PID 0x%X (%d) exists in the two streams, dropping from merged stream", {pid, pid});
            }
        }
        if (_main_pids.test(pid)) {
            // The same PID already exists in the main PID, drop from merged stream.
            // Error message already reported.
            return TSP_NULL;
        }
    }

    // Collect and process time stamps.
    if (_pcr_restamp) {

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

        const MergedPIDContextPtr ctx(getContext(pid));
        const uint64_t pcr = pkt.getPCR();
        const uint64_t dts = pkt.getDTS();
        const uint64_t pts = pkt.getPTS();

        // The last DTS and PTS are stored for all PID's.
        if (dts != INVALID_DTS) {
            ctx->last_dts = dts;
            ctx->last_dts_pkt = current_pkt;
        }
        if (pts != INVALID_PTS) {
            ctx->last_pts = pts;
            ctx->last_pts_pkt = current_pkt;
        }

        // PCR's are stored, modified or reset.
        if (pcr == INVALID_PCR) {
            // No PCR, do nothing.
        }
        else if (ctx->last_pcr == INVALID_PCR) {
            // First time we see a PCR in this PID.
            // Save the initial PCR value but do not modify it.
            ctx->first_pcr = ctx->last_pcr = pcr;
            ctx->first_pcr_pkt = ctx->last_pcr_pkt = current_pkt;
        }
        else if (main_bitrate > 0) {
            // This is not the first PCR in this PID.
            // Compute the transmission time since some previous PCR in PCR units.
            // We base the result on the main stream bitrate and the number of packets.
            // By default, compute PCR based on distance from first PCR.
            // On the long run, this is more precise on CBR but can be devastating on VBR.
            uint64_t base_pcr = ctx->first_pcr;
            PacketCounter base_pkt = ctx->first_pcr_pkt;
            if (_incremental_pcr) {
                // Compute PCR based in increment from the last one. Small errors may accumulate.
                base_pcr = ctx->last_pcr;
                base_pkt = ctx->last_pcr_pkt;
            }
            assert(base_pkt < current_pkt);
            ctx->last_pcr = base_pcr + ((current_pkt - base_pkt) * 8 * PKT_SIZE * SYSTEM_CLOCK_FREQ) / uint64_t(main_bitrate);
            ctx->last_pcr_pkt = current_pkt;

            // When --pcr-reset-backwards is specified, check if DTS or PTS have moved backwards PCR.
            // This may occur after slow drift in PCR restamping.
            bool update_pcr = true;
            if (_pcr_reset_backwards) {
                // Restamped PCR value in PTS/DTS units:
                const uint64_t subpcr = ctx->last_pcr / SYSTEM_CLOCK_SUBFACTOR;
                // Loop on all PID's which use current PID as PCR PID, searching for a reason not to update the PCR.
                for (auto it = _merged_ctx.begin(); update_pcr && it != _merged_ctx.end(); ++it) {
                    if (it->second->pcr_pid == pid) {
                        // Extrapolated current PTS/DTS of this PID at current packet.
                        const uint64_t pdts = it->second->adjustedPDTS(current_pkt, main_bitrate);
                        if (pdts != INVALID_DTS && pdts <= subpcr) {
                            // PTS or DTS moved backwards PCR -> reset PCR restamping.
                            update_pcr = false;
                            ctx->first_pcr = ctx->last_pcr = pcr;
                            ctx->first_pcr_pkt = ctx->last_pcr_pkt = current_pkt;
                            tsp->verbose(u"resetting PCR restamping in PID 0x%X (%<d) after DTS/PTS moved backwards restamped PCR", {pid});
                        }
                    }
                }
            }

            // Update the PCR in the packet if required.
            if (update_pcr) {
                pkt.setPCR(ctx->last_pcr);
                // In debug mode, report the displacement of the PCR.
                // This may go back and forth around zero but should never diverge (--pcr-reset-backwards case).
                // Report it at debug level 2 only since it occurs on almost all merged packets with PCR.
                const SubSecond moved = SubSecond(ctx->last_pcr) - SubSecond(pcr);
                tsp->log(2, u"adjusted PCR by %+'d (%+'d ms) in PID 0x%X (%<d)", {moved, (moved * MilliSecPerSec) / SYSTEM_CLOCK_FREQ, pid});
            }
        }
    }

    // Apply labels on merged packets.
    pkt_data.setLabels(_setLabels);
    pkt_data.clearLabels(_resetLabels);

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Receives all PMT's of all services in the merged stream.
//----------------------------------------------------------------------------

void ts::MergePlugin::handlePMT(const PMT& pmt, PID pid)
{
    // Record the PCR PID for each component in the service.
    if (pmt.pcr_pid != PID_NULL) {
        for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
            // it->first is the PID of the component
            getContext(it->first)->pcr_pid = pmt.pcr_pid;
        }
    }
}


//----------------------------------------------------------------------------
// Get the description of a PID inside the merged stream.
//----------------------------------------------------------------------------

ts::MergePlugin::MergedPIDContextPtr ts::MergePlugin::getContext(PID pid)
{
    const auto ctx = _merged_ctx.find(pid);
    if (ctx != _merged_ctx.end()) {
        return ctx->second;
    }
    else {
        MergedPIDContextPtr ptr(new MergedPIDContext(pid));
        CheckNonNull(ptr.pointer());
        _merged_ctx[pid] = ptr;
        return ptr;
    }
}


//----------------------------------------------------------------------------
// Constructor of PID context in the merged stream.
//----------------------------------------------------------------------------

ts::MergePlugin::MergedPIDContext::MergedPIDContext(PID p) :
    pid(p),
    pcr_pid(p),  // each PID is its own PCR PID until proven otherwise in a PMT
    first_pcr(INVALID_PCR),
    first_pcr_pkt(0),
    last_pcr(INVALID_PCR),
    last_pcr_pkt(0),
    last_pts(INVALID_PTS),
    last_pts_pkt(0),
    last_dts(INVALID_DTS),
    last_dts_pkt(0)
{
}


//----------------------------------------------------------------------------
// Get the adjusted DTR or PTS according to a bitrate and current packet.
//----------------------------------------------------------------------------

uint64_t ts::MergePlugin::MergedPIDContext::adjustedPDTS(PacketCounter current_pkt, BitRate bitrate) const
{
    // Compute adjusted DTS and PTS.
    uint64_t dts = last_dts;
    uint64_t pts = last_pts;
    if (bitrate != 0) {
        if (dts != INVALID_DTS) {
            dts += ((current_pkt - last_dts_pkt) * 8 * PKT_SIZE * SYSTEM_CLOCK_SUBFREQ) / bitrate;
        }
        if (pts != INVALID_PTS) {
            pts += ((current_pkt - last_pts_pkt) * 8 * PKT_SIZE * SYSTEM_CLOCK_SUBFREQ) / bitrate;
        }
    }

    if (dts == INVALID_DTS) {
        return pts; // can be INVALID_PTS
    }
    else if (pts == INVALID_PTS) {
        return dts; // only DTS is valid
    }
    else {
        return std::min(pts, dts);
    }
}
