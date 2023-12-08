//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a multilingual_network_name_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractMultilingualDescriptor.h"

namespace ts {
    //!
    //! Representation of a multilingual_network_name_descriptor.
    //! @see ETSI EN 300 468, 6.2.24.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MultilingualNetworkNameDescriptor : public AbstractMultilingualDescriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        MultilingualNetworkNameDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MultilingualNetworkNameDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Virtual destructor
        //!
        virtual ~MultilingualNetworkNameDescriptor() override;
    };
}
