//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Transport Stream Description Table (TSDT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptorsTable.h"

namespace ts {
    //!
    //! Representation of a Transport Stream Description Table (TSDT)
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.12
    //! @ingroup table
    //!
    class TSDUCKDLL TSDT : public AbstractDescriptorsTable
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        TSDT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        TSDT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        TSDT(const TSDT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        TSDT& operator=(const TSDT& other) = default;

        //!
        //! Virtual destructor
        //!
        virtual ~TSDT() override;

        // Inherited methods
        virtual bool isPrivate() const override;
    };
}
