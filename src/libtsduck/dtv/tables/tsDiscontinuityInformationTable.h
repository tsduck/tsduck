//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Discontinuity Information Table (DIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a Discontinuity Information Table (DIT)
    //! @see ETSI EN 300 468, 7.1.1
    //! @ingroup table
    //!
    class TSDUCKDLL DiscontinuityInformationTable : public AbstractTable
    {
    public:
        // Public members:
        bool transition = false;  //!< Transport stream transition.

        //!
        //! Default constructor.
        //! @param [in] tr Transport stream transition.
        //!
        DiscontinuityInformationTable(bool tr = false);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DiscontinuityInformationTable(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
