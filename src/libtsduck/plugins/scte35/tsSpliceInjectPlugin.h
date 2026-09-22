//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to inject SCTE-35 splice commands in a transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSpliceInformationTable.h"
#include "tsServiceDiscovery.h"
#include "tsUDPReceiver.h"
#include "tsPollFiles.h"
#include "tsPacketizer.h"
#include "tsMessagePriorityQueue.h"
#include "tsThread.h"

namespace ts {
    //!
    //! Plugin to inject SCTE-35 splice commands in a transport stream.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SpliceInjectPlugin: public ProcessorPlugin, private SignalizationHandlerInterface, private SectionProviderInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SpliceInjectPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Default maximum number of sections in queue.
        static constexpr size_t DEFAULT_SECTION_QUEUE_SIZE = 100;

        // Default interval between two poll operations.
        static constexpr cn::milliseconds DEFAULT_POLL_INTERVAL = cn::milliseconds(500);

        // Default minimum file stability delay.
        static constexpr cn::milliseconds DEFAULT_MIN_STABLE_DELAY = cn::milliseconds(500);

        // Default start delay for non-immediate splice_insert() and time_signal() commands.
        static constexpr cn::milliseconds DEFAULT_START_DELAY = cn::milliseconds(2000);

        // Default inject interval for non-immediate splice_insert() and time_signal() commands.
        static constexpr cn::milliseconds DEFAULT_INJECT_INTERVAL = cn::milliseconds(800);

        // Default inject count for non-immediate splice_insert() and time_signal() commands.
        static constexpr size_t DEFAULT_INJECT_COUNT = 2;

        // Default max size for files.
        static constexpr std::uintmax_t DEFAULT_MAX_FILE_SIZE = 2048;

        // Stack size of listener threads.
        static constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;

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
        cn::milliseconds _start_delay {};
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

        class SpliceCommand: public StringifyInterface
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

        class FileListener: public Thread, private PollFilesListener
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

        class UDPListener: public Thread
        {
            TS_NOBUILD_NOCOPY(UDPListener);
        public:
            UDPListener(SpliceInjectPlugin* plugin);
            bool open();
            void stop();

        private:
            SpliceInjectPlugin* const _plugin;
            volatile bool _terminate = false;
            UDPReceiver   _client {_plugin};

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
        volatile bool           _wfb_received = false;   // First batch was received.
        std::mutex              _wfb_mutex {};           // Mutex waiting for _wfb_received.
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
