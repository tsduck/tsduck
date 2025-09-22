//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Inject SCTE 35 splice commands in a transport stream.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSpliceInformationTable.h"
#include "tsServiceDiscovery.h"
#include "tsSectionFile.h"
#include "tsUDPReceiver.h"
#include "tsPollFiles.h"
#include "tsPacketizer.h"
#include "tsMessagePriorityQueue.h"
#include "tsThread.h"
#include "tsNullReport.h"
#include "tsReportBuffer.h"
#include "tsErrCodeReport.h"

namespace {
    // Default maximum number of sections in queue.
    constexpr size_t DEFAULT_SECTION_QUEUE_SIZE = 100;

    // Default interval between two poll operations.
    constexpr cn::milliseconds DEFAULT_POLL_INTERVAL = cn::milliseconds(500);

    // Default minimum file stability delay.
    constexpr cn::milliseconds DEFAULT_MIN_STABLE_DELAY = cn::milliseconds(500);

    // Default start delay for non-immediate splice_insert() and time_signal() commands.
    constexpr cn::milliseconds DEFAULT_START_DELAY = cn::milliseconds(2000);

    // Default inject interval for non-immediate splice_insert() and time_signal() commands.
    constexpr cn::milliseconds DEFAULT_INJECT_INTERVAL = cn::milliseconds(800);

    // Default inject count for non-immediate splice_insert() and time_signal() commands.
    constexpr size_t DEFAULT_INJECT_COUNT = 2;

    // Default max size for files.
    constexpr std::uintmax_t DEFAULT_MAX_FILE_SIZE = 2048;

    // Stack size of listener threads.
    constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;
}


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SpliceInjectPlugin:
        public ProcessorPlugin,
        private SignalizationHandlerInterface,
        private SectionProviderInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SpliceInjectPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool             _use_files = false;         // Use file polling input.
        bool             _use_udp = false;           // Use UDP input.
        bool             _delete_files = false;
        bool             _reuse_port = false;
        bool             _ignore_pid_conflict = false;
        bool             _wait_first_batch = false;  // Option --wait-first-batch (wfb).
        PID              _inject_pid_opt = PID_NULL; // PID for injection, as specified in cmmand line.
        PID              _pcr_pid_opt = PID_NULL;    // PID containing PCR's, as specified in cmmand line.
        PID              _pts_pid_opt = PID_NULL;    // PID containing PTS's, as specified in cmmand line.
        BitRate          _min_bitrate = 0;
        PacketCounter    _min_inter_packet = 0;
        UString          _files {};
        UString          _service_ref {};            // Service name or id.
        IPSocketAddress  _server_address {};
        size_t           _sock_buf_size = 0;
        size_t           _inject_count = 0;
        cn::milliseconds _inject_interval {};
        cn::milliseconds _start_delay = {};
        cn::milliseconds _poll_interval {};
        cn::milliseconds _min_stable_delay {};
        std::uintmax_t   _max_file_size = 0;
        size_t           _queue_size = 0;
        SectionPtr       _null_splice {};            // A null splice section to maintain PID bitrate.

        // The plugin contains two internal threads in addition to the packet processing thread.
        // One thread polls input files and another thread receives UDP messages.

        // ------------------------------------------
        // Splice command object as stored internally
        // ------------------------------------------

        class SpliceCommand : public StringifyInterface
        {
            TS_NOBUILD_NOCOPY(SpliceCommand);
        private:
            SpliceInjectPlugin* const _plugin;
        public:
            SpliceCommand(SpliceInjectPlugin* plugin, const SectionPtr& sec);

            SpliceInformationTable sit {};      // The analyzed Splice Information Table.
            SectionPtr section {};              // The binary SIT section.
            uint64_t   next_pts = INVALID_PTS;  // Next PTS after which the section shall be inserted (INVALID_PTS means immediate).
            uint64_t   last_pts = INVALID_PTS;  // PTS after which the section shall no longer be inserted (INVALID_PTS means never).
            uint64_t   interval = cn::duration_cast<PTS>(_plugin->_inject_interval).count(); // Interval between two insertions in PTS units.
            size_t     count = 1;               // Remaining number of injections.

            // A comparison function to sort commands in the queues.
            bool operator<(const SpliceCommand& other) const;

            // Implementation of StringifyInterface
            virtual UString toString() const override;
        };

        // Splice commands are passed from the server threads to the plugin thread using a message queue.
        // The next pts field is used as sort criteria. In the queue, all immediate commands come first.
        // Then, the non-immediate commands come in order of next_pts.
        using CommandQueue = MessagePriorityQueue<SpliceCommand>;

        // Message queues enqueue smart pointers to the message type.
        using CommandPtr = CommandQueue::MessagePtr;

        // --------------------
        // File listener thread
        // --------------------

        class FileListener : public Thread, private PollFilesListener
        {
            TS_NOBUILD_NOCOPY(FileListener);
        public:
            FileListener(SpliceInjectPlugin* plugin);
            void stop();

        private:
            SpliceInjectPlugin* const _plugin;
            volatile bool _terminate = false;
            PollFiles _poller {UString(), this, PollFiles::DEFAULT_POLL_INTERVAL, PollFiles::DEFAULT_MIN_STABLE_DELAY, *_plugin};

            // Implementation of Thread.
            virtual void main() override;

            // Implementation of PollFilesListener.
            virtual bool handlePolledFiles(const PolledFileList& files) override;
            virtual bool updatePollFiles(UString& wildcard, cn::milliseconds& poll_interval, cn::milliseconds& min_stable_delay) override;
        };

        // -------------------
        // UDP listener thread
        // -------------------

        class UDPListener : public Thread
        {
            TS_NOBUILD_NOCOPY(UDPListener);
        public:
            UDPListener(SpliceInjectPlugin* plugin);
            bool open();
            void stop();

        private:
            SpliceInjectPlugin* const _plugin;
            volatile bool _terminate = false;
            UDPReceiver _client {*_plugin};

            // Implementation of Thread.
            virtual void main() override;
        };

        // -------------------
        // Plugin working data
        // -------------------

        bool             _abort = false;                 // Error found, abort asap.
        bool             _pid_conflict_detected = false; // A PID conflict was detected with the splice PID.
        bool             _input_pids_checked = false;    // _input_pids has been checked right after finding _inject_pid_act.
        ServiceDiscovery _service {duck, this};          // Service holding the SCTE 35 injection.
        FileListener     _file_listener {this};          // File listener thread.
        UDPListener      _udp_listener {this};           // UDP listener thread.
        CommandQueue     _queue {};                      // Queue for splice commands.
        Packetizer       _packetizer {duck, PID_NULL, this};  // Packetizer for Splice Information sections.
        uint64_t         _last_pts = INVALID_PTS;        // Last PTS value from a clock reference.
        PID              _inject_pid_act = PID_NULL;     // PID for injection, actual.
        PID              _pcr_pid_act = PID_NULL;        // PID containing PCR's, actual.
        PID              _pts_pid_act = PID_NULL;        // PID containing PTS's, actual.
        PacketCounter    _last_inject_pkt = 0;           // Insertion point of last splice command packet.
        PacketCounter    _inter_packet = 0;              // Interval between two splice command packets (0 if none specified).
        PIDSet           _input_pids {};                 // Keep track of input PID's as long as splice PID is unknown.

        // Specific support for deterministic start (wfb = wait first batch, non-regression testing).
        volatile bool    _wfb_received = false;          // First batch was received.
        std::mutex       _wfb_mutex {};                  // Mutex waiting for _wfb_received.
        std::condition_variable _wfb_condition{};        // Condition waiting for _wfb_received.

        // Implementation of SignalizationHandlerInterface.
        virtual void handlePMT(const PMT&, PID) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;

        // Process a section file or message. Invoked from listener threads.
        void processSectionMessage(const uint8_t*, size_t);

        // Check if the specified PID creates a conflict with the inject PID. Return false on error.
        bool checkPIDConflict(PID pid);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"spliceinject", ts::SpliceInjectPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::SpliceInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject SCTE 35 splice commands in a transport stream", u"[options]")
{
    // Build a null splice command section for PID stuffing.
    SpliceInformationTable null_splice;
    null_splice.splice_command_type = SPLICE_NULL;
    BinaryTable bin_null_splice;
    null_splice.serialize(duck, bin_null_splice);
    assert(bin_null_splice.isValid());
    assert(bin_null_splice.sectionCount() > 0);
    _null_splice = bin_null_splice.sectionAt(0);

    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    setIntro(u"The splice commands are injected as splice information sections, as defined by "
             u"the SCTE 35 standard. All forms of splice information sections can be injected. "
             u"The sections shall be provided by some external equipment, in real time. The "
             u"format of the section can be binary, XML or JSON. There are two possible mechanisms "
             u"to provide the sections: files or UDP.\n"
             u"\n"
             u"Files shall be specified as one single specification with optional wildcards. "
             u"Example: --files '/path/to/dir/*'. All files which are copied or updated into "
             u"this directory are automatically loaded and injected. It is possible to automatically "
             u"delete all files after being loaded.\n"
             u"\n"
             u"UDP datagrams shall contain exactly one XML or JSON document or binary sections. "
             u"The sections are injected upon reception.");

    option(u"buffer-size", 0, UNSIGNED);
    help(u"buffer-size",
         u"Specifies the UDP socket receive buffer size in bytes (socket option).");

    option(u"delete-files", 'd');
    help(u"delete-files",
         u"Specifies that the input files should be deleted after being loaded. By default, "
         u"the files are left unmodified after being loaded. When a loaded file is "
         u"modified later, it is reloaded and re-injected.");

    option(u"files", 'f', FILENAME);
    help(u"files", u"'file-wildcard'",
         u"A file specification with optional wildcards indicating which files should be polled. "
         u"When such a file is created or updated, it is loaded and its "
         u"content is interpreted as binary, XML or JSON tables. "
         u"All tables shall be splice information tables.");

    option(u"ignore-pid-conflict");
    help(u"ignore-pid-conflict",
         u"Ignore conflicts between the splice PID (option --pid) and an existing identical PID. "
         u"By default, this is an error if existing input packets have the same PID as the splice PID. "
         u"Using this option may lead to incorrect streams, use with care. "
         u"A verbose-level message is still displayed the first time a PID conflict is detected.");

    option(u"inject-count", 0, UNSIGNED);
    help(u"inject-count",
         u"For non-immediate splice_insert() and time_signal() commands, specifies the number of times "
         u"the same splice information section is injected. The default is " +
         UString::Decimal(DEFAULT_INJECT_COUNT) + u". "
         u"Other splice commands are injected once only.");

    option<cn::milliseconds>(u"inject-interval");
    help(u"inject-interval",
         u"For non-immediate splice_insert() and time_signal() commands, specifies the interval "
         u"in milliseconds between two insertions of the same splice information section. "
         u"The default is " + UString::Chrono(DEFAULT_INJECT_INTERVAL) + u".");

    option(u"max-file-size", 0, UNSIGNED);
    help(u"max-file-size",
         u"Files larger than the specified size are ignored. This avoids loading "
         u"large spurious files which could clutter memory. The default is " +
         UString::Decimal(DEFAULT_MAX_FILE_SIZE) + u" bytes.");

    option<BitRate>(u"min-bitrate");
    help(u"min-bitrate",
         u"The minimum bitrate to maintain in the PID carrying the splice information tables. "
         u"By default, the PID remains inactive when there is no splice information. "
         u"If this is a problem for monitoring tools, an artificial bitrate can be maintained using null splice commands.");

    option(u"min-inter-packet", 0, UNSIGNED);
    help(u"min-inter-packet",
         u"This option can be used instead of --min-bitrate when the bitrate of the transport stream is unknown or unreliable. "
         u"The specified value is the number of TS packets between two splice commands.");

    option<cn::milliseconds>(u"min-stable-delay");
    help(u"min-stable-delay",
         u"A file size needs to be stable during that duration for the file to be reported as added or modified. "
         u"This prevents too frequent poll notifications when a file is being written and his size modified at each poll. "
         u"The default is " + UString::Chrono(DEFAULT_MIN_STABLE_DELAY, true) + u".");

    option(u"no-reuse-port");
    help(u"no-reuse-port",
         u"Disable the reuse port socket option. Do not use unless completely necessary.");

    option(u"pcr-pid", 0, PIDVAL);
    help(u"pcr-pid",
         u"Specifies the PID carrying PCR reference clock. By default, use the PCR "
         u"PID as declared in the PMT of the service.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specifies the PID for the injection of the splice information tables. By "
         u"default, the injection of splice commands is done in the component of the "
         u"service with a stream type equal to 0x86 in the PMT, as specified by SCTE 35 "
         u"standard.");

    option<cn::milliseconds>(u"poll-interval");
    help(u"poll-interval",
         u"Specifies the interval between two poll operations. "
         u"The default is " + UString::Chrono(DEFAULT_POLL_INTERVAL, true) + u".");

    option(u"pts-pid", 0, PIDVAL);
    help(u"pts-pid",
         u"Specifies the PID carrying PTS reference clock. By default, use the video "
         u"PID as declared in the PMT of the service.");

    option(u"queue-size", 0, UINT32);
    help(u"queue-size",
         u"Specifies the maximum number of sections in the internal queue, sections "
         u"which are received from files or UDP but not yet inserted into the TS. "
         u"The default is " + UString::Decimal(DEFAULT_SECTION_QUEUE_SIZE) + u".");

    option(u"reuse-port", 'r');
    help(u"reuse-port",
         u"Set the reuse port socket option. This is now enabled by default, the option "
         u"is present for legacy only.");

    option(u"service", 's', STRING);
    help(u"service",
         u"Specifies the service for the insertion of the splice information tables. "
         u"If the argument is an integer value (either decimal or hexadecimal), it is interpreted as a service id. "
         u"If it is an empty string or \"-\", the first service in the PAT is selected. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored. "
         u"If the input TS does not contain an SDT, use service ids only. "
         u"If no service is specified, the options --pid and --pts-pid must be specified (--pcr-pid is optional).");

    option<cn::milliseconds>(u"start-delay");
    help(u"start-delay",
         u"For non-immediate splice_insert() commands, start to insert the first "
         u"section this number of milliseconds before the specified splice PTS value. "
         u"The default is " + UString::Chrono(DEFAULT_START_DELAY) + u".");

    option(u"udp", 'u', IPSOCKADDR_OA);
    help(u"udp",
         u"Specifies the local UDP port on which the plugin listens for incoming "
         u"binary or XML splice information tables. When present, the optional "
         u"address shall specify a local IP address or host name (by default, the "
         u"plugin accepts connections on any local IP interface).");

    option(u"wait-first-batch", 'w');
    help(u"wait-first-batch",
         u"When this option is specified, the start of the plugin is suspended until "
         u"the first batch of splice commands is loaded and queued. Without this option, "
         u"the input files or messages are loaded and queued asynchronously.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::getOptions()
{
    duck.loadArgs(*this);
    getValue(_service_ref, u"service");
    getValue(_files, u"files");
    _use_files = !_files.empty();
    _use_udp = present(u"udp");
    getSocketValue(_server_address, u"udp");
    getIntValue(_inject_pid_opt, u"pid", PID_NULL);
    getIntValue(_pcr_pid_opt, u"pcr-pid", PID_NULL);
    getIntValue(_pts_pid_opt, u"pts-pid", PID_NULL);
    getValue(_min_bitrate, u"min-bitrate");
    getIntValue(_min_inter_packet, u"min-inter-packet");
    _delete_files = present(u"delete-files");
    _reuse_port = !present(u"no-reuse-port");
    getIntValue(_sock_buf_size, u"buffer-size");
    getIntValue(_inject_count, u"inject-count", DEFAULT_INJECT_COUNT);
    getChronoValue(_inject_interval, u"inject-interval", DEFAULT_INJECT_INTERVAL);
    getChronoValue(_start_delay, u"start-delay", DEFAULT_START_DELAY);
    getIntValue(_max_file_size, u"max-file-size", DEFAULT_MAX_FILE_SIZE);
    getChronoValue(_poll_interval, u"poll-interval", DEFAULT_POLL_INTERVAL);
    getChronoValue(_min_stable_delay, u"min-stable-delay", DEFAULT_MIN_STABLE_DELAY);
    getIntValue(_queue_size, u"queue-size", DEFAULT_SECTION_QUEUE_SIZE);
    _wait_first_batch = present(u"wait-first-batch");
    _ignore_pid_conflict = present(u"ignore-pid-conflict");

    // We need either a service or specified PID's.
    if (_service_ref.empty() && (_inject_pid_opt == PID_NULL || _pts_pid_opt == PID_NULL)) {
        error(u"specify --service or --pid and --pts-pid");
        return false;
    }

    // We need at least one of --files and --udp.
    if (!_use_files && !_use_udp) {
        error(u"specify at least one of --files and --udp");
        return false;
    }

    // At most one way of specifying the splice bitrate.
    if (_min_bitrate > 0 && _min_inter_packet > 0) {
        error(u"specify at most one of --min-bitrate and --min-inter-packet");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::start()
{
    // The reference PID's can be taken from the command line or discovered later.
    _inject_pid_act = _inject_pid_opt;
    _pcr_pid_act = _pcr_pid_opt;
    _pts_pid_act = _pts_pid_opt;

    // PID conflict detection.
    _pid_conflict_detected = false;
    _input_pids_checked = _inject_pid_act != PID_NULL;
    _input_pids.reset();

    // Interval between two splice command packets.
    const BitRate initial_bitrate = tsp->bitrate();
    if (_min_bitrate > 0 && initial_bitrate > 0) {
        _inter_packet = std::max<PacketCounter>(1, (initial_bitrate / _min_bitrate).toInt());
    }
    else {
        _inter_packet = _min_inter_packet;
    }

    // Initialize service discovery.
    _service.clear();
    _service.set(_service_ref);

    // The packetizer generates packets for the inject PID.
    _packetizer.reset();
    _packetizer.setPID(_inject_pid_act);

    // Tune the section queue.
    _queue.clear();
    _queue.setMaxMessages(_queue_size);

    // Clear the "first message received" flag.
    _wfb_received = false;

    // Initialize the UDP receiver.
    if (_use_udp) {
        if (!_udp_listener.open()) {
            return false;
        }
        _udp_listener.start();
    }

    // Start the file polling.
    if (_use_files) {
        _file_listener.start();
    }

    _last_inject_pkt = 0;
    _last_pts = INVALID_PTS;
    _abort = false;

    // If --wait-first-batch was specified, suspend until a first batch of commands is queued.
    if (_wait_first_batch) {
        verbose(u"waiting for first batch of commands");
        std::unique_lock<std::mutex> lock(_wfb_mutex);
        _wfb_condition.wait(lock, [this]() { return _wfb_received; });
        verbose(u"received first batch of commands");
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::stop()
{
    // Stop the internal threads.
    if (_use_files) {
        _file_listener.stop();
    }
    if (_use_udp) {
        _udp_listener.stop();
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SpliceInjectPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Detect PID conflict with injection PID.
    const PID pid = pkt.getPID();
    checkPIDConflict(pid);

    // Feed the service finder with the packet as long as the required PID's are not found.
    if (_inject_pid_act == PID_NULL || _pts_pid_act == PID_NULL) {
        _service.feedPacket(pkt);
        if (_service.nonExistentService()) {
            return TSP_END;
        }
    }

    // Abort in case of error.
    if (_abort) {
        return TSP_END;
    }

    if (pid == PID_NULL) {
        // Replace null packets with splice information section data, when available.
        if (_packetizer.getNextPacket(pkt)) {
            // Remember position of last injected packet.
            _last_inject_pkt = tsp->pluginPackets();
        }
    }
    else if (pid == _pts_pid_act) {
        if (pkt.hasPTS()) {
            // Get a PTS from the PTS clock reference.
            _last_pts = pkt.getPTS();
        }
        else if (pkt.hasPCR()) {
            // If there is no PTS but a PCR is present, use it.
            _last_pts = pkt.getPCR() / SYSTEM_CLOCK_SUBFACTOR;
        }
    }
    else if (pid == _pcr_pid_act && pkt.hasPCR()) {
        // Get a PCR from the PCR clock reference.
        _last_pts = pkt.getPCR() / SYSTEM_CLOCK_SUBFACTOR;
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Check if the specified PID creates a conflict with the inject PID.
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::checkPIDConflict(PID pid)
{
    // Keep track of input PID's as long as the splice PID is unknown.
    if (_inject_pid_act == PID_NULL) {
        _input_pids.set(pid);
        return true;
    }

    // First time after _inject_pid_act becomes non-null.
    if (!_input_pids_checked) {
        _input_pids_checked = true;
        if (_input_pids.test(_inject_pid_act)) {
            _pid_conflict_detected = true;
            log(_ignore_pid_conflict ? Severity::Verbose : Severity::Error, u"PID conflict: PID %n for SCTE-35 injection was previously detected in input stream", _inject_pid_act);
            if (!_ignore_pid_conflict) {
                _abort = true;
                return false;
            }
        }
    }

    // Finally check if the specified PID conflicts.
    if (pid == _inject_pid_act) {
        if (!_ignore_pid_conflict) {
            error(u"PID conflict: PID %n for SCTE-35 injection detected in input stream", pid);
            _abort = true;
            return false;
        }
        if (!_pid_conflict_detected) {
            _pid_conflict_detected = true;
            verbose(u"PID conflict: PID %n for SCTE-35 injection detected in input stream", pid);
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Invoked when the PMT of the service is found.
// Implementation of SignalizationHandlerInterface.
//----------------------------------------------------------------------------

void ts::SpliceInjectPlugin::handlePMT(const PMT& pmt, PID pid)
{
    // Get the PID with PCR's.
    if (_pcr_pid_act == PID_NULL) {
        _pcr_pid_act = pmt.pcr_pid;
    }

    // Inspect all components.
    for (const auto& it : pmt.streams) {
        // By default, PTS are taken from the first video PID.
        if (_pts_pid_act == PID_NULL && it.second.isVideo(duck)) {
            _pts_pid_act = it.first;
        }
        // Look for a component with a stream type 0x86.
        if (_inject_pid_act == PID_NULL && it.second.stream_type == ST_SCTE35_SPLICE) {
            // Found an SCTE 35 splice information stream, use its PID.
            _inject_pid_act = it.first;
            _packetizer.setPID(_inject_pid_act);
            checkPIDConflict(pid);
        }
    }

    // If PTS PID is missing, use the PCR one.
    if (_pts_pid_act == PID_NULL) {
        _pts_pid_act = _pcr_pid_act;
    }

    // If no PID is found for clock reference or splice command injection, abort.
    if (_inject_pid_act == PID_NULL) {
        error(u"could not find an SCTE 35 splice information stream in service, use option --pid");
        _abort = true;
    }
    if (_pts_pid_act == PID_NULL) {
        error(u"could not find a PID with PCR or PTS in service, use option --pts-pid");
        _abort = true;
    }
}


//----------------------------------------------------------------------------
// Invoked when a new splice information section is required.
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

void ts::SpliceInjectPlugin::provideSection(SectionCounter counter, SectionPtr& section)
{
    // The default is to return no section, meaning do not insert splice information.
    section.reset();

    // If injection PID is unknown or if we have no time reference, do nothing.
    if (_inject_pid_act == PID_NULL || _last_pts == INVALID_PTS) {
        return;
    }

    // Loop on queued splice commands.
    for (;;) {

        // Get next splice command from the queue.
        CommandPtr cmd(_queue.peek());
        if (cmd == nullptr) {
            // No splice command available, nothing to do.
            break;
        }
        assert(cmd->sit.isValid());

        // If the command has a termination PTS and this PTS is in the past,
        // drop the command and loop on next command from the queue.
        if (cmd->last_pts != INVALID_PTS && SequencedPTS(cmd->last_pts, _last_pts)) {
            CommandPtr cmd2;
            [[maybe_unused]] const bool dequeued = _queue.dequeue(cmd2, cn::milliseconds::zero());
            assert(dequeued);
            assert(cmd2 == cmd);
            verbose(u"dropping %s, obsolete, current PTS: 0x%09X", *cmd2, _last_pts);
        }
        else {
            // Give up if the command is not immediate and not yet ready to start.
            if (cmd->next_pts != INVALID_PTS && SequencedPTS(_last_pts, cmd->next_pts)) {
                break;
            }

            // We must process this command, remove it from the queue.
            CommandPtr cmd2;
            [[maybe_unused]] const bool dequeued = _queue.dequeue(cmd2, cn::milliseconds::zero());
            assert(dequeued);
            assert(cmd2 == cmd);

            // Now we have a section to send.
            section = cmd->section;
            verbose(u"injecting %s, current PTS: 0x%09X", *cmd, _last_pts);

            // If the command must be repeated, compute next PTS and requeue.
            if (cmd->count > 1) {
                cmd->count--;
                cmd->next_pts = (cmd->next_pts + cmd->interval) & PTS_DTS_MASK;
                if (SequencedPTS(cmd->next_pts, cmd->last_pts)) {
                    // The next PTS is still in range, requeue at the next position.
                    verbose(u"requeueing %s", *cmd);
                    _queue.forceEnqueue(cmd);
                }
            }
            break;
        }
    }

    // Recompute inter-packet interval based on bitrate.
    if (_min_bitrate > 0) {
        const BitRate current_bitrate = tsp->bitrate();
        if (current_bitrate > 0) {
            _inter_packet = std::max<PacketCounter>(1, (current_bitrate / _min_bitrate).toInt());
        }
    }

    // Inject null splice commands when necessary to fill the PID.
    if (section == nullptr && _inter_packet > 0 && tsp->pluginPackets() >= _last_inject_pkt + _inter_packet) {
        // It is time to insert a null splice command.
        section = _null_splice;
    }
}


//----------------------------------------------------------------------------
// Shall we perform section stuffing.
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::doStuffing()
{
    // Splice Information Table are rare and mostly contained in one or two
    // TS packets. We always stuff to the end of packets after a section.
    return true;
}


//----------------------------------------------------------------------------
// Process a section message. Invoked from listener threads.
//----------------------------------------------------------------------------

void ts::SpliceInjectPlugin::processSectionMessage(const uint8_t* addr, size_t size)
{
    assert(addr != nullptr);

    // Try to determine the file type, binary or XML.
    ts::SectionFormat type = ts::SectionFormat::UNSPECIFIED;
    if (size > 0) {
        if (addr[0] == TID_SCTE35_SIT) {
            // First byte is the table id of a splice information table.
            type = ts::SectionFormat::BINARY;
        }
        else if (addr[0] == '<') {
            // Typically the start of an XML definition.
            type = ts::SectionFormat::XML;
        }
        else if (addr[0] == '{' || addr[0] == '[') {
            // Typically the start of a JSON definition.
            type = ts::SectionFormat::JSON;
        }
        else {
            // We need to search a bit more. First, skip UTF-8 BOM if present.
            if (size >= UString::UTF8_BOM_SIZE && MemEqual(addr, UString::UTF8_BOM, UString::UTF8_BOM_SIZE)) {
                addr += UString::UTF8_BOM_SIZE;
                size -= UString::UTF8_BOM_SIZE;
            }
            // Then skip anything like a space.
            while (size > 0 && (addr[0] == ' ' || addr[0] == '\n' || addr[0] == '\r' || addr[0] == '\t')) {
                addr++;
                size--;
            }
            // Does this look like XML or JSON now ?
            if (size > 0) {
                if (addr[0] == '<') {
                    type = ts::SectionFormat::XML;
                }
                else if (addr[0] == '{' || addr[0] == '[') {
                    type = ts::SectionFormat::JSON;
                }
            }
        }
    }

    // Give up if we cannot find a valid format.
    if (type == ts::SectionFormat::UNSPECIFIED) {
        error(u"cannot find received data type, %d bytes, %s ...", size, UString::Dump(addr, std::min<size_t>(size, 8), UString::SINGLE_LINE));
        return;
    }

    // Consider the memory as a C++ input stream.
    std::istringstream strm(std::string(reinterpret_cast<const char*>(addr), size));
    if (debug()) {
        debug(u"parsing section:\n%s", UString::Dump(addr, size, UString::HEXA | UString::ASCII, 4));
    }

    // Analyze the message as a binary, XML or JSON section file.
    SectionFile secFile(duck);
    if (!secFile.load(strm, type)) {
        // Error loading sections, error message already reported.
        return;
    }

    // Loop on all sections in the file or message.
    // Each section is expected to be a splice information section.
    for (auto it = secFile.sections().begin(); it != secFile.sections().end(); ++it) {
        SectionPtr sec(*it);
        if (sec != nullptr) {
            if (sec->tableId() != TID_SCTE35_SIT) {
                error(u"unexpected section, %s, ignored", TIDName(duck, sec->tableId(), sec->sourcePID(), CASID_NULL, NamesFlags::NAME_VALUE));
            }
            else {
                CommandPtr cmd(new SpliceCommand(this, sec));
                if (cmd == nullptr || !cmd->sit.isValid()) {
                    error(u"received invalid splice information section, ignored");
                }
                else {
                    verbose(u"enqueuing %s", *cmd);
                    if (!_queue.enqueue(cmd, cn::milliseconds::zero())) {
                        warning(u"queue overflow, dropped one section");
                    }
                }
            }
        }
    }

    // If --wait-first-batch was specified, signal when the first batch of commands is queued.
    if (_wait_first_batch && !_wfb_received) {
        std::lock_guard<std::mutex> lock(_wfb_mutex);
        _wfb_received = true;
        _wfb_condition.notify_one();
    }
}


//----------------------------------------------------------------------------
// Splice command object constructor
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::SpliceCommand::SpliceCommand(SpliceInjectPlugin* plugin, const SectionPtr& sec) :
    _plugin(plugin),
    section(sec)
{
    // Analyze the section.
    if (section == nullptr || !section->isValid()) {
        // Not a valid section.
        sit.invalidate();
    }
    else {
        // Try to interpret the section as a SIT.
        BinaryTable table;
        table.addSection(section, false, false);
        sit.deserialize(_plugin->duck, table);
    }

    // The initial values for the member fields are set for one immediate injection.
    // This must be changed for non-immediate splice_insert() and time_signal() commands.
    if (sit.isValid() &&
        ((sit.splice_command_type == SPLICE_TIME_SIGNAL && sit.time_signal.has_value()) ||
         (sit.splice_command_type == SPLICE_INSERT && !sit.splice_insert.canceled && !sit.splice_insert.immediate)))
    {
        // Compute the splice event PTS value. This will be the last time for
        // the splice command injection since the event is obsolete afterward.
        if (sit.splice_command_type == SPLICE_INSERT) {
            if (sit.splice_insert.programSplice()) {
                // Common PTS value, program-wide.
                if (sit.splice_insert.program_pts.has_value()) {
                    last_pts = sit.splice_insert.program_pts.value();
                }
            }
            else {
                // Compute the earliest PTS in all components.
                for (const auto& it : sit.splice_insert.components_pts) {
                    if (it.second.has_value()) {
                        if (last_pts == INVALID_PTS || SequencedPTS(it.second.value(), last_pts)) {
                            last_pts = it.second.value();
                        }
                    }
                }
            }
        }
        else if (sit.splice_command_type == SPLICE_TIME_SIGNAL) {
            last_pts = sit.time_signal.value();
        }
        // If we could not find the event PTS, keep one single immediate injection.
        // Otherwise, compute initial PTS and injection count.
        if (last_pts != INVALID_PTS) {
            last_pts = (last_pts + sit.pts_adjustment) & PTS_DTS_MASK;
            count = _plugin->_inject_count;
            // Preceding delay for injection in PTS units.
            const uint64_t preceding = cn::duration_cast<ts::PTS>(_plugin->_start_delay).count();
            // Compute the first PTS time for injection.
            next_pts = (last_pts - preceding) & PTS_DTS_MASK;
        }
    }
}


//----------------------------------------------------------------------------
// SpliceCommand comparison function to sort commands in the queues.
// The next pts field is used as sort criteria. In the queue, all immediate
// commands come first (always "less" than non-immediate ones). Then, the
// non-immediate commands come in order of next_pts.
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::SpliceCommand::operator<(const SpliceCommand& other) const
{
    if (next_pts == other.next_pts) {
        // Either both elements are immediate or non-immediate with same starting point.
        // We always consider this object greater than other so that messages with equal
        // starting points are queued in order of appearance.
        return false;
    }
    else if (next_pts == INVALID_PTS) {
        // This object is immediate, other is not.
        return true;
    }
    else {
        // This object is not immediate.
        return other.next_pts != INVALID_PTS && next_pts < other.next_pts;
    }
}


//----------------------------------------------------------------------------
// SpliceCommand string conversion for debug.
//----------------------------------------------------------------------------

ts::UString ts::SpliceInjectPlugin::SpliceCommand::toString() const
{
    if (section == nullptr) {
        return u"null";
    }
    else if (!sit.isValid()) {
        return u"invalid";
    }
    else {
        // Command name.
        UString name(NameFromSection(u"dtv", u"SpliceCommandType", sit.splice_command_type));
        if (sit.splice_command_type == SPLICE_INSERT) {
            name.append(sit.splice_insert.splice_out ? u" out" : u" in");
        }
        if (sit.splice_command_type == SPLICE_INSERT &&
            !sit.splice_insert.canceled &&
            sit.splice_insert.programSplice() &&
            sit.splice_insert.program_pts.has_value())
        {
            name.append(UString::Format(u" @0x%09X", sit.splice_insert.program_pts.value()));
        }
        if (next_pts == INVALID_PTS) {
            name.append(u", immediate");
        }
        else {
            name.append(UString::Format(u", start: 0x%09X", next_pts));
        }
        if (last_pts != INVALID_PTS) {
            name.append(UString::Format(u", end: 0x%09X", last_pts));
        }
        if (count > 1) {
            name.append(UString::Format(u", %s times", count));
        }
        return name;
    }
}


//----------------------------------------------------------------------------
// File listener thread.
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::FileListener::FileListener(SpliceInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin)
{
}

// Terminate the thread.
void ts::SpliceInjectPlugin::FileListener::stop()
{
    // Will be used at next poll.
    _terminate = true;

    // Wait for actual thread termination
    Thread::waitForTermination();
}


// Invoked in the context of the server thread.
void ts::SpliceInjectPlugin::FileListener::main()
{
    _plugin->debug(u"file server thread started");

    _poller.setFileWildcard(_plugin->_files);
    _poller.setPollInterval(_plugin->_poll_interval);
    _poller.setMinStableDelay(_plugin->_min_stable_delay);
    _poller.pollRepeatedly();

    _plugin->debug(u"file server thread completed");
}

// Invoked before polling.
bool ts::SpliceInjectPlugin::FileListener::updatePollFiles(UString& wildcard, cn::milliseconds& poll_interval, cn::milliseconds& min_stable_delay)
{
    return !_terminate;
}

// Invoked with modified files.
bool ts::SpliceInjectPlugin::FileListener::handlePolledFiles(const PolledFileList& files)
{
    // Loop on all changed files.
    for (const auto& it : files) {
        const PolledFile& file(*it);
        if (file.getStatus() == PolledFile::ADDED || file.getStatus() == PolledFile::MODIFIED) {
            // Process added or modified files.
            const UString name(file.getFileName());
            ByteBlock data;
            if (file.getSize() != FS_ERROR && file.getSize() > _plugin->_max_file_size) {
                _plugin->warning(u"file %s is too large, %'d bytes, ignored", name, file.getSize());
            }
            else if (data.loadFromFile(name, size_t(_plugin->_max_file_size), _plugin)) {
                // File correctly loaded, ingest it.
                _plugin->verbose(u"loaded file %s, %d bytes", name, data.size());
                _plugin->processSectionMessage(data.data(), data.size());

                // Delete file after successful load when required.
                if (_plugin->_delete_files) {
                    fs::remove(name, &ErrCodeReport(*_plugin, u"error deleting", name));
                }
            }
        }
    }
    return !_terminate;
}


//----------------------------------------------------------------------------
// UDP listener thread.
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::UDPListener::UDPListener(SpliceInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin)
{
}

// Open the UDP socket.
bool ts::SpliceInjectPlugin::UDPListener::open()
{
    UDPReceiverArgs args;
    args.setUnicast(_plugin->_server_address, _plugin->_reuse_port, _plugin->_sock_buf_size);
    _client.setParameters(args);
    return _client.open(*_plugin);
}

// Terminate the thread.
void ts::SpliceInjectPlugin::UDPListener::stop()
{
    // Close the UDP receiver.
    // This will force the server thread to terminate.
    _terminate = true;
    _client.close(NULLREP);

    // Wait for actual thread termination
    Thread::waitForTermination();
}

// Invoked in the context of the server thread.
void ts::SpliceInjectPlugin::UDPListener::main()
{
    _plugin->debug(u"UDP server thread started");

    uint8_t inbuf[65536];
    size_t insize = 0;
    IPSocketAddress sender;
    IPSocketAddress destination;

    // Get receive errors in a buffer since some errors are normal.
    ReportBuffer<ThreadSafety::None> error(_plugin->maxSeverity());

    // Loop on incoming messages.
    while (_client.receive(inbuf, sizeof(inbuf), insize, sender, destination, _plugin->tsp, error)) {
        _plugin->verbose(u"received message, %d bytes, from %s", insize, sender);
        _plugin->processSectionMessage(inbuf, insize);
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.empty()) {
        _plugin->info(error.messages());
    }

    _plugin->debug(u"UDP server thread completed");
}
