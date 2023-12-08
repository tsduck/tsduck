//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Terrestrial Virtual Channel Table (TVCT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsVCT.h"

namespace ts {
    //!
    //! Representation of an ATSC Terrestrial Virtual Channel Table (TVCT)
    //! @see ATSC A/65, section 6.3.1.
    //! @ingroup table
    //!
    class TSDUCKDLL TVCT : public VCT
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        TVCT(uint8_t version = 0, bool is_current = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        TVCT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Virtual destructor.
        //!
        virtual ~TVCT() override;
    };
}
