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
//  Representation of a logical_channel_number_descriptor.
//  Private descriptor, must be preceeded by the EACEM/EICTA PDS.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    class TSDUCKDLL LogicalChannelNumberDescriptor : public AbstractDescriptor
    {
    public:
        // Service entry
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t service_id;
            bool   visible;
            uint16_t lcn;

            // Contructor
            Entry (uint16_t id_ = 0, bool visible_ = true, uint16_t lcn_ = 0): service_id (id_), visible (visible_), lcn (lcn_) {}
        };
        typedef std::list<Entry> EntryList;

        // Maximum number of entries to fit in 255 bytes
        static const size_t MAX_ENTRIES = 63;

        // LogicalChannelNumberDescriptor public members:
        EntryList entries;

        // Default constructor:
        LogicalChannelNumberDescriptor();

        // Constructor from a binary descriptor
        LogicalChannelNumberDescriptor (const Descriptor&);

        // Constructor using a variable-length argument list.
        // Each entry is described by 2 arguments: service_id and lcn.
        // All services are marked as visible by default.
        // All arguments are int, not uint16_t, since integer literals are int
        // by default. The end of the argument list must be marked by -1.
        LogicalChannelNumberDescriptor (int service_id, int lcn, ...);

        // Inherited methods
        virtual void serialize (Descriptor&) const;
        virtual void deserialize (const Descriptor&);
    };
}
