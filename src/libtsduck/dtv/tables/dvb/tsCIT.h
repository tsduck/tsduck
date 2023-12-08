//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB Content Identifier Table (CIT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"

namespace ts {
    //!
    //! Representation of DVB Content Identifier Table (CIT).
    //! @see ETSI TS 102 323, 12.2
    //! @ingroup table
    //!
    class TSDUCKDLL CIT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a CRID entry (Content Reference Identifier).
        //!
        class TSDUCKDLL CRID
        {
        public:
            CRID() = default;                   //!< Constructor.
            uint16_t crid_ref = 0;              //!< CRID reference.
            uint8_t  prepend_string_index = 0;  //!< Index in prepend_strings vector, 0xFF if none.
            UString  unique_string {};          //!< CRID unique part.
        };

        //!
        //! List of CRID entries.
        //!
        typedef std::list<CRID> CRIDList;

        // CIT public members:
        uint16_t      service_id = 0;           //!< Service id.
        uint16_t      transport_stream_id = 0;  //!< Transport stream id.
        uint16_t      original_network_id = 0;  //!< Original network id.
        UStringVector prepend_strings {};       //!< Strings to prepend to unique_string in CRID.
        CRIDList      crids {};                 //!< List of CRID.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        CIT(uint8_t version = 0, bool is_current = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        CIT(DuckContext& duck, const BinaryTable& table);

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
