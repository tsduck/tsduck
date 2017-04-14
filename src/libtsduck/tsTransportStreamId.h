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
//  Identification of a transport stream
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    struct TSDUCKDLL TransportStreamId
    {
        // Public members:
        uint16_t transport_stream_id;
        uint16_t original_network_id;

        // Constructor:
        TransportStreamId (uint16_t tsid = 0, uint16_t onid = 0) :
            transport_stream_id (tsid),
            original_network_id (onid)
        {
        }

        // "Normalized" 32-bits identifier.
        // Upper 16-bit contain the original network id.
        // Lower 16-bit contain the TS id
        uint32_t normalized() const
        {
            return uint32_t (transport_stream_id) | (uint32_t (original_network_id) << 16);
        }

        // Comparison
        bool operator== (const TransportStreamId& tsid) const
        {
            return transport_stream_id == tsid.transport_stream_id && original_network_id == tsid.original_network_id;
        }
        bool operator!= (const TransportStreamId& tsid) const
        {
            return transport_stream_id != tsid.transport_stream_id || original_network_id != tsid.original_network_id;
        }
        bool operator< (const TransportStreamId& tsid) const
        {
            return normalized() < tsid.normalized();
        }
        bool operator<= (const TransportStreamId& tsid) const
        {
            return normalized() <= tsid.normalized();
        }
        bool operator> (const TransportStreamId& tsid) const
        {
            return normalized() > tsid.normalized();
        }
        bool operator>= (const TransportStreamId& tsid) const
        {
            return normalized() >= tsid.normalized();
        }
    };

    // Containers:
    typedef std::set <TransportStreamId> TransportStreamIdSet;
    typedef std::vector <TransportStreamId> TransportStreamIdVector;
}
