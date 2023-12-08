//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Free TV Australia logical_channel_descriptor.
//!  This is a private descriptor, must be preceded by the Free TV
//!  Australia PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLogicalChannelDescriptor.h"

namespace ts {
    //!
    //! Representation of a Free TV Australia logical_channel_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the Free TV Australia PDS.
    //! @see Free TV Australia Operational Practice OP-41, section 2.2
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AustraliaLogicalChannelDescriptor : public AbstractLogicalChannelDescriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        AustraliaLogicalChannelDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AustraliaLogicalChannelDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Destructor.
        //!
        virtual ~AustraliaLogicalChannelDescriptor() override;
    };
}
