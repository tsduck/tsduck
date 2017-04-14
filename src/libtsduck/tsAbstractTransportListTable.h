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
//  Abstract base class for tables containing a list of transport stream
//  descriptions. Common code for BAT and NIT.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsTransportStreamId.h"
#include "tsDescriptorList.h"

namespace ts {

    class TSDUCKDLL AbstractTransportListTable : public AbstractLongTable
    {
    public:
        // List of DescriptorList's, indexed by TransportStreamId.
        typedef std::map <TransportStreamId, DescriptorList> TransportMap;

        // NIT/BAT common public members:
        DescriptorList descs;       // top-level descriptor list
        TransportMap   transports;  // key=onid/tsid, value=descriptor_list

        // Section serialization "hint". Used in serialize() only.
        // Indicate in which section a TS should be preferably serialized.
        // When unspecified for a TS, the corresponding TS description is
        // serialized in an arbitrary section.
        typedef std::map <TransportStreamId, int> SectionHintsMap;
        SectionHintsMap section_hints;

        // Inherited methods
        virtual void serialize (BinaryTable& table) const;
        virtual void deserialize (const BinaryTable& table);

    protected:
        // Interpretation of TID-extension differs between NIT and BAT
        uint16_t _tid_ext;

        // Default constructor
        AbstractTransportListTable (TID tid_, uint16_t tid_ext_, uint8_t version_, bool is_current_);

        // Constructor from a binary table
        AbstractTransportListTable (TID tid, const BinaryTable& table);

    private:
        typedef std::set <TransportStreamId> TransportStreamIdSet;

        // Add a new section to a table being serialized.
        // Session number is incremented. Data and remain are reinitialized.
        void addSection (BinaryTable& table,
                         int& section_number,
                         uint8_t* payload,
                         uint8_t*& data,
                         size_t& remain) const;

        // Same as previous, while being inside the transport loop.
        void addSection (BinaryTable& table,
                         int& section_number,
                         uint8_t* payload,
                         uint8_t*& tsll_addr,
                         uint8_t*& data,
                         size_t& remain) const;

        // Select a transport stream for serialization in current section.
        // If found, set ts_id, remove the ts id from the set and return true.
        // Otherwise, return false.
        bool getNextTransport (TransportStreamIdSet& ts_set, 
                               TransportStreamId& ts_id,
                               int section_number) const;
    };
}
