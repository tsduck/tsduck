//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to extract a TS from MPE (Multi-Protocol Encapsulation).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSingleMPEPlugin.h"

namespace ts {
    //!
    //! Plugin to extract a TS from MPE (Multi-Protocol Encapsulation).
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL MPEExtractPlugin: public AbstractSingleMPEPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(MPEExtractPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;
        virtual void handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe) override;

    private:
        // Description of a data block containing TS packets.
        class DataBlock
        {
        public:
            PCR        timestamp {};
            TimeSource source = TimeSource::UNDEFINED;
            size_t     next_index = 0;    // Next byte index in data.
            size_t     packet_count = 0;  // Remaining packets in data.
            size_t     packet_size = 0;   // Packet size in bytes.
            ByteBlock  data {};
        };

        // Command line options.
        IPSocketAddress _opt_destination {};

        // Plugin private fields.
        IPSocketAddress      _actual_destination {};
        size_t               _packet_size = 0;  // TS packet size in last MPE packet.
        std::list<DataBlock> _output {};        // List of contents of segment files to output.
    };
}
