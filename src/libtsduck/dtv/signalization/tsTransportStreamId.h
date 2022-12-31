//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Full identification of a DVB transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"

namespace ts {
    //!
    //! Full identification of a DVB transport stream.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TransportStreamId: public StringifyInterface
    {
    public:
        // Public members:
        uint16_t transport_stream_id;  //!< Transport stream id.
        uint16_t original_network_id;  //!< Original network id.

        //!
        //! Constructor.
        //! @param [in] tsid Transport stream id.
        //! @param [in] onid Original network id.
        //!
        TransportStreamId(uint16_t tsid = 0, uint16_t onid = 0);

        //!
        //! Clear the content of this object.
        //!
        virtual void clear();

        //!
        //! Get a "normalized" 32-bit identifier.
        //! The upper 16-bit contain the original network id.
        //! The lower 16-bit contain the TS id.
        //! @return The "normalized" 32-bit identifier of the TS.
        //!
        uint32_t normalized() const
        {
            return uint32_t(transport_stream_id) | (uint32_t(original_network_id) << 16);
        }

        //!
        //! Comparison operator.
        //! @param [in] tsid Another instance to compare.
        //! @return True if this object == @a tsid.
        //!
        bool operator==(const TransportStreamId& tsid) const
        {
            return transport_stream_id == tsid.transport_stream_id && original_network_id == tsid.original_network_id;
        }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Comparison operator.
        //! @param [in] tsid Another instance to compare.
        //! @return True if this object != @a tsid.
        //!
        bool operator!=(const TransportStreamId& tsid) const
        {
            return transport_stream_id != tsid.transport_stream_id || original_network_id != tsid.original_network_id;
        }
#endif

        //!
        //! Comparison operator.
        //! @param [in] tsid Another instance to compare.
        //! @return True if this object < @a tsid.
        //!
        bool operator<(const TransportStreamId& tsid) const
        {
            return normalized() < tsid.normalized();
        }

        //!
        //! Comparison operator.
        //! @param [in] tsid Another instance to compare.
        //! @return True if this object <= @a tsid.
        //!
        bool operator<=(const TransportStreamId& tsid) const
        {
            return normalized() <= tsid.normalized();
        }

        //!
        //! Comparison operator.
        //! @param [in] tsid Another instance to compare.
        //! @return True if this object > @a tsid.
        //!
        bool operator>(const TransportStreamId& tsid) const
        {
            return normalized() > tsid.normalized();
        }

        //!
        //! Comparison operator.
        //! @param [in] tsid Another instance to compare.
        //! @return True if this object >= @a tsid.
        //!
        bool operator>=(const TransportStreamId& tsid) const
        {
            return normalized() >= tsid.normalized();
        }

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };

    // Containers:
    typedef std::set<TransportStreamId> TransportStreamIdSet;        //!< Set of TransportStreamId.
    typedef std::vector<TransportStreamId> TransportStreamIdVector;  //!< Vector of TransportStreamId.
}
