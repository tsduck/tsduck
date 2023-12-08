//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Cable Virtual Channel Table (CVCT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsVCT.h"

namespace ts {
    //!
    //! Representation of an ATSC Cable Virtual Channel Table (CVCT)
    //! @see ATSC A/65, section 6.3.2.
    //! @ingroup table
    //!
    class TSDUCKDLL CVCT : public VCT
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        CVCT(uint8_t version = 0, bool is_current = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        CVCT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Virtual destructor.
        //!
        virtual ~CVCT() override;
    };
}
