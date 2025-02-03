//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TransportStreamId: public StringifyInterface
    {
    public:
        // Public members:
        uint16_t original_network_id = 0;  //!< Original network id.
        uint16_t transport_stream_id = 0;  //!< Transport stream id.

        //!
        //! Default constructor.
        //!
        TransportStreamId() = default;

        //!
        //! Constructor.
        //! @param [in] tsid Transport stream id.
        //! @param [in] onid Original network id.
        //!
        TransportStreamId(uint16_t tsid, uint16_t onid) :
            original_network_id(onid),
            transport_stream_id(tsid)
        {
        }

        //! @cond nodoxygen
        auto operator<=>(const TransportStreamId&) const = default;
        //! @endcond

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

        // Implementation of StringifyInterface.
        virtual UString toString() const override;
    };

    // Containers:
    using TransportStreamIdSet = std::set<TransportStreamId>;        //!< Set of TransportStreamId.
    using TransportStreamIdVector = std::vector<TransportStreamId>;  //!< Vector of TransportStreamId.
}
