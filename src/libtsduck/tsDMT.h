//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Representation of a Download Marker Table (DMT)
//  This is a Logiways private table.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"
#include "tsVariable.h"
#include "tsTSPacket.h"

namespace ts {

    class TSDUCKDLL DMT : public AbstractTable
    {
    public:
        // Description of one component
        struct Entry;
        typedef std::vector<Entry> EntryVector;

        // Public members:
        uint32_t           asset_id;
        uint16_t           remaining_broadcast_count;
        Variable<uint64_t> time_stamp;
        EntryVector      entries;

        // Maximum number of entries so that the DMT fits in one TS packet.
        static const size_t MAX_ENTRIES = 18;

        // Default constructor:
        DMT (uint32_t asset_id_ = 0, uint16_t remaining_ = 0);

        // Constructor from a binary table
        DMT (const BinaryTable& table);

        // Inherited methods
        virtual void serialize (BinaryTable& table) const;
        virtual void deserialize (const BinaryTable& table);

        // Write a DMT into one TS packet.
        void serialize (TSPacket& pkt, PID pid = PID_NULL, uint8_t cc = 0) const;

        // Count current and total packets, for all components
        uint32_t getTotalPacketCount() const;
        uint32_t getPacketCount() const;

        // Search for an entry matching a specified component tag.
        // Return this->entries.end() if not found.
        EntryVector::const_iterator search (uint8_t component_tag) const;
        EntryVector::iterator search (uint8_t component_tag);

        // Description of one component
        struct TSDUCKDLL Entry
        {
            uint8_t  component_tag;
            uint32_t packet_count;
            uint32_t total_packet_count;
            
            Entry (uint8_t comp = 0, uint32_t count = 0, uint32_t total = 0) :
                component_tag (comp),
                packet_count (count),
                total_packet_count (total)
            {
            }
        };
    };
}
