//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC Extended Text Table (ETT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsATSCMultipleString.h"

namespace ts {
    //!
    //! Representation of an ATSC Extended Text Table (ETT)
    //! @see ATSC A/65, section 6.6.
    //! @ingroup table
    //!
    class TSDUCKDLL ETT : public AbstractLongTable
    {
    public:
        // ETT public members:
        uint16_t           ETT_table_id_extension = 0;  //!< Table extension, for ETT segmentation.
        uint8_t            protocol_version = 0;        //!< ATSC protocol version.
        uint32_t           ETM_id = 0;                  //!< Extended text message id.
        ATSCMultipleString extended_text_message {};    //!< Extended text message.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //!
        ETT(uint8_t version = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        ETT(DuckContext& duck, const BinaryTable& table);

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
