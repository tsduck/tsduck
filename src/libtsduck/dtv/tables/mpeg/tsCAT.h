//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Conditional Access Table (CAT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptorsTable.h"

namespace ts {
    //!
    //! Representation of a Conditional Access Table (CAT).
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.6
    //! @ingroup table
    //!
    class TSDUCKDLL CAT : public AbstractDescriptorsTable
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        CAT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        CAT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        CAT(const CAT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        CAT& operator=(const CAT& other) = default;

        //!
        //! Virtual destructor.
        //!
        virtual ~CAT() override;

        // Inherited methods
        virtual bool isPrivate() const override;
    };
}
