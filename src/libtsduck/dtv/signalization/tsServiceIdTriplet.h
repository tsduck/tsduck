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
//!  Full identification of a DVB service (aka "DVB triplet")
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTransportStreamId.h"

namespace ts {
    //!
    //! Full identification of a DVB service (aka "DVB triplet").
    //! @ingroup mpeg
    //!
    class TSDUCKDLL ServiceIdTriplet: public TransportStreamId
    {
    public:
        // Public members:
        uint16_t service_id;  //!< Service id.
        uint8_t  version;     //!< General-purpose version (typically a table version), not part of the DVB triplet.

        //!
        //! Constructor.
        //! @param [in] svid Service id.
        //! @param [in] tsid Transport stream id.
        //! @param [in] onid Original network id.
        //! @param [in] vers Optional version.
        //!
        ServiceIdTriplet(uint16_t svid = 0, uint16_t tsid = 0, uint16_t onid = 0, uint8_t vers = 0);

        //!
        //! Constructor.
        //! @param [in] svid Service id.
        //! @param [in] tsid Full transport stream id.
        //! @param [in] vers Optional version.
        //!
        ServiceIdTriplet(uint16_t svid, const TransportStreamId& tsid, uint8_t vers = 0) :
            TransportStreamId(tsid),
            service_id(svid),
            version(vers)
        {
        }

        //!
        //! Get a "normalized" 64-bit identifier.
        //! This is a value containing the original network id, TS id, service id and version.
        //! @return The "normalized" 64-bit identifier of the TS.
        //!
        uint64_t normalized() const
        {
            return (uint64_t(original_network_id) << 40) | (uint64_t(transport_stream_id) << 24) | (uint64_t(service_id) << 8) | uint64_t(version);
        }

        //!
        //! Comparison operator.
        //! @param [in] svid Another instance to compare.
        //! @return True if this object == @a svid.
        //!
        bool operator==(const ServiceIdTriplet& svid) const
        {
            return normalized() == svid.normalized();
        }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Comparison operator.
        //! @param [in] svid Another instance to compare.
        //! @return True if this object != @a svid.
        //!
        bool operator!=(const ServiceIdTriplet& svid) const
        {
            return normalized() != svid.normalized();
        }
#endif

        //!
        //! Comparison operator.
        //! @param [in] svid Another instance to compare.
        //! @return True if this object < @a svid.
        //!
        bool operator<(const ServiceIdTriplet& svid) const
        {
            return normalized() < svid.normalized();
        }

        //!
        //! Comparison operator.
        //! @param [in] svid Another instance to compare.
        //! @return True if this object <= @a svid.
        //!
        bool operator<=(const ServiceIdTriplet& svid) const
        {
            return normalized() <= svid.normalized();
        }

        //!
        //! Comparison operator.
        //! @param [in] svid Another instance to compare.
        //! @return True if this object > @a svid.
        //!
        bool operator>(const ServiceIdTriplet& svid) const
        {
            return normalized() > svid.normalized();
        }

        //!
        //! Comparison operator.
        //! @param [in] svid Another instance to compare.
        //! @return True if this object >= @a svid.
        //!
        bool operator>=(const ServiceIdTriplet& svid) const
        {
            return normalized() >= svid.normalized();
        }

        // Inherited methods.
        virtual void clear() override;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };

    // Containers:
    typedef std::set<ServiceIdTriplet> ServiceIdTripletSet;        //!< Set of ServiceIdTriplet.
    typedef std::vector<ServiceIdTriplet> ServiceIdTripletVector;  //!< Vector of ServiceIdTriplet.
}
