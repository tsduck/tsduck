//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Definitions:
//  - Main stream: the TS which is processed by tsp, including this plugin.
//  - Merged stream: the additional TS which is read by this plugin through a pipe.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPCRMerger.h"
#include "tsPSIMerger.h"
#include "tsTSForkPipe.h"
#include "tsTSPacketQueue.h"
#include "tsPacketInsertionController.h"
#include "tsThread.h"
#include "tsFatal.h"

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
        MergePlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        UString          _command;             // Command which generates the main stream.
        TSPacketFormat   _format;              // Packet format on the pipe
        size_t           _max_queue;           // Maximum number of queued packets.
        size_t           _accel_threshold;     // Queue threshold after which insertion is accelerated.
        bool             _no_wait;             // Do not wait for command completion.
        bool             _merge_psi;           // Merge PSI/SI information.
        bool             _pcr_restamp;         // Restamp PCR from the merged stream.
        bool             _incremental_pcr;     // Use incremental method to restamp PCR's.
        bool             _merge_smoothing;     // Smoothen packet insertion.
        bool             _ignore_conflicts;    // Ignore PID conflicts.
        bool             _pcr_reset_backwards; // Reset PCR restamping when DTS/PTD move backwards the PCR.
        bool             _terminate;           // Terminate processing after last merged packet.
        bool             _restart;             // Restart command after termination.
        MilliSecond      _restart_interval;    // Interval before restarting the merge command.
        BitRate          _user_bitrate;        // User-specified bitrate of the merged stream.
        PIDSet           _allowed_pids;        // List of PID's to merge (other PID's from the merged stream are dropped).
        TSPacketLabelSet _set_labels;          // Labels to set on output packets.
        TSPacketLabelSet _reset_labels;        // Labels to reset on output packets.

        // The ForkPipe is dynamically allocated to avoid reusing the same object when the command is restarted.
        typedef SafePtr<TSForkPipe> TSForkPipePtr;

        // Working data.
        bool          _got_eof;            // Got end of merged stream.
        volatile bool _stopping;           // Plugin stop in progress.
        PacketCounter _merged_count;       // Number of merged packets.
        PacketCounter _hold_count;         // Number of times we didn't try to merge to perform smoothing insertion.
        PacketCounter _empty_count;        // Number of times we could merge but there was no packet to merge.
        TSForkPipePtr _pipe;               // Executed command.
        TSPacketQueue _queue;              // TS packet queur from merge to main.
        PIDSet        _main_pids;          // Set of detected PID's in main stream.
        PIDSet        _merge_pids;         // Set of detected PID's in merged stream that we pass in main stream.
        PCRMerger     _pcr_merger;         // Adjust PCR's in merged stream.
        PSIMerger     _psi_merger;         // Used to merge PSI/SI from both streams.
        PacketInsertionController _insert_control;  // Used to control insertion points for the merge

        // Start/restart/stop the merge command.
        bool startStopCommand(bool do_close, bool do_start);

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
    _restart(false),
    _restart_interval(0),
    _user_bitrate(0),
    _allowed_pids(),
    _set_labels(),
    _reset_labels(),
    _got_eof(false),
    _stopping(false),
    _merged_count(0),
    _hold_count(0),
    _empty_count(0),
    _pipe(),
    _queue(),
    _main_pids(),
    _merge_pids(),
    _pcr_merger(duck),
    _psi_merger(duck, PSIMerger::NONE),
    _insert_control(*tsp)
{
    _insert_control.setMainStreamName(u"main stream");
    _insert_control.setSubStreamName(u"merged stream");

    DefineTSPacketFormatInputOption(*this, 'f');

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

    option<BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"Specify the target bitrate of the merged stream, in bits/seconds. "
         u"By default, the bitrate of the merged stream is computed from its PCR. "
         u"The bitrate of the merged stream is used to smoothen packet insertion "
         u"in the main stream.");

    option(u"drop", 'd', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"drop", u"pid[-pid]",
         u"Drop the specified PID or range of PID's from the merged stream. By "
         u"default, the PID's 0x00 to 0x1F are dropped and all other PID's are "
         u"passed. This can be modified using options --drop and --pass. Several "
         u"options --drop can be specified.");

    option(u"ignore-conflicts", 'i');
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

    option(u"max-queue", 'm', POSITIVE);
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
         u"PAT, CAT, SDT and EIT are merged so that the services from the merged stream "
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

    option(u"pass", 'p', PIDVAL, 0, UNLIMITED_COUNT);
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

    option(u"restart", 'r');
    help(u"restart",
         u"Restart the merge command whenever it terminates or fails. "
         u"By default, when packet insertion is complete, the transmission continues and the stuffing is no longer modified. "
         u"The options --restart and --terminate are mutually exclusive.");

    option(u"restart-interval", 0, POSITIVE);
    help(u"restart-interval", u"milliseconds",
         u"With --restart, specify the number of milliseconds to wait before restarting the merge command. "
         u"By default, with --restart, the merge command is restarted immediately after termination.");

    option(u"terminate");
    help(u"terminate",
        u"Terminate packet processing when the merged stream is terminated. "
        u"By default, when packet insertion is complete, the transmission continues and the stuffing is no longer modified. "
        u"The options --restart and --terminate are mutually exclusive.");

    option(u"transparent", 't');
    help(u"transparent",
         u"Pass all PID's without logical transformation. "
         u"Equivalent to --no-psi-merge --ignore-conflicts --pass 0x00-0x1F.");

    option(u"set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"set-label", u"label1[-label2]",
         u"Set the specified labels on the merged packets. "
         u"Apply to original packets from the merged stream only, not to updated PSI. "
         u"Several --set-label options may be specified.");

    option(u"reset-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
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
    getIntValue(_max_queue, u"max-queue", DEFAULT_MAX_QUEUED_PACKETS);
    getIntValue(_accel_threshold, u"acceleration-threshold", _max_queue / 2);
    _merge_psi = !transparent && !present(u"no-psi-merge");
    _pcr_restamp = !present(u"no-pcr-restamp");
    _incremental_pcr = present(u"incremental-pcr-restamp");
    _merge_smoothing = !present(u"no-smoothing");
    _ignore_conflicts = transparent || present(u"ignore-conflicts");
    _pcr_reset_backwards = present(u"pcr-reset-backwards");
    _terminate = present(u"terminate");
    _restart = present(u"restart");
    getIntValue(_restart_interval, u"restart-interval", 0);
    getValue(_user_bitrate, u"bitrate");
    tsp->useJointTermination(present(u"joint-termination"));
    getIntValues(_set_labels, u"set-label");
    getIntValues(_reset_labels, u"reset-label");
    _format = LoadTSPacketFormatInputOption(*this);

    if (_restart + _terminate + tsp->useJointTermination() > 1) {
        tsp->error(u"--restart, --terminate and --joint-termination are mutually exclusive");
        return false;
    }

    // Compute list of allowed PID's from the merged stream. Start with all PID's allowed.
    _allowed_pids.set();

    // By default (without --transparent), drop all base PSI/SI (PID 0x00 to 0x1F).
    if (!transparent) {
        for (PID pid = 0x00; pid <= PID_DVB_LAST; ++pid) {
            _allowed_pids.reset(pid);
        }
    }

    // Process --drop and --pass options.
    PIDSet pids;
    getIntValues(pids, u"drop");
    _allowed_pids &= ~pids;
    pids.reset();
    getIntValues(pids, u"pass");
    _allowed_pids |= pids;

    // By default (without --no-psi-merge), let the PSI Merger manage the packets from the merged PID's.
    if (_merge_psi) {
        // No need to allow PAT, CAT and SDT PID, they are nullified by the PSIMerger.
        // Need to keep EIT PID since the PSI merger balances EIT packets in both streams.
        _allowed_pids.set(PID_EIT);
    }

    return true;
}


//----------------------------------------------------------------------------
// Start/restart the merge command.
//----------------------------------------------------------------------------

bool ts::MergePlugin::startStopCommand(bool do_close, bool do_restart)
{
    // Multi-threading warning: Closing the pipe can be done from the main plugin thread while the merge
    // thread is reading the pipe or restarting the command (this method). Manipulating the safe pointer
    // is protected by an internal mutex. Here, the safe pointer shall never be set to a null pointer to
    // ensure that all calls are valid.

    if (do_close) {
        tsp->debug(u"closing merge process pipe");
        _pipe->close(*tsp);
    }

    if (_stopping || !do_restart) {
        // Stopping or no restart requested, stop here.
        return true;
    }

    // At this point, a start is requested.
    if (do_close) {
        // This is a restart, not a simple initial start. Optionally wait before restart.
        SleepThread(_restart_interval);
        // Because of the previous failure, we probably had error messages.
        // Inform the user that we restart and the error is not permanent.
        tsp->info(u"restarting merge command");
    }

    // Allocate the new object. Atomically swap the safe pointer. This action
    // will synchronously deallocate the previous object.
    _pipe = new TSForkPipe;
    CheckNonNull(_pipe.pointer());

    // Note on buffer size: we use DEFAULT_MAX_QUEUED_PACKETS instead of _max_queue
    // because this is the size of the system pipe buffer (Windows only). This is
    // a limited resource and we cannot let a user set an arbitrary large value for it.
    // The user can only change the queue size in tsp's virtual memory.

    // Start the command.
    return _pipe->open(_command,
                       _no_wait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                       PKT_SIZE * DEFAULT_MAX_QUEUED_PACKETS,
                       *tsp,
                       ForkPipe::STDOUT_PIPE,
                       ForkPipe::STDIN_NONE,
                       _format);
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

    // Configure the PCR merger.
    _pcr_merger.reset();
    _pcr_merger.setIncremental(_incremental_pcr);
    _pcr_merger.setResetBackwards(_pcr_reset_backwards);

    // Configure insertion control when somothing insertion.
    _insert_control.reset();
    _insert_control.setMainBitRate(tsp->bitrate());
    _insert_control.setSubBitRate(_user_bitrate); // zero if unspecified
    _insert_control.setWaitPacketsAlertThreshold(_accel_threshold);

    // Other states.
    _main_pids.reset();
    _merge_pids.reset();
    _merged_count = _hold_count = _empty_count = 0;
    _got_eof = false;
    _stopping = false;

    // Create pipe & process, then start the internal thread which receives the TS to merge.
    return startStopCommand(false, true) && Thread::start();
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
    _stopping = true;
    startStopCommand(true, false);

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
    bool success = true;
    while (success && !_queue.stopped()) {

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
        // Loop on error / restart.
        while (success && read_size == 0) {
            // Perform one read. Multi-threading warning: a close operation can occur
            // in the meantime (when the plugin stops) but no one will restart it.
            // So, the object which is pointed to by _pipe does not change.

            // We request to read only multiples of 188 bytes (the packet size).
            success = _pipe->readStreamChunks(buffer, PKT_SIZE * buffer_size, PKT_SIZE, read_size, *tsp);

            if (!success) {
                // Read error or end of file.
                if (_restart && !_stopping) {
                    success = startStopCommand(true, true);
                }
                else {
                    // Signal end-of-file to plugin thread.
                    _queue.setEOF();
                }
            }
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
    return pid == PID_NULL ? processMergePacket(pkt, pkt_data) : TSP_OK;
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

    // Adjust PCR when needed.
    if (_pcr_restamp) {
        _pcr_merger.processPacket(pkt, current_pkt, main_bitrate);
    }

    // Collect and merge PSI/SI when needed.
    if (_merge_psi) {
        _psi_merger.feedMergedPacket(pkt);
    }

    // Drop selected PID's from merged stream. Replace them with a null packet.
    const PID pid = pkt.getPID();
    if (!_allowed_pids.test(pid)) {
        return TSP_NULL;
    }

    // Check PID conflicts. EIT PID are already merged by the PSIMerger (without --no-psi-merge).
    if (!_ignore_conflicts && pid != PID_NULL && (pid != PID_EIT || !_merge_psi)) {
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

    // Apply labels on merged packets.
    pkt_data.setLabels(_set_labels);
    pkt_data.clearLabels(_reset_labels);

    return TSP_OK;
}
