//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018-2025, Tristan Claverie
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
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL ApplicationIdentifier
    {
    public:
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

        //! @cond nodoxygen
        auto operator<=>(const ApplicationIdentifier&) const = default;
        //! @endcond
    };
}
