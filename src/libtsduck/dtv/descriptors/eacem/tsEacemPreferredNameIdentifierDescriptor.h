//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an eacem_preferred_name_identifier_descriptor.
//!  This is a private descriptor, must be preceded by the EACEM/EICTA PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractPreferredNameIdentifierDescriptor.h"

namespace ts {
    //!
    //! Representation of an eacem_preferred_name_identifier_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the EACEM/EICTA PDS.
    //! @see EACEM Technical Report Number TR-030, 9.2.11.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EacemPreferredNameIdentifierDescriptor : public AbstractPreferredNameIdentifierDescriptor
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] name_id Service name id from an EacemPreferredNameListDescriptor.
        //!
        EacemPreferredNameIdentifierDescriptor(uint8_t name_id = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EacemPreferredNameIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Destructor.
        //!
        virtual ~EacemPreferredNameIdentifierDescriptor() override;
    };
}
