//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to inject tables into a transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsCyclingPacketizer.h"
#include "tsFileNameRateList.h"
#include "tsSectionFileArgs.h"

namespace ts {
    //!
    //! Plugin to inject tables into a transport stream, replacing a PID or stealing packets from stuffing.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL InjectPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(InjectPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // To avoid long prefixes
        using StuffPolicy = CyclingPacketizer::StuffingPolicy;

        // Default packet interval to re-evaluate bitrate.
        static constexpr PacketCounter DEFAULT_EVALUATE_INTERVAL = 100;

        // Default interval to poll files.
        static constexpr cn::milliseconds DEFAULT_POLL_FILE = cn::milliseconds(1000);

        // Number of retries to open files
        static constexpr size_t FILE_RETRY = 3;

        // Command line options:
        FileNameRateList  _infiles {};                // Input file names and repetition rates
        SectionFormat     _intype = SectionFormat::UNSPECIFIED; // Input files type
        SectionFileArgs   _sections_opt {};           // Section processing options
        bool              _specific_rates = false;    // Some input files have specific repetition rates
        bool              _undefined_rates = false;   // At least one file has no specific repetition rate.
        bool              _use_files_bitrate = false; // Use the bitrate from the repetition rates in files
        PID               _inject_pid = PID_NULL;     // Target PID
        CRC32::Validation _crc_op = CRC32::CHECK;     // Validate/recompute CRC32
        StuffPolicy       _stuffing_policy = StuffPolicy::NEVER; // Stuffing policy at end of section or cycle
        bool              _replace = false;           // Replace existing PID content
        bool              _terminate = false;         // Terminate processing when insertion is complete
        bool              _poll_files = false;        // Poll the presence of input files at regular intervals
        cn::milliseconds  _poll_files_ms = DEFAULT_POLL_FILE; // Interval between two file polling, currently hard-coded
        size_t            _repeat_count = 0;          // Repeat cycle, zero means infinite
        BitRate           _pid_bitrate = 0;           // Target bitrate for new PID
        PacketCounter     _pid_inter_pkt = 0;         // # TS packets between 2 new PID packets
        PacketCounter     _eval_interval = 0;         // PID bitrate re-evaluation interval

        // Working data:
        Time              _poll_file_next {};         // Next UTC time of poll file
        bool              _completed = false;         // Last cycle terminated
        BitRate           _files_bitrate = 0;         // Bitrate from the repetition rates in files
        PacketCounter     _pid_next_pkt = 0;          // Next time to insert a packet
        PacketCounter     _packet_count = 0;          // TS packet counter
        PacketCounter     _pid_packet_count = 0;      // Packet counter in -PID to replace
        PacketCounter     _cycle_count = 0;           // Number of insertion cycles
        CyclingPacketizer _pzer {duck, PID_NULL, StuffPolicy::NEVER};

        // Reload files, reset packetizer. Return true on success, false on error.
        bool reloadFiles();

        // Process bitrates and compute inter-packet distance.
        bool processBitRates();

        // Replace current packet with one from the packetizer.
        void replacePacket(TSPacket& pkt);
    };
}
