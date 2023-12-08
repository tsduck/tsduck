//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DTG preferred_name_identifier_descriptor.
//!  This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractPreferredNameIdentifierDescriptor.h"

namespace ts {
    //!
    //! Representation of a DTG preferred_name_identifier_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
    //! @see The D-Book 7 Part A (DTG), section 8.5.3.8
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DTGPreferredNameIdentifierDescriptor : public AbstractPreferredNameIdentifierDescriptor
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] name_id Service name id from an EacemPreferredNameListDescriptor.
        //!
        DTGPreferredNameIdentifierDescriptor(uint8_t name_id = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DTGPreferredNameIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Destructor.
        //!
        virtual ~DTGPreferredNameIdentifierDescriptor() override;
    };
}
