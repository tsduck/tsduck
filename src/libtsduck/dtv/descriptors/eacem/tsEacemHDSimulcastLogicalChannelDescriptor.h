//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an EACEM HD_simulcast_logical_channel_descriptor.
//!  This is a private descriptor, must be preceded by the EACEM/EICTA PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLogicalChannelDescriptor.h"

namespace ts {
    //!
    //! Representation of an EACEM-defined HD_simulcast_logical_channel_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the EACEM/EICTA PDS.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EacemHDSimulcastLogicalChannelDescriptor : public AbstractLogicalChannelDescriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        EacemHDSimulcastLogicalChannelDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EacemHDSimulcastLogicalChannelDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Destructor.
        //!
        virtual ~EacemHDSimulcastLogicalChannelDescriptor() override;
    };
}
