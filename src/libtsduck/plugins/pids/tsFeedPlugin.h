//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to extract an encapsulated TS from an outer feed TS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsTSFile.h"

namespace ts {
    //!
    //! Plugin to extract an encapsulated TS from an outer feed TS.
    //! This plugin is experimental and implements no particular specification.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL FeedPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(FeedPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        static constexpr uint8_t DEFAULT_SERVICE_TYPE = 0x80;   // Service type carrying an inner TS.
        static constexpr uint8_t DEFAULT_STREAM_TYPE  = 0x90;   // Stream type of a PID component carrying an inner TS.

        // Command line options:
        bool              _replace_ts = false;                  // Replace extracted TS.
        PID               _feed_pid = PID_NULL;                 // Original value for --pid.
        TSFile::OpenFlags _outfile_flags = TSFile::NONE;        // Open flags for output file.
        fs::path          _outfile_name {};                     // Output file name.
        uint8_t           _service_type = DEFAULT_SERVICE_TYPE; // Service type carrying an inner TS.
        uint8_t           _stream_type = DEFAULT_STREAM_TYPE;   // Service type carrying an inner TS.

        // Working data.
        bool              _abort = false;               // Error, abort asap.
        bool              _sync = false;                // Synchronized extraction of packets.
        uint8_t           _last_cc = 0xFF;              // Continuity counter from last packet in the PID.
        PID               _extract_pid = PID_NULL;      // PID carrying the T2-MI encapsulation.
        TSFile            _outfile {this};              // Output file for extracted stream.
        ByteBlock         _outdata {};                  // Output data buffer.
        SectionDemux      _demux {duck, this};          // A demux to extract all interesting tables.
        std::set<uint16_t>          _all_services {};   // All declared service ids in the TS.
        std::map<uint16_t, uint8_t> _service_types {};  // Service id -> service type.
        std::map<uint16_t, PID>     _service_pids {};   // Service id -> candidate PID.

        // Resynchronize the output buffer.
        void resyncBuffer();

        // Implementation of TableHandlerInterface
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;
    };
}
