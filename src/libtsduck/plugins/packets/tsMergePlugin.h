//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to merge TS packets coming from the standard output of a command.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsPCRMerger.h"
#include "tsPSIMerger.h"
#include "tsTSForkPipe.h"
#include "tsTSPacketQueue.h"
#include "tsPacketInsertionController.h"
#include "tsThread.h"

namespace ts {
    //!
    //! Plugin to merge TS packets coming from the standard output of a command.
    //! @ingroup libtsduck plugin
    //!
    //! Definitions:
    //! - Main stream: the TS which is processed by tsp, including this plugin.
    //! - Merged stream: the additional TS which is read by this plugin through a pipe.
    //!
    class TSDUCKDLL MergePlugin: public ProcessorPlugin, private Thread
    {
        TS_PLUGIN_CONSTRUCTORS(MergePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Default size in packet of the inter-thread queue.
        static constexpr size_t DEFAULT_MAX_QUEUED_PACKETS = 1000;

        // Size in byte of the thread stack.
        static constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;

        // Command line options.
        UString          _command {};                                       // Command which generates the main stream.
        TSPacketFormat   _format = TSPacketFormat::AUTODETECT;              // Packet format on the pipe
        size_t           _max_queue = DEFAULT_MAX_QUEUED_PACKETS;           // Maximum number of queued packets.
        size_t           _accel_threshold = DEFAULT_MAX_QUEUED_PACKETS / 2; // Queue threshold after which insertion is accelerated.
        bool             _no_wait = false;              // Do not wait for command completion.
        bool             _merge_psi = false;            // Merge PSI/SI information.
        bool             _pcr_restamp = false;          // Restamp PCR from the merged stream.
        bool             _incremental_pcr = false;      // Use incremental method to restamp PCR's.
        bool             _merge_smoothing = false;      // Smoothen packet insertion.
        bool             _ignore_conflicts = false;     // Ignore PID conflicts.
        bool             _pcr_reset_backwards = false;  // Reset PCR restamping when DTS/PTD move backwards the PCR.
        bool             _terminate = false;            // Terminate processing after last merged packet.
        bool             _restart = false;              // Restart command after termination.
        cn::milliseconds _restart_interval {};          // Interval before restarting the merge command.
        BitRate          _user_bitrate = 0;             // User-specified bitrate of the merged stream.
        PIDSet           _allowed_pids {};              // List of PID's to merge (other PID's from the merged stream are dropped).
        TSPacketLabelSet _set_labels {};                // Labels to set on output packets.
        TSPacketLabelSet _reset_labels {};              // Labels to reset on output packets.

        // The ForkPipe is dynamically allocated to avoid reusing the same object when the command is restarted.
        using TSForkPipePtr = std::shared_ptr<TSForkPipe>;

        // Working data.
        bool          _got_eof = false;    // Got end of merged stream.
        volatile bool _stopping = false;   // Plugin stop in progress.
        PacketCounter _merged_count = 0;   // Number of merged packets.
        PacketCounter _hold_count = 0;     // Number of times we didn't try to merge to perform smoothing insertion.
        PacketCounter _empty_count = 0;    // Number of times we could merge but there was no packet to merge.
        TSForkPipePtr _pipe {};            // Executed command.
        TSPacketQueue _queue {};           // TS packet queue from merge to main.
        PIDSet        _main_pids {};       // Set of detected PID's in main stream.
        PIDSet        _merge_pids {};      // Set of detected PID's in merged stream that we pass in main stream.
        PCRMerger     _pcr_merger {duck};  // Adjust PCR's in merged stream.
        PSIMerger     _psi_merger {duck, PSIMerger::NONE};  // Used to merge PSI/SI from both streams.
        PacketInsertionController _insert_control {*this};  // Used to control insertion points for the merge

        // Start/restart/stop the merge command.
        bool startStopCommand(bool do_close, bool do_start);

        // There is one thread which receives packet from the created process and passes
        // them to the main plugin thread. The following method is the thread main code.
        virtual void main() override;

        // Process one packet coming from the merged stream.
        PacketProcessStatus processMergePacket(TSPacket&, TSPacketMetadata&);
    };
}
