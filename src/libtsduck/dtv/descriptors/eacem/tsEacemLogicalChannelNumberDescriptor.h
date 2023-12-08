//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an EACEM logical_channel_number_descriptor.
//!  This is a private descriptor, must be preceded by the EACEM/EICTA PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLogicalChannelDescriptor.h"

namespace ts {
    //!
    //! Representation of an EACEM-defined logical_channel_number_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the EACEM/EICTA PDS.
    //! @see EACEM Technical Report Number TR-030, 9.2.11.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EacemLogicalChannelNumberDescriptor : public AbstractLogicalChannelDescriptor

    {
    public:
        //!
        //! Default constructor.
        //!
        EacemLogicalChannelNumberDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EacemLogicalChannelNumberDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Destructor.
        //!
        virtual ~EacemLogicalChannelNumberDescriptor() override;
    };
}
