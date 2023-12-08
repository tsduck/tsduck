//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Bouquet Association Table (BAT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTransportListTable.h"

namespace ts {
    //!
    //! Representation of a Bouquet Association Table (BAT).
    //! @see ETSI EN 300 468, 5.2.2
    //! @ingroup table
    //!
    class TSDUCKDLL BAT : public AbstractTransportListTable
    {
    public:
        // BAT public members:
        uint16_t& bouquet_id;  //!< Bouquet identifier.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //! @param [in] id Bouquet identifier.
        //!
        BAT(uint8_t vers = 0, bool cur = true, uint16_t id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        BAT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        BAT(const BAT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        BAT& operator=(const BAT& other);

        // Inherited methods.
        DeclareDisplaySection();

    protected:
        // Inherited methods.
        virtual size_t maxPayloadSize() const override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
