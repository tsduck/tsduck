//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018-2023, Tristan Claverie
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an Application Identifier.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Representation of an Application Identifier
    //! @ingroup mpeg
    //!
    struct TSDUCKDLL ApplicationIdentifier
    {
        uint32_t organization_id; //!< The organization identifier
        uint16_t application_id;  //!< The application identifier

        //!
        //! Constructor from two ids.
        //! @param [in] org_id Organization identifier.
        //! @param [in] app_id Application identifier.
        //!
        ApplicationIdentifier(uint32_t org_id = 0, uint16_t app_id = 0) :
             organization_id(org_id),
             application_id(app_id)
        {
        }

        //!
        //! Equality operator.
        //! @param[in] that Identifier to compare to.
        //! @return True if both identifiers are equals, False otherwise.
        //!
        bool operator==(const ApplicationIdentifier& that) const
        {
            return organization_id == that.organization_id && application_id == that.application_id;
        }
        TS_UNEQUAL_OPERATOR(ApplicationIdentifier)

        //!
        //! Lower than operator. It compares first the organization id, then the application id.
        //! @param[in] that Identifier to compare to.
        //! @return True if the identifier is lower than the other one, False otherwise.
        //!
        bool operator<(const ApplicationIdentifier& that) const
        {
            return organization_id < that.organization_id || (organization_id == that.organization_id && application_id < that.application_id);
        }
    };
}
