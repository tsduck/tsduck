//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to extract T2-MI (DVB-T2 Modulator Interface) packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsT2MIDemux.h"
#include "tsTSFile.h"

namespace ts {
    //!
    //! Plugin to extract T2-MI (DVB-T2 Modulator Interface) packets.
    //! @see ETSI TS 102 775
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL T2MIPlugin: public ProcessorPlugin, private T2MIHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(T2MIPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Set of identified PLP's in a PID (with --identify).
        using PLPSet = std::bitset<256>;

        // Set of identified T2-MI PID's with their PLP's (with --identify).
        using IdentifiedSet = std::map<PID, PLPSet>;

        // Command line options:
        bool                   _extract = false;              // Extract encapsulated TS.
        bool                   _replace_ts = false;           // Replace transferred TS.
        bool                   _log = false;                  // Log T2-MI packets.
        bool                   _identify = false;             // Identify T2-MI PID's and PLP's in the TS or PID.
        std::optional<PID>     _original_pid {};              // Original value for --pid.
        std::optional<uint8_t> _original_plp {};              // Original value for --plp.
        TSFile::OpenFlags      _ts_file_flags = TSFile::NONE; // Open flags for output file.
        fs::path               _ts_file_name {};              // Output file name for extracted TS.
        fs::path               _t2mi_file_name {};            // Output file name for T2-MI packets.

        // Working data:
        bool                   _abort = false;       // Error, abort asap.
        std::optional<PID>     _extract_pid {};      // The PID containing the T2MI stream to extract.
        std::optional<uint8_t> _extract_plp {};      // The PLP to extract in that PID.
        TSFile                 _ts_file {this};      // Output file for extracted TS.
        std::ofstream          _t2mi_file {};        // Output file for extracted T2-MI packets.
        PacketCounter          _t2mi_count = 0;      // Number of input T2-MI packets.
        PacketCounter          _ts_count = 0;        // Number of extracted TS packets.
        T2MIDemux              _demux {duck, this};  // T2-MI demux.
        IdentifiedSet          _identified {};       // Map of identified PID's and PLP's.
        std::deque<TSPacket>   _ts_queue {};         // Queue of demuxed TS packets.

        // Inherited methods.
        virtual void handleT2MINewPID(T2MIDemux& demux, const PMT& pmt, PID pid, const T2MIDescriptor& desc) override;
        virtual void handleT2MIPacket(T2MIDemux& demux, const T2MIPacket& pkt) override;
        virtual void handleTSPacket(T2MIDemux& demux, const T2MIPacket& t2mi, const TSPacket& ts) override;
    };
}
