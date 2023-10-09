//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
