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
//!
//!  @file
//!  Representation of a linkage_descriptor for SSU.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    //!
    //!  Representation of a linkage_descriptor for system software update.
    //!  SSU uses linkage type 0x09.
    //!
    class TSDUCKDLL SSULinkageDescriptor : public AbstractDescriptor
    {
    public:
        // OUI entry
        struct TSDUCKDLL Entry
        {
            // Public members
            uint32_t    oui;  // 24 bits
            ByteBlock selector;

            // Contructor
            Entry (uint32_t oui_ = 0): oui (oui_), selector () {}
        };
        typedef std::list<Entry> EntryList;

        // SSULinkageDescriptor public members:
        uint16_t    ts_id;      // transport_stream_id
        uint16_t    onetw_id;   // original_network_id
        uint16_t    service_id;
        EntryList entries;
        ByteBlock private_data;

        // Default constructor:
        SSULinkageDescriptor (uint16_t ts = 0, uint16_t onetw = 0, uint16_t service = 0);

        // Constructor with one OUI
        SSULinkageDescriptor (uint16_t ts, uint16_t onetw, uint16_t service, uint32_t oui);

        // Constructor from a binary descriptor
        SSULinkageDescriptor (const Descriptor&);

        // Inherited methods
        virtual void serialize (Descriptor&) const;
        virtual void deserialize (const Descriptor&);
    };
}
