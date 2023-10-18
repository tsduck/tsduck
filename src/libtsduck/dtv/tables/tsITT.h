//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Index Transmission information Table (ITT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"

namespace ts {
    //!
    //! Representation of an ISDB Index Transmission information Table (ITT).
    //! @see ARIB STD-B10, Part 3, 5.1.3
    //! @ingroup table
    //!
    class TSDUCKDLL ITT : public AbstractLongTable
    {
    public:
        // ITT public members:
        uint16_t       event_id = 0;  //!< Event id.
        DescriptorList descs;         //!< Descriptor list.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        ITT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        ITT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        ITT(const ITT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        ITT& operator=(const ITT& other) = default;

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
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
