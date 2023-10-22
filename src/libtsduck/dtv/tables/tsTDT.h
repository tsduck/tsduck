//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Time & Date Table (TDT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of a Time & Date Table (TDT)
    //! @see ETSI EN 300 468, 5.2.5
    //! @ingroup table
    //!
    class TSDUCKDLL TDT : public AbstractTable
    {
    public:
        // Public members:
        Time utc_time {};  //!< UTC time.

        //!
        //! Default constructor.
        //! @param [in] utc_time UTC time.
        //!
        TDT(const Time& utc_time = Time::Epoch);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        TDT(DuckContext& duck, const BinaryTable& table);

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
