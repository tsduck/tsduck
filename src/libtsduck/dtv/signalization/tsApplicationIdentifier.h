//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018-2023, Tristan Claverie
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Inequality operator.
        //! @param[in] that Identifier to compare to.
        //! @return True if both identifiers are not equals, False otherwise.
        //!
        bool operator!=(const ApplicationIdentifier& that) const
        {
            return organization_id != that.organization_id || application_id != that.application_id;
        }
#endif

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
