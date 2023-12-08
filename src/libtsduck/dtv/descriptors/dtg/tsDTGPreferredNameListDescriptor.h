//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DTG preferred_name_list_descriptor.
//!  This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractPreferredNameListDescriptor.h"

namespace ts {
    //!
    //! Representation of a DTG preferred_name_list_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the DTG/OFCOM PDS.
    //! @see The D-Book 7 Part A (DTG), section 8.5.3.7
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DTGPreferredNameListDescriptor : public AbstractPreferredNameListDescriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        DTGPreferredNameListDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DTGPreferredNameListDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Destructor.
        //!
        virtual ~DTGPreferredNameListDescriptor() override;
    };
}
